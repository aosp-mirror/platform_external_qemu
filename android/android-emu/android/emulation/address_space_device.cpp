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
#include "android/emulation/address_space_device.h"
#include "android/emulation/AddressSpaceService.h"
#include "android/emulation/address_space_host_memory_allocator.h"
#include "android/emulation/control/vm_operations.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_map>
#include <memory>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using namespace android::emulation;

#define AS_DEVICE_DEBUG 0

#if AS_DEVICE_DEBUG
#define AS_DEVICE_DPRINT(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define AS_DEVICE_DPRINT(fmt,...)
#endif

class AddressSpaceDeviceState {
public:
    AddressSpaceDeviceState() = default;
    ~AddressSpaceDeviceState() = default;

    uint32_t genHandle() {
        AutoLock lock(mLock);
        auto res = mHandleIndex;
        ++mHandleIndex;
        AS_DEVICE_DPRINT("new handle: %u", res);
        return res;
    }

    void destroyHandle(uint32_t handle) {
        AS_DEVICE_DPRINT("erase handle: %u", handle);
        AutoLock lock(mLock);
        mContexts.erase(handle);
    }

    void tellPingInfo(uint32_t handle, uint64_t gpa) {
        AutoLock lock(mLock);
        auto& contextDesc = mContexts[handle];
        contextDesc.pingInfo =
            (AddressSpaceDevicePingInfo*)
            gQAndroidVmOperations->physicalMemoryGetAddr(gpa);
        AS_DEVICE_DPRINT("Ping info: gpa 0x%llx @ %p\n", (unsigned long long)gpa,
                         contextDesc.pingInfo);
    }

    void ping(uint32_t handle) {
        AutoLock lock(mLock);
        auto& contextDesc = mContexts[handle];
        AddressSpaceDevicePingInfo* pingInfo = contextDesc.pingInfo;

        const uint64_t phys_addr = pingInfo->phys_addr;
        const uint64_t size = pingInfo->size;
        const uint64_t metadata = pingInfo->metadata;

        AS_DEVICE_DPRINT(
                "handle %u data 0x%llx -> %p size %llu meta 0x%llx\n", handle,
                (unsigned long long)phys_addr,
                gQAndroidVmOperations->physicalMemoryGetAddr(phys_addr),
                (unsigned long long)size, (unsigned long long)metadata);

        AddressSpaceDeviceContext *device_context = contextDesc.device_context.get();
        if (device_context) {
            device_context->perform(pingInfo);
        } else {
            // The first ioctl establishes the device type
            const AddressSpaceDeviceType device_type =
                static_cast<AddressSpaceDeviceType>(pingInfo->metadata);

            contextDesc.device_context = buildAddressSpaceDeviceContext(device_type);
        }
    }

private:
    Lock mLock;
    uint32_t mHandleIndex = 0;
    std::unordered_map<uint32_t, AddressSpaceContextDescription> mContexts;

    std::unique_ptr<AddressSpaceDeviceContext>
    buildAddressSpaceDeviceContext(const AddressSpaceDeviceType device_type) {
        typedef std::unique_ptr<AddressSpaceDeviceContext> DeviceContextPtr;

        switch (device_type) {
        case AddressSpaceDeviceType::Graphics:
            return nullptr;
        case AddressSpaceDeviceType::Media:
            return nullptr;
        case AddressSpaceDeviceType::Sensors:
            return nullptr;
        case AddressSpaceDeviceType::Power:
            return nullptr;
        case AddressSpaceDeviceType::GenericPipe:
            return nullptr;
        case AddressSpaceDeviceType::HostMemoryAllocator:
            return DeviceContextPtr(new AddressSpaceHostMemoryAllocatorContext);

        default:
            return nullptr;
        }
    }
};

namespace {

static LazyInstance<AddressSpaceDeviceState> sAddressSpaceDeviceState =
        LAZY_INSTANCE_INIT;

static uint32_t sCurrentHandleIndex = 0;

static uint32_t sAddressSpaceDeviceGenHandle() {
    return sAddressSpaceDeviceState->genHandle();
}

static void sAddressSpaceDeviceDestroyHandle(uint32_t handle) {
    sAddressSpaceDeviceState->destroyHandle(handle);
}

static void sAddressSpaceDeviceTellPingInfo(uint32_t handle, uint64_t gpa) {
    sAddressSpaceDeviceState->tellPingInfo(handle, gpa);
}

static void sAddressSpaceDevicePing(uint32_t handle) {
    sAddressSpaceDeviceState->ping(handle);
}

} // namespace

extern "C" {

static struct address_space_device_control_ops* sAddressSpaceDeviceOps =
    nullptr;

struct address_space_device_control_ops*
create_or_get_address_space_device_control_ops(void) {
    if (sAddressSpaceDeviceOps) return sAddressSpaceDeviceOps;

    sAddressSpaceDeviceOps = new address_space_device_control_ops;

    sAddressSpaceDeviceOps->gen_handle = sAddressSpaceDeviceGenHandle;
    sAddressSpaceDeviceOps->destroy_handle = sAddressSpaceDeviceDestroyHandle;
    sAddressSpaceDeviceOps->tell_ping_info = sAddressSpaceDeviceTellPingInfo;
    sAddressSpaceDeviceOps->ping = sAddressSpaceDevicePing;

    return sAddressSpaceDeviceOps;
}

} // extern "C"
