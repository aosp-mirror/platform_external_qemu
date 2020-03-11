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
#include "android/crashreport/crash-handler.h"

namespace android {
namespace emulation {

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
        result = unallocate(info);
        break;

    default:
        result = -1;
        break;
    }

    info->metadata = result;
}

uint64_t AddressSpaceSharedSlotsHostMemoryAllocatorContext::allocate(AddressSpaceDevicePingInfo *info) {
    return -1;
}

uint64_t AddressSpaceSharedSlotsHostMemoryAllocatorContext::unallocate(AddressSpaceDevicePingInfo *info) {
    return -1;
}

AddressSpaceDeviceType AddressSpaceSharedSlotsHostMemoryAllocatorContext::getDeviceType() const {
    return AddressSpaceDeviceType::SharedSlotsHostMemoryAllocator;
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::save(base::Stream* stream) const {
}

bool AddressSpaceSharedSlotsHostMemoryAllocatorContext::load(base::Stream* stream) {
    clear();
    return true;
}

void AddressSpaceSharedSlotsHostMemoryAllocatorContext::clear() {
}

}  // namespace emulation
}  // namespace android
