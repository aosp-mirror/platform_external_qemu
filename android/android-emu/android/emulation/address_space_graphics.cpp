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

#include "android/emulation/address_space_graphics.h"

#include "android/emulation/address_space_device.hpp"
#include "android/emulation/address_space_device.h"
#include "android/emulation/control/vm_operations.h"
#include "android/base/AlignedBuf.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/SubAllocator.h"
#include "android/base/synchronization/Lock.h"
#include "android/crashreport/crash-handler.h"
#include "android/globals.h"

#include <memory>

#define ASGFX_DEBUG 0

#if ASGFX_DEBUG
#define ASGFX_LOG(fmt,...) printf("%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define ASGFX_LOG(fmt,...)
#endif

using android::base::AutoLock;
using android::base::Lock;
using android::base::LazyInstance;
using android::base::SubAllocator;

namespace android {
namespace emulation {
namespace asg {

struct Block {
    char* buffer = nullptr;
    SubAllocator* subAlloc = nullptr;
    uint64_t offsetIntoPhys = 0; // guest claimShared/mmap uses this
    // size: implicitly ADDRESS_SPACE_GRAPHICS_BLOCK_SIZE
    bool isEmpty = true;
    bool dedicated = false;
    bool usesVirtioGpuHostmem = false;
    uint64_t hostmemId = 0;
};

class Globals {
public:
    Globals() :
        mPerContextBufferSize(
                android_hw->hw_gltransport_asg_writeBufferSize) { }

    ~Globals() { clear(); }

    void initialize(const address_space_device_control_ops* ops) {
        AutoLock lock(mLock);

        if (mInitialized) return;

        mControlOps = ops;
        mInitialized = true;
    }

    void setConsumer(ConsumerInterface interface) {
        mConsumerInterface = interface;
    }

    ConsumerInterface getConsumerInterface() {
        if (!mConsumerInterface.create ||
            !mConsumerInterface.destroy ||
            !mConsumerInterface.save ||
            !mConsumerInterface.load) {
            crashhandler_die("Consumer interface has not been set\n");
        }
        return mConsumerInterface;
    }

    const address_space_device_control_ops* controlOps() {
        return mControlOps;
    }

    void clear() { }

    uint64_t perContextBufferSize() const {
        return mPerContextBufferSize;
    }

    Allocation newAllocation(
        uint64_t wantedSize,
        std::vector<Block>& existingBlocks,
        bool dedicated = false,
        bool usesVirtioGpuHostmem = false) {

        AutoLock lock(mLock);

        if (wantedSize > ADDRESS_SPACE_GRAPHICS_BLOCK_SIZE) {
            crashhandler_die(
                "wanted size 0x%llx which is "
                "greater than block size 0x%llx",
                (unsigned long long)wantedSize,
                (unsigned long long)ADDRESS_SPACE_GRAPHICS_BLOCK_SIZE);
        }

        size_t index = 0;

        Allocation res;

        for (auto& block : existingBlocks) {

            uint64_t hostmemId;
            if (block.isEmpty) {
                fillBlockLocked(
                        block,
                        dedicated,
                        wantedSize,
                        usesVirtioGpuHostmem,
                        &hostmemId);
            }

            auto buf = block.subAlloc->alloc(wantedSize);

            if (buf) {
                res.buffer = (char*)buf;
                res.blockIndex = index;
                res.offsetIntoPhys =
                    block.offsetIntoPhys +
                    block.subAlloc->getOffset(buf);
                res.size = wantedSize;
                res.dedicated = dedicated;
                res.hostmemId = hostmemId;
                return res;
            } else {
                // block full
            }

            ++index;
        }

        uint64_t hostmemId;
        Block newBlock;
        fillBlockLocked(
            newBlock,
            dedicated,
            wantedSize,
            usesVirtioGpuHostmem,
            &hostmemId);

        auto buf = newBlock.subAlloc->alloc(wantedSize);

        if (!buf) {
            crashhandler_die(
                "failed to allocate size 0x%llx "
                "(no free slots or out of host memory)",
                (unsigned long long)wantedSize);
        }

        existingBlocks.push_back(newBlock);

        res.buffer = (char*)buf;
        res.blockIndex = index;
        res.offsetIntoPhys =
            newBlock.offsetIntoPhys +
            newBlock.subAlloc->getOffset(buf);
        res.size = wantedSize;
        res.dedicated = dedicated;
        res.hostmemId = hostmemId;

        return res;
    }

