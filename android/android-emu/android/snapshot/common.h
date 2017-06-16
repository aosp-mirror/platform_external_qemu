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

#include "android/base/StringView.h"
#include <cstdint>

namespace android {
namespace snapshot {

// A function to check if a range of memory is all zeroes.
// It's better to be as well optimized as possible - both saving and loading
// classes call it a lot.
using ZeroChecker = bool (*)(const uint8_t* ptr, int size);

struct RamBlock {
    base::StringView id;
    int64_t startOffset;
    uint8_t* hostPtr;
    int64_t totalSize;
    int32_t pageSize;
};

}  // namespace snapshot
}  // namespace android
