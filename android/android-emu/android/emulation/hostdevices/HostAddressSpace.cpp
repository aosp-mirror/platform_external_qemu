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
#include "android/base/synchronization/Lock.h"
#include "android/emulation/address_space_device.h"
#include "android/emulation/address_space_device.hpp"

#include <unordered_map>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::SubAllocator;
using android::emulation::AddressSpaceDevicePingInfo;

namespace android {

class HostAddressSpaceDevice::Impl {
public:
    Impl() : mControlOps(get_address_space_device_control_ops()),
             mPhysicalOffsetAllocator(0, 16ULL * 1024ULL * 1048576ULL, 4096) { }

    void clear() {
        std::vector<uint32_t> handlesToClose;
        for (auto it : mEntries) {
            handlesToClose.push_back(it.first);
        }
        for (auto handle : handlesToClose) {
            close(handle);
        }
        mSharedRegions.clear();
        mPhysicalOffsetAllocator.freeAll();
    }

    uint32_t open() {
        AutoLock lock(mLock);
        uint32_t handle = mControlOps->gen_handle();
        auto& entry = mEntries[handle];

        entry.pingInfo = new AddressSpaceDevicePingInfo;

        lock.unlock();

        mControlOps->tell_ping_info(handle, (uint64_t)(uintptr_t)entry.pingInfo);
        return handle;
    }

    void close(uint32_t handle) {
        AutoLock lock(mLock);
        mControlOps->destroy_handle(handle);
        auto& entry = mEntries[handle];
        delete entry.pingInfo;
        mEntries.erase(handle);
    }

    uint64_t allocBlock(uint32_t handle, size_t size, uint64_t* physAddr) {
        AutoLock lock(mLock);
        return allocBlockLocked(handle, size, physAddr);
    }

    void freeBlock(uint32_t handle, uint64_t off) {
        AutoLock lock(mLock);
        freeBlockLocked(handle, off);
    }

    void setHostAddr(uint32_t handle, size_t off, void* hva) {
        AutoLock lock(mLock);
        auto& entry = mEntries[handle];
        auto& mem = entry.blocks[off];
        mem.hva = hva;
    }

    void setHostAddrByPhysAddr(uint64_t physAddr, void* hva) {
        AutoLock lock(mLock);
        if (!physAddr) return;

        uint64_t off = physAddr - mPciStart;

        for (auto &it : mEntries) {
            for (auto &it2 : it.second.blocks) {
                if (it2.first == off) {
                    it2.second.hva = hva;
                }
            }
        }

        for (auto &it : mSharedRegions) {
            if (it.first == off) {
                it.second.hva = hva;
            }
        }
    }

    void unsetHostAddrByPhysAddr(uint64_t physAddr) {
        AutoLock lock(mLock);
        if (!physAddr) return;

        uint64_t off = physAddr - mPciStart;

        for (auto &it : mEntries) {
            for (auto &it2 : it.second.blocks) {
                if (it2.first == off) {
                    it2.second.hva = nullptr;
                }
            }
        }

        for (auto &it : mSharedRegions) {
            if (it.first == off) {
                it.second.hva = nullptr;
            }
        }
    }

    void* getHostAddr(uint64_t physAddr) {
        AutoLock lock(mLock);
        if (!physAddr) return nullptr;

        uint64_t off = physAddr - mPciStart;

        // First check ping infos
        for (const auto &it : mEntries) {
            if ((uint64_t)(uintptr_t)it.second.pingInfo == physAddr) return it.second.pingInfo;
        }

        for (const auto &it : mEntries) {
            for (const auto &it2 : it.second.blocks) {
                if (it2.first == off) {
                    return it2.second.hva;
                }
            }
        }

        for (auto &it : mSharedRegions) {
            if (it.first == off) {
                return it.second.hva;
            }
        }

        return nullptr;
    }

    void ping(uint32_t handle, AddressSpaceDevicePingInfo* pingInfo) {
        AutoLock lock(mLock);

        auto& entry = mEntries[handle];
        memcpy(entry.pingInfo, pingInfo, sizeof(AddressSpaceDevicePingInfo));

        lock.unlock();

        mControlOps->ping(handle);

        lock.lock();
        memcpy(pingInfo, entry.pingInfo, sizeof(AddressSpaceDevicePingInfo));
    }