    void deleteAllocation(const Allocation& alloc, std::vector<Block>& existingBlocks) {
        if (!alloc.buffer) return;

        AutoLock lock(mLock);

        if (existingBlocks.size() <= alloc.blockIndex) {
            crashhandler_die(
                "should be a block at index %zu "
                "but it is not found", alloc.blockIndex);
        }

        auto& block = existingBlocks[alloc.blockIndex];

        if (block.dedicated) {
            destroyBlockLocked(block);
            return;
        }

        if (!block.subAlloc->free(alloc.buffer)) {
            crashhandler_die(
                "failed to free %p (block start: %p)",
                alloc.buffer,
                block.buffer);
        }

        if (shouldDestryBlockLocked(block)) {
            destroyBlockLocked(block);
        }
    }

    Allocation allocRingStorage() {
        return newAllocation(
            sizeof(struct asg_ring_storage), mRingBlocks);
    }

    void freeRingStorage(const Allocation& alloc) {
        if (alloc.isView) return;
        deleteAllocation(alloc, mRingBlocks);
    }

    Allocation allocBuffer() {
        return newAllocation(
            mPerContextBufferSize, mBufferBlocks);
    }

    void freeBuffer(const Allocation& alloc) {
        if (alloc.isView) return;
        deleteAllocation(alloc, mBufferBlocks);
    }

    Allocation allocRingAndBufferStorageDedicated() {
        return newAllocation(
            sizeof(struct asg_ring_storage) + mPerContextBufferSize,
            mCombinedBlocks,
            true /* dedicated */,
            true /* virtio-gpu hostmem id mode */);
    }

    Allocation allocRingViewIntoCombined(const Allocation& alloc) {
        Allocation res = alloc;
        res.buffer = alloc.buffer;
        res.size = sizeof(struct asg_ring_storage);
        res.isView = true;
        return res;
    }

    Allocation allocBufferViewIntoCombined(const Allocation& alloc) {
        Allocation res = alloc;
        res.buffer = alloc.buffer + sizeof(asg_ring_storage);
        res.size = mPerContextBufferSize;
        res.isView = true;
        return res;
    }

    void freeRingAndBuffer(const Allocation& alloc) {
        deleteAllocation(alloc, mCombinedBlocks);
    }

    void save(base::Stream* stream) {
        // mSubAllocator->save(stream);
    }

    bool load(base::Stream* stream) {
        // clear();
        // mSubAllocator->load(stream);
        return true;
    }

private:

