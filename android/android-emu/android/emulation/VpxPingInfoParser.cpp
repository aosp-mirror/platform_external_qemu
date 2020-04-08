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

#include "android/emulation/VpxPingInfoParser.h"

#define VPX_DEBUG 0

#if VPX_DEBUG
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"
#define VPX_PRINT(color, fmt, ...)                                    \
    fprintf(stderr, color "VpxPingInfoParser: %s:%d " fmt "\n" RESET, \
            __func__, __LINE__, ##__VA_ARGS__);
#else
#define VPX_PRINT(fmt, ...)
#endif

#define VPX_INFO(fmt, ...) VPX_PRINT(RESET, fmt, ##__VA_ARGS__);
#define VPX_WARN(fmt, ...) VPX_PRINT(YEL, fmt, ##__VA_ARGS__);
#define VPX_ERROR(fmt, ...) VPX_PRINT(RED, fmt, ##__VA_ARGS__);

namespace android {
namespace emulation {

uint32_t VpxPingInfoParser::parseVersion(void* ptr) {
    uint8_t* xptr = (uint8_t*)ptr;
    uint32_t version = *(uint32_t*)(xptr + 1 * 8);
    return version;
}

uint64_t VpxPingInfoParser::parseId(void* ptr) {
    if (nullptr == ptr)
        return 0;
    uint64_t key = (*reinterpret_cast<uint64_t*>(ptr));
    return key;
}

void* VpxPingInfoParser::getReturnAddress(void* ptr) {
    uint8_t* xptr = (uint8_t*)ptr;
    void* pint = (void*)(xptr + 256);
    return pint;
}

void VpxPingInfoParser::parseInitContextParams(void* ptr,
                                               InitContextParam& param) {
    param.id = parseId(ptr);
    param.version = parseVersion(ptr);
}

void VpxPingInfoParser::parseDecodeFrameParams(void* ptr,
                                               DecodeFrameParam& param) {
    param.id = parseId(ptr);
    uint8_t* xptr = (uint8_t*)ptr;
    uint64_t offset = *(uint64_t*)(xptr + 1 * 8);
    param.p_data = xptr + offset;
    param.size = *(size_t*)(xptr + 2 * 8);
    param.user_priv = *(uint64_t*)(xptr + 3 * 8);
}

void VpxPingInfoParser::parseGetImageParams(void* ptr, GetImageParam& param) {
    param.id = parseId(ptr);
    uint8_t* xptr = (uint8_t*)ptr;
    param.outputBufferWidth = *(size_t*)(xptr + 1 * 8);
    param.outputBufferHeight = *(size_t*)(xptr + 2 * 8);
    param.width = *(size_t*)(xptr + 3 * 8);
    param.height = *(size_t*)(xptr + 4 * 8);
    param.bpp = *(size_t*)(xptr + 5 * 8);
    if (mVersion == 100) {
        param.hostColorBufferId = -1;
    } else if (mVersion == 200) {
        param.hostColorBufferId = *(int32_t*)(xptr + 6 * 8);
    }
    uint64_t offset = *(uint64_t*)(xptr + 7 * 8);
    param.p_dst = (uint8_t*)ptr + offset;

    // return
    uint8_t* retptr = (uint8_t*)getReturnAddress(ptr);
    param.p_error = (int*)(retptr);
    param.p_fmt = (uint32_t*)(retptr + 1 * 8);
    param.p_d_w = (uint32_t*)(retptr + 2 * 8);
    param.p_d_h = (uint32_t*)(retptr + 3 * 8);
    param.p_user_priv = (uint64_t*)(retptr + 4 * 8);
}

void VpxPingInfoParser::parseDestroyParams(void* ptr, DestroyParam& param) {
    param.id = parseId(ptr);
}

void VpxPingInfoParser::parseFlushParams(void* ptr, FlushParam& param) {
    param.id = parseId(ptr);
}

VpxPingInfoParser::VpxPingInfoParser(void* ptr) {
    mVersion = parseVersion(ptr);
}

VpxPingInfoParser::VpxPingInfoParser(uint32_t version) {
    mVersion = version;
}

}  // namespace emulation
}  // namespace android
