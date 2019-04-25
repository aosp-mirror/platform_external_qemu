#include "android/emulation/control/WebRtcBridge.h"

#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <memory>
#include <nlohmann/json.hpp>

#include "android/base/ArraySize.h"
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

static std::string generateUniqueVideoHandle() {
    // Create a unique module identifier of at most 32 chars.
    char handle[32];
    Uuid id = Uuid::generate();
    uint64_t* uniqueId = (uint64_t*)id.bytes();
    snprintf(handle, ARRAY_SIZE(handle), "emuvid%016" PRIX64 "%016" PRIX64,
             uniqueId[0], uniqueId[1]);
    return std::string(handle);
}

WebRtcBridge::WebRtcBridge(AsyncSocketAdapter* socket,
                           const QAndroidRecordScreenAgent* const screenAgent,
                           int fps,
                           int videoBridgePort)
    : mProtocol(this),
      mTransport(&mProtocol, socket),
      mScreenAgent(screenAgent),
      mFps(fps),
      mVideoBridgePort(videoBridgePort) {
    mVideoModule = generateUniqueVideoHandle();
}

WebRtcBridge::~WebRtcBridge() {
    terminate();
}

bool WebRtcBridge::connect(std::string identity) {
    mMapLock.lockRead();
    if (mId.find(identity) == mId.end()) {
        mMapLock.unlockRead();
        AutoWriteLock upgrade(mMapLock);
        std::shared_ptr<Lock> bufferLock(new Lock());
        std::shared_ptr<MessageQueue> queue(
                new MessageQueue(kMaxMessageQueueLen, *(bufferLock.get())));
        mId[identity] = queue;
        mLocks[queue.get()] = bufferLock;
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

    auto queue = mId[identity];
    mId.erase(identity);
    mLocks.erase(queue.get());

    // Notify the video bridge.
    json msg;
    msg["topic"] = identity;
    msg["msg"] = "disconnected";
    mProtocol.write(&mTransport, msg);
}

bool WebRtcBridge::nextMessage(std::string identity,
                               std::string* nextMessage,
                               System::Duration blockAtMostMs) {
    std::shared_ptr<MessageQueue> queue;
    std::shared_ptr<Lock> lock;
    {
        AutoReadLock readerLock(mMapLock);
        if (mId.find(identity) == mId.end()) {
            LOG(ERROR) << "Unknown identity: " << identity;
            *nextMessage =
                    std::move(std::string("{ \"bye\" : \"disconnected\" }"));
            return false;
        }
        queue = mId[identity];
        lock = mLocks[queue.get()];
        lock->lock();
    }

    System::Duration blockUntil =
            System::get()->getUnixTimeUs() + blockAtMostMs * 1000;
    bool status = queue->popLockedBefore(nextMessage, blockUntil) ==
                  BufferQueueResult::Ok;
    lock->unlock();
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
    mTransport.close();
    if (mBridgePid) {
        System::get()->killProcess(mBridgePid);
    }
    // Note, closing the shared memory region can crash the bridge as it might
    // attempt to read inaccessible memory.
    mScreenAgent->stopWebRtcModule();
}

void WebRtcBridge::received(SocketTransport* from, json object) {
    LOG(VERBOSE) << "Received from video bridge: " << object.dump(2);
    // It's either  a message, or goodbye..
    bool message = object.count("msg") && object.count("topic");
    bool bye = object.count("bye") && object.count("topic");
    if (!message && !bye) {
        LOG(ERROR) << "Ignoring incorrect message: " << object.dump(2);
        return;
    }

    std::string dest = object["topic"];
    if (bye) {
        // We are saying good bye..
        AutoWriteLock lock(mMapLock);
        if (mId.find(dest) != mId.end()) {
            auto queue = mId[dest];
            mLocks.erase(queue.get());
            mId.erase(dest);
        }
    } else {
        std::string msg = object["msg"];
        AutoReadLock lock(mMapLock);
        LOG(INFO) << "forward to: " << dest;
        if (mId.find(dest) != mId.end()) {
            std::string msg = object["msg"];
            auto queue = mId[dest];
            {
                AutoLock pushLock(*mLocks[queue.get()]);
                if (queue->tryPushLocked(object["msg"]) !=
                    BufferQueueResult::Ok) {
                    LOG(ERROR) << "Unable to push message "
                               << (std::string)object["msg"] << "dropping it";
                }
            }
        }
    }
}

RtcBridge::BridgeState WebRtcBridge::state() {
    return mState;
}

void WebRtcBridge::stateConnectionChange(SocketTransport* connection,
                                         State current) {
    // Clear out everyone, as the video bridge disappeared / just connected..
    AutoWriteLock lock(mMapLock);
    mId.clear();
    mLocks.clear();
    switch (current) {
        case State::CONNECTED:
            mState = BridgeState::Connected;
            break;
        case State::DISCONNECTED:
            mState = BridgeState::Disconnected;
            break;
        case State::CONNECTING:
            mState = BridgeState::Pending;
            break;
    }
}

bool WebRtcBridge::start() {
    mState = BridgeState::Pending;
    std::string executable =
            System::get()->findBundledExecutable(kVideoBridgeExe);

    // no video bridge?
    if (executable.empty()) {
        mState = BridgeState::Disconnected;
        return false;
    }

    // TODO(jansen): We should pause the recorder when no connections are
    // active.
    if (!mScreenAgent->startWebRtcModule(mVideoModule.c_str(), mFps)) {
        LOG(ERROR) << "Failed to start webrtc module on " << mVideoModule
                   << ", no video available.";
        return false;
    }
    std::vector<std::string> cmdArgs{executable,
                                     "--logdir",
                                     System::get()->getTempDir(),
                                     "--port",
                                     std::to_string(mVideoBridgePort),
                                     "--handle",
                                     mVideoModule};

    std::string invoke = "";
    for (auto arg : cmdArgs) {
        invoke += arg + " ";
    }

    if (!System::get()->runCommand(cmdArgs, RunOptions::DontWait,
                                   System::kInfinite, nullptr, &mBridgePid)) {
        // Failed to start video bridge! Mission abort!
        LOG(INFO) << "Failed to start " << invoke;
        terminate();
        return false;
    }
    LOG(INFO) << "Launched " << invoke << ", pid:" << mBridgePid;

    // Let's connect the socket transport if needed.
    if (mTransport.state() == State::CONNECTED) {
        mState = BridgeState::Connected;
    }
    return mTransport.connect();
}

RtcBridge* WebRtcBridge::create(
        int port,
        const AndroidConsoleAgents* const consoleAgents) {
    std::string executable =
            System::get()->findBundledExecutable(kVideoBridgeExe);
    if (executable.empty()) {
        LOG(ERROR) << "WebRTC: couldn't get path to " << kVideoBridgeExe;
        return new NopRtcBridge();
    }

    Looper* looper = android::base::ThreadLooper::get();
    AsyncSocket* socket = new AsyncSocket(looper, port);
    return new WebRtcBridge(socket, consoleAgents->record,
                            WebRtcBridge::kMaxFPS, port);
}
}  // namespace control
}  // namespace emulation
}  // namespace android