    void fillBlockLocked(
        Block& block,
        bool dedicated = false,
        size_t dedicatedSize = 0,
        bool usesVirtioGpuHostmem = false,
        uint64_t* hostmemIdOut = nullptr) {

        if (dedicated) {
            if (usesVirtioGpuHostmem) {
                void* buf =
                    aligned_buf_alloc(
                        ADDRESS_SPACE_GRAPHICS_PAGE_SIZE,
                        dedicatedSize);

                uint64_t hostmemId = mControlOps->hostmem_register(
                    (uint64_t)(uintptr_t)buf,
                    dedicatedSize);

                if (hostmemIdOut) *hostmemIdOut = hostmemId;

                block.buffer = (char*)buf;
                block.subAlloc =
                    new SubAllocator(
                        buf, dedicatedSize,
                        ADDRESS_SPACE_GRAPHICS_PAGE_SIZE);
                block.offsetIntoPhys = 0;

                block.isEmpty = false;
                block.usesVirtioGpuHostmem = usesVirtioGpuHostmem;
                block.hostmemId = hostmemId;
                block.dedicated = true;

            } else {
                crashhandler_die(
                    "Cannot use dedicated allocation without virtio-gpu hostmem id");
            }
        } else {
            if (usesVirtioGpuHostmem) {
                crashhandler_die(
                    "Only dedicated allocation allowed in virtio-gpu hostmem id path");
            } else {
                uint64_t offsetIntoPhys;
                int allocRes = get_address_space_device_hw_funcs()->
                    allocSharedHostRegionLocked(
                        ADDRESS_SPACE_GRAPHICS_BLOCK_SIZE, &offsetIntoPhys);

                if (allocRes) {
                    crashhandler_die(
                        "Failed to allocate physical address graphics backing memory.");
                }

                void* buf =
                    aligned_buf_alloc(
                        ADDRESS_SPACE_GRAPHICS_PAGE_SIZE,
                        ADDRESS_SPACE_GRAPHICS_BLOCK_SIZE);

                mControlOps->add_memory_mapping(
                    get_address_space_device_hw_funcs()->getPhysAddrStartLocked() +
                        offsetIntoPhys, buf,
                    ADDRESS_SPACE_GRAPHICS_BLOCK_SIZE);

                block.buffer = (char*)buf;
                block.subAlloc =
                    new SubAllocator(
                        buf, ADDRESS_SPACE_GRAPHICS_BLOCK_SIZE,
                        ADDRESS_SPACE_GRAPHICS_PAGE_SIZE);
                block.offsetIntoPhys = offsetIntoPhys;

                block.isEmpty = false;
            }
        }
    }

    void destroyBlockLocked(Block& block) {

        if (block.usesVirtioGpuHostmem) {
            mControlOps->hostmem_unregister(block.hostmemId);
        } else {
            mControlOps->remove_memory_mapping(
                get_address_space_device_hw_funcs()->getPhysAddrStartLocked() +
                    block.offsetIntoPhys,
                block.buffer,
                ADDRESS_SPACE_GRAPHICS_BLOCK_SIZE);

            get_address_space_device_hw_funcs()->freeSharedHostRegionLocked(
                block.offsetIntoPhys);
        }

        delete block.subAlloc;

        aligned_buf_free(block.buffer);

        block.isEmpty = true;
    }

    bool shouldDestryBlockLocked(const Block& block) const {
        return block.subAlloc->empty();
    }

