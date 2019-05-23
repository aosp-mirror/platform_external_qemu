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
#include "android/emulation/hostdevices/HostAddressSpace.h"

#include "android/base/SubAllocator.h"
#include "android/base/memory/LazyInstance.h"
#include "android/emulation/address_space_device.h"

#include <unordered_map>

using android::base::LazyInstance;
using android::base::SubAllocator;
using android::emulation::AddressSpaceDevicePingInfo;

namespace android {

class HostAddressSpaceDevice::Impl {
public:
    Impl() : mControlOps(create_or_get_address_space_device_control_ops()),
             mAlloc(0, 16ULL * 1024ULL * 1048576ULL, 4096) { }

    uint32_t open() {
        uint32_t handle = mControlOps->gen_handle();
        auto& entry = mEntries[handle];
        entry.pingInfo = new AddressSpaceDevicePingInfo;
        mControlOps->tell_ping_info(handle, (uint64_t)(uintptr_t)entry.pingInfo);
        return handle;
    }

    void close(uint32_t handle) {
        mControlOps->destroy_handle(handle);
        auto& entry = mEntries[handle];
        delete entry.pingInfo;
        mEntries.erase(handle);
    }

    uint64_t allocBlock(uint32_t handle, size_t size, uint64_t* physAddr) {
        uint64_t off = (uint64_t)(uintptr_t)mAlloc.alloc(size);
        auto& entry = mEntries[handle];
        auto& block = entry.blocks[off];
        block.size = size;
        (void)block;
        *physAddr = mPciStart + off;
        return off;
    }

    void freeBlock(uint32_t handle, uint64_t off) {
        auto& entry = mEntries[handle];
        entry.blocks.erase(off);
        mAlloc.free((void*)(uintptr_t)off);
    }

    void setHostAddr(uint32_t handle, size_t off, void* hva) {
        auto& entry = mEntries[handle];
        auto& mem = entry.blocks[off];
        mem.hva = hva;
    }

    void ping(uint32_t handle, AddressSpaceDevicePingInfo* pingInfo) {
        auto& entry = mEntries[handle];
        memcpy(entry.pingInfo, pingInfo, sizeof(AddressSpaceDevicePingInfo));
        mControlOps->ping(handle);
        memcpy(pingInfo, entry.pingInfo, sizeof(AddressSpaceDevicePingInfo));
    }

    void saveSnapshot(base::Stream* stream) {
        emulation::host_goldfish_address_space_memory_state_save(stream);
    }

    void loadSnapshot(base::Stream* stream) {
        emulation::host_goldfish_address_space_memory_state_load(stream);
    }

private:
    address_space_device_control_ops* mControlOps = nullptr;
    uint64_t mPciStart = 0x1200000000;
    android::base::SubAllocator mAlloc;

    struct BlockMemory {
        size_t size = 0;
        void* hva = nullptr;
    };

    struct Entry {
        AddressSpaceDevicePingInfo* pingInfo = nullptr;
        // Offset, size
        std::unordered_map<uint64_t, BlockMemory> blocks;
    };

    std::unordered_map<uint32_t, Entry> mEntries;
};

static android::base::LazyInstance<HostAddressSpaceDevice> sHostAddressSpace =
    LAZY_INSTANCE_INIT;

HostAddressSpaceDevice::HostAddressSpaceDevice() :
    mImpl(new HostAddressSpaceDevice::Impl()) { }

// static
HostAddressSpaceDevice* HostAddressSpaceDevice::get() {
    return sHostAddressSpace.ptr();
}

uint32_t HostAddressSpaceDevice::open() {
    return mImpl->open();
}

void HostAddressSpaceDevice::close(uint32_t handle) {
    mImpl->close(handle);
}

uint64_t HostAddressSpaceDevice::allocBlock(uint32_t handle, size_t size, uint64_t* physAddr) {
    return mImpl->allocBlock(handle, size, physAddr);
}

void HostAddressSpaceDevice::freeBlock(uint32_t handle, uint64_t off) {
    return mImpl->freeBlock(handle, off);
}

void HostAddressSpaceDevice::setHostAddr(uint32_t handle, size_t off, void* hva) {
    return mImpl->setHostAddr(handle, off, hva);
}

void HostAddressSpaceDevice::ping(uint32_t handle, AddressSpaceDevicePingInfo* pingInfo) {
    mImpl->ping(handle, pingInfo);
}

void HostAddressSpaceDevice::saveSnapshot(base::Stream* stream) {
    mImpl->saveSnapshot(stream);
}

void HostAddressSpaceDevice::loadSnapshot(base::Stream* stream) {
    mImpl->loadSnapshot(stream);
}

} // namespace android
