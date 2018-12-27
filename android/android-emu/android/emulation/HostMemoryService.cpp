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

#include "android/base/AlignedBuf.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"

#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>

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
            AutoLock lock(serviceLock);
            delete sharedRegionHostAddr;
            sService = nullptr;
        }

        static Service* create() {
            AutoLock lock(serviceLock);
            sService = new Service();
            return sService;
        }

        static Service* get() {
            AutoLock lock(serviceLock);
            return sService;
        }

        void postSave(android::base::Stream* stream) override {
            Super::postSave(stream);
        }

        void postLoad(android::base::Stream* stream) override {
            Super::postLoad(stream);
        }

        struct SubRegion {
            void* hva;
            uint64_t offset;
            uint64_t size;
        };

        void onSubRegionAllocatedLocked(uint64_t offset, uint64_t size) {
            auto& subregion = subRegions[offset];
            subregion.hva = sharedRegionHostAddr + offset;
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

        static StaticLock serviceLock;

        HostMemoryServiceCommand cmd = HostMemoryServiceCommand::None;
        CommandStage stage = CommandStage::None;

        bool sharedRegionAllocated = false;
        uint64_t sharedRegionPhysAddr = 0;
        uint64_t sharedRegionSize = 0;
        uint8_t* sharedRegionHostAddr = nullptr;
        std::unordered_map<uint64_t, SubRegion> subRegions = {};
        uint64_t currentSubRegionPhysAddr = 0;
        uint64_t currentSubRegionSize = 0;

    private:
        static Service* sService;
    };

    HostMemoryPipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
        : AndroidAsyncMessagePipe(service, std::move(pipeArgs)) {}

private:

    void onCommand(uint32_t cmd) {
        auto service = Service::get();
        AutoLock lock(service->serviceLock);

        std::vector<uint8_t> cmdResponse(4);
        std::vector<uint8_t> dataResponse(8);

        if (service->cmd != HostMemoryServiceCommand::None) {
            fprintf(stderr, "%s: warning: two commands in a row!\n", __func__);
        }

        service->cmd = (HostMemoryServiceCommand)cmd;

        switch (service->cmd) {
            case HostMemoryServiceCommand::IsSharedRegionAllocated:
                *(uint32_t*)(cmdResponse.data()) =
                    service->sharedRegionAllocated;
                send(std::move(cmdResponse));
                service->cmd = HostMemoryServiceCommand::None;
                break;
            case HostMemoryServiceCommand::AllocSharedRegion:
            case HostMemoryServiceCommand::AllocSubRegion:
            case HostMemoryServiceCommand::FreeSubRegion:
                service->stage = CommandStage::GetAddress;
                break;
            case HostMemoryServiceCommand::GetHostAddrOfSharedRegion:
                *(uint64_t*)(dataResponse.data()) =
                    (uintptr_t)(service->sharedRegionHostAddr);
                send(std::move(dataResponse));
                service->cmd = HostMemoryServiceCommand::None;
                break;
            default:
                break;
        }
    }

    void onData(uint64_t data) {
        auto service = Service::get();
        AutoLock lock(service->serviceLock);

        uint64_t* addr_out = nullptr;
        uint64_t* size_out = nullptr;

        switch (service->cmd) {
            case HostMemoryServiceCommand::AllocSharedRegion:
                addr_out = &(service->sharedRegionPhysAddr);
                size_out = &(service->sharedRegionSize);
                service->sharedRegionAllocated = true;
                break;
            case HostMemoryServiceCommand::AllocSubRegion:
            case HostMemoryServiceCommand::FreeSubRegion:
                addr_out = &(service->currentSubRegionPhysAddr);
                size_out = &(service->currentSubRegionSize);
                break;
            default:
                break;
        }

        if (!addr_out || !size_out) {
            fprintf(stderr, "%s: internal error\n", __func__);
            return;
        }

        switch (service->stage) {
            case CommandStage::GetAddress:
                *addr_out = data;
                service->stage = CommandStage::GetSize;
                break;
            case CommandStage::GetSize:
                *size_out = data;
                onFinishLocked(service, service->cmd);
                service->stage = CommandStage::None;
                service->cmd = HostMemoryServiceCommand::None;
                break;
            default:
                break;
        }
    }

    void onFinishLocked(Service* service, HostMemoryServiceCommand cmd) {
        switch (service->cmd) {
            case HostMemoryServiceCommand::AllocSharedRegion:
                service->sharedRegionHostAddr =
                    (uint8_t*)aligned_buf_alloc(
                        4096, service->sharedRegionSize);
                service->sharedRegionAllocated = true;
                break;
            case HostMemoryServiceCommand::AllocSubRegion:
                service->onSubRegionAllocatedLocked(
                    service->currentSubRegionPhysAddr,
                    service->currentSubRegionSize);
                break;
            case HostMemoryServiceCommand::FreeSubRegion:
                service->onSubRegionFreedLocked(
                    service->currentSubRegionPhysAddr);
                break;
            default:
                break;
        }
    }

    void onMessage(const std::vector<uint8_t>& input) override {
        uint32_t size = (uint32_t)input.size();
        switch (size) {
            case 4: // standalone commands
                onCommand(*(uint32_t*)input.data());
                break;
            case 20: // command plus an address and size (4 + 8 + 8 = 20)
                onCommand(*(uint32_t*)input.data());
                onData(*(uint64_t*)(input.data() + 4));
                onData(*(uint64_t*)(input.data() + 4 + 8));
                break;
            default:
                fprintf(stderr, "%s: unknown size: %u\n", __func__, size);
        }
    }
};

// static
StaticLock HostMemoryPipe::Service::serviceLock;
HostMemoryPipe::Service* HostMemoryPipe::Service::sService = nullptr;

} // namespace android

void android_host_memory_service_init(void) {
    registerAsyncMessagePipeService(
        android::HostMemoryPipe::Service::create());
}