// Copyright (C) 2019 The Android Open Source Project
// Copyright (C) 2019 Google Inc.
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

#include "android/base/SubAllocator.h"
#include "android/base/memory/LazyInstance.h"

// AddressSpaceHost is a global class for obtaining physical addresses
// for the host-side build.
class AddressSpaceHost {
public:
    AddressSpaceHost() : mAlloc(0, 16ULL * 1024ULL * 1048576ULL, 4096) { }
    uint64_t alloc(size_t size) {
        return (uint64_t)(uintptr_t)mAlloc.alloc(size);
    }
    void free(uint64_t addr) {
        mAlloc.free((void*)(uintptr_t)addr);
    }
private:
    android::base::SubAllocator mAlloc;
};

static android::base::LazyInstance<AddressSpaceHost> sAddressSpaceHost =
    LAZY_INSTANCE_INIT;

