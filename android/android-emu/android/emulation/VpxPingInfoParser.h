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

// A parser for vpx ping info from guest. It will know where to extract
// the specified data.

#pragma once

#include "android/emulation/GoldfishMediaDefs.h"

#include <cstdint>
#include <string>

namespace android {
namespace emulation {

class VpxPingInfoParser {
public:
    struct InitContextParam {
        // input
        uint64_t id;       // unique id > 0
        uint32_t version;  // 100 or 200
    };

    struct FlushParam {
        uint64_t id;
    };

    struct DestroyParam {
        uint64_t id;
    };

    struct DecodeFrameParam {
        // input
        uint64_t id;
        uint8_t* p_data;  // host address
        size_t size;
        uint64_t user_priv;
    };

    struct GetImageParam {
        // input
        uint64_t id;
        size_t outputBufferWidth;
        size_t outputBufferHeight;
        size_t width;
        size_t height;
        size_t bpp;
        int32_t hostColorBufferId;
        uint8_t* p_dst;  // host address to copy decoded frame into

        int* p_error;
        uint32_t* p_fmt;
        uint32_t* p_d_w;
        uint32_t* p_d_h;
        uint64_t* p_user_priv;
    };

public:
    // get the decoder id on the host side that
    // is requested to do the work by the guest
    static uint32_t parseVersion(void* ptr);
    static uint64_t parseId(void* ptr);

    void parseInitContextParams(void* ptr, InitContextParam& param);
    void parseDecodeFrameParams(void* ptr, DecodeFrameParam& param);
    void parseGetImageParams(void* ptr, GetImageParam& param);
    void parseDestroyParams(void* ptr, DestroyParam& param);
    void parseFlushParams(void* ptr, FlushParam& param);

public:
    explicit VpxPingInfoParser(void* ptr);
    VpxPingInfoParser(uint32_t version);
    ~VpxPingInfoParser() = default;

    uint32_t version() const { return mVersion; }

private:
    void* getReturnAddress(void* ptr);
    uint32_t mVersion = 100;
};

}  // namespace emulation
}  // namespace android
