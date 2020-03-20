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
typedef AddressSpaceSharedSlotsHostMemoryAllocatorContext ASSSHMAC;
typedef ASSSHMAC::MemBlock MemBlock;
typedef MemBlock::FreeSubblocks_t FreeSubblocks_t;

using base::AutoLock;
using base::Lock;

constexpr uint32_t kAlignment = 4096;

uint64_t allocateAddressSpaceBlock(const AddressSpaceHwFuncs* hw, uint32_t size) {
    uint64_t offset;
    if (hw->allocSharedHostRegionLocked(size, &offset)) {
        return 0;
    } else {
        return hw->getPhysAddrStartLocked() + offset;
    }
}

int freeAddressBlock(const AddressSpaceHwFuncs* hw, uint64_t phys) {
    const uint64_t start = hw->getPhysAddrStartLocked();
    if (phys < start) { return -1; }
    return hw->freeSharedHostRegionLocked(phys - start);
}

std::map<uint64_t, MemBlock> g_blocks;
Lock g_blocksLock;

std::pair<uint64_t, MemBlock*> translatePhysAddr(uint64_t p) {
    for (auto& kv: g_blocks) {
        MemBlock& block = kv.second;
        if (p >= block.physBaseLoaded && p < block.physBaseLoaded + block.bitsSize) {
            return {block.physBase + (p - block.physBaseLoaded), &block};
        }
    }

    return {0, nullptr};
}
}  // namespace

MemBlock::MemBlock(const address_space_device_control_ops* o, const AddressSpaceHwFuncs* h, uint32_t sz)
        : ops(o), hw(h) {
    bits = android::aligned_buf_alloc(kAlignment, sz);
    bitsSize = sz;
    physBase = allocateAddressSpaceBlock(hw, sz);
    if (!physBase) {
        crashhandler_die("%s:%d: allocateAddressSpaceBlock", __func__, __LINE__);
    }
    physBaseLoaded = 0;
    if (!ops->add_memory_mapping(physBase, bits, bitsSize)) {
        crashhandler_die("%s:%d: add_memory_mapping", __func__, __LINE__);
    }

    if (!freeSubblocks.insert({0, sz}).second) {
        crashhandler_die("%s:%d: freeSubblocks.insert", __func__, __LINE__);
    }
}

MemBlock::MemBlock(MemBlock&& rhs)
    : ops(std::exchange(rhs.ops, nullptr)),
      hw(std::exchange(rhs.hw, nullptr)),
      physBase(std::exchange(rhs.physBase, 0)),
      physBaseLoaded(std::exchange(rhs.physBaseLoaded, 0)),
      bits(std::exchange(rhs.bits, nullptr)),
      bitsSize(std::exchange(rhs.bitsSize, 0)),
      freeSubblocks(std::move(rhs.freeSubblocks)) {
}

MemBlock& MemBlock::operator=(MemBlock rhs) {
    swap(*this, rhs);
    return *this;
}

MemBlock::~MemBlock() {
    if (physBase) {
        ops->remove_memory_mapping(physBase, bits, bitsSize);
        freeAddressBlock(hw, physBase);
        android::aligned_buf_free(bits);
    }
}

void swap(MemBlock& lhs, MemBlock& rhs) {
    using std::swap;

    swap(lhs.physBase,          rhs.physBase);
    swap(lhs.physBaseLoaded,    rhs.physBaseLoaded);
    swap(lhs.bits,              rhs.bits);
    swap(lhs.bitsSize,          rhs.bitsSize);
    swap(lhs.freeSubblocks,     rhs.freeSubblocks);
}


bool MemBlock::isAllFree() const {
    if (freeSubblocks.size() == 1) {
        const auto kv = *freeSubblocks.begin();
        return (kv.first == 0) && (kv.second == bitsSize);
    } else {
        return false;
    }
}

uint64_t MemBlock::allocate(const size_t requestedSize) {
    FreeSubblocks_t::iterator i = findFreeSubblock(&freeSubblocks, requestedSize);
    if (i == freeSubblocks.end()) {
        return 0;
    }

    const uint32_t subblockOffset = i->first;
    const uint32_t subblockSize = i->second;

    freeSubblocks.erase(i);
    if (subblockSize > requestedSize) {
        if (!freeSubblocks.insert({subblockOffset + requestedSize,
                                   subblockSize - requestedSize}).second) {
            crashhandler_die("%s:%d: freeSubblocks.insert", __func__, __LINE__);
        }
    }

    return physBase + subblockOffset;
}

