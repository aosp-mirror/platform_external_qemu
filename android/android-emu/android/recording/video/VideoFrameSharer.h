// Copyright (C) 2018 The Android Open Source Project
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

#include <stddef.h>                            // for size_t
#include <stdint.h>                            // for uint32_t, uint64_t
#include <atomic>                              // for atomic
#include <memory>                              // for unique_ptr
#include <string>                              // for string, basic_string
#include "android/base/memory/SharedMemory.h"  // for SharedMemory
#include "android/opengles.h"                  // for ReadPixelsFunc

namespace android {
namespace recording {
class Producer;

// A class that produces video frames in the YUV-I420 pixel format.
// It has the luma "luminance" plane Y first, then the U chroma plane and last
// the V chroma plane.
//
// The two chroma planes (blue and red projections) are sub-sampled in both the
// horizontal and vertical dimensions by a factor of 2. That is to say, for a
// 2x2 square of pixels, there are 4 Y samples but only 1 U sample and 1 V
// sample.
//
// This format requires 4*8+8+8=48 bits for 4 pixels, so its depth is 12 bits
// per pixel.
//
// The VideoFrameSharer will listen to frame events from the Producer and
// convert the incoming video format to I420 and copy it to the shared memory
// location. It will produce AT MOST "fps" frames per second.
//
// Note, that the existing frame will be overwritten, this can cause encoder
// artifacts. See https://en.wikipedia.org/wiki/Screen_tearing for background
// information on vsync issues. Vsync is tracked in b/77969099.
//
// The process on the other side can feed this data to the WebRTC system that
// will take care of the actual encoding and serialization of the video frames.
class VideoFrameSharer {
public:
    struct VideoInfo {
        uint32_t width;
        uint32_t height;
        uint32_t fps;          // Target framerate.
        uint32_t frameNumber;  // Frames are ordered
        uint64_t tsUs;         // Timestamp when this frame was received.
    };

    VideoFrameSharer(uint32_t fbWidth,
                     uint32_t fbHeight,
                     const std::string& handle);
    ~VideoFrameSharer();

    bool initialize();
    void start();
    void stop();

private:
    static void frameCallbackForwarder(void* opaque);
    void frameAvailable();
    static size_t getPixelBytes(VideoInfo info);

    VideoInfo mVideo = {0};
    std::string mHandle;
    base::SharedMemory mMemory;
    ReadPixelsFunc mReadPixels;
    std::unique_ptr<Producer> mVideoProducer;
    size_t mPixelBufferSize;

    static std::atomic<uint64_t> sFrameCounter;
};
}  // namespace recording
}  // namespace android
