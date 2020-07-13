#include "android/emulation/control/WebRtcBridge.h"

#include <nlohmann/json.hpp>                    // for json, basic_json<>::v...
#include <stdio.h>                              // for sscanf
#include <chrono>                               // for operator+, seconds
#include <memory>                               // for shared_ptr, shared_pt...
#include <utility>                              // for move
#include <vector>                               // for vector

#include "android/base/Log.h"                   // for LOG, LogMessage, LogS...
#include "android/base/Optional.h"              // for Optional
#include "android/base/async/AsyncSocket.h"     // for AsyncSocket
#include "android/base/sockets/ScopedSocket.h"  // for ScopedSocket
#include "android/base/sockets/SocketUtils.h"   // for socketGetPort, socket...
#include "android/base/system/System.h"         // for System, System::Pid
#include "android/emulation/ConfigDirs.h"       // for ConfigDirs

namespace android {
namespace base {
class Looper;
}  // namespace base
}  // namespace android
namespace emulator {
namespace net {
class AsyncSocketAdapter;
}  // namespace net
}  // namespace emulator

using nlohmann::json;

namespace android {
namespace emulation {
namespace control {

using namespace android::base;

const std::string WebRtcBridge::kVideoBridgeExe = "goldfish-webrtc-bridge";
std::string WebRtcBridge::BRIDGE_RECEIVER = "WebrtcVideoBridge";

WebRtcBridge::WebRtcBridge(AsyncSocketAdapter* socket,
                           const AndroidConsoleAgents* const agents,
                           int fps,
                           int videoBridgePort,
                           std::string turncfg)
    : mProtocol(this),
      mTransport(&mProtocol, socket),
      mEventDispatcher(agents),
      mScreenAgent(agents->record),
      mFps(fps),
      mVideoBridgePort(videoBridgePort),
      mTurnConfig(turncfg) {}

WebRtcBridge::~WebRtcBridge() {
    terminate();
}

void WebRtcBridge::updateBridgeState() {
    if (mId.size() > 0) {
        LOG(INFO) << "Starting shared memory module";
        mScreenAgent->startSharedMemoryModule(mFps);
    } else {
        LOG(INFO) << "Stopping shared memory module";
        mScreenAgent->stopSharedMemoryModule();
    }
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
        updateBridgeState();
    } else {
        mMapLock.unlockRead();
    }

