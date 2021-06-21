// Copyright 2021 The Android Open Source Project
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

// A generic memory allocator interface which could be used to allocate
// a certain size of memory region, or memory region for arrays / strings.
// How the memory are recycled / freed is up to derived classes.
class Allocator {
public:
    Allocator() = default;
    virtual ~Allocator() = default;

    virtual void* alloc(size_t wantedSize) = 0;

    // Convenience function to allocate an array
    // of objects of type T.
    template <class T>
    T* allocArray(size_t count) {
        size_t bytes = sizeof(T) * count;
        void* res = alloc(bytes);
        return (T*)res;
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
};

}  // namespace base
}  // namespace android