    int claimShared(uint32_t handle, uint64_t off) {
        auto& entry = mEntries[handle];

        if (mSharedRegions.find(off) == mSharedRegions.end()) {
            fprintf(stderr, "%s: failed, no shared region at offset 0x%llx\n", __func__,
                    (unsigned long long)off);
            return -EINVAL;
        }

        auto& block = mSharedRegions[off];

        if (entry.blocks.find(off) != entry.blocks.end()) {
            fprintf(stderr, "%s: failed, entry already owns offset 0x%llx\n", __func__,
                    (unsigned long long)off);
            return -EINVAL;
        }

        auto& entryBlock = entry.blocks[off];
        entryBlock.size = block.size;
        return 0;
    }

    int unclaimShared(uint32_t handle, uint64_t off) {
        auto& entry = mEntries[handle];

        if (mSharedRegions.find(off) == mSharedRegions.end()) {
            fprintf(stderr, "%s: failed, no shared region at offset 0x%llx\n", __func__,
                    (unsigned long long)off);
            return -EINVAL;
        }

        auto& block = mSharedRegions[off];

        if (entry.blocks.find(off) == entry.blocks.end()) {
            fprintf(stderr, "%s: failed, entry does not own offset 0x%llx\n", __func__,
                    (unsigned long long)off);
            return -EINVAL;
        }

        entry.blocks.erase(off);
        return 0;
    }

    void saveSnapshot(base::Stream* stream) {
        emulation::goldfish_address_space_memory_state_save(stream);
    }

    void loadSnapshot(base::Stream* stream) {
        emulation::goldfish_address_space_memory_state_load(stream);
    }

    // Simulated host interface
    int allocSharedHostRegion(uint64_t page_aligned_size, uint64_t* offset) {

        if (!offset) return -EINVAL;

        AutoLock lock(mLock);

        return allocSharedHostRegionLocked(page_aligned_size, offset);
    }

    int allocSharedHostRegionLocked(uint64_t page_aligned_size, uint64_t* offset) {
        if (!offset) return -EINVAL;

        uint64_t off = (uint64_t)(uintptr_t)mPhysicalOffsetAllocator.alloc(page_aligned_size);
        auto& block = mSharedRegions[off];
        block.size = page_aligned_size;
        (void)block;
        *offset = off;
        return 0;
    }

    int freeSharedHostRegion(uint64_t offset) {
        AutoLock lock(mLock);
        return freeSharedHostRegionLocked(offset);
    }

    int freeSharedHostRegionLocked(uint64_t offset) {
        if (mSharedRegions.find(offset) == mSharedRegions.end()) {
            fprintf(stderr, "%s: could not free shared region, offset 0x%llx is not a start\n", __func__,
                    (unsigned long long)offset);
            return -EINVAL;
        }

        mSharedRegions.erase(offset);
        mPhysicalOffsetAllocator.free((void*)(uintptr_t)offset);

        return 0;
    }

    uint64_t getPhysAddrStart() {
        AutoLock lock(mLock);
        return getPhysAddrStartLocked();
    }

    uint64_t getPhysAddrStartLocked() {
        return mPciStart;
    }

private:
    uint64_t allocBlockLocked(uint32_t handle, size_t size, uint64_t* physAddr) {
        uint64_t off = (uint64_t)(uintptr_t)mPhysicalOffsetAllocator.alloc(size);
        auto& entry = mEntries[handle];
        auto& block = entry.blocks[off];
        block.size = size;
        (void)block;
        *physAddr = mPciStart + off;
        return off;
    }

    void freeBlockLocked(uint32_t handle, uint64_t off) {
        auto& entry = mEntries[handle];
        entry.blocks.erase(off);
        mPhysicalOffsetAllocator.free((void*)(uintptr_t)off);
    }

    bool mInitialized = false;

