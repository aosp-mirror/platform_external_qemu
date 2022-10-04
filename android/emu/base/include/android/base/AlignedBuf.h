// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "android/base/Compiler.h"

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <vector>

#include <stdio.h>

#ifdef _WIN32
#include <malloc.h>
#endif
#include "android/utils/debug.h"
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
            std::copy(other.mBuffer, other.mBuffer + other.mSize, mBuffer);
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
        mSize = other.mSize;

        other.mBuffer = nullptr;
        other.mSize = 0;

        return *this;
    }

    ~AlignedBuf() { if (mBuffer) freeImpl(mBuffer); } // account for getting moved out

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

    T* data() { return mBuffer; }

    T& operator[](size_t index) { return mBuffer[index]; }

    const T& operator[](size_t index) const { return mBuffer[index]; }

    bool operator==(const AlignedBuf& other) const {
        return 0 == std::memcmp(mBuffer, other.mBuffer, sizeof(T) * std::min(mSize, other.mSize));
    }

private:

    void resizeImpl(size_t newSize) {
        if (newSize) {
            size_t pad = std::max(align, sizeof(T));
            size_t keepSize = std::min(newSize, mSize);
            size_t newSizeBytes = ((align - 1 + newSize * sizeof(T) + pad) / align) * align;

            std::vector<T> temp(mBuffer, mBuffer + keepSize);
            mBuffer = static_cast<T*>(reallocImpl(mBuffer, newSizeBytes));
            std::copy(temp.data(), temp.data() + keepSize, mBuffer);
        } else {
            if (mBuffer) freeImpl(mBuffer);
            mBuffer = nullptr;
        }

        mSize = newSize;
    }

    void* reallocImpl(void* oldPtr, size_t sizeBytes) {
        if (oldPtr) { freeImpl(oldPtr); }
        // Platform aligned malloc might not behave right
        // if we give it an alignemnt value smaller than sizeof(void*).
        size_t actualAlign = std::max(align, sizeof(void*));
#ifdef _WIN32
        return _aligned_malloc(sizeBytes, actualAlign);
#else
        void* res;
        if (posix_memalign(&res, actualAlign, sizeBytes)) {
            derror("%s: failed to alloc aligned memory", __func__);
            abort();
        }
        return res;
#endif
    }

    void freeImpl(void* ptr) {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        free(ptr);
#endif

    }

    T* mBuffer = nullptr;
    size_t mSize = 0;
};

// Convenience function for aligned malloc across platforms
void* aligned_buf_alloc(size_t align, size_t size);
void aligned_buf_free(void* buf);

}  // namespace android
