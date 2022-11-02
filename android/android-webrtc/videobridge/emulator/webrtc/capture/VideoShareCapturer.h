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

#include <absl/types/optional.h>    // for optional
#include <api/scoped_refptr.h>      // for scoped_refptr
#include <api/video/i420_buffer.h>  // for I420Buffer
#include <stddef.h>                 // for size_t
#include <stdint.h>                 // for uint32_t, int32_t

#include <string>  // for string

#include "aemu/base/memory/SharedMemory.h"       // for SharedMemory
#include "emulator/webrtc/capture/VideoCapturer.h"  // for VideoCapturer

using android::base::SharedMemory;

namespace emulator {
namespace webrtc {

// A VideoCapturer that provides a new frame from the emulator shared memory region.
// A new frame will only be delivered if the shared memory region is valid and
// the frame number has increased.
class VideoShareCapturer : public VideoCapturer {
public:
    VideoShareCapturer(std::string handle);
    absl::optional<::webrtc::VideoFrame> getVideoFrame() override;

    int64_t frameNumber() override;

    // Max number of frames per second the capturer can capture.
    uint32_t maxFps() const override;

    // True if the shared memory handles are valid, and we have a positive fps.
    bool isValid() const;

    std::string name() const override;

private:
    struct VideoInfo {
        uint32_t width;
        uint32_t height;
        uint32_t fps;          // Target framerate.
        uint32_t frameNumber;  // Frame number.
        uint64_t tsUs;         // Timestamp when this frame was received.
    };

    static size_t getBytesPerFrame(const VideoInfo& capability);

    SharedMemory mSharedMemory;
    std::string mName;
    VideoInfo mVideoInfo{0};
    uint8_t* mPixelBuffer{nullptr};
    uint32_t mPixelBufferSize = 0;
    rtc::scoped_refptr<::webrtc::I420Buffer>
            mI420Buffer;  // Re-usable I420Buffer.
};
}  // namespace webrtc
}  // namespace emulator
