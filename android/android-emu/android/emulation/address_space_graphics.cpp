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
#include "android/base/ring_buffer.h"
#include "android/base/SubAllocator.h"
#include "android/crashreport/crash-handler.h"
#include "android/globals.h"
#include "android/opengl/GLProcessPipe.h"
#include "android/opengles.h"

#include <memory>

#include <pthread.h>

#define ASGFX_DEBUG 1

#if ASGFX_DEBUG
#define ASGFX_LOG(fmt,...) printf("%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define ASGFX_LOG(fmt,...)
#endif

using android::base::LazyInstance;
using android::base::SubAllocator;

namespace android {
namespace emulation {

// all static
const int AddressSpaceGraphicsContext::kPageSize = 4096;

const int AddressSpaceGraphicsContext::kRingInfoSize = kPageSize;
const int AddressSpaceGraphicsContext::kRingSize = 8192;
const int AddressSpaceGraphicsContext::kWriteBufferSize = 4 * 1048576;
const int AddressSpaceGraphicsContext::kWriteStepSize = 4096;
const int AddressSpaceGraphicsContext::kMaxContexts = 256;

const int AddressSpaceGraphicsContext::kToHostRingInfoOffset = 0;
const int AddressSpaceGraphicsContext::kFromHostRingInfoOffset = kRingInfoSize;
const int AddressSpaceGraphicsContext::kToHostTransferRingInfoOffset = 2 * kRingInfoSize;

const int AddressSpaceGraphicsContext::kToHostRingBufferOffset = kToHostTransferRingInfoOffset + kRingInfoSize;
const int AddressSpaceGraphicsContext::kFromHostRingBufferOffset = kToHostRingBufferOffset + kRingSize;
const int AddressSpaceGraphicsContext::kToHostTransferRingBufferOffset = kFromHostRingBufferOffset + kRingSize;

static emugl::Renderer* sRenderer = 0;

class GraphicsBackingMemory {
public:
    GraphicsBackingMemory() :
        mPerContextDataRingSize(android_hw->hw_gltransport_asg_dataRingSize),
        mPerContextWriteBufferSize(android_hw->hw_gltransport_asg_writeBufferSize),
        mPerContextWriteStepSize(android_hw->hw_gltransport_asg_writeStepSize),
        mPerContextAllocationSize(
            3 * AddressSpaceGraphicsContext::kRingInfoSize +
            2 * AddressSpaceGraphicsContext::kRingSize +
            mPerContextDataRingSize +
            mPerContextWriteBufferSize),
        mWriteBufferOffset(
            3 * AddressSpaceGraphicsContext::kRingInfoSize +
            2 * AddressSpaceGraphicsContext::kRingSize +
            mPerContextDataRingSize),
        mMaxContexts(AddressSpaceGraphicsContext::kMaxContexts),
        mTotalBackingSize(mMaxContexts * mPerContextAllocationSize),
        mMemory(
            aligned_buf_alloc(
                AddressSpaceGraphicsContext::kPageSize,
                mTotalBackingSize)) {
        fprintf(stderr, "%s: per context data ring: %u write buffer size: %u step: %u\n", __func__,
                mPerContextDataRingSize,
                mPerContextWriteBufferSize,
                mPerContextWriteStepSize);
        fprintf(stderr, "%s: alloc size: %u\n", __func__, mPerContextAllocationSize);
        if (!mMemory) {
            crashhandler_die(
                "Failed to allocate graphics backing memory. Wanted %zu bytes",
                mTotalBackingSize);
        }

        sRenderer = android_getOpenglesRenderer().get();

        mSubAllocator.reset(new SubAllocator(
            mMemory,
            mTotalBackingSize,
            AddressSpaceGraphicsContext::kPageSize));
    }

