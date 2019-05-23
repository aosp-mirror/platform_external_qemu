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

#include "android/base/AlignedBuf.h"

#include <vector>
#include <unordered_set>

#include <inttypes.h>

namespace android {
namespace base {

// Class to make it easier to set up memory regions where it is fast
// to allocate buffers AND we don't care about freeing individual pieces,
// BUT it's necessary to preserve previous pointer values in between the first
// alloc() after a freeAll(), and the freeAll() itself, allowing some sloppy use of
// malloc in the first pass while we find out how much data was needed.
class BumpPool {
public:
    BumpPool(size_t startingBytes = 4096) : mStorage(startingBytes / sizeof(uint64_t))  { }
    // All memory allocated by this pool
    // is automatically deleted when the pool
    // is deconstructed.
    ~BumpPool() { }

    void* alloc(size_t wantedSize) {
        size_t wantedSizeRoundedUp =
            sizeof(uint64_t) * ((wantedSize + sizeof(uint64_t) - 1) / (sizeof(uint64_t)));

        mTotalWantedThisGeneration += wantedSizeRoundedUp;
        if (mAllocPos + wantedSizeRoundedUp > mStorage.size()) {
            mNeedRealloc = true;
            void* fallbackPtr = malloc(wantedSizeRoundedUp);
            mFallbackPtrs.insert(fallbackPtr);
            return fallbackPtr;
        }
        size_t avail = mStorage.size() - mAllocPos;
        void* allocPtr = (void*)(mStorage.data() + mAllocPos);
        mAllocPos += wantedSizeRoundedUp;
        return allocPtr;
    }

    void freeAll() {
        mAllocPos = 0;
        if (mNeedRealloc) {
            mStorage.resize((mTotalWantedThisGeneration * 2) / sizeof(uint64_t));
            mNeedRealloc = false;
            for (auto ptr : mFallbackPtrs) {
                free(ptr);
            }
            mFallbackPtrs.clear();
        }
        mTotalWantedThisGeneration = 0;
    }

    // Convenience function to allocate an array
    // of objects of type T.
    template <class T>
    T* allocArray(size_t count) {
        size_t bytes = sizeof(T) * count;
        void* res = alloc(bytes);
        return (T*) res;
    }

    char* strDup(const char* toCopy) {
        size_t bytes = strlen(toCopy) + 1;
        void* res = alloc(bytes);
        memset(res, 0x0, bytes);
        memcpy(res, toCopy, bytes);
        return (char*)res;
    }

    char** strDupArray(const char* const* arrayToCopy, size_t count) {
        char** res = allocArray<char*>(count);

        for (size_t i = 0; i < count; i++) {
            res[i] = strDup(arrayToCopy[i]);
        }

        return res;
    }

    void* dupArray(const void* buf, size_t bytes) {
        void* res = alloc(bytes);
        memcpy(res, buf, bytes);
        return res;
    }

private:
    AlignedBuf<uint64_t, 8> mStorage;
    std::unordered_set<void*> mFallbackPtrs;
    size_t mAllocPos = 0;
    size_t mTotalWantedThisGeneration = 0;
    bool mNeedRealloc = false;
};

} // namespace base
} // namespace android
