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
#include "android/base/files/Stream.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_map>
#include <inttypes.h>

using android::base::Optional;
using android::base::kNullopt;

namespace android {

struct HostmemAllocInfo {
    uint64_t id = 0;
    void* host_ptr = 0;
    uint64_t size = 0;
};

using HostmemAllocMap = std::unordered_map<uint64_t, HostmemAllocInfo>;

class HostmemAlloc {
public:
    // Constructor.
    HostmemAlloc() = default;

    // Destructor.
    virtual ~HostmemAlloc() = default;

    void setGpaRange(uint64_t begin, uint64_t end);
    void setHostPtr(uint64_t id, void* host_ptr, uint64_t size);
    void unsetHostPtr(uint64_t id);

    // get() / set are NOT thread-safe,
    // but we don't expect multiple threads to call this
    // concurrently at init time, and the worst that can happen is to leak
    // a single instance. (c.f. VmLock)
    static HostmemAlloc* get();
    static HostmemAlloc* set(HostmemAlloc* dmaMap);

    // Snapshotting.
    void save(android::base::Stream* stream) const;
    void load(android::base::Stream* stream);

protected:

    // These are meant to be replaced by a real implementation
    // in subclasses.
    virtual bool addRegion(uint64_t gpa, void* hva, uint64_t size) = 0;
    virtual bool removeRegion(uint64_t gpa, uint64_t size) = 0;

    uint64_t mGpaStart = 0;
    uint64_t mGpaEnd = 0;
    uint64_t mFirstFreeGpa = 0;

    HostmemAllocMap mHostmemAllocs;
    android::base::ReadWriteLock mLock;

    DISALLOW_COPY_ASSIGN_AND_MOVE(HostmemAlloc);
};

} // namespace android
