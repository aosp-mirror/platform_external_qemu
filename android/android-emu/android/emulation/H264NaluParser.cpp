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

// A parser for H.264 bitstream. It will determine will kind of Netowrk
// Abstraction Layer Unit (NALU) we have received from the guest.

#include "android/emulation/H264NaluParser.h"

#define H264_DEBUG 1

#if H264_DEBUG
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"
#define H264_PRINT(color,fmt,...) fprintf(stderr, color "H264NaluParser: %s:%d " fmt "\n" RESET, __func__, __LINE__, ##__VA_ARGS__);
#else
#define H264_PRINT(fmt,...)
#endif

#define H264_INFO(fmt,...) H264_PRINT(RESET, fmt, ##__VA_ARGS__);
#define H264_WARN(fmt,...) H264_PRINT(YEL, fmt, ##__VA_ARGS__);
#define H264_ERROR(fmt,...) H264_PRINT(RED, fmt, ##__VA_ARGS__);

namespace android {
namespace emulation {

const std::string& naluTypeToString(H264NaluType n) {
    uint8_t idx = static_cast<uint8_t>(n);
    H264_WARN("%s", kNaluTypesStrings[idx].c_str());
    return kNaluTypesStrings[idx];
}

H264NaluType getFrameNaluType(const uint8_t* frame, size_t szBytes, uint8_t** data) {
    if (szBytes < 4) {
        H264_INFO("Not enough bytes for start code header and NALU type");
        return H264NaluType::Undefined;
    }

    if (frame[0] != 0 || frame[1] != 0) {
        H264_INFO("First two bytes of start code header are not zero");
        return H264NaluType::Undefined;
    }

    // check for 4-byte start code header
    if (frame[2] == 0 && frame[3] == 1) {
        if (szBytes == 4) {
            H264_INFO("Got start code header but no NALU type");
            return H264NaluType::Undefined;
        }
        // nalu type is the lower 5 bits
        uint8_t naluType = 0x1f & frame[4];
//        H264_INFO("frame[4]=0x%2x nalutype=0x%2x", frame[4], naluType);
        if (data) {
            *data = const_cast<uint8_t*>(&frame[4]);
        }
        return (naluType < static_cast<uint8_t>(H264NaluType::Undefined)) ?
                static_cast<H264NaluType>(naluType) :
                H264NaluType::Undefined;
    }

    // check for three-byte start code header
    if (frame[2] == 1) {
        // nalu type is the lower 5 bits
        uint8_t naluType = 0x1f & frame[3];
        if (data) {
            *data = const_cast<uint8_t*>(&frame[3]);
        }
        return (naluType < static_cast<uint8_t>(H264NaluType::Undefined)) ?
                static_cast<H264NaluType>(naluType) :
                H264NaluType::Undefined;
    }

    H264_WARN("Frame did not have a start code header");
    return H264NaluType::Undefined;
}

const uint8_t* getNextStartCodeHeader(const uint8_t* frame, size_t szBytes) {
//    // Start code can either be 0x000001 or 0x00000001. Beware of doing this comparison
//    // by casting, as the frame may be in network-byte order (big endian).
//    const uint8_t* res = nullptr;
//    size_t idx = 0;
//    const int kHeaderMinSize = 3;
//    int64_t remaining = szBytes;
//    
//    while (remaining >= kHeaderMinSize) {
//        if (frame[idx] != 0) {
//            ++idx;
//            --remaining;
//            continue;
//        }
//
//        if (frame[idx + 1] != 0) {
//            idx += 2;
//            remaining -= 2;
//            continue;
//        }
//
//        if (frame[idx + 2] == 1) {
//            res = &frame[idx];
//            break;
//        }
//
//        // check for four byte header if enough bytes left
//        if (frame[idx + 2] == 0) {
//            if (remaining >= 4 && frame[idx + 3] == 1) {
//                res = &frame[idx];
//                break;
//            } else {
//                idx += 4;
//                remaining -= 4;
//                continue;
//            }
//        } else {
//            idx += 3;
//            remaining -= 3;
//            continue;
//        }
//    }
//
//    return res;

    // This implementation uses bit shifting
    constexpr uint32_t startHeaderMask = 0x00000001;
    uint32_t window = 0;
    
    if (szBytes < 3) {
        H264_INFO("Not enough bytes for a start code header");
        return nullptr;
    }

    // we can't do a cast to 32-bit int because it is in network-byte format (big endian).
    window |= (uint32_t)frame[0] << 16;
    window |= (uint32_t)frame[1] << 8;
    window |= frame[2];

    if (szBytes == 3) {
        if (!(window ^ startHeaderMask)) {
            return &frame[0];
        } else {
            return nullptr;
        }
    }

    size_t remaining = szBytes - 3;
    while (remaining > 0) {
        window = (window << 8) | (uint32_t)frame[szBytes - remaining];
        if (!(window ^ startHeaderMask)) {
            // four-byte header match
            return &frame[szBytes - remaining - 3];
        } else if (!((window & 0x00ffffff) ^ startHeaderMask)) {
            // three-byte header match
            return &frame[szBytes - remaining - 2];
        }
        --remaining;
    }
    return nullptr;
}

}  // namespace emulation
}  // namespace android
