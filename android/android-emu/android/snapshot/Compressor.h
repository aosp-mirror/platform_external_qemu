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

#pragma once

#include <cstdint>
#include "lz4.h"

namespace android {
namespace snapshot {
namespace compress {

int workerCount();
int32_t compress(const uint8_t* data,
                 int32_t size,
                 uint8_t* out,
                 int32_t outSize);

constexpr int32_t maxCompressedSize(int32_t dataSize) {
    return LZ4_COMPRESSBOUND(dataSize);
}

}  // namespace compress
}  // namespace snapshot
}  // namespace android
