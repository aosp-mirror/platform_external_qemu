// Copyright 2020 The Android Open Source Project
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

#include "android/emulation/address_space_shared_slots_host_memory_allocator.h"
#include "android/emulation/address_space_device.hpp"
#include "android/emulation/control/vm_operations.h"
#include "android/base/AlignedBuf.h"
#include "android/base/synchronization/Lock.h"
#include "android/crashreport/crash-handler.h"
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <utility>

namespace android {
namespace emulation {
namespace {
using base::AutoLock;
using base::Lock;

constexpr uint32_t alignment = 4096;

uint64_t allocateAddressSpaceBlock(uint32_t size) {
    const auto hw = get_address_space_device_hw_funcs();

    uint64_t offset;
    if (hw->allocSharedHostRegion(size, &offset)) {
        return 0;
    } else {
        return hw->getPhysAddrStart() + offset;
    }
}

int freeAddressBlock(uint64_t phys) {
    const auto hw = get_address_space_device_hw_funcs();
    const uint64_t start = hw->getPhysAddrStart();
    if (phys < start) { return -1; }
    return hw->freeSharedHostRegion(phys - start);
}

struct MemBlock {
    // offset -> size
    typedef std::map<uint32_t, uint32_t> FreeSubblocks_t;
    typedef std::unordered_map<uint32_t, uint32_t> OccupiedSubblocks_t;

    MemBlock(): ops(nullptr), physBase(0), physBaseLoaded(0), bits(nullptr), bitsSize(0) {}

    MemBlock(const address_space_device_control_ops* o, uint32_t sz): ops(o) {
        bits = android::aligned_buf_alloc(alignment, sz);
        bitsSize = sz;
        physBase = allocateAddressSpaceBlock(sz);
        if (!physBase) {
            // TODO: crash
        }
        physBaseLoaded = 0;
        if (ops->add_memory_mapping(physBase, bits, bitsSize)) {
            // TODO: crash
        }

        freeSubblocks.insert({0, sz});
    }

    MemBlock(MemBlock&& rhs)
        : ops(std::exchange(rhs.ops, nullptr)),
          physBase(std::exchange(rhs.physBase, 0)),
          physBaseLoaded(std::exchange(rhs.physBaseLoaded, 0)),
          bits(std::exchange(rhs.bits, nullptr)),
          bitsSize(std::exchange(rhs.bitsSize, 0)),
          freeSubblocks(std::move(rhs.freeSubblocks)),
          occupiedSubblocks(std::move(rhs.occupiedSubblocks)) {}

    friend void swap(MemBlock& lhs, MemBlock& rhs) {
        using std::swap;

        swap(lhs.physBase,          rhs.physBase);
        swap(lhs.physBaseLoaded,    rhs.physBaseLoaded);
        swap(lhs.bits,              rhs.bits);
        swap(lhs.bitsSize,          rhs.bitsSize);
        swap(lhs.freeSubblocks,     rhs.freeSubblocks);
        swap(lhs.occupiedSubblocks, rhs.occupiedSubblocks);
    }

    ~MemBlock() {
        if (!occupiedSubblocks.empty()) {
            // TODO: crash
        }

        if (freeSubblocks.size() != 1) {
            // TODO: crash
        }

        if (physBase) {
            ops->remove_memory_mapping(physBase, bits, bitsSize);
            freeAddressBlock(physBase);
            android::aligned_buf_free(bits);
        }
    }

    uint64_t allocate(const size_t requestedSize) {
        FreeSubblocks_t::iterator i = findInnerBlock(requestedSize);
        if (i == freeSubblocks.end()) {
            return 0;
        }

        const uint32_t subblockOffset = i->first;
        const uint32_t subblockSize = i->second;

        occupiedSubblocks.insert({subblockOffset, requestedSize});

        freeSubblocks.erase(i);
        if (subblockSize > requestedSize) {
            freeSubblocks.insert({subblockOffset + requestedSize,
                                  subblockSize - requestedSize});
        }

        return physBase + subblockOffset;
    }

    uint64_t unallocate(uint64_t phys) {
        if (phys >= physBase + bitsSize) {
            return ~uint64_t(0);
        }

        const uint32_t subblockOffset = phys - physBase;
        uint32_t subblockSize;
        {
            const auto i = occupiedSubblocks.find(subblockOffset);
            if (i == occupiedSubblocks.end()) {
                return ~uint64_t(0);
            }
            subblockSize = i->second;
            occupiedSubblocks.erase(i);
        }

        auto r = freeSubblocks.insert({subblockOffset, subblockSize});

        FreeSubblocks_t::iterator i = r.first;
        if (i != freeSubblocks.begin()) {
            i = tryMergeSubblocks(i, std::prev(i), i);
        }
        FreeSubblocks_t::iterator next = std::next(i);
        if (next != freeSubblocks.end()) {
            i = tryMergeSubblocks(i, i, next);
        }

        return 0;
    }

