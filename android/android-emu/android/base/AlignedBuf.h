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

#include <vector>

#include <inttypes.h>
#include <stdlib.h>

namespace android {

template<class T, uint64_t alignment>
class AlignedBuf {
public:
    AlignedBuf(uint64_t size) {
        resize(size);
    }

    ~AlignedBuf() {
        free(mBuffer);
    }

    void resize(uint64_t newSize) {
        uint64_t pad = alignment > sizeof(T) ? alignment : sizeof(T);
        uint64_t newSizeBytes = newSize * sizeof(T) + pad;
        mBuffer = (uint8_t*)realloc(mBuffer, newSizeBytes);
        mAligned = (T*)((uintptr_t)(mBuffer + pad) & ~(alignment - 1));
        mSize = newSize;
    }

    uint64_t size() const {
        return mSize;
    }

    T* data() {
        return mAligned;
    }

private:
    uint8_t* mBuffer = nullptr;
    T* mAligned = nullptr;
    uint64_t mSize = 0;
};

} // namespace android
