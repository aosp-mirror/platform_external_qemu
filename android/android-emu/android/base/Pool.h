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

#include <unordered_set>

#include <inttypes.h>
#include <stddef.h>
#include <string.h>

namespace android {
namespace base {

// Class to make it easier to set up memory regions where it is fast
// to allocate/deallocate buffers that have size within
// the specified range.
class Pool {
public:
    // minSize/maxSize: the target range of sizes for which we want to
    // make allocations fast. the greater the range, the more space
    // traded off.
    // chunksPerSize: the target maximum number of live objects of
    // each size that are expected. the higher it is, the more space
    // traded off.
    //
    // Rough space cost formula:
    // O(chunksPerSize * log2(maxSize / minSize) * maxSize)
    Pool(size_t minSize = 8,
         size_t maxSize = 4096,
         size_t chunksPerSize = 256);

    // All memory allocated by this pool
    // is automatically deleted when the pool
    // is deconstructed.
    ~Pool();

    void* alloc(size_t wantedSize);
    void free(void* ptr);

    // Convenience function to free everything currently allocated.
    void freeAll();

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
    class Impl;
    Impl* mImpl = nullptr;

    std::unordered_set<void*> mFallbackPtrs;
};

} // namespace base
} // namespace android
