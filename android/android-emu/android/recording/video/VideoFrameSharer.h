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

#include "android/emulation/control/display_agent.h"
#include "android/recording/Producer.h"
#include "android/base/memory/SharedMemory.h"

namespace android {
namespace recording {


// A Class that produces I420 on a shared memory
class VideoFrameSharer {
public:
    struct VideoInfo {
        uint32_t width;
        uint32_t height;
        uint8_t fps;
    };

    VideoFrameSharer(uint32_t fbWidth,
                   uint32_t fbHeight,
                   uint8_t fps,
                   const std::string& handle);
    ~VideoFrameSharer();

    bool attachProducer(std::unique_ptr<Producer> producer);
    void start();
    void stop();

private :
    bool marshallFrame(const Frame* frame);
    size_t getPixelBytes(VideoInfo info);

    VideoInfo mVideo;
    base::SharedMemory mMemory;
    std::unique_ptr<Producer> mVideoProducer;
    int mFormat;
};


void start_webrtc_module(std::string handle);

void stop_webrtc_module();
}  // namespace recording
}  // namespace android

