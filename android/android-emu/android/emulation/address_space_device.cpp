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
#include "aemu/base/logging/Log.h"
#include "host-common/address_space_device.h"
#include "host-common/AddressSpaceService.h"
#include "host-common/address_space_graphics.h"
#ifndef AEMU_MIN
#include "host-common/address_space_host_media.h"
#endif
#include "host-common/address_space_host_memory_allocator.h"
#include "host-common/address_space_shared_slots_host_memory_allocator.h"
#include "host-common/vm_operations.h"

#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/synchronization/Lock.h"

#include <map>
#include <unordered_map>
#include <memory>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::Stream;
using android::emulation::asg::AddressSpaceGraphicsContext;

using namespace android::emulation;

#define AS_DEVICE_DEBUG 0

#if AS_DEVICE_DEBUG
#define AS_DEVICE_DPRINT(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define AS_DEVICE_DPRINT(fmt,...)
#endif

const QAndroidVmOperations* sVmOps = nullptr;

namespace {

class AddressSpaceDeviceState {
public:
    AddressSpaceDeviceState() = default;
    ~AddressSpaceDeviceState() = default;

    uint32_t genHandle() {
        AutoLock lock(mContextsLock);

        auto res = mHandleIndex;

        if (!res) {
            ++res;
            mHandleIndex += 2;
        } else {
            ++mHandleIndex;
        }

        AS_DEVICE_DPRINT("new handle: %u", res);
        return res;
    }

    void destroyHandle(uint32_t handle) {
        AS_DEVICE_DPRINT("erase handle: %u", handle);
        AutoLock lock(mContextsLock);
        mContexts.erase(handle);
    }

    void tellPingInfo(uint32_t handle, uint64_t gpa) {
        AutoLock lock(mContextsLock);
        auto& contextDesc = mContexts[handle];
        contextDesc.pingInfo =
            (AddressSpaceDevicePingInfo*)
            sVmOps->physicalMemoryGetAddr(gpa);
        contextDesc.pingInfoGpa = gpa;
        AS_DEVICE_DPRINT("Ping info: gpa 0x%llx @ %p\n", (unsigned long long)gpa,
                         contextDesc.pingInfo);
    }

    void createInstance(const struct AddressSpaceCreateInfo& create) {
        AutoLock lock(mContextsLock);
        auto& contextDesc = mContexts[create.handle];
        contextDesc.device_context = buildAddressSpaceDeviceContext(create);
    }

    void ping(uint32_t handle) {
        AutoLock lock(mContextsLock);
        auto& contextDesc = mContexts[handle];
        AddressSpaceDevicePingInfo* pingInfo = contextDesc.pingInfo;

        const uint64_t phys_addr = pingInfo->phys_addr;
        const uint64_t size = pingInfo->size;
        const uint64_t metadata = pingInfo->metadata;

        AS_DEVICE_DPRINT(
                "handle %u data 0x%llx -> %p size %llu meta 0x%llx\n", handle,
                (unsigned long long)phys_addr,
                sVmOps->physicalMemoryGetAddr(phys_addr),
                (unsigned long long)size, (unsigned long long)metadata);

        AddressSpaceDeviceContext *device_context = contextDesc.device_context.get();
        if (device_context) {
            device_context->perform(pingInfo);
        } else {
            // The first ioctl establishes the device type
            struct AddressSpaceCreateInfo create = {0};
            create.type = static_cast<AddressSpaceDeviceType>(pingInfo->metadata);
            create.physAddr = phys_addr;

            contextDesc.device_context = buildAddressSpaceDeviceContext(create);
            pingInfo->metadata = contextDesc.device_context ? 0 : -1;
        }
    }

