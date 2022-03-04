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
#include <api/scoped_refptr.h>                           // for scoped_refptr
#include <grpcpp/grpcpp.h>                               // for ClientContext
#include <memory>                                        // for unique_ptr
#include <mutex>                                         // for mutex
#include <string>                                        // for string
#include <thread>                                        // for thread

#include "emulator/webrtc/RtcConnection.h"               // for RtcConnection
#include "emulator/webrtc/capture/VideoTrackReceiver.h"  // for VideoTrackRe...
#include "rtc_service.pb.h"                              // for RtcId

#include "android/base/async/AsyncSocketServer.h"
#include "emulator/net/SocketForwarder.h"
#include "android/emulation/control/TurnConfig.h"


namespace android {
namespace emulation {
namespace control {
class KeyboardEvent;
class MouseEvent;
class TouchEvent;
}  // namespace control
}  // namespace emulation
}  // namespace android

namespace emulator {
namespace webrtc {
class Participant;

using android::base::AsyncSocketServer;
using android::emulation::control::KeyboardEvent;
using android::emulation::control::MouseEvent;
using android::emulation::control::RtcId;
using android::emulation::control::TouchEvent;
using android::emulation::control::TurnConfig;

using ClosedCallback = std::function<void()>;
// A StandaloneConnection is a single connection to a remote running emulator
// over webrtc.
class StandaloneConnection : public RtcConnection {
public:
    StandaloneConnection(EmulatorGrpcClient* client, int adbPort, TurnConfig turnconfig);
    ~StandaloneConnection();

    // The connection has actually closed, and can be properly garbage
    // collected.
    void rtcConnectionClosed(const std::string participant) override;

    // Used by participant to forward jsep messages.
    void send(std::string to, json msg) override;

    // Connect and start the jsep protocol
    void connect();

    // True if a participant is active.
    bool isActive() { return !mParticipant.expired(); }

    // The receiver of the actual video frames.
    VideoTrackReceiver* videoReceiver() { return &mVideoReceiver; };

    // Sends a key event over the "keyboard" data channel
    void sendKey(const KeyboardEvent* keyEvent);

    // Sends a mouse event over the "mouse" data channel
    void sendMouse(const MouseEvent* mouseEvent);

    // Sends a touch event over the "touch" data channel
    void sendTouch(const TouchEvent* touchEvent);

    void registerConnectionClosedListener(ClosedCallback callback);

    // Disconnect the ongoing webrtc connection.
    void close();

private:
    void driveJsep();
    bool connectAdbSocket(int fd);
    std::shared_ptr<AsyncSocketServer> createAdbForwarder();

    // Owned by the jsep thread.
    std::weak_ptr<Participant> mParticipant;
    std::weak_ptr<grpc::ClientContext> mCtx;
    std::weak_ptr<AsyncSocketServer> mAdbSocketServer;

    std::unique_ptr<std::thread> mJsepThread;

    std::vector<net::SocketForwarder> mAdbForwarders;
    VideoTrackReceiver mVideoReceiver;
    RtcId mGuid;
    bool mAlive{false};
    int mAdbPort = 0;
    ClosedCallback mClosedCallback;
    TurnConfig mTurnConfig;

    DISALLOW_COPY_AND_ASSIGN(StandaloneConnection);
};
}  // namespace webrtc
}  // namespace emulator
