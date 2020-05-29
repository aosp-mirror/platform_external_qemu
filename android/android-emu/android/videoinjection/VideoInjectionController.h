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

#include "android/base/Optional.h"
#include "android/base/Result.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "offworld.pb.h"
#include <string>

namespace android {
namespace videoinjection {

enum class VideoInjectionError {
    InvalidFilename,
    FileOpenError,
    InvalidRequest,
    FileIsNotLoaded,
    AnotherVideoIsPlaying,
    VideoIsNotStarted,
    InternalError,
};

struct RequestContext {
    ::offworld::VideoInjectionRequest request;
    base::Optional<android::AsyncMessagePipeHandle> pipe;
    uint32_t asyncId;
};

using VideoInjectionResult = base::Result<void, VideoInjectionError>;
std::ostream& operator<<(std::ostream& os, const VideoInjectionError& value);

//
// Receives video injection requests from the offworld pipe. Provides requests
// to its consumer, for instance the video playback camera.
//

class VideoInjectionController {
protected:
    DISALLOW_COPY_AND_ASSIGN(VideoInjectionController);

    // VideoInjectionController is a singleton, use get() to get an instance.
    VideoInjectionController() = default;
public:
    virtual ~VideoInjectionController() = default;

    // Initialize the VideoInjectionController, called in qemu-setup.cpp.
    static void initialize();

    // Shutdown the VideoInjectionController, called in qemu-setup.cpp.
    static void shutdown();

    // Get the global instance of the VideoInjectionController. Asserts if
    // called before initialize().
    static VideoInjectionController& get();

    // Create an instance for test usage.
    static std::unique_ptr<VideoInjectionController> createForTest(
        std::function<void(android::AsyncMessagePipeHandle,
                           const ::offworld::Response&)>
                sendMessageCallback = nullptr);

    // Return the next video injection request in the queue if any and the
    // VideoInjectionController has been created. The
    static base::Optional<android::videoinjection::RequestContext> tryGetNextRequestContext();

    //Try to send the async response back to java video injection controller
    static void trySendAsyncResponse(
            uint32_t async_id,
            VideoInjectionResult result,
            bool isCompleted,
            const std::string& errorDetails);

    // Reset the current state. Clear the queue. Send response for any pending
    // request.
    virtual void reset() = 0;

    // Handles the previous request execution result.
    //
    // Returns the next video injection request from the queue if any.
    virtual base::Optional<android::videoinjection::RequestContext> getNextRequestContext() = 0;

    //
    // Offworld API
    //

    // Handle a video injection request and add it into the queue.
    virtual VideoInjectionResult handleRequest(
        android::AsyncMessagePipeHandle pipe,
        ::offworld::VideoInjectionRequest event,
        uint32_t asyncId) = 0;

    // Called on pipe close, to cancel any pending operations.
    virtual void pipeClosed(android::AsyncMessagePipeHandle pipe) = 0;

    // Send async response containing error message to java video injection
    // controller.
    virtual void sendFollowUpAsyncResponse(
            uint32_t async_id,
            android::videoinjection::VideoInjectionResult result,
            bool isCompleted,
            const std::string& errorDetails) = 0;
};

}  // namespace videoinjection
}  // namespace android