    void pingAtHva(uint32_t handle, AddressSpaceDevicePingInfo* pingInfo) {
        AutoLock lock(mContextsLock);
        auto& contextDesc = mContexts[handle];

        const uint64_t phys_addr = pingInfo->phys_addr;
        const uint64_t size = pingInfo->size;
        const uint64_t metadata = pingInfo->metadata;

        AS_DEVICE_DPRINT(
                "handle %u data 0x%llx -> %p size %llu meta 0x%llx\n", handle,
                (unsigned long long)phys_addr,
                sVmOps->physicalMemoryGetAddr(phys_addr),
                (unsigned long long)size, (unsigned long long)metadata);

        AddressSpaceDeviceContext *device_context = contextDesc.device_context.get();
        if (device_context) {
            device_context->perform(pingInfo);
        } else {
            struct AddressSpaceCreateInfo create = {0};
            create.type = static_cast<AddressSpaceDeviceType>(pingInfo->metadata);
            create.physAddr = phys_addr;

            contextDesc.device_context = buildAddressSpaceDeviceContext(create);
            pingInfo->metadata = contextDesc.device_context ? 0 : -1;
        }
    }

    void registerDeallocationCallback(uint64_t gpa, void* context, address_space_device_deallocation_callback_t func) {
        AutoLock lock(mContextsLock);
        auto& currentCallbacks = mDeallocationCallbacks[gpa];

        DeallocationCallbackEntry entry = {
            context,
            func,
        };

        currentCallbacks.push_back(entry);
    }

    void runDeallocationCallbacks(uint64_t gpa) {
        AutoLock lock(mContextsLock);

        auto it = mDeallocationCallbacks.find(gpa);
        if (it == mDeallocationCallbacks.end()) return;

        auto& callbacks = it->second;

        for (auto& entry: callbacks) {
            entry.func(entry.context, gpa);
        }

        mDeallocationCallbacks.erase(gpa);
    }

    AddressSpaceDeviceContext* handleToContext(uint32_t handle) {
        AutoLock lock(mContextsLock);
        if (mContexts.find(handle) == mContexts.end()) return nullptr;

        auto& contextDesc = mContexts[handle];
        return contextDesc.device_context.get();
    }

    uint64_t hostmemRegister(const struct MemEntry* entry) {
        return sVmOps->hostmemRegister(entry);
    }

    void hostmemUnregister(uint64_t id) {
        sVmOps->hostmemUnregister(id);
    }

    void save(Stream* stream) const {
        // Pre-save
        for (const auto &kv : mContexts) {
            const AddressSpaceContextDescription &desc = kv.second;
            const AddressSpaceDeviceContext *device_context = desc.device_context.get();
            if (device_context) {
                device_context->preSave();
            }
        }

        AddressSpaceGraphicsContext::globalStatePreSave();

        // Save
        AddressSpaceSharedSlotsHostMemoryAllocatorContext::globalStateSave(stream);
        AddressSpaceGraphicsContext::globalStateSave(stream);

        stream->putBe32(mHandleIndex);
        stream->putBe32(mContexts.size());

        for (const auto &kv : mContexts) {
            const uint32_t handle = kv.first;
            const AddressSpaceContextDescription &desc = kv.second;
            const AddressSpaceDeviceContext *device_context = desc.device_context.get();

            stream->putBe32(handle);
            stream->putBe64(desc.pingInfoGpa);

            if (device_context) {
                stream->putByte(1);
                stream->putBe32(device_context->getDeviceType());
                device_context->save(stream);
            } else {
                stream->putByte(0);
            }
        }

        // Post save

        AddressSpaceGraphicsContext::globalStatePostSave();

        for (const auto &kv : mContexts) {
            const AddressSpaceContextDescription &desc = kv.second;
            const AddressSpaceDeviceContext *device_context = desc.device_context.get();
            if (device_context) {
                device_context->postSave();
            }
        }
    }