    json start;
    start["start"] = identity;
    start["handles"] = std::vector<std::string>({mVideoModule});
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
    updateBridgeState();

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
    LOG(INFO) << "sending " << msg.dump(2);
    return mProtocol.write(&mTransport, msg);
}

void WebRtcBridge::terminate() {
    LOG(INFO) << "Closing transport.";
    if (mBridgePid) {
        LOG(INFO) << "Terminating video bridge.";
        System::get()->killProcess(mBridgePid);
    }
    mTransport.close();
    // Note, closing the shared memory region can crash the bridge as it might
    // attempt to read inaccessible memory.
    LOG(INFO) << "Stopping the rtc module.";
    mScreenAgent->stopSharedMemoryModule();
}

void WebRtcBridge::received(SocketTransport* from, json object) {
    // It's either  a message, or goodbye..
    bool message = object.count("msg") && object.count("topic");
    bool bye = object.count("bye") && object.count("topic");
    if (!message && !bye) {
        LOG(ERROR) << "Ignoring incorrect message: " << object.dump(2);
        return;
    }

    std::string dest = object["topic"];
    if (dest == BRIDGE_RECEIVER) {
        // Handle an event..
        mEventDispatcher.dispatchEvent(object);
        return;
    }

    if (bye) {
        // We are saying good bye..
        AutoWriteLock lock(mMapLock);
        if (mId.find(dest) != mId.end()) {
            auto queue = mId[dest];
            mLocks.erase(queue.get());
            mId.erase(dest);
            updateBridgeState();
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
                               << object["msg"].get<std::string>()
                               << "dropping it";
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
    std::unique_lock<std::mutex> lck(mStateLock);
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
    mCvConnected.notify_all();
}

static Optional<System::Pid> launchAsDaemon(std::string executable,
                                            int port,
                                            std::string videomodule,
                                            std::string turnConfig) {
    std::vector<std::string> cmdArgs{
            executable,    "--daemon",
            "--logdir",    System::get()->getTempDir(),
            "--discovery", ConfigDirs::getCurrentDiscoveryPath(),
            "--port",      std::to_string(port),
            "--handle",    videomodule,
            "--turn",      turnConfig};

    System::Pid bridgePid;
    std::string invoke = "";
    for (auto arg : cmdArgs) {
        invoke += arg + " ";
    }

    // This either works or not.. We are not waiting around.
    const System::Duration kHalfSecond = 500;
    System::ProcessExitCode exitCode;
    LOG(INFO) << "Launching: " << invoke;
    auto pidStr = System::get()->runCommandWithResult(cmdArgs, kHalfSecond,
                                                      &exitCode);

    if (exitCode != 0 || !pidStr) {
        // Failed to start video bridge! Mission abort!
        LOG(INFO) << "Failed to start " << invoke;
        return {};
    }
    sscanf(pidStr->c_str(), "%d", &bridgePid);
    LOG(INFO) << "Launched " << invoke << ", pid:" << bridgePid;
    return bridgePid;
}  // namespace control

static Optional<System::Pid> launchInBackground(std::string executable,
                                                int port,
                                                std::string videomodule,
                                                std::string turnConfig) {
    std::vector<std::string> cmdArgs{executable,
                                     "--logdir",
                                     System::get()->getTempDir(),
                                     "--discovery",
                                     ConfigDirs::getCurrentDiscoveryPath(),
                                     "--port",
                                     std::to_string(port),
                                     "--handle",
                                     videomodule,
                                     "--turn",
                                     turnConfig};
    System::Pid bridgePid;
    std::string invoke = "";
    for (auto arg : cmdArgs) {
        invoke += arg + " ";
    }

    LOG(INFO) << "Launching: " << invoke << " &";
    if (!System::get()->runCommand(cmdArgs, RunOptions::DontWait,
                                   System::kInfinite, nullptr, &bridgePid)) {
        // Failed to start video bridge! Mission abort!
        LOG(INFO) << "Failed to start " << invoke;
        return {};
    }

    return bridgePid;
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
    mVideoModule = mScreenAgent->startSharedMemoryModule(mFps);
    if (mVideoModule.empty()) {
        LOG(ERROR) << "Failed to start webrtc module, no video available.";
        return false;
    }

// Daemonized version is only properly supported on Linux
#ifdef __linux__
    Optional<System::Pid> bridgePid = launchAsDaemon(
            executable, mVideoBridgePort, mVideoModule, mTurnConfig);
#else
    // Windows does not have fork, darwin has security requirements that are
    // not easily met
    Optional<System::Pid> bridgePid = launchInBackground(
            executable, mVideoBridgePort, mVideoModule, mTurnConfig);
#endif

    if (!bridgePid.hasValue()) {
        LOG(ERROR) << "WebRTC bridge disabled";
        terminate();
        return false;
    }

    mBridgePid = *bridgePid;
    mTransport.connect();
    std::unique_lock<std::mutex> lock(mStateLock);
    auto future = std::chrono::system_clock::now() + std::chrono::seconds(2);
    bool timedOut = !mCvConnected.wait_until(
            lock, future, [=]() { return mState == BridgeState::Connected; });
    bool connected = mState == BridgeState::Connected;
    LOG(INFO) << "WebRtcBridge state: "
              << (connected ? "Connected" : "Disconnected")
              << "at pid: " << *bridgePid;
    return connected;
}

RtcBridge* WebRtcBridge::create(android::base::Looper* looper,
                                int port,
                                const AndroidConsoleAgents* const consoleAgents,
                                std::string turncfg) {
    if (port == 0) {
        // Find a free port.
        android::base::ScopedSocket s0(socketTcp4LoopbackServer(0));
        port = android::base::socketGetPort(s0.get());
    }

    std::string executable =
            System::get()->findBundledExecutable(kVideoBridgeExe);
    if (executable.empty()) {
        LOG(ERROR) << "WebRTC: couldn't get path to " << kVideoBridgeExe;
        return new NopRtcBridge();
    }

    AsyncSocket* socket = new AsyncSocket(looper, port);
    return new WebRtcBridge(socket, consoleAgents, kMaxFPS, port, turncfg);
}
}  // namespace control
}  // namespace emulation
}  // namespace android
