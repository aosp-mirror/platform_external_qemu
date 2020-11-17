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

#include <nlohmann/json.hpp>

#include "android/emulation/control/RtcBridge.h"  // for RtcBridge...
#include "emulator/net/EmulatorGrcpClient.h"      // for Emula...
#include "emulator/webrtc/capture/GoldfishAudioDeviceModule.h"
#include "emulator/webrtc/capture/VideoCapturerFactory.h"

namespace emulator {

namespace net {
class AsyncSocketAdapter;
class EmulatorConnection;
}  // namespace net

namespace webrtc {

class Participant;

using json = nlohmann::json;
using android::emulation::control::RtcBridge;
using MessageQueue = android::base::BufferQueue<std::string>;
using android::base::Lock;
using android::base::ReadWriteLock;
using android::base::System;

// A switchboard manages a set of web rtc participants:
//
// 1. It creates new participants when a user starts a session
// 2. It removes participants when a user disconnects
// 3. It routes the webrtc json signals to the proper participant.
// 4. Participants that are no longer streaming need to be finalized.
class Switchboard : public RtcBridge {
public:
    Switchboard(EmulatorGrpcClient client,
                const std::string& shmPath,
                const std::string& turnconfig);
    ~Switchboard();

    // Connect will initiate the RTC stream if not yet in progress.
    bool connect(std::string identity) override;

    // Disconnects the RTC stream in progress.
    void disconnect(std::string identity) override;

    // Accept the incoming jsep message from the given identity
    bool acceptJsepMessage(std::string identity, std::string msg) override;

    // Blocks and waits until a message becomes available for the given
    // identity. Returns the next message if one is available. Returns false and
    // a by message if the identity does not exist.
    bool nextMessage(std::string identity,
                     std::string* nextMessage,
                     System::Duration blockAtMostMs) override;

    // Disconnect and stop the rtc bridge.
    void terminate() override{};

    // Asynchronously starts the rtc bridge..
    bool start() override { return true; };

    BridgeState state() override { return BridgeState::Connected; };

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
    EmulatorGrpcClient& emulatorClient() { return mClient; }

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
    const std::string mShmPath = "/tmp";
    std::vector<std::string>
            mTurnConfig;  // Process to invoke to retrieve turn config.

    // Maximum number of messages we are willing to queue, before we start
    // dropping them.
    static const uint16_t kMaxMessageQueueLen = 128;

    GoldfishAudioDeviceModule mGoldfishAdm;
    VideoCapturerFactory mCaptureFactory;
    // Worker threads for all the participants.
    std::unique_ptr<rtc::Thread> mWorker;
    std::unique_ptr<rtc::Thread> mSignaling;
    std::unique_ptr<rtc::Thread> mNetwork;
    std::unique_ptr<::webrtc::TaskQueueFactory> mTaskFactory;

    rtc::scoped_refptr<::webrtc::PeerConnectionFactoryInterface>
            mConnectionFactory;

    // Message queues used to store messages received from the videobridge.
    std::map<std::string, std::shared_ptr<MessageQueue>> mId;
    std::map<MessageQueue*, std::shared_ptr<Lock>> mLocks;
    ReadWriteLock mMapLock;

    // Network/communication things.
    EmulatorGrpcClient mClient;
    rtc::CriticalSection mCleanupCS;
    rtc::CriticalSection mCleanupClosedCS;
};
}  // namespace webrtc
}  // namespace emulator
