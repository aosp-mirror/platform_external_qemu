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

#include "android/base/memory/SharedMemory.h"
#include "android/emulation/control/display_agent.h"
#include "android/recording/Producer.h"

namespace android {
namespace recording {

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
        uint32_t fps;

    };

    VideoFrameSharer(uint32_t fbWidth,
                     uint32_t fbHeight,
                     uint8_t fps,
                     const std::string& handle);
    ~VideoFrameSharer();

    bool attachProducer(std::unique_ptr<Producer> producer);
    void start();
    void stop();

private:
    bool marshallFrame(const Frame* frame);
    static size_t getPixelBytes(VideoInfo info);

    VideoInfo mVideo;
    base::SharedMemory mMemory;
    std::unique_ptr<Producer> mVideoProducer;
    int mSourceFormat;
};
}  // namespace recording
}  // namespace android