    Lock mLock;
    uint64_t mPerContextBufferSize;
    bool mInitialized = false;
    const address_space_device_control_ops* mControlOps = 0;
    ConsumerInterface mConsumerInterface;
    std::vector<Block> mRingBlocks;
    std::vector<Block> mBufferBlocks;
    std::vector<Block> mCombinedBlocks;
};

static LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

// static
void AddressSpaceGraphicsContext::init(const address_space_device_control_ops* ops) {
    sGlobals->initialize(ops);
}

// static
void AddressSpaceGraphicsContext::clear() {
    sGlobals->clear();
}

// static
void AddressSpaceGraphicsContext::setConsumer(
    ConsumerInterface interface) {
    sGlobals->setConsumer(interface);
}

AddressSpaceGraphicsContext::AddressSpaceGraphicsContext(bool isVirtio) :
    mConsumerCallbacks((ConsumerCallbacks){
        [this] { return onUnavailableRead(); },
        [](uint64_t physAddr) {
            return (char*)sGlobals->controlOps()->get_host_ptr(physAddr);
        },
    }),
    mConsumerInterface(sGlobals->getConsumerInterface()),
    mIsVirtio(isVirtio) {

    if (mIsVirtio) {
        mCombinedAllocation = sGlobals->allocRingAndBufferStorageDedicated();
        mRingAllocation = sGlobals->allocRingViewIntoCombined(mCombinedAllocation);
        mBufferAllocation = sGlobals->allocBufferViewIntoCombined(mCombinedAllocation);
    } else {
        mRingAllocation = sGlobals->allocRingStorage();
        mBufferAllocation = sGlobals->allocBuffer();
    }

    if (!mRingAllocation.buffer) {
        crashhandler_die(
            "Failed to allocate ring for ASG context");
    }

    if (!mBufferAllocation.buffer) {
        crashhandler_die(
            "Failed to allocate buffer for ASG context");
    }

    mHostContext = asg_context_create(
        mRingAllocation.buffer,
        mBufferAllocation.buffer,
        sGlobals->perContextBufferSize());
    mHostContext.ring_config->buffer_size =
        sGlobals->perContextBufferSize();
    mHostContext.ring_config->flush_interval =
        android_hw->hw_gltransport_asg_writeStepSize;
    mHostContext.ring_config->host_consumed_pos = 0;
    mHostContext.ring_config->transfer_mode = 1;
    mHostContext.ring_config->transfer_size = 0;
    mHostContext.ring_config->in_error = 0;

    mSavedConfig = *mHostContext.ring_config;
}

AddressSpaceGraphicsContext::~AddressSpaceGraphicsContext() {
    if (mCurrentConsumer) {
        mExiting = 1;
        *(mHostContext.host_state) = ASG_HOST_STATE_EXIT;
        mConsumerMessages.send(ConsumerCommand::Exit);
        mConsumerInterface.destroy(mCurrentConsumer);
    }

    sGlobals->freeBuffer(mBufferAllocation);
    sGlobals->freeRingStorage(mRingAllocation);
    sGlobals->freeRingAndBuffer(mCombinedAllocation);
}

void AddressSpaceGraphicsContext::perform(AddressSpaceDevicePingInfo* info) {
    switch (static_cast<asg_command>(info->metadata)) {
    case ASG_GET_RING:
        info->metadata = mRingAllocation.offsetIntoPhys;
        info->size = mRingAllocation.size;
        break;
    case ASG_GET_BUFFER:
        info->metadata = mBufferAllocation.offsetIntoPhys;
        info->size = mBufferAllocation.size;
        break;
    case ASG_SET_VERSION: {
        auto guestVersion = (uint32_t)info->size;
        info->size = (uint64_t)(mVersion > guestVersion ? guestVersion : mVersion);
        mVersion = (uint32_t)info->size;
        mCurrentConsumer = mConsumerInterface.create(
            mHostContext, mConsumerCallbacks);

        if (mIsVirtio) {
            info->metadata = mCombinedAllocation.hostmemId;
        }
        break;
    }
    case ASG_NOTIFY_AVAILABLE:
        mConsumerMessages.trySend(ConsumerCommand::Wakeup);
        info->metadata = 0;
        break;
    case ASG_GET_CONFIG:
        *mHostContext.ring_config = mSavedConfig;
        info->metadata = 0;
        break;
    }
}

int AddressSpaceGraphicsContext::onUnavailableRead() {
    static const uint32_t kMaxUnavailableReads = 8;

    ++mUnavailableReadCount;
    ring_buffer_yield();

    ConsumerCommand cmd;

    if (mExiting) {
        mUnavailableReadCount = kMaxUnavailableReads;
    }

    if (mUnavailableReadCount >= kMaxUnavailableReads) {
        mUnavailableReadCount = 0;

sleep:
        *(mHostContext.host_state) = ASG_HOST_STATE_NEED_NOTIFY;
        mConsumerMessages.receive(&cmd);

        switch (cmd) {
            case ConsumerCommand::Wakeup:
                *(mHostContext.host_state) = ASG_HOST_STATE_CAN_CONSUME;
                break;
            case ConsumerCommand::Exit:
                *(mHostContext.host_state) = ASG_HOST_STATE_EXIT;
                return -1;
            case ConsumerCommand::Sleep:
                goto sleep;
            default:
                crashhandler_die(
                    "AddressSpaceGraphicsContext::onUnavailableRead: "
                    "Unknown command: 0x%x\n",
                    (uint32_t)cmd);
        }

        return 1;
    }
    return 0;
}

AddressSpaceDeviceType AddressSpaceGraphicsContext::getDeviceType() const {
    return AddressSpaceDeviceType::Graphics;
}

void AddressSpaceGraphicsContext::save(base::Stream*) const { }
bool AddressSpaceGraphicsContext::load(base::Stream*) { return true; }

}  // namespace asg
}  // namespace emulation
}  // namespace android