    bool load(Stream* stream) {
        // First destroy all contexts, because
        // this can be done while an emulator is running
        clear();

        if (!AddressSpaceSharedSlotsHostMemoryAllocatorContext::globalStateLoad(
                stream,
                get_address_space_device_control_ops(),
                get_address_space_device_hw_funcs())) {
            return false;
        }

        asg::AddressSpaceGraphicsContext::init(get_address_space_device_control_ops());

        if (!AddressSpaceGraphicsContext::globalStateLoad(
                stream)) {
            return false;
        }

        const uint32_t handleIndex = stream->getBe32();
        const size_t size = stream->getBe32();

        Contexts contexts;
        for (size_t i = 0; i < size; ++i) {
            const uint32_t handle = stream->getBe32();
            const uint64_t pingInfoGpa = stream->getBe64();

            std::unique_ptr<AddressSpaceDeviceContext> context;
            switch (stream->getByte()) {
            case 0:
                break;

            case 1: {
                struct AddressSpaceCreateInfo create = {0};
                create.type = static_cast<AddressSpaceDeviceType>(stream->getBe32());
                create.physAddr = pingInfoGpa;
                create.fromSnapshot = true;

                context = buildAddressSpaceDeviceContext(create);
                if (!context || !context->load(stream)) {
                    return false;
                    }
                }
                break;

            default:
                return false;
            }

            auto &desc = contexts[handle];
            desc.pingInfoGpa = pingInfoGpa;
            if (desc.pingInfoGpa == ~0ULL) {
                dwarning("%s: warning: restoring hva-only ping", __func__);
            } else {
                desc.pingInfo = (AddressSpaceDevicePingInfo*)
                    sVmOps->physicalMemoryGetAddr(pingInfoGpa);
            }
            desc.device_context = std::move(context);
        }

        {
           AutoLock lock(mContextsLock);
           mHandleIndex = handleIndex;
           mContexts = std::move(contexts);
        }

        return true;
    }

    void clear() {
        AutoLock lock(mContextsLock);
        mContexts.clear();
        AddressSpaceSharedSlotsHostMemoryAllocatorContext::globalStateClear();
        auto it = mMemoryMappings.begin();
        std::vector<std::pair<uint64_t, uint64_t>> gpasSizesToErase;
        for (auto it: mMemoryMappings) {
            auto gpa = it.first;
            auto size = it.second.second;
            gpasSizesToErase.push_back({gpa, size});
        }
        for (const auto& gpaSize : gpasSizesToErase) {
            removeMemoryMappingLocked(gpaSize.first, gpaSize.second);
        }
        mMemoryMappings.clear();
    }

    bool addMemoryMapping(uint64_t gpa, void *ptr, uint64_t size) {
        AutoLock lock(mMemoryMappingsLock);
        return addMemoryMappingLocked(gpa, ptr, size);
    }

    bool removeMemoryMapping(uint64_t gpa, uint64_t size) {
        AutoLock lock(mMemoryMappingsLock);
        return removeMemoryMappingLocked(gpa, size);
    }

    void *getHostPtr(uint64_t gpa) const {
        AutoLock lock(mMemoryMappingsLock);
        return getHostPtrLocked(gpa);
    }

private:
    mutable Lock mContextsLock;
    uint32_t mHandleIndex = 1;
    typedef std::unordered_map<uint32_t, AddressSpaceContextDescription> Contexts;
    Contexts mContexts;

    std::unique_ptr<AddressSpaceDeviceContext> buildAddressSpaceDeviceContext(
        const struct AddressSpaceCreateInfo& create) {
        typedef std::unique_ptr<AddressSpaceDeviceContext> DeviceContextPtr;

        switch (create.type) {
            case AddressSpaceDeviceType::Graphics:
                asg::AddressSpaceGraphicsContext::init(get_address_space_device_control_ops());
                return DeviceContextPtr(new asg::AddressSpaceGraphicsContext(create));
#ifndef AEMU_MIN
        case AddressSpaceDeviceType::Media:
            AS_DEVICE_DPRINT("allocating media context");
            return DeviceContextPtr(
                new AddressSpaceHostMediaContext(create, get_address_space_device_control_ops()));
#endif
        case AddressSpaceDeviceType::Sensors:
            return nullptr;
        case AddressSpaceDeviceType::Power:
            return nullptr;
        case AddressSpaceDeviceType::GenericPipe:
            return nullptr;
        case AddressSpaceDeviceType::HostMemoryAllocator:
            return DeviceContextPtr(new AddressSpaceHostMemoryAllocatorContext(
                get_address_space_device_control_ops(),
                get_address_space_device_hw_funcs()));
        case AddressSpaceDeviceType::SharedSlotsHostMemoryAllocator:
            return DeviceContextPtr(new AddressSpaceSharedSlotsHostMemoryAllocatorContext(
                get_address_space_device_control_ops(),
                get_address_space_device_hw_funcs()));

        case AddressSpaceDeviceType::VirtioGpuGraphics:
            asg::AddressSpaceGraphicsContext::init(get_address_space_device_control_ops());
            return DeviceContextPtr(new asg::AddressSpaceGraphicsContext(create));

        default:
            AS_DEVICE_DPRINT("Bad device type");
            return nullptr;
        }
    }

