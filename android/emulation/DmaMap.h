// Copyright 2016 The Android Open Source Project
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
#include "android/base/synchronization/Lock.h"

#include <unordered_map>
#include <inttypes.h>

namespace android {

struct DmaBufferInfo {
    void* hwpipe = nullptr;
    uint64_t guestAddr = 0;
    uint64_t sz = 0;
    uint64_t currHostAddr = 0;
    bool hostAddrValid = true;
};

class DmaMap {
public:
    // Constructor.
    DmaMap() = default;

    // Destructor.
    virtual ~DmaMap() = default;

    virtual void addBuffer(void* hwpipe, uint64_t addr, uint64_t sz);
    virtual void removeBuffer(uint64_t addr);
    virtual uint64_t getHostAddr(uint64_t addr);
    virtual void invalidateHostMappings(void);
    virtual void* getPipeInstance(uint64_t addr);

    static DmaMap* get();
    static DmaMap* set(DmaMap* dmaMap);
protected:

    virtual void createMappingLocked(DmaBufferInfo* info);
    virtual void removeMappingLocked(DmaBufferInfo* info);

    // These are meant to be replaced by a real implementation
    // in subclasses.
    virtual uint64_t doMap(uint64_t addr, uint64_t sz) {
        return addr;
    }
    virtual void doUnmap(uint64_t addr, uint64_t sz) { }

    std::unordered_map<uint64_t, DmaBufferInfo> mDmaBuffers;
    android::base::ReadWriteLock mLock;
    DISALLOW_COPY_ASSIGN_AND_MOVE(DmaMap);
};

} // namespace android
