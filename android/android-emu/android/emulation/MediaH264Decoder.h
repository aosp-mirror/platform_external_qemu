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
#include "android/emulation/MediaCodec.h"

#include <stddef.h>

namespace android {
namespace emulation {

class MediaH264Decoder : public MediaCodec {
public:
    // Platform dependent
    static MediaH264Decoder* create();
    virtual ~MediaH264Decoder() = default;

    enum class PixelFormat : uint8_t {
        YUV420P = 0,
        UYVY422 = 1,
        BGRA8888 = 2,
    };
    enum class Err : int {
        NoErr = 0,
        NoDecodedFrame = -1,
        InitContextFailed = -2,
        DecoderRestarted = -3, // Can happen when receiving a new set of SPS/PPS frames
        NALUIgnored = -4, // Can happen if we receive picture data w/o the SPS/PPS NALUs.
    };

    // For snapshots
    virtual void save(base::Stream* stream) const = 0;
    virtual bool load(base::Stream* stream) = 0;
protected:
    MediaH264Decoder() = default;
};

}  // namespace emulation
}  // namespace android
