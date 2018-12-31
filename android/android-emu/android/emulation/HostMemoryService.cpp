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
#include "android/base/SubAllocator.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "android/emulation/control/vm_operations.h"

#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::StaticLock;
using android::base::SubAllocator;

namespace android {

class HostMemoryPipe : public AndroidAsyncMessagePipe {
public:
    class Service : public AndroidAsyncMessagePipe::Service<HostMemoryPipe> {
    typedef AndroidAsyncMessagePipe::Service<HostMemoryPipe> Super;
    public:
        // Allocate at page granularity
        static constexpr uint64_t kAtomSize = 4096;

        Service() : Super("HostMemoryPipe") {}

        ~Service() {
            AutoLock lock(sServiceLock);
            sService = nullptr;
            gQAndroidVmOperations->unmapUserBackedRam(
                mSharedRegionPhysAddr,
                mSharedRegionSize);
        }

        static Service* create() {
            AutoLock lock(sServiceLock);
            sService = new Service();
            return sService;
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
            gQAndroidVmOperations->mapUserBackedRam(
                mSharedRegionPhysAddr,
                sharedRegionHostAddrLocked(),
                mSharedRegionSize);
            mSubAlloc.reset(
                new SubAllocator(
                    mSharedRegionHostBuf.data(),
                    mSharedRegionSize,
                    kAtomSize));
        }

        bool isSharedRegionAllocatedLocked() const {
            return mSharedRegionSize > 0;
        }

        uint64_t allocSubRegionLocked(size_t size) {
            if (!mSubAlloc) {
                fprintf(stderr, "%s: shared region not allocated. fail\n", __func__);
                return SubAllocator::kFailedAlloc;
            }

            auto ptr = mSubAlloc->alloc(size);

            if (!ptr) {
                fprintf(stderr, "%s: ran out of space in shared region. fail\n", __func__);
                return SubAllocator::kFailedAlloc;
            }

            auto offset = mSubAlloc->getOffset(ptr);
            return offset;
        }

        void freeSubRegionLocked(size_t offset) {
            mSubAlloc->freeOffset(offset);
        }

        static StaticLock sServiceLock;

        uint64_t mSharedRegionPhysAddr = 0;
        uint64_t mSharedRegionSize = 0;
        AlignedBuf<uint8_t, 4096> mSharedRegionHostBuf { 0 };
        std::unique_ptr<android::base::SubAllocator> mSubAlloc = {};
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
                if (payloadBytes < 2 * sizeof(uint64_t)) {
                    fprintf(stderr, "%s: Error: not enough bytes to read region cmd info\n",
                            __func__);
                    return;
                } else {
                    uint64_t addr =
                        *reinterpret_cast<const uint64_t*>(data);
                    uint64_t size =
                        *reinterpret_cast<const uint64_t*>(data + sizeof(uint64_t));
                    service->allocSharedRegionLocked(addr, size);
                }
                break;
            case HostMemoryServiceCommand::AllocSubRegion:
            case HostMemoryServiceCommand::FreeSubRegion:
                if (payloadBytes < sizeof(uint64_t)) {
                    fprintf(stderr, "%s: Error: not enough bytes to read subregion cmd info\n",
                            __func__);
                    return;
                } else {
                    uint64_t sizeOrOffset =
                        *reinterpret_cast<const uint64_t*>(data);
                    switch (cmd) {
                        case HostMemoryServiceCommand::AllocSubRegion:
                            *(uint64_t*)(dataResponse.data()) =
                                (uintptr_t)(service->allocSubRegionLocked(sizeOrOffset));
                            send(std::move(dataResponse));
                            break;
                        case HostMemoryServiceCommand::FreeSubRegion:
                            service->freeSubRegionLocked(sizeOrOffset);
                            break;
                        default:
                            fprintf(stderr, "%s: unreachable\n", __func__);
                            abort();
                    }
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
};

// static
StaticLock HostMemoryPipe::Service::sServiceLock;
HostMemoryPipe::Service* HostMemoryPipe::Service::sService = nullptr;

} // namespace android

void android_host_memory_service_init(void) {
    registerAsyncMessagePipeService(
        android::HostMemoryPipe::Service::create());
}
