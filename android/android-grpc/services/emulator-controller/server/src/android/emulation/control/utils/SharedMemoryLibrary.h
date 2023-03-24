// Copyright (C) 2020 The Android Open Source Project
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

#include <stddef.h>  // for size_t

#include <functional>     // for function
#include <memory>         // for unique_ptr
#include <mutex>          // for mutex
#include <string>         // for string, hash, operator==
#include <unordered_map>  // for unordered_map

#include "aemu/base/memory/ScopedPtr.h"     // for makeCustomScopedPtr
#include "aemu/base/memory/SharedMemory.h"  // for SharedMemory

namespace android {
namespace emulation {
namespace control {

using android::base::SharedMemory;

// A SharedMemoryLibrary can be used to borrow shared memory objects that are
// ref counted. You can borrow a shared memory handle for a given region, and
// return it when you are done with it. This makes sure that a memory mapped
// region only has to be initialized once if it is going to be used frequently.
//
// The borrowed objects will be opened with read+write access, and only the
// initial size specification will be used.
//
// For example:
//
// SharedMemoryLibrary lib;
// {
// auto mem = lib.borrow("foo", 124);
// ... do things with mem ...
// auto mem2 = lib.borrow("foo", 124);
// ... do things with mem2 (points to same memory region)
// }
class SharedMemoryLibrary {
public:
    using deleter = std::function<void(SharedMemory*)>;
    using LibraryEntry =
            std::unique_ptr<SharedMemory, android::base::FuncDelete<deleter>>;

    // Borrow the given shared memory region, opening it if it has not been
    // opened yet. The LibraryEntry will automatically return the shared memory
    // handle when it goes out of scope.
    LibraryEntry borrow(std::string handle, size_t size);

private:
    // Returns the given handle, releasing it completely if the refcount is 0.
    void release(std::string handle);

    std::mutex mAccess;
    std::unordered_map<std::string, int> mHandlesCnt;
    std::unordered_map<std::string, std::unique_ptr<SharedMemory>> mMemMap;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
