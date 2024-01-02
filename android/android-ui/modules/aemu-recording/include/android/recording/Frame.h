// Copyright (C) 2017 The Android Open Source Project
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
#include <cstdint>                // for uint8_t, uint32_t, uint64_t
#include <vector>                 // for vector

#include "android/recording/AvFormat.h"

extern "C" {
#include "libavutil/pixfmt.h"     // for AVPixelFormat
#include "libavutil/samplefmt.h"  // for AVSampleFormat
}

namespace android {
namespace recording {

// The video format size in bytes
int getVideoFormatSize(VideoFormat format);

// Conversion from VideoFormat to AVPixelFormat
AVPixelFormat toAVPixelFormat(VideoFormat r);

// The audio format size in bytes
int getAudioFormatSize(AudioFormat format);

// Conversion from AudioFormat to AVSampleFormat
AVSampleFormat toAVSampleFormat(AudioFormat r);

// A small structure to encapsulate audio/video data.
struct Frame {
    uint64_t tsUs = 0llu;  // timestamp(microsec)
    uint16_t width {0};
    uint16_t height {0};
    std::vector<uint8_t> dataVec;
    AVFormat format;

    // Allocates the space with capacity |cap| and sets each element to |val|
    Frame(uint32_t cap = 0, uint8_t val = 0) : dataVec(cap, val) {}

    // Move constructor
    Frame(Frame&& f) = default;
    // Move assignment
    Frame& operator=(Frame&& f) = default;
};

}  // namespace recording
}  // namespace android
