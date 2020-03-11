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

    AddressSpaceSharedSlotsHostMemoryAllocatorContext(const address_space_device_control_ops *ops);
    ~AddressSpaceSharedSlotsHostMemoryAllocatorContext();

    void perform(AddressSpaceDevicePingInfo *info) override;

    AddressSpaceDeviceType getDeviceType() const override;
    void save(base::Stream* stream) const override;
    bool load(base::Stream* stream) override;

    static void globalStateSave(base::Stream* stream);
    static bool globalStateLoad(base::Stream* stream,
                                const address_space_device_control_ops *ops);
    static void globalStateClear();

private:
    uint64_t allocate(AddressSpaceDevicePingInfo *info);
    uint64_t unallocate(uint64_t phys);
    void clear();

    uint64_t populatePhysAddr(AddressSpaceDevicePingInfo *info,
                              uint64_t phys,
                              uint32_t sz,
                              void*);
    uint64_t unallocateLocked(uint64_t phys, int allowedEmpty);

    // physAddr->{size, owner}
    std::unordered_map<uint64_t, std::pair<uint32_t, void*>> m_allocations;
    const address_space_device_control_ops *m_ops;  // do not save/load
};

}  // namespace emulation
}  // namespace android