    void clear() { 
        mSubAllocator->freeAll();

        get_address_space_device_hw_funcs()->freeSharedHostRegionLocked(
            mPhysAddr -
            get_address_space_device_hw_funcs()->getPhysAddrStartLocked());

        if (mInitialized && m_ops) {
            m_ops->remove_memory_mapping(mPhysAddr, mMemory, mTotalBackingSize);
            mInitialized = false;
            mPhysAddr = 0;
        }
    }

    ~GraphicsBackingMemory() {
        clear();

        mSubAllocator.reset();

        if (mMemory)
            aligned_buf_free(mMemory);
    }

    uint64_t getAllocSize() const {
        return mPerContextAllocationSize;
    }

    uint32_t writeBufferOffset() {
        return mWriteBufferOffset;
    }

    void initialize(const address_space_device_control_ops* ops) {
        if (mInitialized) return;

        int allocRes = get_address_space_device_hw_funcs()->allocSharedHostRegionLocked(
                mTotalBackingSize, &mSharedRegionOffset);

        if (allocRes) {
            crashhandler_die(
                "Failed to allocate physical address graphics backing memory.");
        }

        mPhysAddr =
            get_address_space_device_hw_funcs()->getPhysAddrStartLocked() +
                mSharedRegionOffset;

        m_ops = ops;
        m_ops->add_memory_mapping(mPhysAddr, mMemory, mTotalBackingSize);
        mInitialized = true;

        ASGFX_LOG("shared region: [0x%llx 0x%llx]",
                  (unsigned long long)mSharedRegionOffset,
                  (unsigned long long)mSharedRegionOffset + mTotalBackingSize);
    }

    char* allocContextBuffer() {
        return (char*)mSubAllocator->alloc(mPerContextAllocationSize);
    }

    uint64_t getOffsetIntoPhysmem(char* ptr) {
        ASGFX_LOG("suballoc offset 0x%llx, shared region offset 0x%llx",
            (unsigned long long)mSubAllocator->getOffset(ptr),
            (unsigned long long)mSharedRegionOffset);
        return mSubAllocator->getOffset(ptr) + mSharedRegionOffset;
    }

    void freeContextBuffer(char* ptr) {
        mSubAllocator->free(ptr);
    }

    void save(base::Stream* stream) {
        mSubAllocator->save(stream);
    }

    bool load(base::Stream* stream) {
        clear();
        mSubAllocator->load(stream);
        return true;
    }

private:
    uint32_t mPerContextDataRingSize = 0;
    uint32_t mPerContextWriteBufferSize = 0;
    uint32_t mPerContextWriteStepSize = 0;
    uint32_t mPerContextAllocationSize = 0;
    uint32_t mWriteBufferOffset = 0;
    uint32_t mMaxContexts = 0;
    uint32_t mTotalBackingSize = 0;
    
    uint64_t mSharedRegionOffset = 0;
    uint64_t mPhysAddr = 0;
    void* mMemory = 0;
    std::unique_ptr<SubAllocator> mSubAllocator;
    const address_space_device_control_ops *m_ops = 0;  // do not save/load
    bool mInitialized = false;
};

static LazyInstance<GraphicsBackingMemory> sGraphicsBackingMemory = LAZY_INSTANCE_INIT;

// static
void AddressSpaceGraphicsContext::init(const address_space_device_control_ops* ops) {
    sGraphicsBackingMemory->initialize(ops);
}

// static
void AddressSpaceGraphicsContext::clear() {
    sGraphicsBackingMemory->clear();
}

AddressSpaceGraphicsContext::AddressSpaceGraphicsContext() :
    mThread([this] { threadFunc(); }) {
}

AddressSpaceGraphicsContext::~AddressSpaceGraphicsContext() {
    if (mBuffer) sGraphicsBackingMemory->freeContextBuffer(mBuffer);
    mExiting = 1;
    mThreadMessages.send(ThreadCommand::Exit);
    mThread.wait();
    if (mCurrentConsumer) mConsumerDestroyFunc(mCurrentConsumer);
}

static void initGraphicsContextConfig(
    uint32_t writeBufferSize,
    uint32_t writeStepSize,
    uint32_t dataRingSize,
    uint32_t* configOut) {
    configOut[0] = writeBufferSize;
    configOut[1] = writeStepSize;
    configOut[2] = dataRingSize;
}

static void initGraphicsContextRings(
    char* buffer,
    struct ring_buffer_with_view& toHost,
    struct ring_buffer_with_view& fromHost,
    struct ring_buffer_with_view& toHostData) {

