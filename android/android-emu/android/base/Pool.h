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

namespace android {
namespace base {

class Pool {
public:
    Pool(size_t minAllocSize = 8,
         size_t maxFastSize = 4096,
         size_t chunksPerSize = 256);
    ~Pool();

    void* alloc(size_t wantedSize);
    void free(void* ptr);

private:
    class Impl;
    Impl* mImpl = nullptr;

    std::unordered_set<void*> mFallbackPtrs;
};

} // namespace base
} // namespace android
