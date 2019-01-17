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

#include "android/automation/proto/automation.pb.h"
#include "android/base/Result.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"

// Forward declarations.
namespace offworld {
class Response;
}  // namespace offworld

namespace android {
namespace automation {

namespace pb = emulator_automation;

enum class VideoInjectionError {
    InvalidFilename,
    FileOpenError,
    InvalidCommand,
    FileIsNotLoaded,
    AnotherVideoIsPlaying,
    VideoIsNotStarted,
    InternalError,
};

using VideoInjectionResult = base::Result<void, VideoInjectionError>;
std::ostream& operator<<(std::ostream& os, const VideoInjectionError& value);

//
// Receives automation events from other emulator subsystems so that they may
// be recorded or streamed.
//

class VideoInjectionManager {
DISALLOW_COPY_AND_ASSIGN(VideoInjectionManager);
public:

    virtual ~VideoInjectionManager() = default;

    virtual VideoInjectionResult addCommand(
        android::AsyncMessagePipeHandle pipe,
        android::base::StringView command,
        uint32_t asyncId) = 0;

    virtual std::unique_ptr<pb::VideoInjectionCommand> getNextCommand(
        VideoInjectionResult previousResult) = 0;

    static std::unique_ptr<VideoInjectionManager> create(
        std::function<void(android::AsyncMessagePipeHandle,
                           const ::offworld::Response&)> sendMessageCallback);

protected:
    VideoInjectionManager() = default;
};

}  // namespace automation
}  // namespace android