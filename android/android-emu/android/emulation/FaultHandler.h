// Copyright 2019 The Android Open Source Project
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

#include "android/base/synchronization/Lock.h"
#include "android/snapshot/MemoryWatch.h"

#include <functional>
#include <unordered_map>

#include <inttypes.h>

namespace android {

// Class to register some callbacks when the guest accesses certain memory addresses.
// Assumes the snapshot for the page has been loaded.
class FaultHandler {
public:
    using OnFaultCallback = std::function<void()>;
    using Id = int;

    FaultHandler();
    ~FaultHandler();

    static FaultHandler* get();

    void registerMemoryMappingFuncs(
        guest_mem_map_t guest_mem_map_call,
        guest_mem_unmap_t guest_mem_unmap_call,
        guest_mem_protect_t guest_mem_protect_call,
        guest_mem_remap_t guest_mem_remap_call,
        guest_mem_protection_supported_t guest_mem_protection_supported_call
    ) {
        mGuestMemMap = guest_mem_map_call;
        mGuestMemUnmap = guest_mem_unmap_call;
        mGuestMemProtect = guest_mem_protect_call;
        mGuestMemRemap = guest_mem_remap_call;
        mGuestMemProtectionSupported = guest_mem_protection_supported_call;
    }

    bool supportsFaultHandling() const;

    Id addFaultHandler(
        uint64_t gpaStart, uint64_t size, OnFaultCallback cb);
    void removeFaultHandler(Id id);

    void setupFault(Id id);
    void teardownFault(Id id);
    void runHandlersForFault(void* hva, uint64_t size);

private:

    struct HandlerInfo {
        uint64_t gpaStart;
        uint64_t size;
        OnFaultCallback cb;
    };

    std::unordered_map<Id, HandlerInfo> mStorage;
    Id mNextId = 0;
    base::Lock mLock;

    guest_mem_map_t mGuestMemMap;
    guest_mem_unmap_t mGuestMemUnmap;
    guest_mem_protect_t mGuestMemProtect;
    guest_mem_remap_t mGuestMemRemap;
    guest_mem_protection_supported_t mGuestMemProtectionSupported;
};

} // namespace android