    toHost.ring = reinterpret_cast<ring_buffer*>(buffer + AddressSpaceGraphicsContext::kToHostRingInfoOffset);
    fromHost.ring = reinterpret_cast<ring_buffer*>(buffer + AddressSpaceGraphicsContext::kFromHostRingInfoOffset);
    toHostData.ring = reinterpret_cast<ring_buffer*>(buffer + AddressSpaceGraphicsContext::kToHostTransferRingInfoOffset);

    ring_buffer_view_init(
        toHost.ring,
        &toHost.view,
        reinterpret_cast<uint8_t*>(buffer + AddressSpaceGraphicsContext::kToHostRingBufferOffset),
        AddressSpaceGraphicsContext::kRingSize);

    initGraphicsContextConfig(
        android_hw->hw_gltransport_asg_writeBufferSize,
        android_hw->hw_gltransport_asg_writeStepSize,
        android_hw->hw_gltransport_asg_dataRingSize,
        toHost.ring->config);

    ring_buffer_view_init(
        fromHost.ring,
        &fromHost.view,
        reinterpret_cast<uint8_t*>(buffer + AddressSpaceGraphicsContext::kFromHostRingBufferOffset),
        AddressSpaceGraphicsContext::kRingSize);

    ring_buffer_view_init(
        toHostData.ring,
        &toHostData.view,
        reinterpret_cast<uint8_t*>(buffer + AddressSpaceGraphicsContext::kToHostTransferRingBufferOffset),
        android_hw->hw_gltransport_asg_dataRingSize);
}

static void doEcho(struct ring_buffer_with_view& toHost,
                   struct ring_buffer_with_view& fromHost) {
    uint32_t availableToEcho =
        ring_buffer_available_read(
            toHost.ring, &toHost.view);

    ASGFX_LOG("start. avail: 0x%x", availableToEcho);

    if (!availableToEcho) return;

    std::vector<char> echoBuffer(availableToEcho);

    for (uint32_t i = 0; i < availableToEcho; ++i) {
        ring_buffer_view_read(
            toHost.ring, &toHost.view,
            echoBuffer.data() + i, 1, 1);
    }

    ring_buffer_write_fully(
        fromHost.ring,
        &fromHost.view,
        echoBuffer.data(),
        echoBuffer.size());

