// Copyright (C) 2018 The Android Open Source Project
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

#include "automation.pb.h"
#include "android/base/Compiler.h"
#include "android/base/async/Looper.h"
#include "android/base/files/Stream.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_map>
#include <unordered_set>

namespace android {
namespace automation {

namespace pb = emulator_automation;

enum class StreamEncoding { BinaryPbChunks, TextPb };

// Forward declarations.
class AutomationController;
class AutomationControllerImpl;

//
// Receives automation events from other emulator subsystems so that they may
// be recorded or streamed.
//

class AutomationEventSink {
    DISALLOW_COPY_AND_ASSIGN(AutomationEventSink);
    friend class AutomationControllerImpl;

    // Can only be created by AutomationController.
    AutomationEventSink(base::Looper* looper);

public:
    // Shutdown the event sink.
    void shutdown();

    // Register a stream to write recorded events out to.
    // |stream| - Stream pointer, assumes that the pointer remains valid until
    //            unregisterStream is called.
    // |encoding| - Encoding to write messages with.
    void registerStream(android::base::Stream* stream, StreamEncoding encoding);

    // Unregister a stream.
    void unregisterStream(android::base::Stream* stream);

    // Record an event from the physical model.
    void recordPhysicalModelEvent(uint64_t timeNs,
                                  pb::PhysicalModelEvent& event);

    // Get the last event time for a specific stream.
    uint64_t getLastEventTimeForStream(android::base::Stream* stream);

private:
    void handleEvent(uint64_t timeNs, const pb::RecordedEvent& event);

    base::Looper* const mLooper;

    android::base::Lock mLock;
    bool mShutdown = false;
    std::unordered_set<android::base::Stream*> mBinaryStreams;
    std::unordered_set<android::base::Stream*> mTextStreams;

    std::unordered_map<android::base::Stream*, uint64_t> mLastEventTime;
};

}  // namespace automation
}  // namespace android
