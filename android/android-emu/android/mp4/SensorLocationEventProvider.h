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

#include <stdint.h>                              // for uint64_t
#include <memory>                                // for shared_ptr

#include "android/automation/EventSource.h"      // for DurationNs, EventSource
#include "offworld.pb.h"  // for DatasetInfo
#include "automation.pb.h"                       // for RecordedEvent

extern "C" {
#include "libavcodec/avcodec.h"                  // for AVPacket
}

namespace android {
namespace mp4 {

typedef ::offworld::DatasetInfo DatasetInfo;
typedef automation::DurationNs DurationNs;
typedef automation::NextCommandStatus NextCommandStatus;
typedef emulator_automation::RecordedEvent RecordedEvent;

// SensorLocationEventProvider turns AVPackets containing sensor/location event
// information into actual event objects that could be consumed by
// AutomationController.
class SensorLocationEventProvider : public automation::EventSource {
public:
    virtual ~SensorLocationEventProvider(){};
    static std::shared_ptr<SensorLocationEventProvider> create(
            const DatasetInfo& datasetInfo);
    // Create a RecordedEvent from the packet
    virtual int createEvent(const AVPacket* packet) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    // Set the timestamp from which the playback event should start
    virtual void startFromTimestamp(uint64_t startingTimestamp) = 0;
    // Set the event playback's ending timestamp, a.k.a. the max timestamp of
    // replayable events
    virtual void setEndingTimestamp(uint64_t endingTimestamp) = 0;
    virtual RecordedEvent consumeNextCommand() = 0;
    virtual NextCommandStatus getNextCommandDelay(DurationNs* outDelay) = 0;

protected:
    SensorLocationEventProvider() = default;
};

}  // namespace mp4
}  // namespace android
