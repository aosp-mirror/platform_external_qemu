#include "android/emulation/control/WebRtcBridge.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <memory>
#include <nlohmann/json.hpp>

#include "android/base/Uuid.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/sockets/SocketErrors.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/control/window_agent.h"
#include "emulator/net/AsyncSocket.h"

using emulator::net::AsyncSocket;
using nlohmann::json;

namespace android {
namespace emulation {
namespace control {

using namespace android::base;

const std::string WebRtcBridge::kVideoBridgeExe = "goldfish-webrtc-bridge";

WebRtcBridge::WebRtcBridge(AsyncSocketAdapter* connection,
                           const QAndroidRecordScreenAgent* const screenAgent)
    : mProtocol(this),
      mTransport(&mProtocol, connection),
      mScreenAgent(screenAgent) {}

WebRtcBridge::~WebRtcBridge() {
    terminate();
}

bool WebRtcBridge::connect(std::string identity) {
    mMapLock.lockRead();
    if (mId.find(identity) == mId.end()) {
        mMapLock.unlockRead();
        AutoWriteLock upgrade(mMapLock);
        Lock* l = new Lock();
        MessageQueue* queue = new MessageQueue(128, *l);
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
    return mProtocol.write(&mTransport, msg);
}

void WebRtcBridge::terminate() {
    LOG(INFO) << "WebRtcBridge::terminate()";
    // Shutdowns do not unblock the read in macos.
    mScreenAgent->stopWebRtcModule();
    mTransport.close();
}

void WebRtcBridge::received(SocketTransport* from, json object) {
    LOG(INFO) << "Received from video bridge: " << object.dump(2);
    if (!object.count("msg") || !object.count("topic")) {
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

bool WebRtcBridge::openConnection() {
    return true;
}
void WebRtcBridge::stateConnectionChange(SocketTransport* connection,
        State current) {
    LOG(ERROR) << "Bad news.. Someone must gone missing: " << (int) current;
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

    Looper* looper =
            android::base::ThreadLooper::get();
    AsyncSocket* socket = new AsyncSocket(looper);
    while (!socket->loopbackConnect(port)) {
        Thread::sleepMs(100);
    }

    return new WebRtcBridge(socket, consoleAgents->record);
}
}  // namespace control
}  // namespace emulation
}  // namespace android
