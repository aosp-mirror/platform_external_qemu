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

#define HASD_DEBUG 0

#if HASD_DEBUG
#define HASD_LOG(fmt,...) printf("%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define HASD_LOG(fmt,...)
#endif

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
        mControlOps->clear();
    }

    uint32_t open() {
        uint32_t handle = mControlOps->gen_handle();

        AutoLock lock(mLock);
        auto& entry = mEntries[handle];

        entry.pingInfo = new AddressSpaceDevicePingInfo;

        lock.unlock();

        mControlOps->tell_ping_info(handle, (uint64_t)(uintptr_t)entry.pingInfo);
        return handle;
    }

    void close(uint32_t handle) {
        mControlOps->destroy_handle(handle);

        AutoLock lock(mLock);
        auto& entry = mEntries[handle];
        delete entry.pingInfo;
        mEntries.erase(handle);
    }

    uint64_t allocBlock(uint32_t handle, size_t size, uint64_t* physAddr) {
        AutoLock lock(mLock);
        return allocBlockLocked(handle, size, physAddr);
    }

    void freeBlock(uint32_t handle, uint64_t off) {
        // mirror hw/pci/goldfish_address_space.c:
        // first run deallocation callbacks, then update the state
        mControlOps->run_deallocation_callbacks(kPciStart + off);

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
        if (!physAddr) return;
        const uint64_t off = physAddr - kPciStart;

        AutoLock lock(mLock);
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
        if (!physAddr) return;
        const uint64_t off = physAddr - kPciStart;

        AutoLock lock(mLock);
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
        HASD_LOG("get hva of 0x%llx", (unsigned long long)physAddr);

        if (!physAddr) return nullptr;
        const uint64_t off = physAddr - kPciStart;

        AutoLock lock(mLock);
        HASD_LOG("get hva of off 0x%llx", (unsigned long long)off);
        void* res = 0;

        // First check ping infos
        for (const auto &it : mEntries) {
            if ((uint64_t)(uintptr_t)it.second.pingInfo == physAddr) return it.second.pingInfo;
        }

        for (const auto &it : mEntries) {
            for (const auto &it2 : it.second.blocks) {
                if (blockContainsOffset(it2.first, it2.second, off)) {
                    HASD_LOG("entry [0x%llx 0x%llx] contains. hva: %p",
                             (unsigned long long)it2.first,
                             (unsigned long long)it2.first + it2.second.size,
                             it2.second.hva);
                    res = ((char*)it2.second.hva) +
                        offsetIntoBlock(it2.first, it2.second, off);
                }
            }
        }

        for (auto &it : mSharedRegions) {
            if (blockContainsOffset(it.first, it.second, off)) {
                HASD_LOG("shared region [0x%llx 0x%llx] contains. hva: %p",
                         (unsigned long long)it.first,
                         (unsigned long long)it.first + it.second.size,
                         it.second.hva);
                res = ((char*)it.second.hva) +
                    offsetIntoBlock(it.first, it.second, off);
            }
        }

        return res;
    }

    static uint64_t offsetToPhysAddr(uint64_t offset) {
        return kPciStart + offset;
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

    int claimShared(uint32_t handle, uint64_t off, uint64_t size) {
        auto& entry = mEntries[handle];

        if (entry.blocks.find(off) != entry.blocks.end()) {
            fprintf(stderr, "%s: failed, entry already owns offset 0x%llx\n", __func__,
                    (unsigned long long)off);
            return -EINVAL;
        }

        if (!enclosingSharedRegionExists(mSharedRegions, off, size)) {
            fprintf(stderr, "%s: failed, no shared region enclosing [0x%llx 0x%llx]\n", __func__,
                    (unsigned long long)off,
                    (unsigned long long)off + size);
            return -EINVAL;
        }

        auto& entryBlock = entry.blocks[off];
        entryBlock.size = size;
        return 0;
    }

    int unclaimShared(uint32_t handle, uint64_t off) {
        auto& entry = mEntries[handle];

        if (entry.blocks.find(off) == entry.blocks.end()) {
            fprintf(stderr, "%s: failed, entry does not own offset 0x%llx\n", __func__,
                    (unsigned long long)off);
            return -EINVAL;
        }

        if (!enclosingSharedRegionExists(mSharedRegions, off, entry.blocks[off].size)) {
            fprintf(stderr, "%s: failed, no shared region enclosing [0x%llx 0x%llx]\n", __func__,
                    (unsigned long long)off,
                    (unsigned long long)off + entry.blocks[off].size);
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

        HASD_LOG("new shared region: [0x%llx 0x%llx]",
                 (unsigned long long)off,
                 (unsigned long long)off + page_aligned_size);
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

        HASD_LOG("free shared region @ 0x%llx",
                 (unsigned long long)offset);

        mSharedRegions.erase(offset);
        mPhysicalOffsetAllocator.free((void*)(uintptr_t)offset);

        return 0;
    }

    static uint64_t getPhysAddrStart() {
        return kPciStart;
    }

private:
    struct BlockMemory {
        size_t size = 0;
        void* hva = nullptr;
    };

    using MemoryMap = std::unordered_map<uint64_t, BlockMemory>;

    uint64_t allocBlockLocked(uint32_t handle, size_t size, uint64_t* physAddr) {
        uint64_t off = (uint64_t)(uintptr_t)mPhysicalOffsetAllocator.alloc(size);
        auto& entry = mEntries[handle];
        auto& block = entry.blocks[off];
        block.size = size;
        (void)block;
        *physAddr = kPciStart + off;
        return off;
    }

    void freeBlockLocked(uint32_t handle, uint64_t off) {
        auto& entry = mEntries[handle];
        entry.blocks.erase(off);
        mPhysicalOffsetAllocator.free((void*)(uintptr_t)off);
    }

    bool blockContainsOffset(
        uint64_t offset,
        const BlockMemory& block,
        uint64_t physAddr) const {
        return offset <= physAddr &&
               offset + block.size > physAddr;
    }

    uint64_t offsetIntoBlock(
        uint64_t offset,
        const BlockMemory& block,
        uint64_t physAddr) const {
        if (!blockContainsOffset(offset, block, physAddr)) {
            fprintf(stderr, "%s: block at [0x%" PRIx64 " 0x%" PRIx64"] does not contain 0x%" PRIx64 "!\n", __func__,
                    offset,
                    offset + block.size,
                    physAddr);
            abort();
        }
        return physAddr - offset;
    }

    bool enclosingSharedRegionExists(
        const MemoryMap& memoryMap, uint64_t offset, uint64_t size) const {
        for (const auto it : memoryMap) {
            if (it.first <= offset &&
                it.first + it.second.size >= offset + size)
                return true;
        }
        return false;
    }

    bool mInitialized = false;

    static const uint64_t kPciStart = 0x0101010100000000;

    Lock mLock;
    address_space_device_control_ops* mControlOps = nullptr;
    android::base::SubAllocator mPhysicalOffsetAllocator;

    struct Entry {
        AddressSpaceDevicePingInfo* pingInfo = nullptr;
        MemoryMap blocks;
    };

    std::unordered_map<uint32_t, Entry> mEntries;
    MemoryMap mSharedRegions;
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

uint64_t HostAddressSpaceDevice::offsetToPhysAddr(uint64_t offset) const {
    return mImpl->offsetToPhysAddr(offset);
}

void HostAddressSpaceDevice::ping(uint32_t handle, AddressSpaceDevicePingInfo* pingInfo) {
    mImpl->ping(handle, pingInfo);
}

int HostAddressSpaceDevice::claimShared(uint32_t handle, uint64_t off, uint64_t size) {
    return mImpl->claimShared(handle, off, size);
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

static const AddressSpaceHwFuncs sAddressSpaceHwFuncs = {
    &HostAddressSpaceDevice::allocSharedHostRegion,
    &HostAddressSpaceDevice::freeSharedHostRegion,
    &HostAddressSpaceDevice::allocSharedHostRegionLocked,
    &HostAddressSpaceDevice::freeSharedHostRegionLocked,
    &HostAddressSpaceDevice::getPhysAddrStart,
    &HostAddressSpaceDevice::getPhysAddrStart,
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
