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

struct MemBlock {
    typedef std::map<uint32_t, uint32_t> InnerBlocks;  // subblock offset -> size

    MemBlock(uint32_t sz) {
        physBase = 77; // TODO
        bits = android::aligned_buf_alloc(alignment, sz);
        bitsSize = sz;
        freeSubblocks.insert({0, sz});
    }

    ~MemBlock() {
        if (!occupiedSubblocks.empty()) {
            // TODO: crash
        }

        if (freeSubblocks.size() != 1) {
            // TODO: crash
        }

        if (physBase) {
            android::aligned_buf_free(bits);
        }
    }

    MemBlock(MemBlock&& rhs)
        : physBase(std::exchange(rhs.physBase, 0)),
          bits(std::exchange(rhs.bits, nullptr)),
          bitsSize(std::exchange(rhs.bitsSize, 0)),
          freeSubblocks(std::move(rhs.freeSubblocks)),
          occupiedSubblocks(std::move(rhs.occupiedSubblocks)) {}

    friend void swap(MemBlock& lhs, MemBlock& rhs) {
        using std::swap;

        swap(lhs.physBase,          rhs.physBase);
        swap(lhs.bits,              rhs.bits);
        swap(lhs.bitsSize,          rhs.bitsSize);
        swap(lhs.freeSubblocks,     rhs.freeSubblocks);
        swap(lhs.occupiedSubblocks, rhs.occupiedSubblocks);
    }

    uint64_t allocate(const size_t requestedSize) {
        InnerBlocks::iterator i = findInnerBlock(requestedSize);
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

        InnerBlocks::iterator i = r.first;
        if (i != freeSubblocks.begin()) {
            i = tryMergeSubblocks(i, std::prev(i), i);
        }
        InnerBlocks::iterator next = std::next(i);
        if (next != freeSubblocks.end()) {
            i = tryMergeSubblocks(i, i, next);
        }

        return 0;
    }

    InnerBlocks::iterator tryMergeSubblocks(InnerBlocks::iterator ret,
                                            InnerBlocks::iterator lhs,
                                            InnerBlocks::iterator rhs) {
        if (lhs->first + lhs->second == rhs->first) {
            const uint32_t subblockOffset = lhs->first;
            const uint32_t subblockSize = lhs->second + rhs->second;

            freeSubblocks.erase(lhs);
            freeSubblocks.erase(rhs);
            auto r = freeSubblocks.insert({subblockOffset, subblockSize});

            return r.first;
        } else {
            return ret;
        }
    }

    void save() const {
    }

    static MemBlock load() {
        return MemBlock(42);
    }

    InnerBlocks::iterator findInnerBlock(const size_t sz) {
        InnerBlocks::iterator best = freeSubblocks.end();
        size_t bestSize = ~size_t(0);

        for (InnerBlocks::iterator i = freeSubblocks.begin(); i != freeSubblocks.end(); ++i) {
            if (i->second >= sz && sz < bestSize) {
                best = i;
                bestSize = i->second;
            }
        }

        return best;
    }

    uint64_t physBase;
    void* bits;
    uint32_t bitsSize;
    InnerBlocks freeSubblocks;
    std::unordered_map<uint32_t, uint32_t> occupiedSubblocks;

    MemBlock(const MemBlock&) = delete;
    MemBlock& operator=(const MemBlock&) = delete;

    MemBlock& operator=(MemBlock rhs) {
        swap(*this, rhs);
        return *this;
    }
};

std::map<uint64_t, MemBlock> g_blocks;
Lock g_blocksLock;
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
    MemBlock newBlock(std::max(alignedSize, defaultSize));
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
    // TODO
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
    // TODO
}

bool AddressSpaceSharedSlotsHostMemoryAllocatorContext::load(base::Stream* stream) {
    clear();
    // TODO
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

}  // namespace emulation
}  // namespace android
