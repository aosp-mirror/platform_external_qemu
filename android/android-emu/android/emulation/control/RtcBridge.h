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

#include <string>
#include "android/base/containers/BufferQueue.h"

namespace android {
namespace emulation {
namespace control {

using MessageQueue = base::BufferQueue<std::string>;
using base::Lock;
using base::System;
// Represents a bridge to a WebRTC module that will understand
// the jsep protocol.
class RtcBridge {
public:
    enum class BridgeState { Pending, Connected, Disconnected };

    virtual ~RtcBridge() = default;

    // Connect will initiate the RTC stream if not yet in progress.
    virtual bool connect(std::string identity) = 0;

    // Disconnects the RTC stream in progress.
    virtual void disconnect(std::string identity) = 0;

    // Accept the incoming jsep message from the given identity
    virtual bool acceptJsepMessage(std::string identity, std::string msg) = 0;

    // Blocks and waits until a message becomes available for the given
    // identity. Returns the next message if one is available. Returns false and
    // a by message if the identity does not exist.
    virtual bool nextMessage(std::string identity,
                             std::string* nextMessage,
                             System::Duration blockAtMostMs) = 0;

    // Disconnect and stop the rtc bridge.
    virtual void terminate() = 0;

    // Asynchronously starts the rtc bridge..
    virtual bool start() = 0;

    virtual BridgeState state() = 0;
};

class NopRtcBridge : public RtcBridge {
public:
    NopRtcBridge() = default;
    bool connect(std::string identity) override;
    void disconnect(std::string identity) override;
    bool acceptJsepMessage(std::string identity, std::string msg) override;
    bool nextMessage(std::string identity,
                     std::string* nextMessage,
                     System::Duration blockAtMostMs) override;
    void terminate() override;
    bool start() override;
    BridgeState state() override;
};
}  // namespace control
}  // namespace emulation
}  // namespace android