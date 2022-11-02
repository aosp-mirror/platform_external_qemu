// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once
#include <absl/types/optional.h>
#include <api/video/video_frame.h>
#include "aemu/base/Compiler.h"

namespace emulator {
namespace webrtc {

// An interface that can capture a screen.
class VideoCapturer {
public:
    VideoCapturer() = default;
    virtual ~VideoCapturer() = default;

    // Returns the framenumber that is available, this should be
    // monotonically increasing, and is used to determine if a new
    // call to the getVideoFrame should be made.
    virtual int64_t frameNumber() = 0;

    // Gets the current screen frame of the device. Return NULL when you failed
    // to produce a video frame.
    virtual absl::optional<::webrtc::VideoFrame> getVideoFrame() = 0;

    // Max number of frames per second the capturer can capture.
    virtual uint32_t maxFps() const = 0;

    // Unique name identifying this capturer
    virtual std::string name() const = 0;

    DISALLOW_COPY_AND_ASSIGN(VideoCapturer);
};
}  // namespace webrtc
}  // namespace emulator
