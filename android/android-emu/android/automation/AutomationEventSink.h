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

#include "android/automation/proto/automation.pb.h"
#include "android/base/Compiler.h"
#include "android/base/files/Stream.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_set>

namespace android {
namespace automation {

namespace pb = emulator_automation;

// Receives automation events from other emulator subsystems so that they may
// be recorded or streamed.

enum class StreamEncoding { BinaryPbChunks, TextPb };

// Forward declarations.
class AutomationControllerImpl;

class AutomationEventSink {
    DISALLOW_COPY_AND_ASSIGN(AutomationEventSink);
    friend class AutomationControllerImpl;

    // Can only be created by AutomationController.
    AutomationEventSink() = default;

public:
    // Helper method to the global instance of the AutomationEventSink,
    // equivalent to calling AnimationController::get().getEventSink().
    static AutomationEventSink& get();

    // Register a stream to write recorded events out to.
    // |stream| - Stream pointer, assumes that the pointer remains valid until
    //            unregisterStream is called.
    // |encoding| - Encoding to write messages with.
    void registerStream(android::base::Stream* stream, StreamEncoding encoding);

    // Unregister a stream.
    void unregisterStream(android::base::Stream* stream);

    // TODO(jwmcglynn): Create a time source that we can use to get
    // current time.
    void recordPhysicalModelEvent(pb::Time& time,
                                  pb::PhysicalModelEvent& event);

private:
    void handleEvent(pb::RecordedEvent& event);

    android::base::Lock mLock;
    std::unordered_set<android::base::Stream*> mBinaryStreams;
    std::unordered_set<android::base::Stream*> mTextStreams;
};

}  // namespace automation
}  // namespace android
