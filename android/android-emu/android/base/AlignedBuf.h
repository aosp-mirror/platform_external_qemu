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

#include <cinttypes>
#include <cstdlib>

namespace android {

template<class T, size_t alignment>
class AlignedBuf {
public:
    AlignedBuf(size_t size) {
        resize(size);
    }

    ~AlignedBuf() {
        free(mBuffer);
    }

    void resize(size_t newSize) {
        size_t pad = alignment > sizeof(T) ? alignment : sizeof(T);
        size_t newSizeBytes = newSize * sizeof(T) + pad;

        mBuffer =
            static_cast<uint8_t*>(realloc(mBuffer, newSizeBytes));

        mAligned =
            reinterpret_cast<T*>(
                (reinterpret_cast<uintptr_t>(mBuffer) + pad) &
                    ~(alignment - 1));

        mSize = newSize;
    }

    size_t size() const {
        return mSize;
    }

    T* data() {
        return mAligned;
    }

private:
    uint8_t* mBuffer = nullptr;
    T* mAligned = nullptr;
    size_t mSize = 0;
};

} // namespace android
