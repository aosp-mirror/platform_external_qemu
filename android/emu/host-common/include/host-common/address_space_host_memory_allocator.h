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

#include "android/emulation/AddressSpaceService.h"
#include "android/emulation/address_space_device.h"

#include <unordered_map>

namespace android {
namespace emulation {

class AddressSpaceHostMemoryAllocatorContext : public AddressSpaceDeviceContext {
public:
    enum class HostMemoryAllocatorCommand {
        Allocate = 1,
        Unallocate = 2
    };

    AddressSpaceHostMemoryAllocatorContext(const address_space_device_control_ops *ops);
    ~AddressSpaceHostMemoryAllocatorContext();

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
    uint64_t unallocate(AddressSpaceDevicePingInfo *info);
    void *allocate_impl(uint64_t phys_addr, uint64_t size);
    void clear();

    std::unordered_map<uint64_t, std::pair<void *, size_t>> m_paddr2ptr;
    const address_space_device_control_ops *m_ops;  // do not save/load
};

}  // namespace emulation
}  // namespace android
