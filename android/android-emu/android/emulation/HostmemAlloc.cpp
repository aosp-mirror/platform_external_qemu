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

#include "android/emulation/HostmemAlloc.h"

#include "android/base/containers/Lookup.h"
#include "android/base/files/StreamSerializing.h"
#include "android/base/memory/LazyInstance.h"
#include "android/emulation/goldfish_hostmem.h"

#include <type_traits>
#include <inttypes.h>
#include <stdio.h>

#define DEBUG 0

#if DEBUG >= 1
#define D(fmt,...) fprintf(stderr, "HostmemAlloc: %s: " fmt "\n", __func__, ##__VA_ARGS__);
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2
#define DD(fmt,...) fprintf(stderr, "HostmemAlloc: %s: " fmt "\n", __func__, ##__VA_ARGS__);
#else
#define DD(...) (void)0
#endif

#define E(fmt,...) fprintf(stderr, "HostmemAlloc: ERROR: %s: " fmt "\n", __func__, ##__VA_ARGS__);

namespace android {

static HostmemAlloc* sInstance = nullptr;

HostmemAlloc* HostmemAlloc::get() {
    return sInstance;
}

HostmemAlloc* HostmemAlloc::set(HostmemAlloc* dmaMap) {
    HostmemAlloc* result = sInstance;
    sInstance = dmaMap;
    return result;
}

void HostmemAlloc::setGpaRange(uint64_t begin, uint64_t end) {
    D("%llu - %llu", (unsigned long long)begin, (unsigned long long)end);
    // TODO
}

void HostmemAlloc::setHostPtr(uint64_t id, void* host_ptr, uint64_t size) {
    D("id %llu host %p sz %llu", (unsigned long long)id, host_ptr, (unsigned long long)size);
    goldfish_hostmem_set_ptr(id, host_ptr, size, 1);
    // TODO
}

void HostmemAlloc::unsetHostPtr(uint64_t id) {
    D("id %llu",  (unsigned long long)id);
    // TODO
}

void HostmemAlloc::save(android::base::Stream* stream) const {
    // TODO
}

void HostmemAlloc::load(android::base::Stream* stream) {
    mHostmemAllocs.clear();
    // TODO
}

}  // namespace android
