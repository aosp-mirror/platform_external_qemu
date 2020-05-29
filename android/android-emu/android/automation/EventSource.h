// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <chrono>

#include "automation.pb.h"
#include "android/base/async/Looper.h"

namespace android {
namespace automation {

typedef android::base::Looper::DurationNs DurationNs;

// The status when requesting the next command
enum class NextCommandStatus {
    NONE,
    OK,
    PAUSED,
};

// An EventSource provides a series of RecordedEvent's to AutomationController
class EventSource {
public:
    virtual ~EventSource() = default;

    // Return the next command from the source.
    virtual emulator_automation::RecordedEvent consumeNextCommand() = 0;

    // Get the next command delay from the event source. Returns:
    // - OK if the next event is ready to be consumed
    // - PAUSED if the next event exists but is not ready to be consumed
    // - NONE if there are no events remaining.
    virtual NextCommandStatus getNextCommandDelay(DurationNs* outDelay) = 0;
};

}  // namespace automation
}  // namespace android