    Lock mLock;
    address_space_device_control_ops* mControlOps = nullptr;
    uint64_t mPciStart = 0x0101010100000000;
    android::base::SubAllocator mPhysicalOffsetAllocator;

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
    std::unordered_map<uint64_t, BlockMemory> mSharedRegions;
};

static android::base::LazyInstance<HostAddressSpaceDevice> sHostAddressSpace =
    LAZY_INSTANCE_INIT;

HostAddressSpaceDevice::HostAddressSpaceDevice() :
    mImpl(new HostAddressSpaceDevice::Impl()) { }

// static
HostAddressSpaceDevice* HostAddressSpaceDevice::get() {
    auto res = sHostAddressSpace.ptr();
    res->initialize();
    return res;
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

void HostAddressSpaceDevice::setHostAddrByPhysAddr(uint64_t physAddr, void* hva) {
    mImpl->setHostAddrByPhysAddr(physAddr, hva);
}

void HostAddressSpaceDevice::unsetHostAddrByPhysAddr(uint64_t physAddr) {
    mImpl->unsetHostAddrByPhysAddr(physAddr);
}

void* HostAddressSpaceDevice::getHostAddr(uint64_t physAddr) {
    return mImpl->getHostAddr(physAddr);
}

void HostAddressSpaceDevice::ping(uint32_t handle, AddressSpaceDevicePingInfo* pingInfo) {
    mImpl->ping(handle, pingInfo);
}

int HostAddressSpaceDevice::claimShared(uint32_t handle, uint64_t off) {
    return mImpl->claimShared(handle, off);
}

int HostAddressSpaceDevice::unclaimShared(uint32_t handle, uint64_t off) {
    return mImpl->unclaimShared(handle, off);
}

void HostAddressSpaceDevice::saveSnapshot(base::Stream* stream) {
    mImpl->saveSnapshot(stream);
}

void HostAddressSpaceDevice::loadSnapshot(base::Stream* stream) {
    mImpl->loadSnapshot(stream);
}

// static
HostAddressSpaceDevice::Impl* HostAddressSpaceDevice::getImpl() {
    return HostAddressSpaceDevice::get()->mImpl.get();
}

int HostAddressSpaceDevice::allocSharedHostRegion(uint64_t page_aligned_size, uint64_t* offset) {
    return HostAddressSpaceDevice::getImpl()->allocSharedHostRegion(page_aligned_size, offset);
}

int HostAddressSpaceDevice::freeSharedHostRegion(uint64_t offset) {
    return HostAddressSpaceDevice::getImpl()->freeSharedHostRegion(offset);
}

int HostAddressSpaceDevice::allocSharedHostRegionLocked(uint64_t page_aligned_size, uint64_t* offset) {
    return HostAddressSpaceDevice::getImpl()->allocSharedHostRegionLocked(page_aligned_size, offset);
}

int HostAddressSpaceDevice::freeSharedHostRegionLocked(uint64_t offset) {
    return HostAddressSpaceDevice::getImpl()->freeSharedHostRegionLocked( offset);
}

uint64_t HostAddressSpaceDevice::getPhysAddrStart() {
    return HostAddressSpaceDevice::getImpl()->getPhysAddrStart();
}

uint64_t HostAddressSpaceDevice::getPhysAddrStartLocked() {
    return HostAddressSpaceDevice::getImpl()->getPhysAddrStartLocked();
}

static const AddressSpaceHwFuncs sAddressSpaceHwFuncs = {
    &HostAddressSpaceDevice::allocSharedHostRegion,
    &HostAddressSpaceDevice::freeSharedHostRegion,
    &HostAddressSpaceDevice::allocSharedHostRegionLocked,
    &HostAddressSpaceDevice::freeSharedHostRegionLocked,
    &HostAddressSpaceDevice::getPhysAddrStart,
    &HostAddressSpaceDevice::getPhysAddrStartLocked,
};

void HostAddressSpaceDevice::initialize() {
    if (mInitialized) return;
    address_space_set_hw_funcs(&sAddressSpaceHwFuncs);
    mInitialized = true;
}

void HostAddressSpaceDevice::clear() {
    mImpl->clear();
}

} // namespace android
