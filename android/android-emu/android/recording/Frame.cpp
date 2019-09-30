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

#include "android/recording/Frame.h"

#include <libavutil/pixfmt.h>     // for AVPixelFormat, AV_PIX_FMT_BGRA, AV_...
#include <libavutil/samplefmt.h>  // for AVSampleFormat, AV_SAMPLE_FMT_NONE

namespace android {
namespace recording {

// The video format size in bytes
int getVideoFormatSize(VideoFormat format) {
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
AVPixelFormat toAVPixelFormat(VideoFormat r) {
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
int getAudioFormatSize(AudioFormat format) {
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
AVSampleFormat toAVSampleFormat(AudioFormat r) {
    switch (r) {
        case AudioFormat::AUD_FMT_S16:
            return AV_SAMPLE_FMT_S16;
        case AudioFormat::AUD_FMT_U8:
            return AV_SAMPLE_FMT_U8;
        default:
            return AV_SAMPLE_FMT_NONE;
    };
}

}  // namespace recording
}  // namespace android
