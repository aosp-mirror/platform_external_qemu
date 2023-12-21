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

// A parser for H.265 bitstream. It will determine will kind of Netowrk
// Abstraction Layer Unit (NALU) we have received from the guest.

#pragma once

#include <cstdint>
#include <string>

namespace android {
namespace emulation {

// All H.265 frames are structured in the following way:
//
// ===============================================================================
// | start code header | forbidden zero bit | nal_unit_type | 1 bit |  data |
// | 3 or 4 bytes      | 1 bit              | 6 bits        |       |  any  |
// ===============================================================================
//
// - Start code header:
//
//   Can be either 0x00 00 01 or 0x00 00 00 01. The start code header indicates
//   where each Network Abstraction Layer Unit (NALU) begins, since this is a
//   continuous stream of data.
//
// - forbidden zero bit:
//
//   Used as an error-indicator. 0 means the transmission is normal, while 1
//   means there is a syntax violation.
//
// - nal_unit_type:
//
//   The type of data contained in the frame. See HevcNaluType below for a list
//   of the types.
//

class HevcNaluParser {
public:
    enum class HevcNaluType : uint8_t {
        TRAIL_N = 0,
        TRAIL_R = 1,
        TSA_N = 2,
        TSA_R = 3,
        STSA_N = 4,
        STSA_R = 5,
        RADL_N = 6,
        RADL_R = 7,
        RASL_N = 8,
        RASL_R = 9,

        RSV_VCL_N10 = 10,
        RSV_VCL_N11 = 11,
        RSV_VCL_N12 = 12,
        RSV_VCL_N13 = 13,
        RSV_VCL_N14 = 14,
        RSV_VCL_N15 = 15,
        BLA_W_LP = 16,
        BLA_W_RADL = 17,
        BLA_N_LP = 18,
        IDR_W_RADL = 19,

        IDR_N_LP = 20,
        CRA_NUT = 21,
        RSV_IRAP_VCL22 = 22,
        RSV_IRAP_VCL23 = 23,
        RSV_IRAP_VCL24 = 24,
        RSV_IRAP_VCL25 = 25,
        RSV_IRAP_VCL26 = 26,
        RSV_IRAP_VCL27 = 27,
        RSV_IRAP_VCL28 = 28,
        RSV_IRAP_VCL29 = 29,

        RSV_IRAP_VCL30 = 30,
        RSV_IRAP_VCL31 = 31,
        VPS = 32,
        SPS = 33,
        PPS = 34,
        AUD = 35,
        EOS_NUT = 36,
        EOB_NUT = 37,
        FD_NUT = 38,
        SEI_PREFIX = 39,

        SEI_SUFFIX = 40,
        RSV_NVCL41 = 41,
        RSV_NVCL42 = 42,
        RSV_NVCL43 = 43,
        RSV_NVCL44 = 44,
        RSV_NVCL45 = 45,
        RSV_NVCL46 = 46,
        RSV_NVCL47 = 47,
        RSV_NVCL48 = 48,
        RSV_NVCL49 = 49,

        RSV_NVCL50 = 50,
        RSV_NVCL51 = 51,
        RSV_NVCL52 = 52,
        RSV_NVCL53 = 53,
        RSV_NVCL54 = 54,
        RSV_NVCL55 = 55,
        RSV_NVCL56 = 56,
        RSV_NVCL57 = 57,
        RSV_NVCL58 = 58,
        RSV_NVCL59 = 59,

        RSV_NVCL60 = 60,
        RSV_NVCL61 = 61,
        RSV_NVCL62 = 62,
        RSV_NVCL63 = 63,
        Undefined = 64,  // not in H.265 nalu type; just here to cap the enums
                         // so checking is simpler.
    };

    static bool checkSpsFrame(const uint8_t* frame, size_t szBytes);
    static bool checkPpsFrame(const uint8_t* frame, size_t szBytes);
    static bool checkVpsFrame(const uint8_t* frame, size_t szBytes);
    static bool checkIFrame(const uint8_t* frame, size_t szBytes);
    // Returns the description of the HevcNaluType.
    static const std::string& naluTypeToString(HevcNaluType n);
    // Parses |frame| for the NALU type. |szBytes| is the size of |frame|,
    // |data| will be set to the location after the start code header. This
    // means that the first byte of |data| will still contain the NALU type in
    // the lower 5-bits.
    static HevcNaluType getFrameNaluType(const uint8_t* frame,
                                         size_t szBytes,
                                         uint8_t** data = nullptr);

    // Finds the position of the next start code header. Returns null if not
    // found, otherwise returns a pointer to the beginning of the next start
    // code header.
    static const uint8_t* getNextStartCodeHeader(const uint8_t* frame,
                                                 size_t szBytes);
};

}  // namespace emulation
}  // namespace android
