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
#include <memory>
#include <type_traits>

namespace android {

template <class T, size_t align>
class AlignedBuf {
public:
    explicit AlignedBuf(size_t size) : mRefCount(new std::atomic<int>(1)) {
        static_assert(align && ((align & (align - 1)) == 0),
                      "AlignedBuf only supports power-of-2 aligments.");
        resizeImpl(size);
    }

    AlignedBuf(const AlignedBuf& other)
        : mBuffer(other.mBuffer),
          mAligned(other.mAligned),
          mSize(other.mSize),
          mRefCount(other.mRefCount) {
        incRef();
    }

    AlignedBuf& operator=(const AlignedBuf& other) {
        if (this != &other) {
            decRef();

            mBuffer = other.mBuffer;
            mAligned = other.mAligned;
            mSize = other.mSize;
            mRefCount = other.mRefCount;
        }

        return *this;
    }

    AlignedBuf(AlignedBuf&& other) { *this = std::move(other); }

    AlignedBuf& operator=(AlignedBuf&& other) {
        if (this != &other) {
            decRef();

            mBuffer = other.mBuffer;
            mAligned = other.mAligned;
            mSize = other.mSize;
            mRefCount = other.mRefCount;

            other.mBuffer = nullptr;
            other.mAligned = nullptr;
            other.mSize = 0;
            other.mRefCount = nullptr;
        }

        return *this;
    }

    ~AlignedBuf() { decRef(); }

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

private:
    void incRef() {
        std::atomic_fetch_add_explicit(mRefCount, 1, std::memory_order_seq_cst);
    }

    bool decRef() {
        if (!mRefCount)
            return true;  // Got moved out

        bool lastRef = std::atomic_fetch_sub_explicit(
                               mRefCount, 1, std::memory_order_seq_cst) == 1;

        if (lastRef) {
            free(mBuffer);
            delete mRefCount;
        }

        return lastRef;
    }

    void resizeImpl(size_t newSize) {
        size_t pad = std::max(align, sizeof(T));
        size_t newSizeBytes = newSize * sizeof(T) + pad;

        mBuffer = static_cast<uint8_t*>(realloc(mBuffer, newSizeBytes));

        mAligned = reinterpret_cast<T*>(
                (reinterpret_cast<uintptr_t>(mBuffer) + pad) & ~(align - 1));

        mSize = newSize;
    }

    uint8_t* mBuffer = nullptr;
    T* mAligned = nullptr;
    size_t mSize = 0;
    std::atomic<int>* mRefCount = nullptr;
};

}  // namespace android
