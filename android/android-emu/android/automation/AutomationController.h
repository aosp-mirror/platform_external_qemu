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

#include "android/base/Compiler.h"
#include "android/base/Result.h"
#include "android/utils/looper.h"  // For DurationNs

namespace android { 
namespace automation {

// Forward declarations.
class AutomationEventSink;

enum class StartError {
    InvalidFilename,
    FileOpenError,
    AlreadyStarted,
    InternalError,
    PlaybackFileCorrupt
};
using StartResult = base::Result<void, StartError>;
std::ostream& operator<<(std::ostream& os, const StartError& value);

enum class StopError { NotStarted };
using StopResult = base::Result<void, StopError>;
std::ostream& operator<<(std::ostream& os, const StopError& value);

//
// Controls recording and playback of emulator automation events.
//

class AutomationController {
protected:
    DISALLOW_COPY_AND_ASSIGN(AutomationController);

    // AutomationController is a singleton, use get() to get an instance.
    AutomationController() = default;
    virtual ~AutomationController() = default;

public:
    // Get the global instance of the AutomationController.
    static AutomationController& get();

    // Get the event sink for recording automation events.
    virtual AutomationEventSink& getEventSink() = 0;

    // Advance the state and process any playback events.
    // Returns the current time.
    virtual DurationNs advanceTime() = 0;

    // Start a recording to a file.
    virtual StartResult startRecording(android::base::StringView filename) = 0;

    // Stops a recording to a file.
    virtual StopResult stopRecording() = 0;

    // Start a playback from a file.
    virtual StartResult startPlayback(android::base::StringView filename) = 0;

    // Stop playback from a file.
    virtual StopResult stopPlayback() = 0;
};

}  // namespace automation
}  // namespace android
