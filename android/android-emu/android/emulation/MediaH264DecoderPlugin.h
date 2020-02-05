// Copyright (C) 2019 The Android Open Source Project
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

#include "android/emulation/GoldfishMediaDefs.h"
#include "android/emulation/MediaH264Decoder.h"

#include <stddef.h>

namespace android {
namespace emulation {

// this is an interface for platform specific implementations
// such as VideoToolBox, CUVID, etc
class MediaH264DecoderPlugin {
public:
    using PixelFormat = MediaH264Decoder::PixelFormat;
    using Err = MediaH264Decoder::Err;

    virtual void initH264Context(unsigned int width,
                                 unsigned int height,
                                 unsigned int outWidth,
                                 unsigned int outHeight,
                                 PixelFormat pixFmt) = 0;
    virtual void reset(unsigned int width,
                       unsigned int height,
                       unsigned int outWidth,
                       unsigned int outHeight,
                       PixelFormat pixFmt) = 0;
    virtual MediaH264DecoderPlugin* clone() = 0;
    virtual void destroyH264Context() = 0;
    virtual void decodeFrame(void* ptr, const uint8_t* frame, size_t szBytes, uint64_t pts) = 0;
    virtual void flush(void* ptr) = 0;
    virtual void getImage(void* ptr) = 0;

    MediaH264DecoderPlugin() = default;
    virtual ~MediaH264DecoderPlugin() = default;
};

}  // namespace emulation
}  // namespace android
