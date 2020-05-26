// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>                                         // for uint16_t
#include <map>                                              // for map
#include <memory>                                           // for shared_ptr
#include <mutex>                                            // for condition...
#include <string>                                           // for string

#include "android/base/containers/BufferQueue.h"            // for BufferQueue
#include "android/base/synchronization/Lock.h"              // for Lock (ptr...
#include "android/base/system/System.h"                     // for System
#include "android/console.h"                                // for AndroidCo...
#include "android/emulation/control/EventDispatcher.h"      // for EventDisp...
#include "android/emulation/control/RtcBridge.h"            // for RtcBridge...
#include "android/emulation/control/record_screen_agent.h"  // for QAndroidR...
#include "emulator/net/JsonProtocol.h"                      // for JsonProtocol
#include "emulator/net/SocketTransport.h"                   // for SocketTra...

namespace emulator {
namespace net {
class AsyncSocketAdapter;
}  // namespace net
}  // namespace emulator

namespace android {
namespace base {
class Looper;
}  // namespace base

namespace emulation {
namespace control {

using MessageQueue = base::BufferQueue<std::string>;
using base::Lock;
using base::ReadWriteLock;
using emulator::net::AsyncSocketAdapter;
using emulator::net::JsonProtocol;
using emulator::net::JsonReceiver;
using emulator::net::SocketTransport;
using emulator::net::State;

// The WebRtcBridge is responsible for marshalling the message from the gRPC
// endpoint to the actual goldfish-webrtc-videobridge. It will:
//
// - Launch the video bridge
// - Start the webrtc module inside the emulator
// - Attempt to open a socket connection to the video bridge
// - Forward/Receive messages from the goldfish video bridge.
// - Terminate the video bridge on shutdown
//
// Messages send to the video bridge will be send immediately, messages received
// from the video bridge will be stored in a message queue, until a client
// requests it.
//
// Note: the videobridge will send a bye message to the webrtc bridge when a
// connection was removed, this will cleanup the message buffer.
class WebRtcBridge : public RtcBridge, public JsonReceiver {
public:
    WebRtcBridge(AsyncSocketAdapter* socket,
                 const AndroidConsoleAgents* const consoleAgents,
                 int desiredFps,
                 int videoBridgePort,
                 std::string turncfg = "");
    ~WebRtcBridge();

    bool connect(std::string identity) override;
    void disconnect(std::string identity) override;
    bool acceptJsepMessage(std::string identity, std::string msg) override;
    bool nextMessage(std::string identity,
                     std::string* nextMessage,
                     System::Duration blockAtMostMs) override;
    void terminate() override;
    bool start() override;
    BridgeState state() override;

    // Returns a webrtc bridge, or NopBridge in case of failures..
    static RtcBridge* create(Looper* looper,
                             int port,
                             const AndroidConsoleAgents* const consoleAgents,
                             std::string turncfg);

    // Socket events..
    void received(SocketTransport* from, const json object) override;
    void stateConnectionChange(SocketTransport* connection,
                               State current) override;

    // Default framerate we will use..
    // The emulator will produce frames at this rate, and the encoder in
    // the video bridge will run at this framerate as well.
    static const int kMaxFPS = 24;
    static const std::string kVideoBridgeExe;

    // Maximum number of messages we are willing to queue, before we start
    // dropping them.
    static const uint16_t kMaxMessageQueueLen = 128;

    static std::string BRIDGE_RECEIVER;

private:
    void received(std::string msg);
    void updateBridgeState();

    int mFps = kMaxFPS;
    int mVideoBridgePort;
    System::Pid mBridgePid;
    std::string mVideoModule;
    std::string mTurnConfig = "";
    BridgeState mState = BridgeState::Disconnected;

    // Needed to start/stop the emulators streaming rtc module..
    const QAndroidRecordScreenAgent* const mScreenAgent;
    EventDispatcher mEventDispatcher;

    // Message queues used to store messages received from the videobridge.
    std::map<std::string, std::shared_ptr<MessageQueue>> mId;
    std::map<MessageQueue*, std::shared_ptr<Lock>> mLocks;
    ReadWriteLock mMapLock;

    // Transport layer.
    SocketTransport mTransport;
    JsonProtocol mProtocol;
    std::mutex mStateLock;
    std::condition_variable mCvConnected;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
