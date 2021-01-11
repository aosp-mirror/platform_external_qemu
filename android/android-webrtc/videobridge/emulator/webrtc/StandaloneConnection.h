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
class EmulatorGrpcClient;
class Participant;

using android::emulation::control::KeyboardEvent;
using android::emulation::control::MouseEvent;
using android::emulation::control::RtcId;
using android::emulation::control::TouchEvent;
using android::base::AsyncSocketServer;

// A StandaloneConnection is a single connection to a remote running emulator
// over webrtc.
class StandaloneConnection : public RtcConnection {
public:
    StandaloneConnection(EmulatorGrpcClient* client, int adbPort);
    ~StandaloneConnection();

    // Called when a participant is unable to continue the rtc stream.
    // The participant will no longer be in use and close can be called.
    void rtcConnectionDropped(std::string participant) override;

    // The connection has actually closed, and can be properly garbage
    // collected.
    void rtcConnectionClosed(std::string participant) override;

    // Used by participant to forward jsep messages.
    void send(std::string to, json msg) override;

    // Connect and start the jsep protocol
    void connect();

    // Disconnect the ongoing webrtc connection.
    void close();

    // True if a participant is active.
    bool isActive() { return mParticipant != nullptr; }

    // The receiver of the actual video frames.
    VideoTrackReceiver* videoReceiver() { return &mVideoReceiver; };

    // Sends a key event over the "keyboard" data channel
    void sendKey(const KeyboardEvent* keyEvent);

    // Sends a mouse event over the "mouse" data channel
    void sendMouse(const MouseEvent* mouseEvent);

    // Sends a touch event over the "touch" data channel
    void sendTouch(const TouchEvent* touchEvent);

private:
    void driveJsep();
    bool connectAdbSocket(int fd);
    bool createAdbForwarder();

    rtc::scoped_refptr<Participant> mParticipant;
    std::unique_ptr<std::thread> mJsepThread;
    std::unique_ptr<grpc::ClientContext> mCtx;


    std::unique_ptr<AsyncSocketServer> mAdbSocketServer;
    std::vector<net::SocketForwarder> mAdbForwarders;
    VideoTrackReceiver mVideoReceiver;
    RtcId mGuid;
    std::mutex mCloseMutex;
    bool mAlive{false};
    int mAdbPort = 0;
};
}  // namespace webrtc
}  // namespace emulator
