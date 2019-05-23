// Copyright 2018 The Android Open Source Project
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

#include "android/base/Compiler.h"

#include <functional>

#include <inttypes.h>

// Convenience class ContiguousRangeMapper which makes it easier to collect
// contiguous ranges and run some function over the coalesced range.

namespace android {
namespace base {

class ContiguousRangeMapper {
public:
    using Func = std::function<void(uintptr_t, uintptr_t)>;

    ContiguousRangeMapper(Func&& mapFunc, uintptr_t batchSize = 0) :
        mMapFunc(std::move(mapFunc)), mBatchSize(batchSize) { }

    // add(): adds [start, start + size) to the internally tracked range.
    // mMapFunc on the previous range runs if the new piece is not contiguous.

    void add(uintptr_t start, uintptr_t size);

    // Signals end of collection of pieces, running mMapFunc over any
    // outstanding range.
    void finish();

    // Destructor all runs mMapFunc if there is an outstanding range.
    ~ContiguousRangeMapper();

private:
    Func mMapFunc = {};
    uintptr_t mBatchSize = 0;
    bool mHasRange = false;
    uintptr_t mStart = 0;
    uintptr_t mEnd = 0;
    DISALLOW_COPY_ASSIGN_AND_MOVE(ContiguousRangeMapper);
};

}  // namespace base
}  // namespace android
