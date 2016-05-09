// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/misc/Base64.h"

#include <vector>

#include <stdio.h>

namespace android {

using android::base::kNullopt;
using android::base::kInplace;

static const char kEncodingTable[65] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void encodeTriplet(char* dst,
                          uint8_t c0,
                          uint8_t c1,
                          uint8_t c2) {
    dst[0] = kEncodingTable[c0 >> 2];
    dst[1] = kEncodingTable[((c0 & 3) << 4) | ((c1 & 0xf0) >> 4)];
    dst[2] = kEncodingTable[((c1 & 0xf) << 2) | ((c2 & 0xc0) >> 6)];
    dst[3] = kEncodingTable[c2 & 0x3f];
}

std::string base64Encode(const void* src0, size_t srcLen) {
    std::string result(base64EncodedSizeForInputSize(srcLen), '\0');
    char* dst = const_cast<char*>(result.c_str());
    const uint8_t* src = static_cast<const uint8_t*>(src0);

    size_t n = 0;
    size_t m = 0;
    for (; n + 3 <= srcLen; n += 3, m += 4) {
        encodeTriplet(&dst[m], src[n], src[n + 1], src[n + 2]);
    }

    if (n < srcLen) {
        encodeTriplet(&dst[m], src[n],
                      n + 1 < srcLen ? src[n + 1] : 0U,
                      n + 2 < srcLen ? src[n + 2] : 0U);
        if (n + 1 >= srcLen) {
            dst[m + 2] = '=';
        }
        if (n + 2 >= srcLen) {
            dst[m + 3] = '=';
        }
        m += 4;
    }

    return result;
}

static const uint8_t kDecodingTable[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57,
        58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 64, 0,  1,  2,  3,  4,  5,  6,
        7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
        25, 64, 64, 64, 64, 64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
        37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64};

base::Optional<std::string> base64Decode(const void* src0, size_t srcLen) {
    size_t inputSize = srcLen;
    auto src = static_cast<const char*>(src0);

    // If size is a multiple of 4, we allow padding with '=' at the end.
    if (inputSize > 0 && (inputSize & 3) == 0) {
        if (src[inputSize - 1] == '=') {
            inputSize--;
        }
        if (src[inputSize - 1] == '=') {
            inputSize--;
        }
    }

    // Cannot have only one trailing char in a valid string.
    if ((inputSize & 3) == 1) {
        return kNullopt;
    }

    // Check for invalid characters.
    size_t inputLen = 0;
    for (; inputLen < inputSize; ++inputLen) {
        if (kDecodingTable[(uint8_t)src[inputLen]] >= 64) {
            return kNullopt;
        }
    }

    size_t outputLen = ((inputLen + 3) / 4) * 3;
    base::Optional<std::string> result(kInplace, outputLen, '\0');
    uint8_t* dst = (uint8_t*)(result->c_str());

    size_t n = 0;
    size_t m = 0;
    for (; n + 4 <= inputLen; n += 4, m += 3) {
        uint8_t b0 = kDecodingTable[(uint8_t)src[n]];
        uint8_t b1 = kDecodingTable[(uint8_t)src[n + 1]];
        uint8_t b2 = kDecodingTable[(uint8_t)src[n + 2]];
        uint8_t b3 = kDecodingTable[(uint8_t)src[n + 3]];
        dst[m] = (b0 << 2) | (b1 >> 4);
        dst[m + 1] = (b1 << 4) | (b2 >> 2);
        dst[m + 2] = (b2 << 6) | b3;
    }

    if (n + 2 <= inputLen) {
        uint8_t b0 = kDecodingTable[(uint8_t)src[n]];
        uint8_t b1 = kDecodingTable[(uint8_t)src[n + 1]];
        dst[m++] = (b0 << 2) | (b1 >> 4);
    }
    if (n + 3 <= inputLen) {
        uint8_t b1 = kDecodingTable[(uint8_t)src[n + 1]];
        uint8_t b2 = kDecodingTable[(uint8_t)src[n + 2]];
        dst[m++] = (b1 << 4) | (b2 >> 2);
    }
    if (n + 4 <= inputLen) {
        uint8_t b2 = kDecodingTable[(uint8_t)src[n + 2]];
        uint8_t b3 = kDecodingTable[(uint8_t)src[n + 3]];
        dst[m++] = (b2 << 6) | b3;
    }

    result->resize(m);
    return result;
}

}  // namespace android
