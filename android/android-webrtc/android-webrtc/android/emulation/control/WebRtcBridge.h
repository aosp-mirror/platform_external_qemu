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

#include <map>
#include <memory>
#include <string>
#include "android/base/threads/FunctorThread.h"
#include "android/console.h"
#include "android/emulation/control/RtcBridge.h"

namespace android {
namespace emulation {
namespace control {

using MessageQueue = base::BufferQueue<std::string>;
using base::Lock;
using base::ReadWriteLock;

// The WebRtcBridge is responsible for marshalling the message from the gRPC
// endpoint to the actual goldfish-webrtc-videobridge. It will:
//
// - Launch the video bridge
// - Start the webrtc module inside the emulator
// - Attempt to open a socket connection to the video bridge
// - Forward/Receive messages from the goldfish video bridge.
//
// Messages send to the video bridge will be send immediately, messages received
// from the video bridge will be stored in a message queue, until a client
// requests it.
//
// Note: the videobridge will send a bye message to the webrtc bridge when a
// connection was removed, this will cleanup the message buffer.
class WebRtcBridge : public RtcBridge {
public:
    ~WebRtcBridge();

    bool connect(std::string identity) override;
    void disconnect(std::string identity) override;
    bool acceptJsepMessage(std::string identity, std::string msg) override;
    bool nextMessage(std::string identity,
                     std::string* nextMessage,
                     System::Duration blockAtMostMs) override;
    void terminate() override;

    // Returns a webrtc bridge, or NopBridge in case of failures..
    static RtcBridge* create(int port,
                             const AndroidConsoleAgents* const consoleAgents);
    bool run();

private:
    void received(std::string msg);
    bool openConnection();
    WebRtcBridge(int port, const QAndroidRecordScreenAgent* const screenAgent);

    static const std::string kVideoBridgeExe;
    const uint16_t kRecBufferSize = 0xffff;

    bool mStop = false;

    // Needed to start/stop the emulators streaming rtc module..
    const QAndroidRecordScreenAgent* const mScreenAgent;

    // Message queues used to store messages received from the videobridge.
    std::map<std::string, MessageQueue*> mId;
    std::map<MessageQueue*, Lock*> mLocks;
    ReadWriteLock mMapLock;

    // Thread, socket and port we use to communicate with the goldfish video
    // bridge.
    int mSo;
    int mPort;
    std::unique_ptr<base::FunctorThread> mThread;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
