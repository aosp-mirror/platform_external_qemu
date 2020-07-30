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
#pragma once

#include "android/emulation/AddressSpaceService.h"

#include <memory>

#include <inttypes.h>
namespace android {

namespace base {

class Stream;

} // namespace base

class HostAddressSpaceDevice {
public:
    HostAddressSpaceDevice();
    static HostAddressSpaceDevice* get();

    void initialize();

    void clear();

    uint32_t open();
    void close(uint32_t handle);

    uint64_t allocBlock(uint32_t handle, size_t size, uint64_t* physAddr);
    void freeBlock(uint32_t handle, uint64_t off);
    void setHostAddr(uint32_t handle, size_t off, void* hva);

    void setHostAddrByPhysAddr(uint64_t physAddr, void* hva);
    void unsetHostAddrByPhysAddr(uint64_t physAddr);
    void* getHostAddr(uint64_t physAddr);
    uint64_t offsetToPhysAddr(uint64_t offset) const;

    void ping(uint32_t handle, emulation::AddressSpaceDevicePingInfo* pingInfo);

    int claimShared(uint32_t handle, uint64_t off, uint64_t size);
    int unclaimShared(uint32_t handle, uint64_t off);

    void saveSnapshot(base::Stream* stream);
    void loadSnapshot(base::Stream* stream);

    // Callbacks for AddressSpaceHwFuncs.
    static int allocSharedHostRegion(uint64_t page_aligned_size, uint64_t* offset);
    static int freeSharedHostRegion(uint64_t offset);
    static int allocSharedHostRegionLocked(uint64_t page_aligned_size, uint64_t* offset);
    static int freeSharedHostRegionLocked(uint64_t offset);
    static uint64_t getPhysAddrStart();
    static uint64_t getPhysAddrStartLocked();

private:
    class Impl;
    static Impl* getImpl();

    bool mInitialized = false;
    std::unique_ptr<Impl> mImpl;
};

} // namespace android
