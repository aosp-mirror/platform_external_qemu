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

#include <mutex>

#include "android/emulation/address_space_host_memory_allocator.h"
#include "android/emulation/control/vm_operations.h"
#include "android/base/AlignedBuf.h"

namespace android {
namespace emulation {

namespace {
enum class HostMemoryAllocatorCommand {
    Allocate = 1,
    Unallocate = 2
};

std::unordered_map<uint64_t, std::pair<void *, size_t>> s_paddr2ptr;
std::mutex s_paddr2ptr_mutex;

}  // namespace

AddressSpaceHostMemoryAllocatorContext::~AddressSpaceHostMemoryAllocatorContext() {
    std::lock_guard<std::mutex> lock(s_paddr2ptr_mutex);

    for (const auto& kv : m_paddr2ptr) {
        gQAndroidVmOperations->unmapUserBackedRam(kv.first, kv.second.second);
        android::aligned_buf_free(kv.second.first);
        s_paddr2ptr.erase(kv.first);
    }
}

void AddressSpaceHostMemoryAllocatorContext::perform(AddressSpaceDevicePingInfo *info) {
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

void *AddressSpaceHostMemoryAllocatorContext::allocate_impl(const uint64_t phys_addr,
                                                            const uint64_t size) {
    constexpr uint64_t alignment = 4096;
    const uint64_t aligned_size = ((size + alignment - 1) / alignment) * alignment;

    void *host_ptr = android::aligned_buf_alloc(alignment, aligned_size);
    if (host_ptr) {
        std::lock_guard<std::mutex> lock(s_paddr2ptr_mutex);

        if (s_paddr2ptr.insert({phys_addr, {host_ptr, aligned_size}}).second) {
            m_paddr2ptr.insert({phys_addr, {host_ptr, aligned_size}});
            gQAndroidVmOperations->mapUserBackedRam(phys_addr, host_ptr, aligned_size);

            return host_ptr;
        } else {
            android::aligned_buf_free(host_ptr);
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

uint64_t AddressSpaceHostMemoryAllocatorContext::allocate(AddressSpaceDevicePingInfo *info) {
    void* host_ptr = allocate_impl(info->phys_addr, info->size);
    if (host_ptr) {
        return 0;
    } else {
        return -1;
    }
}

uint64_t AddressSpaceHostMemoryAllocatorContext::unallocate(AddressSpaceDevicePingInfo *info) {
    std::lock_guard<std::mutex> lock(s_paddr2ptr_mutex);

    const uint64_t phys_addr = info->phys_addr;
    const auto i = s_paddr2ptr.find(phys_addr);
    if (i != s_paddr2ptr.end()) {
        void* host_ptr = i->second.first;
        const uint64_t aligned_size = i->second.second;

        gQAndroidVmOperations->unmapUserBackedRam(phys_addr, aligned_size);
        android::aligned_buf_free(host_ptr);
        s_paddr2ptr.erase(i);
        m_paddr2ptr.erase(phys_addr);

        return 0;
    } else {
        return -1;
    }
}

AddressSpaceDeviceType AddressSpaceHostMemoryAllocatorContext::getDeviceType() const {
    return AddressSpaceDeviceType::HostMemoryAllocator;
}

void AddressSpaceHostMemoryAllocatorContext::save(base::Stream* stream) const {
    stream->putBe32(m_paddr2ptr.size());

    for (const auto &kv : m_paddr2ptr) {
        const uint64_t phys_addr = kv.first;
        const uint64_t size = kv.second.second;
        const void *mem = kv.second.first;

        stream->putBe64(phys_addr);
        stream->putBe64(size);
        stream->write(mem, size);
    }
}

bool AddressSpaceHostMemoryAllocatorContext::load(base::Stream* stream) {
    size_t size = stream->getBe32();

    for (size_t i = 0; i < size; ++i) {
        uint64_t phys_addr = stream->getBe64();
        uint64_t size = stream->getBe64();
        void *mem = allocate_impl(phys_addr, size);
        if (mem) {
            if (stream->read(mem, size) != static_cast<ssize_t>(size)) {
                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}

void* AddressSpaceHostMemoryAllocatorContext::getHostAddr(uint64_t phys_addr) {
    std::lock_guard<std::mutex> lock(s_paddr2ptr_mutex);

    const auto i = s_paddr2ptr.find(phys_addr);
    if (i != s_paddr2ptr.end()) {
        return i->second.first;
    } else {
        return NULL;
    }
}

}  // namespace emulation
}  // namespace android
