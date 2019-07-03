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

#include "android/emulation/address_space_host_memory_allocator.h"
#include "android/emulation/address_space_device.hpp"
#include "android/emulation/control/vm_operations.h"
#include "android/base/AlignedBuf.h"
#include "android/crashreport/crash-handler.h"

namespace android {
namespace emulation {

namespace {
enum class HostMemoryAllocatorCommand {
    Allocate = 1,
    Unallocate = 2
};

}  // namespace

AddressSpaceHostMemoryAllocatorContext::AddressSpaceHostMemoryAllocatorContext(
    const address_space_device_control_ops *ops)
  : m_ops(ops) {
}

AddressSpaceHostMemoryAllocatorContext::~AddressSpaceHostMemoryAllocatorContext() {
    for (const auto& kv : m_paddr2ptr) {
        uint64_t phys_addr = kv.first;
        void *host_ptr = kv.second.first;
        size_t size = kv.second.second;

        if (m_ops->remove_memory_mapping(phys_addr, host_ptr, size)) {
            fprintf(stderr, "%s:%d free host_ptr=%p\n", __func__, __LINE__, host_ptr);
            android::aligned_buf_free(host_ptr);
        } else {
            crashhandler_die("Failed remove a memory mapping {phys_addr=%lx, host_ptr=%p, size=%lu}",
                             phys_addr, host_ptr, size);
        }
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
    fprintf(stderr, "%s:%d alloc host_ptr=%p size=%u\n", __func__, __LINE__, host_ptr, (unsigned int)aligned_size);
    if (host_ptr) {
        auto r = m_paddr2ptr.insert({phys_addr, {host_ptr, aligned_size}});
        if (r.second) {
            if (m_ops->add_memory_mapping(phys_addr, host_ptr, aligned_size)) {
                fprintf(stderr, "%s:%d OK host_ptr=%p\n", __func__, __LINE__, host_ptr);
                return host_ptr;
            } else {
                m_paddr2ptr.erase(r.first);
                fprintf(stderr, "%s:%d free host_ptr=%p\n", __func__, __LINE__, host_ptr);
                android::aligned_buf_free(host_ptr);
                return nullptr;
            }
        } else {
            fprintf(stderr, "%s:%d free host_ptr=%p\n", __func__, __LINE__, host_ptr);
            android::aligned_buf_free(host_ptr);
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

uint64_t AddressSpaceHostMemoryAllocatorContext::allocate(AddressSpaceDevicePingInfo *info) {
    void* host_ptr = allocate_impl(info->phys_addr, info->size);
    fprintf(stderr, "%s:%d host_ptr=%p\n", __func__, __LINE__, host_ptr);
    if (host_ptr) {
        return 0;
    } else {
        return -1;
    }
}

uint64_t AddressSpaceHostMemoryAllocatorContext::unallocate(AddressSpaceDevicePingInfo *info) {
    const uint64_t phys_addr = info->phys_addr;
    const auto i = m_paddr2ptr.find(phys_addr);
    if (i != m_paddr2ptr.end()) {
        void* host_ptr = i->second.first;
        const uint64_t size = i->second.second;

        if (m_ops->remove_memory_mapping(phys_addr, host_ptr, size)) {
            fprintf(stderr, "%s:%d free host_ptr=%p\n", __func__, __LINE__, host_ptr);
            android::aligned_buf_free(host_ptr);
            m_paddr2ptr.erase(i);
            return 0;
        } else {
            crashhandler_die("Failed remove a memory mapping {phys_addr=%lx, host_ptr=%p, size=%lu}",
                             phys_addr, host_ptr, size);
        }
    } else {
        return -1;
    }
}

AddressSpaceDeviceType AddressSpaceHostMemoryAllocatorContext::getDeviceType() const {
    return AddressSpaceDeviceType::HostMemoryAllocator;
}

void AddressSpaceHostMemoryAllocatorContext::save(base::Stream* stream) const {
    fprintf(stderr, "%s:%d size=%zu\n", __func__, __LINE__, m_paddr2ptr.size());

    stream->putBe32(m_paddr2ptr.size());

    for (const auto &kv : m_paddr2ptr) {
        const uint64_t phys_addr = kv.first;
        const uint64_t size = kv.second.second;
        const void *mem = kv.second.first;

        fprintf(stderr, "%s:%d phys_addr=%llx size=%llu\n", __func__, __LINE__, (unsigned long long)phys_addr, (unsigned long long)size);

        stream->putBe64(phys_addr);
        stream->putBe64(size);
        stream->write(mem, size);
    }
}

bool AddressSpaceHostMemoryAllocatorContext::load(base::Stream* stream) {
    clear();

    size_t size = stream->getBe32();
    fprintf(stderr, "%s:%d size=%zu\n", __func__, __LINE__, size);

    for (size_t i = 0; i < size; ++i) {
        uint64_t phys_addr = stream->getBe64();
        uint64_t size = stream->getBe64();
        void *mem = allocate_impl(phys_addr, size);

        fprintf(stderr, "%s:%d phys_addr=%llx size=%llu mem=%p\n", __func__, __LINE__, (unsigned long long)phys_addr, (unsigned long long)size, mem);

        if (mem) {
            if (stream->read(mem, size) != static_cast<ssize_t>(size)) {
                fprintf(stderr, "%s:%d phys_addr=%llx size=%llu fail\n", __func__, __LINE__, (unsigned long long)phys_addr, (unsigned long long)size);
                return false;
            }
        } else {
            fprintf(stderr, "%s:%d phys_addr=%llx size=%llu fail\n", __func__, __LINE__, (unsigned long long)phys_addr, (unsigned long long)size);
            return false;
        }
    }

    return true;
}

}  // namespace emulation
}  // namespace android
