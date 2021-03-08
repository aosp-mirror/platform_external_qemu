// Copyright (C) 2018 The Android Open Source Project
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
#include "android/emulation/HostMemoryService.h"

#include <stdio.h>                                      // for fprintf, stderr
#include <cstdint>                                      // for uint8_t, uint...
#include <functional>                                   // for __base
#include <unordered_map>                                // for unordered_map
#include <utility>                                      // for move
#include <vector>                                       // for vector, vecto...

#include "android/base/AlignedBuf.h"                    // for AlignedBuf
#include "android/base/memory/LazyInstance.h"           // for LazyInstance
#include "android/base/synchronization/Lock.h"          // for AutoLock, Loc...
#include "android/console.h"                            // for getConsoleAgents
#include "android/emulation/AndroidAsyncMessagePipe.h"  // for AndroidAsyncM...
#include "android/emulation/AndroidPipe.h"              // for AndroidPipe
#include "android/emulation/control/vm_operations.h"    // for QAndroidVmOpe...


using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::StaticLock;

namespace android {

class HostMemoryPipe : public AndroidAsyncMessagePipe {
public:
    class Service : public AndroidAsyncMessagePipe::Service<HostMemoryPipe> {
    typedef AndroidAsyncMessagePipe::Service<HostMemoryPipe> Super;
    public:
        Service() : Super("HostMemoryPipe") {}

        ~Service() {
            AutoLock lock(sServiceLock);
            sService = nullptr;
            getConsoleAgents()->vm->unmapUserBackedRam(
                mSharedRegionPhysAddr,
                mSharedRegionSize);
        }

        static std::unique_ptr<Service> create() {
            AutoLock lock(sServiceLock);
            sService = new Service();
            return std::unique_ptr<Service>(sService);
        }

        static Service* get() {
            AutoLock lock(sServiceLock);
            return sService;
        }

        struct SubRegion {
            void* hva;
            uint64_t offset;
            uint64_t size;
        };

        uint8_t* sharedRegionHostAddrLocked() {
            return mSharedRegionHostBuf.data();
        }

        void allocSharedRegionLocked(uint64_t physAddr, uint64_t size) {
            mSharedRegionHostBuf.resize(size);
            mSharedRegionPhysAddr = physAddr;
            mSharedRegionSize = size;

            getConsoleAgents()->vm->mapUserBackedRam(
                mSharedRegionPhysAddr,
                sharedRegionHostAddrLocked(),
                mSharedRegionSize);
        }

        bool isSharedRegionAllocatedLocked() const {
            return mSharedRegionSize > 0;
        }

        void onSubRegionAllocatedLocked(uint64_t offset, uint64_t size) {
            auto& subregion = subRegions[offset];
            subregion.hva = sharedRegionHostAddrLocked() + offset;
            subregion.offset = offset;
            subregion.size = size;
        }

        void onSubRegionFreedLocked(uint64_t offset) {
            auto it = subRegions.find(offset);
            if (it == subRegions.end()) {
                fprintf(stderr, "%s: warning: subregion offset 0x%llx not found\n", __func__,
                        (unsigned long long)offset);
            }
            subRegions.erase(it);
        }

        static StaticLock sServiceLock;

        uint64_t mSharedRegionPhysAddr = 0;
        uint64_t mSharedRegionSize = 0;
        AlignedBuf<uint8_t, 4096> mSharedRegionHostBuf { 0 };
        std::unordered_map<uint64_t, SubRegion> subRegions = {};
        uint64_t currentSubRegionPhysAddr = 0;
        uint64_t currentSubRegionSize = 0;
    private:
        static Service* sService;
    };

    HostMemoryPipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
        : AndroidAsyncMessagePipe(service, std::move(pipeArgs)) {}

private:

    void onMessage(const std::vector<uint8_t>& input) override {
        uint32_t size = (uint32_t)input.size();
        if (size < sizeof(uint32_t)) {
            fprintf(stderr, "HostMemoryService::%s: unexpected size from guest: %u\n",
                    __func__, size);
            return;
        }

        auto service = Service::get();
        AutoLock lock(service->sServiceLock);

        HostMemoryServiceCommand cmd =
            (HostMemoryServiceCommand)(
                *reinterpret_cast<const uint32_t*>(input.data()));

        onCommandLocked(service, cmd,
            input.data() + sizeof(uint32_t),
            input.size() - sizeof(uint32_t));
    }

    void onCommandLocked(Service* service,
                         HostMemoryServiceCommand cmd,
                         const uint8_t* data,
                         size_t payloadBytes) {

        std::vector<uint8_t> cmdResponse(sizeof(uint32_t));
        std::vector<uint8_t> dataResponse(sizeof(uint64_t));

        switch (cmd) {
            case HostMemoryServiceCommand::IsSharedRegionAllocated:
                *(uint32_t*)(cmdResponse.data()) =
                    service->isSharedRegionAllocatedLocked();
                send(std::move(cmdResponse));
                break;
            case HostMemoryServiceCommand::AllocSharedRegion:
            case HostMemoryServiceCommand::AllocSubRegion:
            case HostMemoryServiceCommand::FreeSubRegion:
                if (payloadBytes < 2 * sizeof(uint64_t)) {
                    fprintf(stderr, "%s: Error: not enough bytes to read region cmd info\n",
                            __func__);
                    return;
                } else {
                    uint64_t addr =
                        *reinterpret_cast<const uint64_t*>(data);
                    uint64_t size =
                        *reinterpret_cast<const uint64_t*>(data + sizeof(uint64_t));
                    onRegionCommandLocked(
                        service, cmd, addr, size);
                }
                break;
            case HostMemoryServiceCommand::GetHostAddrOfSharedRegion:
                *(uint64_t*)(dataResponse.data()) =
                    (uintptr_t)(service->sharedRegionHostAddrLocked());
                send(std::move(dataResponse));
                break;
            default:
                break;
        }
    }

    void onRegionCommandLocked(Service* service,
                               HostMemoryServiceCommand cmd,
                               uint64_t addr,
                               uint64_t size) {
        switch (cmd) {
            case HostMemoryServiceCommand::AllocSharedRegion:
                service->allocSharedRegionLocked(addr, size);
                break;
            case HostMemoryServiceCommand::AllocSubRegion:
                service->onSubRegionAllocatedLocked(addr, size);
                break;
            case HostMemoryServiceCommand::FreeSubRegion:
                service->onSubRegionFreedLocked(addr);
                break;
            default:
                fprintf(stderr, "%s: invalid command 0x%x\n",
                        __func__, (uint32_t)(cmd));
                break;
        }
    }
};

// static
StaticLock HostMemoryPipe::Service::sServiceLock;
HostMemoryPipe::Service* HostMemoryPipe::Service::sService = nullptr;

} // namespace android

void android_host_memory_service_init(void) {
    registerAsyncMessagePipeService(
        android::HostMemoryPipe::Service::create());
}
