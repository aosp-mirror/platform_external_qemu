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
#include <api/scoped_refptr.h>                             // for scoped_refptr
#include <stdint.h>                                        // for uint16_t
#include <map>                                             // for map
#include <memory>                                          // for shared_ptr
#include <string>                                          // for string
#include <unordered_map>                                   // for unordered_map
#include <vector>                                          // for vector

#include "android/base/containers/BufferQueue.h"           // for BufferQueue
#include "android/base/synchronization/Lock.h"             // for Lock (ptr ...
#include "android/base/system/System.h"                    // for System
#include "android/emulation/control/RtcBridge.h"           // for RtcBridge:...
#include "emulator/webrtc/RtcConnection.h"                 // for RtcConnection
#include "android/emulation/control/TurnConfig.h"          // for TurnConfig
#include "emulator/webrtc/capture/VideoCapturerFactory.h"  // for VideoCaptu...

namespace emulator {
namespace webrtc {
class Participant;

using android::emulation::control::RtcBridge;
using MessageQueue = android::base::BufferQueue<std::string>;
using android::base::Lock;
using android::base::ReadWriteLock;
using android::base::System;
using android::emulation::control::TurnConfig;

// A switchboard manages a set of web rtc participants:
//
// 1. It creates new participants when a user starts a session
// 2. It removes participants when a user disconnects
// 3. It routes the webrtc json signals to the proper participant.
// 4. Participants that are no longer streaming need to be finalized.
class Switchboard : public RtcBridge, public RtcConnection {
public:
    Switchboard(EmulatorGrpcClient* client,
                const std::string& shmPath,
                TurnConfig turnconfig,
                int adbPort,
                const std::string& audioDumpFile="");
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

    void send(std::string to, json msg) override;

    // Called when a participant is unable to continue the rtc stream.
    // The participant will no longer be in use and close can be called.
    void rtcConnectionClosed(const std::string participant) override;

    VideoCapturerFactory* getVideoCaptureFactory() { return &mCaptureFactory; }

    static std::string BRIDGE_RECEIVER;

private:
    std::unordered_map<std::string, std::shared_ptr<Participant>>
            mConnections;

    VideoCapturerFactory mCaptureFactory;

    const std::string mShmPath = "/tmp";
    int mAdbPort{0};

    // Turn configuration object.
    TurnConfig mTurnConfig;

    // Maximum number of messages we are willing to queue, before we start
    // dropping them.
    static const uint16_t kMaxMessageQueueLen = 128;

    // Message queues used to store messages received from the videobridge.
    std::map<std::string, std::shared_ptr<MessageQueue>> mId;
    std::map<MessageQueue*, std::shared_ptr<Lock>> mLocks;
    ReadWriteLock mMapLock;

    // Network/communication things.
    std::mutex mCleanupMutex;

    // For gRPC audio Dump
    std::string mAudioDumpFile;
};
}  // namespace webrtc
}  // namespace emulator
