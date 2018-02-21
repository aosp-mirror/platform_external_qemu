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

#include "android/base/system/System.h"

extern "C" {
#include "libavutil/pixfmt.h"
#include "libavutil/samplefmt.h"
}

#include <memory>

namespace android {
namespace recording {

enum class VideoFormat { RGB565, RGBA8888, BGRA8888, INVALID_FMT };
enum class AudioFormat { AUD_FMT_S16, AUD_FMT_U8, INVALID_FMT };

// The video format size in bytes
static int getVideoFormatSize(VideoFormat format) {
    switch (format) {
        case VideoFormat::RGB565:
            return 2;
        case VideoFormat::RGBA8888:
        case VideoFormat::BGRA8888:
            return 4;
        default:
            return -1;
    }
}

// Conversion from VideoFormat to AVPixelFormat
static AVPixelFormat toAVPixelFormat(VideoFormat r) {
    switch (r) {
        case VideoFormat::RGB565:
            return AV_PIX_FMT_RGB565;
        case VideoFormat::RGBA8888:
            return AV_PIX_FMT_RGBA;
        case VideoFormat::BGRA8888:
            return AV_PIX_FMT_BGRA;
        default:
            return AV_PIX_FMT_NONE;
    };
}

// The audio format size in bytes
static int getAudioFormatSize(AudioFormat format) {
    switch (format) {
        case AudioFormat::AUD_FMT_S16:
            return 2;
        case AudioFormat::AUD_FMT_U8:
            return 1;
        default:
            return -1;
    }
}

// Conversion from AudioFormat to AVSampleFormat
static AVSampleFormat toAVSampleFormat(AudioFormat r) {
    switch (r) {
        case AudioFormat::AUD_FMT_S16:
            return AV_SAMPLE_FMT_S16;
        case AudioFormat::AUD_FMT_U8:
            return AV_SAMPLE_FMT_U8;
        default:
            return AV_SAMPLE_FMT_NONE;
    };
}

union AVFormat {
    VideoFormat videoFormat;
    AudioFormat audioFormat;
};

// A small structure to encapsulate audio/video data.
struct Frame {
    uint64_t tsUs = 0llu;  // timestamp(microsec)
    std::vector<uint8_t> dataVec;
    AVFormat format;

    // Need default constructor for std::vector
    Frame() {}

    // Allocates the space with capacity |cap| but with no data
    Frame(uint32_t cap) : dataVec(cap) {}

    // Allocates the space with capacity |cap| and sets each element to |val|
    Frame(uint32_t cap, uint8_t val) : dataVec(cap, val) {}

    // Copies contents of |buf| and sets the timestamp.
    Frame(void* buf, uint32_t size_bytes, uint64_t timestampUs)
        : tsUs(timestampUs) {
        if (buf != nullptr) {
            uint8_t* p = static_cast<uint8_t*>(buf);
            dataVec.assign(p, p + size_bytes);
        }
    }

    // Move constructor
    Frame(Frame&& f) = default;
    // Move assignment
    Frame& operator=(Frame&& f) = default;
};

}  // namespace recording
}  // namespace android
