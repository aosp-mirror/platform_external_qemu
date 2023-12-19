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

// A parser for H.264 bitstream. It will determine will kind of Netowrk
// Abstraction Layer Unit (NALU) we have received from the guest.

#include "android/emulation/HevcNaluParser.h"

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
#define HEVC_PRINT(color, fmt, ...)                                          \
    fprintf(stderr, color "HevcNaluParser: %s:%d " fmt "\n" RESET, __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define HEVC_PRINT(fmt, ...)
#endif

#define HEVC_INFO(fmt, ...) HEVC_PRINT(RESET, fmt, ##__VA_ARGS__);
#define HEVC_WARN(fmt, ...) HEVC_PRINT(YEL, fmt, ##__VA_ARGS__);
#define HEVC_ERROR(fmt, ...) HEVC_PRINT(RED, fmt, ##__VA_ARGS__);

static const std::string kNaluTypesStrings[] = {
        "0: TRAIL_N",
        "1: TRAIL_R",  // P frame
        "2: TSA_N",
        "3: TSA_R",
        "4: STSA_N",
        "5: STSA_R",
        "6: RADL_N",
        "7: RADL_R",
        "8: RASL_N",
        "9: RASL_R",
        "10: RSV_VCL_N10",
        "11: RSV_VCL_N11",
        "12: RSV_VCL_N12",
        "13: RSV_VCL_N13",
        "14: RSV_VCL_N14",
        "15: RSV_VCL_N15",
        "16: BLA_W_LP",
        "17: BLA_W_RADL",
        "18: BLA_N_LP",
        "19: IDR_W_RADL",
        "20: IDR_N_LP",
        "21: CRA_NUT",
        "22: RSV_IRAP_VCL22",
        "23: RSV_IRAP_VCL23",
        "24: RSV_IRAP_VCL24",
        "25: RSV_IRAP_VCL25",
        "26: RSV_IRAP_VCL26",
        "27: RSV_IRAP_VCL27",
        "28: RSV_IRAP_VCL28",
        "29: RSV_IRAP_VCL29",
        "30: RSV_IRAP_VCL30",
        "31: RSV_IRAP_VCL31",
        "32: VPS(NON-VCL)",
        "33: SPS(NON-VCL)",
        "34: PPS(NON-VCL)",
        "35: AUD_NUT(NON-VCL)",
        "36: EOS_NUT(NON-VCL)",
        "37: EOB_NUT(NON-VCL)",
        "38: FD_NUT(NON-VCL)",
        "39: SEI_PREFIX(NON-VCL)",
        "40: SEI_SUFFIX(NON-VCL)",
        "41: RSV_NVCL41",
        "42: RSV_NVCL42",
        "43: RSV_NVCL43",
        "44: RSV_NVCL44",
        "45: RSV_NVCL45",
        "46: RSV_NVCL46",
        "47: RSV_NVCL47",
        "48: RSV_NVCL48",
        "49: RSV_NVCL49",
        "50: RSV_NVCL50",
        "51: RSV_NVCL51",
        "52: RSV_NVCL52",
        "53: RSV_NVCL53",
        "54: RSV_NVCL54",
        "55: RSV_NVCL55",
        "56: RSV_NVCL56",
        "57: RSV_NVCL57",
        "58: RSV_NVCL58",
        "59: RSV_NVCL59",
        "60: RSV_NVCL60",
        "61: RSV_NVCL61",
        "62: RSV_NVCL62",
        "63: RSV_NVCL63",
        "64: Undefied",
};

namespace android {
namespace emulation {

const std::string& HevcNaluParser::naluTypeToString(HevcNaluType n) {
    uint8_t idx = static_cast<uint8_t>(n);
    HEVC_WARN("%s", kNaluTypesStrings[idx].c_str());
    return kNaluTypesStrings[idx];
}

bool HevcNaluParser::checkSpsFrame(const uint8_t* frame, size_t szBytes) {
    HevcNaluParser::HevcNaluType currNaluType =
            HevcNaluParser::getFrameNaluType(frame, szBytes, NULL);
    if (currNaluType != HevcNaluParser::HevcNaluType::SPS) {
        return false;
    }
    HEVC_INFO("found sps");
    return true;
}

bool HevcNaluParser::checkIFrame(const uint8_t* frame, size_t szBytes) {
    HevcNaluParser::HevcNaluType currNaluType =
            HevcNaluParser::getFrameNaluType(frame, szBytes, NULL);
    // TODO: is this true?
    if (currNaluType != HevcNaluParser::HevcNaluType::IDR_W_RADL) {
        return false;
    }
    HEVC_INFO("found i frame");
    return true;
}

bool HevcNaluParser::checkPpsFrame(const uint8_t* frame, size_t szBytes) {
    HevcNaluParser::HevcNaluType currNaluType =
            HevcNaluParser::getFrameNaluType(frame, szBytes, NULL);
    if (currNaluType != HevcNaluParser::HevcNaluType::PPS) {
        return false;
    }
    HEVC_INFO("found pps");
    return true;
}

bool HevcNaluParser::checkVpsFrame(const uint8_t* frame, size_t szBytes) {
    HevcNaluParser::HevcNaluType currNaluType =
            HevcNaluParser::getFrameNaluType(frame, szBytes, NULL);
    if (currNaluType != HevcNaluParser::HevcNaluType::VPS) {
        return false;
    }
    HEVC_INFO("found vps");
    return true;
}

HevcNaluParser::HevcNaluType HevcNaluParser::getFrameNaluType(
        const uint8_t* frame,
        size_t szBytes,
        uint8_t** data) {
    if (szBytes < 4) {
        HEVC_INFO("Not enough bytes for start code header and NALU type");
        return HevcNaluType::Undefined;
    }

    if (frame[0] != 0 || frame[1] != 0) {
        HEVC_INFO("First two bytes of start code header are not zero");
        return HevcNaluType::Undefined;
    }

    // check for 4-byte start code header
    if (frame[2] == 0 && frame[3] == 1) {
        if (szBytes == 4) {
            HEVC_INFO("Got start code header but no NALU type");
            return HevcNaluType::Undefined;
        }
        const bool forbiddenBitIsInvalid = 0x80 & frame[4];
        if (forbiddenBitIsInvalid) {
            HEVC_INFO("Got start code header but no NALU type");
            return HevcNaluType::Undefined;
        }
        // nalu type is the lower 6 bits after shiftting to right 1 bit
        uint8_t naluType = 0x3f & (frame[4] >> 1);
        //        HEVC_INFO("frame[4]=0x%2x nalutype=0x%2x", frame[4],
        //        naluType);
        if (data) {
            *data = const_cast<uint8_t*>(&frame[4]);
        }
        return (naluType < static_cast<uint8_t>(HevcNaluType::Undefined))
                       ? static_cast<HevcNaluType>(naluType)
                       : HevcNaluType::Undefined;
    }

    // check for three-byte start code header
    if (frame[2] == 1) {
        // nalu type is the lower 5 bits
        uint8_t naluType = 0x3f & (frame[3] >> 1);
        if (data) {
            *data = const_cast<uint8_t*>(&frame[3]);
        }
        return (naluType < static_cast<uint8_t>(HevcNaluType::Undefined))
                       ? static_cast<HevcNaluType>(naluType)
                       : HevcNaluType::Undefined;
    }

    HEVC_WARN("Frame did not have a start code header");
    return HevcNaluType::Undefined;
}

const uint8_t* HevcNaluParser::getNextStartCodeHeader(const uint8_t* frame,
                                                      size_t szBytes) {
    //    // Start code can either be 0x000001 or 0x00000001. Beware of doing
    //    this comparison
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
        HEVC_INFO("Not enough bytes for a start code header");
        return nullptr;
    }

    // we can't do a cast to 32-bit int because it is in network-byte format
    // (big endian).
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
