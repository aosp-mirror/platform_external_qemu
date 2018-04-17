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

#include <functional>

#include <inttypes.h>

// Convenience class ContiguousRangeMapper which makes it easier to collect contiguous ranges and run some function over the coalesced range.

namespace android {
namespace base {

class ContiguousRangeMapper {
public:
    using Func = std::function<void(uintptr_t, uintptr_t)>;
    ContiguousRangeMapper(Func&& mapFunc, uintptr_t batchSize = 0) :
        mMapFunc(std::move(mapFunc)), mBatchSize(batchSize) { }

    // add(): adds the piece [start, start + size] to the internally tracked range.
    // If a discontinuity occurs, mMapFunc will run on the previous range.
    void add(uintptr_t start, uintptr_t size);

    // Signals end of collection of pieces, running mMapFunc over any outstanding range.
    void finish();

    ~ContiguousRangeMapper();
private:
    Func mMapFunc = {};
    uintptr_t mBatchSize = 0;
    bool mHasRange = false;
    uintptr_t mStart = 0;
    uintptr_t mEnd = 0;
};

}  // namespace base
}  // namespace android
