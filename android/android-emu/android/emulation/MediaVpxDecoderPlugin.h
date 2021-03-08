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
#include "android/emulation/VpxPingInfoParser.h"

#include <stddef.h>
#include <vector>

namespace android {
namespace emulation {

// this is an interface for platform specific implementations
// such as libvpx, CUVID, etc
class MediaVpxDecoderPlugin {
public:
    using InitContextParam = VpxPingInfoParser::InitContextParam;
    using DecodeFrameParam = VpxPingInfoParser::DecodeFrameParam;
    using GetImageParam = VpxPingInfoParser::GetImageParam;

    virtual void initVpxContext(void* ptr) = 0;
    virtual void destroyVpxContext(void* ptr) = 0;
    virtual void decodeFrame(void* ptr) = 0;
    virtual void flush(void* ptr) = 0;
    virtual void getImage(void* ptr) = 0;

    virtual void save(base::Stream* stream) const {};
    virtual bool load(base::Stream* stream) { return true; };

    MediaVpxDecoderPlugin() = default;
    virtual ~MediaVpxDecoderPlugin() = default;

    // solely for snapshot save load purpose
    enum {
        PLUGIN_TYPE_NONE = 0,
        PLUGIN_TYPE_LIBVPX = 1,
        PLUGIN_TYPE_CUVID = 2,
        PLUGIN_TYPE_FFMPEG = 3,
        PLUGIN_TYPE_GENERIC = 4,
    };

    // this is required by save/load
    virtual int type() const { return PLUGIN_TYPE_NONE; }
    virtual int vpxtype() const { return 8; }
    virtual int version() const { return 200; }

public:
};

}  // namespace emulation
}  // namespace android