    ASGFX_LOG("done");
}

void AddressSpaceGraphicsContext::echoAsync() {
    if (*mConsumerStatePtr == ConsumerState::NeedNotify) {
        mThreadMessages.send(ThreadCommand::Wakeup);
    }
}

AddressSpaceGraphicsContext::ConsumerState
AddressSpaceGraphicsContext::consumeReadbackLoop(
    AddressSpaceGraphicsContext::ConsumerState consumerState) {

    const uint32_t maxUnavailableUntilHangup = 500;
    uint32_t unavailableCount = 0;

    while (1) {
        uint32_t availableToEcho =
            ring_buffer_available_read(
                mToHost.ring, &mToHost.view);

        if (!availableToEcho) {
            ++unavailableCount;

            if (unavailableCount == maxUnavailableUntilHangup) {
                return ConsumerState::NeedNotify;
            }


            if (mExiting) {
                return ConsumerState::NeedNotify;
            }

            mThread.yield();
            continue;
        }

        unavailableCount = 0;

        size_t wantedReadBufferSize =
            (mReadPos + availableToEcho) * 2;

        if (mReadPos + availableToEcho > mReadBuffer.size()) {
            mReadBuffer.resize(wantedReadBufferSize);
        }

        ring_buffer_view_read(
            mToHost.ring, &mToHost.view,
            mReadBuffer.data() + mReadPos, availableToEcho, 1);
        mReadPos += availableToEcho;

        if (mReadPos == 1024) {
            ring_buffer_write_fully(
                mFromHost.ring,
                &mFromHost.view,
                mReadBuffer.data(),
                1024);
            mReadPos = 0;
        }
    }

    crashhandler_die("consumeReadbackLoop: unreachable");
}

void AddressSpaceGraphicsContext::threadFunc() {
    ThreadCommand threadCmd;
    while (1) {
        mThreadMessages.receive(&threadCmd);

        switch (threadCmd) {
            case ThreadCommand::Wakeup:
                *mConsumerStatePtr = ConsumerState::CanConsume;
                break;
            case ThreadCommand::Sleep:
                *mConsumerStatePtr = ConsumerState::NeedNotify;
                break;
            case ThreadCommand::Exit:
                return;
        }

        mThreadReturnMessages.trySend(threadCmd);

        *mConsumerStatePtr = consumeReadbackLoop(*mConsumerStatePtr);
    }
}

int AddressSpaceGraphicsContext::onUnavailableRead() {
    static const uint32_t kMaxUnavailableReads = 800;
    ++mUnavailableReadCount;
    ring_buffer_yield();
    ThreadCommand threadCmd;
    if (mUnavailableReadCount == kMaxUnavailableReads) {
        mUnavailableReadCount = 0;
        *mConsumerStatePtr = ConsumerState::NeedNotify;
        // go to sleep until another ping
        mThreadMessages.receive(&threadCmd);

        switch (threadCmd) {
            case ThreadCommand::Wakeup:
                *mConsumerStatePtr = ConsumerState::CanConsume;
                break;
            case ThreadCommand::Sleep:
                *mConsumerStatePtr = ConsumerState::NeedNotify;
                break;
            case ThreadCommand::Exit:
                mThreadReturnMessages.trySend(threadCmd);
                return -1;
        }

        mThreadReturnMessages.trySend(threadCmd);
    }
    return 0;
}

void AddressSpaceGraphicsContext::getPtrAndSize(uint64_t data, char** ptrOut, size_t* sizeOut) {
    uint32_t offsetRelativeToWriteBufferStart = (uint32_t)data;
    uint32_t size = (uint32_t)(data >> 32);

    *ptrOut = mBuffer + sGraphicsBackingMemory->writeBufferOffset() + offsetRelativeToWriteBufferStart;
    *sizeOut = size;
}

void AddressSpaceGraphicsContext::perform(AddressSpaceDevicePingInfo *info) {
    uint64_t result;

    switch (static_cast<Command>(info->metadata)) {
    case Command::AllocOrGetOffset:
        if (!mBuffer) {
            mBuffer = sGraphicsBackingMemory->allocContextBuffer();
            initGraphicsContextRings(
                mBuffer,
                mToHost, mFromHost, mToHostData);
            mConsumerStatePtr =
                reinterpret_cast<ConsumerState*>(&(mToHost.ring->state));
        }
        result = sGraphicsBackingMemory->getOffsetIntoPhysmem(mBuffer);
        ASGFX_LOG("AllocOrGetOffset: Offset: 0x%llx", (unsigned long long)result);
        break;
    case Command::GetSize:
        result = sGraphicsBackingMemory->getAllocSize();
        break;
    case Command::GuestInitializedRings: {
        uint32_t hostRingVersion =
            mToHost.ring->host_version;
        uint32_t guestRingVersion =
            mToHost.ring->guest_version;
        mRingVersion =
            (hostRingVersion > guestRingVersion) ?
                guestRingVersion : hostRingVersion;
        break;
    }
    case Command::NotifyAvailable: {
        mThreadMessages.trySend(ThreadCommand::Wakeup);
        ring_buffer_yield();
        result = 0;
        break;
    }
    case Command::Echo:
        doEcho(mToHost, mFromHost);
        result = 0;
        break;
    case Command::EchoAsync:
        echoAsync();
        result = 0;
        break;
    case Command::EchoAsyncStart:
        mThread.start();
        result = 0;
        break;
    case Command::EchoAsyncStop:
        mExiting = 1;
        mThreadMessages.send(ThreadCommand::Exit);
        mThread.wait();
        result = 0;
        break;
    case Command::CreateConsumer:
        *mConsumerStatePtr = ConsumerState::NeedNotify;
        mCurrentConsumer = mConsumerCreateFunc(
            mToHost,
            mFromHost,
            [this] {
                return onUnavailableRead();
            });
        break;
    case Command::DestroyConsumer:
        mExiting = 1;
        mThreadMessages.send(ThreadCommand::Exit);
        mConsumerDestroyFunc(mCurrentConsumer);
        mCurrentConsumer = 0;
        break;
    case Command::CreateConsumer2:
        *mConsumerStatePtr = ConsumerState::NeedNotify;
        mCurrentConsumer = mConsumerCreateFunc2(
            mToHost,
            mFromHost,
            mToHostData,
            [this] { return onUnavailableRead(); },
            [this](uint64_t data, char** ptrOut, size_t* sizeOut) { getPtrAndSize(data, ptrOut, sizeOut); });
        break;
    case Command::SetAndCreateOpenglConsumer:
        setConsumer(
            [] (struct ring_buffer_with_view toHost,
                struct ring_buffer_with_view fromHost,
                OnUnavailableReadCallback cb) {
                return sRenderer->createRingRenderThread(
                    toHost, fromHost, cb);
            },
            [] (void* thread) {
                sRenderer->destroyRingRenderThread(thread);
            });
        *mConsumerStatePtr = ConsumerState::NeedNotify;
        mCurrentConsumer = mConsumerCreateFunc(
            mToHost,
            mFromHost,
            [this] {
                return onUnavailableRead();
            });
        break;
    case Command::SetAndCreateOpenglConsumer2:
        setConsumer2(
            [] (struct ring_buffer_with_view toHost,
                struct ring_buffer_with_view fromHost,
                struct ring_buffer_with_view toHostData,
                OnUnavailableReadCallback cb,
                GetPtrAndSizeCallback cb2) {
                return sRenderer->createRingRenderThread2(
                    toHost, fromHost, toHostData, cb, cb2);
            },
            [] (void* thread) {
                sRenderer->destroyRingRenderThread(thread);
            });
        *mConsumerStatePtr = ConsumerState::NeedNotify;
        mCurrentConsumer = mConsumerCreateFunc2(
            mToHost,
            mFromHost,
            mToHostData,
            [this] { return onUnavailableRead(); },
            [this](uint64_t data, char** ptrOut, size_t* sizeOut) { getPtrAndSize(data, ptrOut, sizeOut); });
        break;
    default:
        result = -1;
        break;
    }

    info->metadata = result;
}

AddressSpaceDeviceType AddressSpaceGraphicsContext::getDeviceType() const {
    return AddressSpaceDeviceType::Graphics;
}

void AddressSpaceGraphicsContext::setConsumer(
    AddressSpaceGraphicsContext::ConsumerCreateCallback createFunc,
    AddressSpaceGraphicsContext::ConsumerDestroyCallback destroyFunc) {
    mConsumerCreateFunc = createFunc;
    mConsumerDestroyFunc = destroyFunc;
}

void AddressSpaceGraphicsContext::setConsumer2(
    AddressSpaceGraphicsContext::ConsumerCreateCallback2 createFunc,
    AddressSpaceGraphicsContext::ConsumerDestroyCallback destroyFunc) {
    mConsumerCreateFunc2 = createFunc;
    mConsumerDestroyFunc = destroyFunc;
}

void AddressSpaceGraphicsContext::save(base::Stream* stream) const {
}

bool AddressSpaceGraphicsContext::load(base::Stream* stream) {
    return true;
}

}  // namespace emulation
}  // namespace android
