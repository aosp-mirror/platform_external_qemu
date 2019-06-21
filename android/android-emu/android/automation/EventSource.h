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

#include "android/automation/proto/automation.pb.h"

namespace pb = emulator_automation;

namespace android {
namespace automation {

// An EventSource provides a series of RecordedEvent's to AutomationController
class EventSource {
public:
    virtual ~EventSource() = default;

    // Return the next command from the source.
    virtual pb::RecordedEvent consumeNextCommand() = 0;

    // Get the next command delay from the event source.  Returns false if there
    // are no events remaining.
    virtual bool getNextCommandDelay(DurationNs* outDelay) = 0;
};

}  // namespace automation
}  // namespace android