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
//
//
// frame parsing code is taken from libvpx decoder
/*
 *  Copyright (c) 2013 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "android/emulation/VpxFrameParser.h"

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
#define VPX_PRINT(color, fmt, ...)                                           \
    fprintf(stderr, color "VpxFrameParser: %s:%d " fmt "\n" RESET, __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define VPX_PRINT(fmt, ...)
#endif

#define VPX_INFO(fmt, ...) VPX_PRINT(RESET, fmt, ##__VA_ARGS__);
#define VPX_WARN(fmt, ...) VPX_PRINT(YEL, fmt, ##__VA_ARGS__);
#define VPX_ERROR(fmt, ...) VPX_PRINT(RED, fmt, ##__VA_ARGS__);

namespace android {
namespace emulation {

VpxFrameParser::VpxFrameParser(int vpxtype, const uint8_t* data, size_t size)
    : type(vpxtype), bit_buffer(data), bit_buffer_end(data + size) {
    if (vpxtype == 8) {
        parse_vp8_uncompressed_header();
    } else if (vpxtype == 9) {
        parse_vp9_uncompressed_header();
    }
}

int VpxFrameParser::read_bit() {
    const size_t off = bit_offset;
    const size_t p = off >> 3;
    const int q = 7 - (int)(off & 0x7);
    if (bit_buffer + p < bit_buffer_end) {
        const int bit = (bit_buffer[p] >> q) & 1;
        bit_offset = off + 1;
        return bit;
    } else {
        return 0;
    }
}

int VpxFrameParser::read_literal(int bits) {
    int value = 0, bit;
    for (bit = bits - 1; bit >= 0; bit--)
        value |= read_bit() << bit;
    return value;
}

void VpxFrameParser::parse_vp9_uncompressed_header() {
    // http://downloads.webmproject.org/docs/vp9/vp9-bitstream_superframe-and-uncompressed-header_v1.0.pdf
    // FRAME_MARKER 2bits
    // version 1 bit
    // high 1 bit
    // profile = high << 1 + version
    // optional 1 bit
    // show_existing_frame 1 bit
    // optional 3 bit
    // frame_tyep 1 bit

    read_literal(2);
    int profile = read_bit();
    profile |= read_bit() << 1;
    if (profile > 2)
        profile += read_bit();
    int show_existing_frame = read_bit();
    if (show_existing_frame) {
        read_literal(3);
    }
    m_is_key_frame = (read_bit() == KEY_FRAME);
    VPX_PRINT("found vp9 %s frame", m_is_key_frame ? "KEY" : "NON-KEY");
}

void VpxFrameParser::parse_vp8_uncompressed_header() {
    // https://tools.ietf.org/html/rfc6386#section-9.1
    m_is_key_frame = (read_bit() == KEY_FRAME);
    VPX_PRINT("found vp8 %s frame", m_is_key_frame ? "KEY" : "NON-KEY");
}

}  // namespace emulation
}  // namespace android