void MemBlock::unallocate(
        uint64_t phys, uint32_t subblockSize) {
    if (phys >= physBase + bitsSize) {
        crashhandler_die("%s:%d: phys >= physBase + bitsSize", __func__, __LINE__);
    }

    auto r = freeSubblocks.insert({phys - physBase, subblockSize});
    if (!r.second) {
        crashhandler_die("%s:%d: freeSubblocks.insert", __func__, __LINE__);
    }

    FreeSubblocks_t::iterator i = r.first;
    if (i != freeSubblocks.begin()) {
        i = tryMergeSubblocks(&freeSubblocks, i, std::prev(i), i);
    }
    FreeSubblocks_t::iterator next = std::next(i);
    if (next != freeSubblocks.end()) {
        i = tryMergeSubblocks(&freeSubblocks, i, i, next);
    }
}

FreeSubblocks_t::iterator MemBlock::findFreeSubblock(FreeSubblocks_t* fsb,
                                                     const size_t sz) {
    if (fsb->empty()) {
        return fsb->end();
    } else {
        auto best = fsb->end();
        size_t bestSize = ~size_t(0);

        for (auto i = fsb->begin(); i != fsb->end(); ++i) {
            if (i->second >= sz && sz < bestSize) {
                best = i;
                bestSize = i->second;
            }
        }

        return best;
    }
}

FreeSubblocks_t::iterator MemBlock::tryMergeSubblocks(
        FreeSubblocks_t* fsb,
        FreeSubblocks_t::iterator ret,
        FreeSubblocks_t::iterator lhs,
        FreeSubblocks_t::iterator rhs) {
    if (lhs->first + lhs->second == rhs->first) {
        const uint32_t subblockOffset = lhs->first;
        const uint32_t subblockSize = lhs->second + rhs->second;

        fsb->erase(lhs);
        fsb->erase(rhs);
        auto r = fsb->insert({subblockOffset, subblockSize});
        if (!r.second) {
            crashhandler_die("%s:%d: fsb->insert", __func__, __LINE__);
        }

        return r.first;
    } else {
        return ret;
    }
}

void MemBlock::save(base::Stream* stream) const {
    stream->putBe64(physBase);
    stream->putBe32(bitsSize);
    stream->write(bits, bitsSize);
    stream->putBe32(freeSubblocks.size());
    for (const auto& kv: freeSubblocks) {
        stream->putBe32(kv.first);
        stream->putBe32(kv.second);
    }
}

bool MemBlock::load(base::Stream* stream,
                    const address_space_device_control_ops* ops,
                    const AddressSpaceHwFuncs* hw,
                    MemBlock* block) {
    const uint64_t physBaseLoaded = stream->getBe64();
    const uint32_t bitsSize = stream->getBe32();
    void* const bits = android::aligned_buf_alloc(kAlignment, bitsSize);
    if (!bits) {
        return false;
    }
    if (stream->read(bits, bitsSize) != static_cast<ssize_t>(bitsSize)) {
        android::aligned_buf_free(bits);
        return false;
    }
    const uint64_t physBase = allocateAddressSpaceBlock(hw, bitsSize);
    if (!physBase) {
        android::aligned_buf_free(bits);
        return false;
    }
    if (!ops->add_memory_mapping(physBase, bits, bitsSize)) {
        freeAddressBlock(hw, physBase);
        android::aligned_buf_free(bits);
        return false;
    }

    FreeSubblocks_t freeSubblocks;
    for (uint32_t freeSubblocksSize = stream->getBe32();
         freeSubblocksSize > 0;
         --freeSubblocksSize) {
        const uint32_t off = stream->getBe32();
        const uint32_t sz = stream->getBe32();
        if (!freeSubblocks.insert({off, sz}).second) {
            crashhandler_die("%s:%d: freeSubblocks.insert", __func__, __LINE__);
        }
    }

    block->hw = hw;
    block->ops = ops;
    block->physBase = physBase;
    block->physBaseLoaded = physBaseLoaded;
    block->bits = bits;
    block->bitsSize = bitsSize;
    block->freeSubblocks = std::move(freeSubblocks);

    return true;
}

AddressSpaceSharedSlotsHostMemoryAllocatorContext::AddressSpaceSharedSlotsHostMemoryAllocatorContext(
    const address_space_device_control_ops *ops, const AddressSpaceHwFuncs* hw)
  : m_ops(ops),
    m_hw(hw) {}

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

    case HostMemoryAllocatorCommand::CheckIfSharedSlotsSupported:
        result = 0;
        break;

    default:
        result = -1;
        break;
    }

    info->metadata = result;
}

