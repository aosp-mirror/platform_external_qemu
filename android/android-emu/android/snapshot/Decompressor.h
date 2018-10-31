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

#include <stdint.h>

//
// Decompressor - a simple class for decompressing data.
//

namespace android {
namespace snapshot {

class Decompressor {
public:
    Decompressor() = delete;

    // Decompress |data| into |outData| buffer.
    static bool decompress(const uint8_t* data,
                           int32_t size,
                           uint8_t* outData,
                           int32_t outSize);
};

}  // namespace snapshot
}  // namespace android
