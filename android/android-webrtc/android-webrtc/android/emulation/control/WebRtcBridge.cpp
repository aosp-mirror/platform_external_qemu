#include "android/emulation/control/WebRtcBridge.h"
#include <memory>
#include <nlohmann/json.hpp>
#include "android/base/Log.h"
#include "android/base/Uuid.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/sockets/SocketErrors.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/control/window_agent.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

using nlohmann::json;
namespace android {
namespace emulation {
namespace control {

using namespace android::base;

const std::string WebRtcBridge::kVideoBridgeExe = "goldfish-webrtc-bridge";

WebRtcBridge::WebRtcBridge(int port,
                           const QAndroidRecordScreenAgent* const screenAgent)
    : mPort(port), mScreenAgent(screenAgent) {}

WebRtcBridge::~WebRtcBridge() {
    terminate();
}

bool WebRtcBridge::connect(std::string identity) {
    mMapLock.lockRead();
    if (mId.find(identity) == mId.end()) {
        mMapLock.unlockRead();
        AutoWriteLock upgrade(mMapLock);
        Lock* l = new Lock();
        MessageQueue* queue = new MessageQueue(4096, *l);
        mId[identity] = queue;
        mLocks[queue] = l;
    } else {
        mMapLock.unlockRead();
    }

    json start;
    start["start"] = identity;
    bool fwd = acceptJsepMessage(identity, start.dump());
    if (!fwd) {
        LOG(ERROR) << "json message rejected. disconnecting";
        disconnect(identity);
    }
    return fwd;
}

void WebRtcBridge::disconnect(std::string identity) {
    {
        AutoReadLock lock(mMapLock);
        if (mId.find(identity) == mId.end()) {
            LOG(ERROR) << "Trying to remove unknown queue, ignoring";
            return;
        }
    }
    LOG(INFO) << "disconnect: " << identity;
    AutoWriteLock upgrade(mMapLock);
    // Unlikely but someone could have yanked identity out of the map
    if (mId.find(identity) == mId.end()) {
        LOG(ERROR) << "Trying to remove unknown queue, ignoring";
        return;
    }
    mId.erase(identity);
    // We need some refcounting on these queues/locks.
}

bool WebRtcBridge::nextMessage(std::string identity,
                               std::string* nextMessage,
                               System::Duration blockAtMostMs) {
    MessageQueue* queue;
    Lock* l;
    {
        AutoReadLock lock(mMapLock);
        if (mId.find(identity) == mId.end()) {
            LOG(ERROR) << "Unknown identity: " << identity;
            *nextMessage =
                    std::move(std::string("{ \"bye\" : \"Who are you?\" }"));
            return false;
        }
        queue = mId[identity];
        l = mLocks[queue];
        l->lock();
    }

    System::Duration blockUntil =
            System::get()->getUnixTimeUs() + blockAtMostMs * 1000;
    bool status = queue->popLockedBefore(nextMessage, blockUntil) ==
                  BufferQueueResult::Ok;
    l->unlock();
    return status;
}

bool WebRtcBridge::acceptJsepMessage(std::string identity,
                                     std::string message) {
    {
        AutoReadLock lock(mMapLock);
        if (mId.find(identity) == mId.end()) {
            LOG(ERROR) << "Trying to send to unknown identity.";
            return false;
        }
    }
    json msg;
    msg["from"] = identity;
    msg["msg"] = message;
    std::string serialized = msg.dump();
    LOG(INFO) << "Len + 0x0: " << serialized.length();
    LOG(INFO) << "acceptJsepMessage: identity: " << identity
              << ", msg: " << serialized;

    // c_str() is guaranteed to be NULL terminated, we want to send the 0x00
    int bufferLen = serialized.length() + 1;
    const char* buf = serialized.c_str();
    while (bufferLen > 0) {
        ssize_t ret = socketSend(mSo, buf, bufferLen);
        if (ret <= 0) {
            LOG(ERROR) << "acceptJsepMessage: Failed to send: " << errno
                       << ", ret:" << ret << " over: " << mSo;
            return false;
        }
        buf += ret;
        bufferLen -= ret;
    }
    return true;
}

void WebRtcBridge::terminate() {
    mStop = true;
    LOG(INFO) << "WebRtcBridge::terminate()";
    // Shutdowns do not unblock the read in macos.
    socketShutdownWrites(mSo);
    socketShutdownReads(mSo);
    socketClose(mSo);
    mSo = -1;
    mScreenAgent->stopWebRtcModule();
    if (mThread) {
        mThread->wait();
        mThread.release();
    }
}

void WebRtcBridge::received(std::string envelope) {
    LOG(INFO) << "Received from video bridge: " << envelope;
    if (!json::accept(envelope)) {
        return;
    }

    // We are getting a simple pubsub message.
    // We only care about publish
    json object = json::parse(envelope, nullptr, false);
    if (!object.count("type") || !object.count("envelope") ||
        !object.count("topic") || object["type"] != "msg") {
        LOG(ERROR) << "Ignoring incorrect message: " << object.dump(2);
    }

    std::string dest = object["topic"];
    std::string msg = object["msg"];
    json jsonMsg = json::parse(msg, nullptr, false);
    if (jsonMsg.count("bye")) {
        // We are saying good bye..
        AutoWriteLock lock(mMapLock);
        if (mId.find(dest) != mId.end()) {
            auto queue = mId[dest];
            mLocks.erase(queue);
            // TODO(jansene): We need to actually cleanup the buffers, but when?
        }
    } else {
        AutoReadLock lock(mMapLock);
        if (mId.find(dest) != mId.end()) {
            mId[dest]->tryPushLocked(object["msg"]);
        }
    }
}

bool WebRtcBridge::run() {
    char buffer[kRecBufferSize];
    int so = 0;
    while (so < 1 && !mStop) {
        so = socketTcp4LoopbackClient(mPort);
        if (so < 0) {
            LOG(ERROR) << "failed to open up connection to port: " << mPort
                       << ".. Sleeping...";
            Thread::sleepMs(1000);
        }
    }
    LOG(INFO) << "Connected video bridge on port: " << mPort << ", mSo: " << so;
    std::string jsonmsg;
    mSo = so;
    while (mSo > 0 && !mStop) {
        int bytes = socketRecv(mSo, buffer, sizeof(buffer));
        if (bytes <= 0) {
            // something went wrong
            break;
        }
        for (int i = 0; i < bytes; i++) {
            if (buffer[i] == 0) {
                received(jsonmsg);
                jsonmsg.clear();
            } else {
                jsonmsg += buffer[i];
            }
        }
    }
}  // namespace control

bool WebRtcBridge::openConnection() {
    mThread.reset(new base::FunctorThread([&] { return this->run(); }));
    mThread->start();
    return true;
}

RtcBridge* WebRtcBridge::create(
        int port,
        const AndroidConsoleAgents* const consoleAgents) {
    // Launch vbridge.
    // Open socket..
    // Listen to socket..
    std::string executable =
            System::get()->findBundledExecutable(kVideoBridgeExe);
    if (executable.empty()) {
        LOG(ERROR) << "couldn't get path to " << kVideoBridgeExe;
        return new NopRtcBridge();
    }

    // Kick off the webrtc shared memory engine
    // TODO(jansene): Make this restricts us to running only one emulator
    // with video sharing turned on..
    std::string videoModule = "video0";
    if (!consoleAgents->record->startWebRtcModule(videoModule.c_str(), 60)) {
        LOG(ERROR) << "Failed to start webrtc module on " << videoModule
                   << ", no video available.";
        return new NopRtcBridge();
    }

    // TODO(jansene): Block & fork to guarantee port access in child
    // process?
    std::vector<std::string> cmdArgs{executable, "--port", std::to_string(port),
                                     "--handle", videoModule};
    // System::get()->runCommand(cmdArgs, RunOptions::DontWait);
    LOG(INFO) << "Launched video bridge";

    std::unique_ptr<WebRtcBridge> pThis(
            new WebRtcBridge(port, consoleAgents->record));
    if (!pThis->openConnection()) {
        LOG(ERROR) << "Failed to open connection to video bridge on port: "
                   << port;
        return new NopRtcBridge();
    }

    return pThis.release();
}
}  // namespace control
}  // namespace emulation
}  // namespace android
