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

#include "aemu/base/memory/ContiguousRangeMapper.h"

namespace android {
namespace base {

void ContiguousRangeMapper::add(uintptr_t start, uintptr_t size) {
    if (mHasRange &&
        (mEnd != start ||
         (mBatchSize && (mEnd - mStart >= mBatchSize)))) {
        finish();
    }

    if (mHasRange) {
        mEnd = start + size;
    } else {
        mHasRange = true;
        mStart = start;
        mEnd = start + size;
    }
}

void ContiguousRangeMapper::finish() {
    if (mHasRange) {
        mMapFunc(mStart, mEnd - mStart);
        mHasRange = false;
        mStart = 0;
        mEnd = 0;
    }
}

ContiguousRangeMapper::~ContiguousRangeMapper() {
    finish();
}

}  // namespace base
}  // namespace android