uint64_t
AddressSpaceSharedSlotsHostMemoryAllocatorContext::allocate(
        AddressSpaceDevicePingInfo *info) {
    const uint32_t alignedSize =
        ((info->size + kAlignment - 1) / kAlignment) * kAlignment;

    AutoLock lock(g_blocksLock);
    for (auto& kv : g_blocks) {
        uint64_t physAddr = kv.second.allocate(alignedSize);
        if (physAddr) {
            return populatePhysAddr(info, physAddr, alignedSize, &kv.second);
        }
    }

    const uint32_t defaultSize = 64u << 20;
    MemBlock newBlock(m_ops, m_hw, std::max(alignedSize, defaultSize));
    const uint64_t physAddr = newBlock.allocate(alignedSize);
    if (!physAddr) {
        return -1;
    }

    const uint64_t physBase = newBlock.physBase;
    auto r = g_blocks.insert({physBase, std::move(newBlock)});
    if (!r.second) {
        crashhandler_die("%s:%d: g_blocks.insert", __func__, __LINE__);
    }

    return populatePhysAddr(info, physAddr, alignedSize, &r.first->second);
}

uint64_t
AddressSpaceSharedSlotsHostMemoryAllocatorContext::unallocate(
        const uint64_t physAddr) {
    AutoLock lock(g_blocksLock);

    auto i = m_allocations.find(physAddr);
    if (i == m_allocations.end()) {
        return -1;
    }

    MemBlock* block = i->second.second;
    block->unallocate(physAddr, i->second.first);
    m_allocations.erase(physAddr);

    if (block->isAllFree()) {
        gcEmptyBlocks(1);
    }

    return 0;
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::gcEmptyBlocks(int allowedEmpty) {
    auto i = g_blocks.begin();
    while (i != g_blocks.end()) {
        if (i->second.isAllFree()) {
            if (allowedEmpty > 0) {
                --allowedEmpty;
                ++i;
            } else {
                i = g_blocks.erase(i);
            }
        } else {
            ++i;
        }
    }
}

uint64_t AddressSpaceSharedSlotsHostMemoryAllocatorContext::populatePhysAddr(
        AddressSpaceDevicePingInfo *info,
        const uint64_t physAddr,
        const uint32_t alignedSize,
        MemBlock* owner) {
    info->phys_addr = physAddr - get_address_space_device_hw_funcs()->getPhysAddrStartLocked();
    info->size = alignedSize;
    if (!m_allocations.insert({physAddr, {alignedSize, owner}}).second) {
        crashhandler_die("%s:%d: m_allocations.insert", __func__, __LINE__);
    }
    return 0;
}

AddressSpaceDeviceType AddressSpaceSharedSlotsHostMemoryAllocatorContext::getDeviceType() const {
    return AddressSpaceDeviceType::SharedSlotsHostMemoryAllocator;
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::save(base::Stream* stream) const {
    AutoLock lock(g_blocksLock);

    stream->putBe32(m_allocations.size());
    for (const auto& kv: m_allocations) {
        stream->putBe64(kv.first);
        stream->putBe32(kv.second.first);
    }
}

bool AddressSpaceSharedSlotsHostMemoryAllocatorContext::load(base::Stream* stream) {
    clear();

    AutoLock lock(g_blocksLock);
    for (uint32_t sz = stream->getBe32(); sz > 0; --sz) {
        const uint64_t phys = stream->getBe64();
        const uint32_t size = stream->getBe32();
        const auto r = translatePhysAddr(phys);
        if (phys) {
            if (!m_allocations.insert({r.first, {size, r.second}}).second) {
                crashhandler_die("%s:%d: m_allocations.insert", __func__, __LINE__);
            }
        } else {
            crashhandler_die("%s:%d: translatePhysAddr", __func__, __LINE__);
        }
    }

    return true;
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::clear() {
    AutoLock lock(g_blocksLock);
    for (const auto& kv: m_allocations) {
        MemBlock* block = kv.second.second;
        block->unallocate(kv.first, kv.second.first);
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

// get_address_space_device_hw_funcs()

bool AddressSpaceSharedSlotsHostMemoryAllocatorContext::globalStateLoad(
        base::Stream* stream,
        const address_space_device_control_ops *ops,
        const AddressSpaceHwFuncs* hw) {
    AutoLock lock(g_blocksLock);

    for (uint32_t sz = stream->getBe32(); sz > 0; --sz) {
        MemBlock block;
        if (!MemBlock::load(stream, ops, hw, &block)) { return false; }

        const uint64_t physBase = block.physBase;
        if (!g_blocks.insert({physBase, std::move(block)}).second) {
            crashhandler_die("%s:%d: block->unallocate", __func__, __LINE__);
        }
    }

    return true;
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::globalStateClear() {
    AutoLock lock(g_blocksLock);
    g_blocks.clear();
}

}  // namespace emulation
}  // namespace android
