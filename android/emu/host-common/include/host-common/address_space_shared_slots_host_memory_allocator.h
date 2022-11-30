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
#pragma once

#include "android/emulation/AddressSpaceService.h"
#include "android/emulation/address_space_device.h"
#include <map>
#include <unordered_map>

namespace android {
namespace emulation {

class AddressSpaceSharedSlotsHostMemoryAllocatorContext : public AddressSpaceDeviceContext {
public:
    enum class HostMemoryAllocatorCommand {
        Allocate = 1,
        Unallocate = 2,
        CheckIfSharedSlotsSupported = 3
    };

    struct MemBlock {
        typedef std::map<uint32_t, uint32_t> FreeSubblocks_t;  // offset -> size

        MemBlock() = default;
        MemBlock(const address_space_device_control_ops* o,
                 const AddressSpaceHwFuncs* h,
                 uint32_t sz);
        MemBlock(MemBlock&& rhs);
        MemBlock& operator=(MemBlock rhs);
        ~MemBlock();

        friend void swap(MemBlock& lhs, MemBlock& rhs);

        bool isAllFree() const;
        uint64_t allocate(size_t requestedSize);
        void unallocate(uint64_t phys, uint32_t subblockSize);

        static
        FreeSubblocks_t::iterator findFreeSubblock(FreeSubblocks_t* fsb, size_t sz);

        static
        FreeSubblocks_t::iterator tryMergeSubblocks(FreeSubblocks_t* fsb,
                                                    FreeSubblocks_t::iterator ret,
                                                    FreeSubblocks_t::iterator lhs,
                                                    FreeSubblocks_t::iterator rhs);

        void save(base::Stream* stream) const;
        static bool load(base::Stream* stream,
                         const address_space_device_control_ops* ops,
                         const AddressSpaceHwFuncs* hw,
                         MemBlock* block);

        const address_space_device_control_ops* ops = nullptr;
        const AddressSpaceHwFuncs* hw = nullptr;
        uint64_t physBase = 0;
        uint64_t physBaseLoaded = 0;
        void* bits = nullptr;
        uint32_t bitsSize = 0;
        FreeSubblocks_t freeSubblocks;

        MemBlock(const MemBlock&) = delete;
        MemBlock& operator=(const MemBlock&) = delete;
    };

    AddressSpaceSharedSlotsHostMemoryAllocatorContext(const address_space_device_control_ops *ops,
                                                      const AddressSpaceHwFuncs* hw);
    ~AddressSpaceSharedSlotsHostMemoryAllocatorContext();

    void perform(AddressSpaceDevicePingInfo *info) override;

    AddressSpaceDeviceType getDeviceType() const override;
    void save(base::Stream* stream) const override;
    bool load(base::Stream* stream) override;

    static void globalStateSave(base::Stream* stream);
    static bool globalStateLoad(base::Stream* stream,
                                const address_space_device_control_ops *ops,
                                const AddressSpaceHwFuncs* hw);
    static void globalStateClear();

private:
    uint64_t allocate(AddressSpaceDevicePingInfo *info);
    uint64_t unallocate(uint64_t phys);
    void gcEmptyBlocks(int allowedEmpty);
    void clear();

    uint64_t populatePhysAddr(AddressSpaceDevicePingInfo *info,
                              uint64_t phys,
                              uint32_t sz,
                              MemBlock*);
    uint64_t unallocateLocked(uint64_t phys, int allowedEmpty);

    // physAddr->{size, owner}
    std::unordered_map<uint64_t, std::pair<uint32_t, MemBlock*>> m_allocations;
    const address_space_device_control_ops *m_ops;  // do not save/load
    const AddressSpaceHwFuncs* m_hw;
};

}  // namespace emulation
}  // namespace android
