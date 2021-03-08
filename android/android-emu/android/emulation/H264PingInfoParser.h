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

// A parser for H.264 ping info from guest. It will know where to extract
// the specified data.

#pragma once

#include "android/emulation/GoldfishMediaDefs.h"
#include "android/emulation/MediaH264Decoder.h"

#include <cstdint>
#include <string>

namespace android {
namespace emulation {

class H264PingInfoParser {
public:
    using PixelFormat = MediaH264Decoder::PixelFormat;

    struct InitContextParam {
        // input
        uint32_t version;
        unsigned int width;
        unsigned int height;
        unsigned int outputWidth;
        unsigned int outputHeight;
        PixelFormat outputPixelFormat;
        // output
        uint64_t* pHostDecoderId;
    };

    struct DecodeFrameParam {
        // input
        uint64_t hostDecoderId;
        uint8_t* pData;
        size_t size;
        uint64_t pts;
        // output
        size_t* pConsumedBytes;
        int32_t* pDecoderErrorCode;
    };

    struct ResetParam {
        // input
        uint64_t hostDecoderId;
        unsigned int width;
        unsigned int height;
        unsigned int outputWidth;
        unsigned int outputHeight;
        PixelFormat outputPixelFormat;
    };

    struct GetImageParam {
        // input
        uint64_t hostDecoderId;
        int32_t hostColorBufferId;

        // output
        int32_t* pDecoderErrorCode;
        uint32_t* pRetWidth;
        uint32_t* pRetHeight;
        uint64_t* pRetPts;
        uint32_t* pRetColorPrimaries;
        uint32_t* pRetColorRange;
        uint32_t* pRetColorTransfer;
        uint32_t* pRetColorSpace;
        uint8_t* pDecodedFrame;
    };

public:
    // get the decoder id on the host side that
    // is requested to do the work by the guest
    static uint32_t parseVersion(void* ptr);
    static uint64_t parseHostDecoderId(void* ptr);

    void parseInitContextParams(void* ptr, InitContextParam& param);
    void parseDecodeFrameParams(void* ptr, DecodeFrameParam& param);
    void parseGetImageParams(void* ptr, GetImageParam& param);
    void parseResetParams(void* ptr, ResetParam& param);

public:
    explicit H264PingInfoParser(void* ptr);
    H264PingInfoParser(uint32_t version);
    ~H264PingInfoParser() = default;

    uint32_t version() const { return mVersion; }

private:
    int32_t parseHostColorBufferId(void* ptr);
    void* getReturnAddress(void* ptr);
    uint32_t mVersion = 100;
};

}  // namespace emulation
}  // namespace android