    FreeSubblocks_t::iterator tryMergeSubblocks(FreeSubblocks_t::iterator ret,
                                                FreeSubblocks_t::iterator lhs,
                                                FreeSubblocks_t::iterator rhs) {
        if (lhs->first + lhs->second == rhs->first) {
            const uint32_t subblockOffset = lhs->first;
            const uint32_t subblockSize = lhs->second + rhs->second;

            freeSubblocks.erase(lhs);
            freeSubblocks.erase(rhs);
            auto r = freeSubblocks.insert({subblockOffset, subblockSize});
            if (!r.second) {
                // TODO: crash
            }

            return r.first;
        } else {
            return ret;
        }
    }

    void save(base::Stream* stream) const {
        stream->putBe64(physBase);
        stream->putBe32(bitsSize);
        stream->write(bits, bitsSize);
        stream->putBe32(freeSubblocks.size());
        for (const auto& kv: freeSubblocks) {
            stream->putBe32(kv.first);
            stream->putBe32(kv.second);
        }
        stream->putBe32(occupiedSubblocks.size());
        for (const auto& kv: occupiedSubblocks) {
            stream->putBe32(kv.first);
            stream->putBe32(kv.second);
        }
    }

    static bool load(base::Stream* stream,
                     const address_space_device_control_ops* ops,
                     MemBlock* block) {
        const uint64_t physBaseLoaded = stream->getBe64();
        const uint32_t bitsSize = stream->getBe32();
        void* const bits = android::aligned_buf_alloc(alignment, bitsSize);
        if (!bits) {
            return false;
        }
        if (stream->read(bits, bitsSize) != static_cast<ssize_t>(bitsSize)) {
            android::aligned_buf_free(bits);
            return false;
        }
        const uint64_t physBase = allocateAddressSpaceBlock(bitsSize);
        if (!physBase) {
            android::aligned_buf_free(bits);
            return false;
        }
        if (!ops->add_memory_mapping(physBase, bits, bitsSize)) {
            freeAddressBlock(physBase);
            android::aligned_buf_free(bits);
            return false;
        }

        FreeSubblocks_t freeSubblocks;
        for (uint32_t freeSubblocksSize = stream->getBe32();
             freeSubblocksSize > 0;
             --freeSubblocksSize) {
            const uint32_t off = stream->getBe32();
            const uint32_t sz = stream->getBe32();
            freeSubblocks.insert({off, sz});
        }

        OccupiedSubblocks_t occupiedSubblocks;
        for (uint32_t occupiedSubblocksSize = stream->getBe32();
             occupiedSubblocksSize > 0;
             --occupiedSubblocksSize) {
            const uint32_t off = stream->getBe32();
            const uint32_t sz = stream->getBe32();
            occupiedSubblocks.insert({off, sz});
        }

        block->ops = ops;
        block->physBase = physBase;
        block->physBaseLoaded = physBaseLoaded;
        block->bits = bits;
        block->bitsSize = bitsSize;
        block->freeSubblocks = std::move(freeSubblocks);
        block->occupiedSubblocks = std::move(occupiedSubblocks);

        return true;
    }

    FreeSubblocks_t::iterator findInnerBlock(const size_t sz) {
        FreeSubblocks_t::iterator best = freeSubblocks.end();
        size_t bestSize = ~size_t(0);

        for (FreeSubblocks_t::iterator i = freeSubblocks.begin();
             i != freeSubblocks.end();
             ++i) {
            if (i->second >= sz && sz < bestSize) {
                best = i;
                bestSize = i->second;
            }
        }

        return best;
    }

    const address_space_device_control_ops* ops;
    uint64_t physBase;
    uint64_t physBaseLoaded;
    void* bits;
    uint32_t bitsSize;
    FreeSubblocks_t freeSubblocks;
    OccupiedSubblocks_t occupiedSubblocks;

    MemBlock(const MemBlock&) = delete;
    MemBlock& operator=(const MemBlock&) = delete;

