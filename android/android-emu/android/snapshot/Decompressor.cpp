// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/Decompressor.h"

#include "lz4.h"

#include <stdio.h>
#include <cassert>

namespace android {
namespace snapshot {

bool Decompressor::decompress(const uint8_t* data,
                              int32_t size,
                              uint8_t* outData,
                              int32_t outSize) {
    const int res = LZ4_decompress_safe(reinterpret_cast<const char*>(data),
                                        reinterpret_cast<char*>(outData), size,
                                        outSize);
    if (res != outSize) {
        fprintf(stderr, "Decompression failed: %d\n", res);
    }
    return res == outSize;
}

}  // namespace snapshot
}  // namespace android