    bool addMemoryMappingLocked(uint64_t gpa, void *ptr, uint64_t size) {
        if (mMemoryMappings.insert({gpa, {ptr, size}}).second) {
            sVmOps->mapUserBackedRam(gpa, ptr, size);
            return true;
        } else {
            derror("%s: failed: hva %p -> gpa [0x%llx 0x%llx]", __func__,
                    ptr,
                    (unsigned long long)gpa,
                    (unsigned long long)size);
            return false;
        }
    }

    bool removeMemoryMappingLocked(uint64_t gpa, uint64_t size) {
        if (mMemoryMappings.erase(gpa) > 0) {
            sVmOps->unmapUserBackedRam(gpa, size);
            return true;
        } else {
            derror("%s: failed: gpa [0x%llx 0x%llx]", __func__,
                    (unsigned long long)gpa,
                    (unsigned long long)size);
            *(uint32_t*)(123) = 12;
            return false;
        }
    }

    void *getHostPtrLocked(uint64_t gpa) const {
        auto i = mMemoryMappings.lower_bound(gpa); // i->first >= gpa (or i==end)
        if ((i != mMemoryMappings.end()) && (i->first == gpa)) {
            return i->second.first;  // gpa is exactly the beginning of the range
        } else if (i == mMemoryMappings.begin()) {
            return nullptr;  // can't '--i', see below
        } else {
            --i;

            if ((i->first + i->second.second) > gpa) {
                // move the host ptr by +(gpa-base)
                return static_cast<char *>(i->second.first) + (gpa - i->first);
            } else {
                return nullptr;  // the range does not cover gpa
            }
        }
    }

    mutable Lock mMemoryMappingsLock;
    std::map<uint64_t, std::pair<void *, uint64_t>> mMemoryMappings;  // do not save/load

    struct DeallocationCallbackEntry {
        void* context;
        address_space_device_deallocation_callback_t func;
    };

    std::map<uint64_t, std::vector<DeallocationCallbackEntry>> mDeallocationCallbacks; // do not save/load, users re-register on load
};

static LazyInstance<AddressSpaceDeviceState> sAddressSpaceDeviceState =
        LAZY_INSTANCE_INIT;

static uint32_t sCurrentHandleIndex = 0;

static uint32_t sAddressSpaceDeviceGenHandle() {
    return sAddressSpaceDeviceState->genHandle();
}

static void sAddressSpaceDeviceDestroyHandle(uint32_t handle) {
    sAddressSpaceDeviceState->destroyHandle(handle);
}

static void sAddressSpaceDeviceCreateInstance(const struct AddressSpaceCreateInfo& create) {
    sAddressSpaceDeviceState->createInstance(create);
}

static void sAddressSpaceDeviceTellPingInfo(uint32_t handle, uint64_t gpa) {
    sAddressSpaceDeviceState->tellPingInfo(handle, gpa);
}

static void sAddressSpaceDevicePing(uint32_t handle) {
    sAddressSpaceDeviceState->ping(handle);
}

int sAddressSpaceDeviceAddMemoryMapping(uint64_t gpa, void *ptr, uint64_t size) {
    return sAddressSpaceDeviceState->addMemoryMapping(gpa, ptr, size) ? 1 : 0;
}

int sAddressSpaceDeviceRemoveMemoryMapping(uint64_t gpa, void *ptr, uint64_t size) {
    (void)ptr; // TODO(lfy): remove arg
    return sAddressSpaceDeviceState->removeMemoryMapping(gpa, size) ? 1 : 0;
}

void* sAddressSpaceDeviceGetHostPtr(uint64_t gpa) {
    return sAddressSpaceDeviceState->getHostPtr(gpa);
}

static void* sAddressSpaceHandleToContext(uint32_t handle) {
    return (void*)(sAddressSpaceDeviceState->handleToContext(handle));
}

static void sAddressSpaceDeviceClear() {
    sAddressSpaceDeviceState->clear();
}

static uint64_t sAddressSpaceDeviceHostmemRegister(const struct MemEntry* entry) {
    return sAddressSpaceDeviceState->hostmemRegister(entry);
}

static void sAddressSpaceDeviceHostmemUnregister(uint64_t id) {
    sAddressSpaceDeviceState->hostmemUnregister(id);
}

static void sAddressSpaceDevicePingAtHva(uint32_t handle, void* hva) {
    sAddressSpaceDeviceState->pingAtHva(
        handle, (AddressSpaceDevicePingInfo*)hva);
}

static void sAddressSpaceDeviceRegisterDeallocationCallback(
    void* context, uint64_t gpa, address_space_device_deallocation_callback_t func) {
    sAddressSpaceDeviceState->registerDeallocationCallback(gpa, context, func);
}

static void sAddressSpaceDeviceRunDeallocationCallbacks(uint64_t gpa) {
    sAddressSpaceDeviceState->runDeallocationCallbacks(gpa);
}

static const struct AddressSpaceHwFuncs* sAddressSpaceDeviceControlGetHwFuncs() {
    return get_address_space_device_hw_funcs();
}


} // namespace

