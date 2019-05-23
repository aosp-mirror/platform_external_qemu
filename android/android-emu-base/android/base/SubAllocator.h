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

#include <inttypes.h>
#include <stddef.h>
#include <string.h>

namespace android {
namespace base {

class Stream;

// Class to create sub-allocations in an existing buffer. Similar interface to
// Pool, but underlying mechanism is different as it's difficult to combine
// same-size heaps in Pool with a preallocated buffer.
class SubAllocator {
public:
    // |pageSize| determines both the alignment of pointers returned
    // and the multiples of space occupied.
    SubAllocator(
        void* buffer,
        uint64_t totalSize,
        uint64_t pageSize);

    // Memory is freed from the perspective of the user of
    // SubAllocator, but the prealloced buffer is not freed.
    ~SubAllocator();

    // Snapshotting
    bool save(Stream* stream);
    bool load(Stream* stream);
    bool postLoad(void* postLoadBuffer);

    // returns null if the allocation cannot be satisfied.
    void* alloc(size_t wantedSize);
    // returns true if |ptr| came from alloc(), false otherwise
    bool free(void* ptr);
    void freeAll();
    uint64_t getOffset(void* ptr);

    bool empty() const;

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
};

} // namespace base
} // namespace android
