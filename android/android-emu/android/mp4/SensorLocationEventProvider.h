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

#include <memory>

#include "android/automation/AutomationController.h"
#include "android/automation/EventSource.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

namespace android {
namespace mp4 {

// SensorLocationEventProvider turns packets containing sensor/location event
// information into actual event objects that could be consumed by
// AutomationController.
class SensorLocationEventProvider : public automation::EventSource {
public:
    virtual ~SensorLocationEventProvider() = default;
    static std::unique_ptr<SensorLocationEventProvider> create();
    // Create a RecordedEvent from the packet
    virtual int createEvent(AVPacket* packet) { return -1; };
    pb::RecordedEvent consumeNextCommand();
    bool getNextCommandDelay(automation::DurationNs* outDelay);

protected:
    SensorLocationEventProvider() = default;
};

}  // namespace mp4
}  // namespace android