extern "C" {

static struct address_space_device_control_ops sAddressSpaceDeviceOps = {
    &sAddressSpaceDeviceGenHandle,                     // gen_handle
    &sAddressSpaceDeviceDestroyHandle,                 // destroy_handle
    &sAddressSpaceDeviceTellPingInfo,                  // tell_ping_info
    &sAddressSpaceDevicePing,                          // ping
    &sAddressSpaceDeviceAddMemoryMapping,              // add_memory_mapping
    &sAddressSpaceDeviceRemoveMemoryMapping,           // remove_memory_mapping
    &sAddressSpaceDeviceGetHostPtr,                    // get_host_ptr
    &sAddressSpaceHandleToContext,                     // handle_to_context
    &sAddressSpaceDeviceClear,                         // clear
    &sAddressSpaceDeviceHostmemRegister,               // hostmem register
    &sAddressSpaceDeviceHostmemUnregister,             // hostmem unregister
    &sAddressSpaceDevicePingAtHva,                     // ping_at_hva
    &sAddressSpaceDeviceRegisterDeallocationCallback,  // register_deallocation_callback
    &sAddressSpaceDeviceRunDeallocationCallbacks,      // run_deallocation_callbacks
    &sAddressSpaceDeviceControlGetHwFuncs,             // control_get_hw_funcs
    &sAddressSpaceDeviceCreateInstance,                // create_instance
};

struct address_space_device_control_ops* get_address_space_device_control_ops(void) {
    return &sAddressSpaceDeviceOps;
}

static const struct AddressSpaceHwFuncs* sAddressSpaceHwFuncs = nullptr;

const struct AddressSpaceHwFuncs* address_space_set_hw_funcs(
        const AddressSpaceHwFuncs* hwFuncs) {
    const AddressSpaceHwFuncs* result = sAddressSpaceHwFuncs;
    sAddressSpaceHwFuncs = hwFuncs;
    return result;
}

const struct AddressSpaceHwFuncs* get_address_space_device_hw_funcs(void) {
    return sAddressSpaceHwFuncs;
}

void address_space_set_vm_operations(const QAndroidVmOperations* vmops) {
    sVmOps = vmops;
}

} // extern "C"

namespace android {
namespace emulation {

void goldfish_address_space_set_vm_operations(const QAndroidVmOperations* vmops) {
    sVmOps = vmops;
}

const QAndroidVmOperations* goldfish_address_space_get_vm_operations() {
    return sVmOps;
}

int goldfish_address_space_memory_state_load(android::base::Stream *stream) {
    return sAddressSpaceDeviceState->load(stream) ? 0 : 1;
}

int goldfish_address_space_memory_state_save(android::base::Stream *stream) {
    sAddressSpaceDeviceState->save(stream);
    return 0;
}

}  // namespace emulation
}  // namespace android
