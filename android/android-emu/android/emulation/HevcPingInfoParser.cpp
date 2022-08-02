// Copyright (C) 2022 The Android Open Source Project
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

#include "android/emulation/HevcPingInfoParser.h"

#define HEVC_DEBUG 0

#if HEVC_DEBUG
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"
#define HEVC_PRINT(color, fmt, ...)                                    \
    fprintf(stderr, color "HevcPingInfoParser: %s:%d " fmt "\n" RESET, \
            __func__, __LINE__, ##__VA_ARGS__);
#else
#define HEVC_PRINT(fmt, ...)
#endif

#define HEVC_INFO(fmt, ...) HEVC_PRINT(RESET, fmt, ##__VA_ARGS__);
#define HEVC_WARN(fmt, ...) HEVC_PRINT(YEL, fmt, ##__VA_ARGS__);
#define HEVC_ERROR(fmt, ...) HEVC_PRINT(RED, fmt, ##__VA_ARGS__);

namespace android {
namespace emulation {

HevcPingInfoParser::HevcPingInfoParser(void* ptr) {
    mVersion = parseVersion(ptr);
}

HevcPingInfoParser::HevcPingInfoParser(uint32_t version) {
    mVersion = version;
}

uint32_t HevcPingInfoParser::parseVersion(void* ptr) {
    uint8_t* xptr = (uint8_t*)ptr;
    uint32_t version = *(uint32_t*)xptr;
    return version;
}

void HevcPingInfoParser::parseInitContextParams(void* ptr,
                                                InitContextParam& param) {
    uint8_t* xptr = (uint8_t*)ptr;
    param.version = *(uint32_t*)xptr;
    param.width = *(unsigned int*)(xptr + 8);
    param.height = *(unsigned int*)(xptr + 16);
    param.outputWidth = *(unsigned int*)(xptr + 24);
    param.outputHeight = *(unsigned int*)(xptr + 32);
    param.outputPixelFormat = static_cast<PixelFormat>(*(uint8_t*)(xptr + 40));
    param.pHostDecoderId = static_cast<uint64_t*>(getReturnAddress(ptr));
}

uint64_t HevcPingInfoParser::parseHostDecoderId(void* ptr) {
    if (nullptr == ptr)
        return 0;
    uint64_t key = (*reinterpret_cast<uint64_t*>(ptr));
    return key;
}

void HevcPingInfoParser::parseDecodeFrameParams(void* ptr,
                                                DecodeFrameParam& param) {
    param.hostDecoderId = parseHostDecoderId(ptr);
    uint64_t offset = *(uint64_t*)((uint8_t*)ptr + 8);
    param.pData = (uint8_t*)ptr + offset;
    param.size = *(size_t*)((uint8_t*)ptr + 16);
    param.pts = *(size_t*)((uint8_t*)ptr + 24);
    uint8_t* retptr = (uint8_t*)getReturnAddress(ptr);
    param.pConsumedBytes = (size_t*)(retptr);
    param.pDecoderErrorCode = (int*)(retptr + 8);
}

void HevcPingInfoParser::parseResetParams(void* ptr, ResetParam& param) {
    uint8_t* xptr = (uint8_t*)ptr;
    param.hostDecoderId = parseHostDecoderId(ptr);
    param.width = *(unsigned int*)(xptr + 8);
    param.height = *(unsigned int*)(xptr + 16);
    param.outputWidth = *(unsigned int*)(xptr + 24);
    param.outputHeight = *(unsigned int*)(xptr + 32);
    param.outputPixelFormat = static_cast<PixelFormat>(*(uint8_t*)(xptr + 40));
}

int32_t HevcPingInfoParser::parseHostColorBufferId(void* ptr) {
    // Guest will pass us the hsot color buffer id to send decoded frame to
    uint8_t* xptr = (uint8_t*)ptr;
    int32_t colorBufferId = *(int32_t*)(xptr + 16);
    return colorBufferId;
}

void* HevcPingInfoParser::getReturnAddress(void* ptr) {
    uint8_t* xptr = (uint8_t*)ptr;
    void* pint = (void*)(xptr + 256);
    return pint;
}

void HevcPingInfoParser::parseGetImageParams(void* ptr, GetImageParam& param) {
    param.hostDecoderId = parseHostDecoderId(ptr);
    if (mVersion == 100) {
        param.hostColorBufferId = -1;
    } else if (mVersion == 200) {
        param.hostColorBufferId = parseHostColorBufferId(ptr);
    }
    uint8_t* retptr = (uint8_t*)getReturnAddress(ptr);
    param.pDecoderErrorCode = (int*)(retptr);
    param.pRetWidth = (uint32_t*)(retptr + 8);
    param.pRetHeight = (uint32_t*)(retptr + 16);
    param.pRetPts = (uint64_t*)(retptr + 24);
    param.pRetColorPrimaries = (uint32_t*)(retptr + 32);
    param.pRetColorRange = (uint32_t*)(retptr + 40);
    param.pRetColorTransfer = (uint32_t*)(retptr + 48);
    param.pRetColorSpace = (uint32_t*)(retptr + 56);

    uint8_t* xptr = (uint8_t*)ptr;
    uint64_t offset = *(uint64_t*)(xptr + 8);
    param.pDecodedFrame = (uint8_t*)ptr + offset;
}

void HevcPingInfoParser::parseMetadataParams(void* ptr, MetadataParam& param) {
    uint8_t* xptr = (uint8_t*)ptr;
    param.type = *(uint64_t*)(xptr + 8);
    param.primaries = *(uint64_t*)(xptr + 16);
    param.range = *(uint64_t*)(xptr + 24);
    param.transfer = *(uint64_t*)(xptr + 32);
}

}  // namespace emulation
}  // namespace android
