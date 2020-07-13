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

#include <api/peer_connection_interface.h>
#include <rtc_base/critical_section.h>  // for CritS...
#include <rtc_base/thread.h>            // for Thread

#include "emulator/net/JsonProtocol.h"     // for JsonProtocol (ptr only)
#include "emulator/net/SocketTransport.h"  // for SocketTransport (ptr only)
#include "emulator/webrtc/capture/VideoCapturerFactory.h"
#include "emulator/webrtc/capture/GoldfishAudioDeviceModule.h"

namespace emulator {

namespace net {
class AsyncSocketAdapter;
class EmulatorConnection;
}  // namespace net

namespace webrtc {

class Participant;

using net::AsyncSocketAdapter;
using net::EmulatorConnection;
using net::JsonProtocol;
using net::JsonReceiver;
using net::SocketTransport;
using net::State;

// A switchboard manages a set of web rtc participants:
//
// 1. It creates new participants when a user starts a session
// 2. It removes participants when a user disconnects
// 3. It routes the webrtc json signals to the proper participant.
// 4. Participants that are no longer streaming need to be finalized.
class Switchboard : public JsonReceiver {
public:
    Switchboard(const std::string& discoveryFile,
                const std::string& handle,
                const std::string& turnconfig,
                AsyncSocketAdapter* connection,
                EmulatorConnection* parent);
    ~Switchboard();
    void received(SocketTransport* from, const json object) override;
    void stateConnectionChange(SocketTransport* connection,
                               State current) override;
    void send(std::string to, json msg);

    // Called when a participant is unable to continue the rtc stream.
    // The participant will no longer be in use and close can be called.
    void rtcConnectionDropped(std::string participant);

    // The connection has actually closed, and can be properly garbage
    // collected.
    void rtcConnectionClosed(std::string participant);

    // Cleans up connections that have marked themselves as deleted
    // due to a dropped connection.
    void finalizeConnections();

    VideoCapturerFactory* getVideoCaptureFactory() { return &mCaptureFactory; }

    static std::string BRIDGE_RECEIVER;

private:
    // We want the turn config delivered in under a second.
    const int kMaxTurnConfigTime = 1000;
    std::unordered_map<std::string, rtc::scoped_refptr<Participant>>
            mConnections;
    std::unordered_map<std::string, std::string> mIdentityMap;

    std::vector<std::string> mDroppedConnections;  // Connections that need to
                                                   // be garbage collected.
    std::vector<std::string> mClosedConnections;
    const std::string mHandle = "video0";  // Handle to shared memory region
    const std::string mDiscoveryFile;      // Emulator discovery file.
    std::vector<std::string>
            mTurnConfig;  // Process to invoke to retrieve turn config.
    int32_t mFps = 24;    // Desired fps

    GoldfishAudioDeviceModule mGoldfishAdm;
     VideoCapturerFactory mCaptureFactory;
    // Worker threads for all the participants.
    std::unique_ptr<rtc::Thread> mWorker;
    std::unique_ptr<rtc::Thread> mSignaling;
    std::unique_ptr<rtc::Thread> mNetwork;
    std::unique_ptr<::webrtc::TaskQueueFactory> mTaskFactory;

    rtc::scoped_refptr<::webrtc::PeerConnectionFactoryInterface>
            mConnectionFactory;

    // Network/communication things.
    JsonProtocol mProtocol;
    SocketTransport mTransport;
    net::EmulatorConnection* mEmulator;
    rtc::CriticalSection mCleanupCS;
    rtc::CriticalSection mCleanupClosedCS;
};
}  // namespace webrtc
}  // namespace emulator
