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

#include <memory>

#include "android/automation/EventSource.h"
#include "android/base/Compiler.h"
#include "android/base/Result.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"

// Forward declarations.
typedef struct PhysicalModel PhysicalModel;

namespace offworld {
class Response;
}  // namespace offworld

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

enum class StopError { NotStarted, ParseError };
using StopResult = base::Result<void, StopError>;
std::ostream& operator<<(std::ostream& os, const StopError& value);

enum class ReplayError {
    PlaybackInProgress,
    ParseError,
    InternalError,
};

using ReplayResult = base::Result<void, ReplayError>;
std::ostream& operator<<(std::ostream& os, const ReplayError& value);

enum class ListenError {
    AlreadyListening,
    StartFailed,
};

using ListenResult = base::Result<std::string, ListenError>;
std::ostream& operator<<(std::ostream& os, const ListenError& value);

//
// Controls recording and playback of emulator automation events.
//

class AutomationController {
protected:
    DISALLOW_COPY_AND_ASSIGN(AutomationController);

    // AutomationController is a singleton, use get() to get an instance.
    AutomationController() = default;

public:
    virtual ~AutomationController() = default;

    // Initialize the AutomationController, called in qemu-setup.cpp.
    static void initialize();

    // Shutdown the AutomationController, called in qemu-setup.cpp.
    static void shutdown();

    // Get the global instance of the AutomationController.  Asserts if called
    // before initialize().
    static AutomationController& get();

    // Create an instance of the Looper for test usage.
    static std::unique_ptr<AutomationController> createForTest(
            PhysicalModel* physicalModel,
            base::Looper* looper,
            std::function<void(android::AsyncMessagePipeHandle,
                               const ::offworld::Response&)>
                    sendMessageCallback = nullptr);

    // Advance the time if the AutomationController has been created.
    static void tryAdvanceTime();

    // Get the event sink for recording automation events.
    virtual AutomationEventSink& getEventSink() = 0;

    // Reset the current state and cancel any recordings or playback.
    // Called on snapshot restore, since playback cannot be trivially resumed.
    virtual void reset() = 0;

    // Advance the state and process any playback events.
    // Note that it is *not safe* to call this from a PhysicalModel callback.
    //
    // Returns the current time.
    virtual DurationNs advanceTime() = 0;

    // Start a recording to a file.
    virtual StartResult startRecording(android::base::StringView filename) = 0;

    // Stops a recording to a file.
    virtual StopResult stopRecording() = 0;

    // Start a playback from an EventSource.
    virtual StartResult startPlaybackFromSource(
            std::shared_ptr<EventSource> source) = 0;

    // Start a playback from a file.
    virtual StartResult startPlayback(android::base::StringView filename) = 0;

    // Stop playback from a file.
    virtual StopResult stopPlayback() = 0;

    // Start playback with stop callback.
    virtual StartResult startPlaybackWithCallback(
            android::base::StringView filename,
            void (*onStopCallback)()) = 0;

    // Set the macro name in the header of a file.
    virtual void setMacroName(android::base::StringView macroName,
                              android::base::StringView filename) = 0;

    // Get the macro name from the header of a file.
    virtual std::string getMacroName(android::base::StringView filename) = 0;

    // Get the duration in ns and datetime in ms from a file.
    virtual std::pair<uint64_t, uint64_t> getMetadata(
            android::base::StringView filename) = 0;

    // Get the current timestamp of the looper used by the controller
    virtual uint64_t getLooperNowTimestamp() = 0;

    // Set the timestamp at which the next playback command should be executed
    virtual void setNextPlaybackCommandTime(DurationNs nextTimestamp) = 0;

    //
    // Offworld API
    //

    // Replay an initial state event from the offworld pipe.
    virtual ReplayResult replayInitialState(
            android::base::StringView state) = 0;

    // Replay an event through from the offworld pipe.
    //
    // If successful, replayEvent will send an async response to the pipe handle
    // when replay has completed.  The provided async id will be sent with that
    // response.  If an error is returned, no messages will be sent to the pipe
    // and the caller should propagate the error to the user.
    virtual ReplayResult replayEvent(android::AsyncMessagePipeHandle pipe,
                                     android::base::StringView event,
                                     uint32_t asyncId) = 0;

    virtual void sendEvent(const emulator_automation::RecordedEvent& event) = 0;

    // Listen to the stream of automation events, returns an error if there is
    // already a pending listen operations.
    //
    // If successful, any additional async responses will include the provided
    // asyncId.
    virtual ListenResult listen(android::AsyncMessagePipeHandle pipe,
                                uint32_t asyncId) = 0;

    virtual void stopListening() = 0;

    // Called on pipe close, to cancel any pending operations.
    virtual void pipeClosed(android::AsyncMessagePipeHandle pipe) = 0;
};

}  // namespace automation
}  // namespace android