    MemBlock& operator=(MemBlock rhs) {
        swap(*this, rhs);
        return *this;
    }
};

std::map<uint64_t, MemBlock> g_blocks;
Lock g_blocksLock;

uint64_t translatePhysAddr(uint64_t p) {
    for (const auto& kv: g_blocks) {
        const MemBlock& block = kv.second;
        if (p >= block.physBaseLoaded && p < block.physBaseLoaded + block.bitsSize) {
            return block.physBase + (p - block.physBaseLoaded);
        }
    }

    return 0;
}
}

AddressSpaceSharedSlotsHostMemoryAllocatorContext::AddressSpaceSharedSlotsHostMemoryAllocatorContext(
    const address_space_device_control_ops *ops)
  : m_ops(ops) {
}

AddressSpaceSharedSlotsHostMemoryAllocatorContext::~AddressSpaceSharedSlotsHostMemoryAllocatorContext() {
    clear();
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::perform(AddressSpaceDevicePingInfo *info) {
    uint64_t result;

    switch (static_cast<HostMemoryAllocatorCommand>(info->metadata)) {
    case HostMemoryAllocatorCommand::Allocate:
        result = allocate(info);
        break;

    case HostMemoryAllocatorCommand::Unallocate:
        result = unallocate(info->phys_addr);
        break;

    default:
        result = -1;
        break;
    }

    info->metadata = result;
}

uint64_t AddressSpaceSharedSlotsHostMemoryAllocatorContext::allocate(AddressSpaceDevicePingInfo *info) {
    const uint32_t alignedSize = ((info->size + alignment - 1) / alignment) * alignment;

    AutoLock lock(g_blocksLock);
    for (auto& kv : g_blocks) {
        uint64_t physAddr = kv.second.allocate(alignedSize);
        if (physAddr) {
            return populatePhysAddr(info, physAddr, alignedSize);
        }
    }

    const uint32_t defaultSize = 16u << 20;
    MemBlock newBlock(m_ops, std::max(alignedSize, defaultSize));
    const uint64_t physAddr = newBlock.allocate(alignedSize);
    if (!physAddr) {
        return -1;
    }

    const uint64_t physBase = newBlock.physBase;
    g_blocks.insert({physBase, std::move(newBlock)});

    return populatePhysAddr(info, physAddr, alignedSize);
}

uint64_t AddressSpaceSharedSlotsHostMemoryAllocatorContext::unallocate(const uint64_t physAddr) {
    AutoLock lock(g_blocksLock);
    return unallocateLocked(physAddr, 1);
}

uint64_t AddressSpaceSharedSlotsHostMemoryAllocatorContext::populatePhysAddr(
        AddressSpaceDevicePingInfo *info,
        const uint64_t physAddr,
        const uint32_t alignedSize) {
    info->phys_addr = physAddr;
    info->size = alignedSize;
    m_allocations.insert(physAddr);
    return 0;
}

uint64_t AddressSpaceSharedSlotsHostMemoryAllocatorContext::unallocateLocked(
        const uint64_t physAddr,
        int allowedEmpty) {
    auto i = g_blocks.lower_bound(physAddr);
    if (i == g_blocks.end()) {
        return -1;
    }

    MemBlock& block = i->second;

    if (block.physBase < physAddr) {
        // TODO: crash
    }

    if (block.unallocate(physAddr)) {
        return -1;
    }
    m_allocations.erase(physAddr);

    if (block.occupiedSubblocks.empty()) {
        auto i = g_blocks.begin();
        while (i != g_blocks.end()) {
            if (i->second.occupiedSubblocks.empty()) {
                if (allowedEmpty > 0) {
                    --allowedEmpty;
                    ++i;
                } else {
                    auto ii = std::next(i);
                    g_blocks.erase(i);
                    i = ii;
                }
            } else {
                ++i;
            }
        }
    }

    return 0;
}

AddressSpaceDeviceType AddressSpaceSharedSlotsHostMemoryAllocatorContext::getDeviceType() const {
    return AddressSpaceDeviceType::SharedSlotsHostMemoryAllocator;
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::save(base::Stream* stream) const {
    AutoLock lock(g_blocksLock);

    stream->putBe32(m_allocations.size());
    for (uint32_t phys: m_allocations) {
        stream->putBe64(phys);
    }
}

bool AddressSpaceSharedSlotsHostMemoryAllocatorContext::load(base::Stream* stream) {
    clear();

    AutoLock lock(g_blocksLock);
    for (uint32_t sz = stream->getBe32(); sz > 0; --sz) {
        const uint64_t phys = translatePhysAddr(stream->getBe64());
        if (phys) {
            m_allocations.insert(phys);
        } else {
            // TODO: crash
        }
    }

    return true;
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::clear() {
    AutoLock lock(g_blocksLock);
    for (uint64_t phys: m_allocations) {
        const uint64_t r = unallocateLocked(phys, 0);
        if (r) {
            // TODO: crash
        }
    }
    m_allocations.clear();
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::globalStateSave(base::Stream* stream) {
    AutoLock lock(g_blocksLock);

    stream->putBe32(g_blocks.size());
    for (const auto& kv: g_blocks) {
        kv.second.save(stream);
    }
}

bool AddressSpaceSharedSlotsHostMemoryAllocatorContext::globalStateLoad(
        base::Stream* stream,
        const address_space_device_control_ops *ops) {
    AutoLock lock(g_blocksLock);

    for (uint32_t sz = stream->getBe32(); sz > 0; --sz) {
        MemBlock block;
        if (!MemBlock::load(stream, ops, &block)) { return false; }

        const uint64_t physBase = block.physBase;
        g_blocks.insert({physBase, std::move(block)});
    }

    return true;
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::globalStateClear() {
    AutoLock lock(g_blocksLock);
    g_blocks.clear();
}

}  // namespace emulation
}  // namespace android
