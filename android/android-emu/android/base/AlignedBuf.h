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

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <type_traits>

namespace android {

template <class T, size_t align>
class AlignedBuf {
public:
    explicit AlignedBuf(size_t size) {
        static_assert(align && ((align & (align - 1)) == 0),
                      "AlignedBuf only supports power-of-2 aligments.");
        resizeImpl(size);
    }

    AlignedBuf(const AlignedBuf& other) : AlignedBuf(other.mSize) {
        if (other.mBuffer) { // could have got moved out
            std::copy(other.mAligned, other.mAligned + other.mSize, mAligned);
        }
    }

    AlignedBuf& operator=(const AlignedBuf& other) {
        if (this != &other) {
            AlignedBuf tmp(other);
            *this = std::move(tmp);
        }
        return *this;
    }

    AlignedBuf(AlignedBuf&& other) { *this = std::move(other); }

    AlignedBuf& operator=(AlignedBuf&& other) {
        mBuffer = other.mBuffer;
        mAligned = other.mAligned;
        mSize = other.mSize;

        other.mBuffer = nullptr;
        other.mAligned = nullptr;
        other.mSize = 0;

        return *this;
    }

    ~AlignedBuf() { if (mBuffer) free(mBuffer); } // account for getting moved out

    void resize(size_t newSize) {
#if (defined(__GNUC__) && !defined(__clang__) && __GNUC__ <= 4) || \
        defined(__OLD_STD_VERSION__)
        // Older g++ doesn't support std::is_trivially_copyable.
        constexpr bool triviallyCopyable =
                std::has_trivial_copy_constructor<T>::value;
#else
        constexpr bool triviallyCopyable = std::is_trivially_copyable<T>::value;
#endif
        static_assert(triviallyCopyable,
                      "AlignedBuf can only resize trivially copyable values");

        resizeImpl(newSize);
    }

    size_t size() const { return mSize; }

    T* data() { return mAligned; }

    T& operator[](size_t index) { return mAligned[index]; }

    const T& operator[](size_t index) const { return mAligned[index]; }

    bool operator==(const AlignedBuf& other) const {
        return 0 == std::memcmp(mAligned, other.mAligned, sizeof(T) * std::min(mSize, other.mSize));
    }

private:

    void resizeImpl(size_t newSize) {
        if (newSize) {
            size_t pad = std::max(align, sizeof(T));
            size_t newSizeBytes = newSize * sizeof(T) + pad;
            size_t keepSize = std::min(newSize, mSize);

            std::vector<T> temp(keepSize);
            std::copy(mAligned, mAligned + keepSize, temp.data());
            mBuffer = static_cast<uint8_t*>(realloc(mBuffer, newSizeBytes));

            mAligned = reinterpret_cast<T*>(
                    (reinterpret_cast<uintptr_t>(mBuffer) + pad) & ~(align - 1));
            std::copy(temp.data(), temp.data() + keepSize, mAligned);
        } else {
            if (mBuffer) free(mBuffer);
            mBuffer = nullptr;
            mAligned = nullptr;
        }

        mSize = newSize;
    }

    uint8_t* mBuffer = nullptr;
    T* mAligned = nullptr;
    size_t mSize = 0;
};

}  // namespace android
