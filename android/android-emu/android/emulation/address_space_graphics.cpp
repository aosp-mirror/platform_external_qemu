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
const int AddressSpaceGraphicsContext::kRingSize = 16384;
const int AddressSpaceGraphicsContext::kMaxContexts = 256;
const int AddressSpaceGraphicsContext::kContextAllocationSize = 2 * kRingInfoSize + 2 * kRingSize;
const int AddressSpaceGraphicsContext::kBackingSize = kMaxContexts * kContextAllocationSize;
const int AddressSpaceGraphicsContext::kToHostRingInfoOffset = 0;
const int AddressSpaceGraphicsContext::kFromHostRingInfoOffset = kRingInfoSize;
const int AddressSpaceGraphicsContext::kToHostRingBufferOffset = kRingInfoSize * 2;
const int AddressSpaceGraphicsContext::kFromHostRingBufferOffset = kToHostRingBufferOffset + kRingSize;

class GraphicsBackingMemory {
public:
    GraphicsBackingMemory() :
        mMemory(
            aligned_buf_alloc(
                AddressSpaceGraphicsContext::kPageSize,
                AddressSpaceGraphicsContext::kBackingSize)) {
        if (!mMemory) {
            crashhandler_die(
                "Failed to allocate graphics backing memory. Wanted %zu bytes",
                AddressSpaceGraphicsContext::kBackingSize);
        }

        mSubAllocator.reset(new SubAllocator(
            mMemory,
            AddressSpaceGraphicsContext::kBackingSize,
            AddressSpaceGraphicsContext::kPageSize));
    }

    void clear() { 
        fprintf(stderr, "GraphicsBackingMemory::%s\n", __func__);
        mSubAllocator->freeAll();

        get_address_space_device_hw_funcs()->freeSharedHostRegionLocked(
            mPhysAddr -
            get_address_space_device_hw_funcs()->getPhysAddrStartLocked());
        mPhysAddr = 0;

        if (mInitialized && m_ops) {
            m_ops->remove_memory_mapping(mPhysAddr, mMemory, AddressSpaceGraphicsContext::kBackingSize);
            mInitialized = false;
            fprintf(stderr, "GraphicsBackingMemory::%s not init\n", __func__);
        }
    }

    ~GraphicsBackingMemory() {
        clear();

        mSubAllocator.reset();

        if (mMemory)
            aligned_buf_free(mMemory);
    }

    void initialize(const address_space_device_control_ops* ops) {
        fprintf(stderr, "GraphicsBackingMemory::%s\n", __func__);
        if (mInitialized) return;
        fprintf(stderr, "GraphicsBackingMemory::%s do actual\n", __func__);

        int allocRes = get_address_space_device_hw_funcs()->allocSharedHostRegionLocked(
            AddressSpaceGraphicsContext::kBackingSize, &mSharedRegionOffset);

        if (allocRes) {
            crashhandler_die(
                "Failed to allocate physical address graphics backing memory.");
        }

        mPhysAddr =
            get_address_space_device_hw_funcs()->getPhysAddrStartLocked() +
                mSharedRegionOffset;

        fprintf(stderr, "%s: mMemory: %p\n", __func__, mMemory);
        m_ops = ops;
        m_ops->add_memory_mapping(mPhysAddr, mMemory, AddressSpaceGraphicsContext::kBackingSize);
        mInitialized = true;

        ASGFX_LOG("shared region: [0x%llx 0x%llx]",
                  (unsigned long long)mSharedRegionOffset,
                  (unsigned long long)mSharedRegionOffset + AddressSpaceGraphicsContext::kBackingSize);
    }

    char* allocContextBuffer() {
        return (char*)mSubAllocator->alloc(AddressSpaceGraphicsContext::kContextAllocationSize);
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

static void initGraphicsContextRings(
    char* buffer,
    struct ring_buffer_with_view& toHost,
    struct ring_buffer_with_view& fromHost) {

    toHost.ring = reinterpret_cast<ring_buffer*>(buffer + AddressSpaceGraphicsContext::kToHostRingInfoOffset);
    fromHost.ring = reinterpret_cast<ring_buffer*>(buffer + AddressSpaceGraphicsContext::kFromHostRingInfoOffset);

    ring_buffer_view_init(
        toHost.ring,
        &toHost.view,
        reinterpret_cast<uint8_t*>(buffer + AddressSpaceGraphicsContext::kToHostRingBufferOffset),
        AddressSpaceGraphicsContext::kRingSize);

    ring_buffer_view_init(
        fromHost.ring,
        &fromHost.view,
        reinterpret_cast<uint8_t*>(buffer + AddressSpaceGraphicsContext::kFromHostRingBufferOffset),
        AddressSpaceGraphicsContext::kRingSize);
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
                fprintf(stderr, "%s: hanging up!\n", __func__);
                return ConsumerState::NeedNotify;
            }


            if (mExiting) {
                fprintf(stderr, "%s: exit!\n", __func__);
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

        // fprintf(stderr, "%s: can consume. avail: %u\n", __func__, availableToEcho);
        ring_buffer_view_read(
            mToHost.ring, &mToHost.view,
            mReadBuffer.data() + mReadPos, availableToEcho, 1);
        mReadPos += availableToEcho;

        if (mReadPos == 1024) {
            // fprintf(stderr, "%s: read 1024, send back. view buf [%p %p]. write pos: %u [%p %p]\n", __func__, mFromHost.view.buf,
                    // mFromHost.view.buf + AddressSpaceGraphicsContext::kRingSize, ring_buffer_view_get_ring_pos(&mFromHost.view, mFromHost.ring->write_pos),
                    // mFromHost.view.buf + ring_buffer_view_get_ring_pos(&mFromHost.view, mFromHost.ring->write_pos),
                    // mFromHost.view.buf + ring_buffer_view_get_ring_pos(&mFromHost.view, mFromHost.ring->write_pos) + 1024);
            ring_buffer_write_fully(
                mFromHost.ring,
                &mFromHost.view,
                mReadBuffer.data(),
                1024);
            mReadPos = 0;
            // fprintf(stderr, "%s: read 1024, send back (done)\n", __func__);
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

        mThreadReturnMessages.send(threadCmd);

        *mConsumerStatePtr = consumeReadbackLoop(*mConsumerStatePtr);
    }

    fprintf(stderr, "%s: exit thread\n", __func__);
}

int AddressSpaceGraphicsContext::onUnavailableRead() {
    static const uint32_t kMaxUnavailableReads = 500;
    ++mUnavailableReadCount;
    ring_buffer_yield();
    ThreadCommand threadCmd;
    if (mUnavailableReadCount == kMaxUnavailableReads) {
        *mConsumerStatePtr = ConsumerState::NeedNotify;
        // go to sleep until another ping
        fprintf(stderr, "%s: recv thread cmd\n", __func__);
        mThreadMessages.receive(&threadCmd);

        switch (threadCmd) {
            case ThreadCommand::Wakeup:
                fprintf(stderr, "%s: got wakeup cmd\n", __func__);
                *mConsumerStatePtr = ConsumerState::CanConsume;
                break;
            case ThreadCommand::Sleep:
                fprintf(stderr, "%s: got sleep cmd\n", __func__);
                *mConsumerStatePtr = ConsumerState::NeedNotify;
                break;
            case ThreadCommand::Exit:
                fprintf(stderr, "%s: got exit cmd\n", __func__);
                return -1;
        }

        fprintf(stderr, "%s: send return\n", __func__);
        mThreadReturnMessages.send(threadCmd);
    }
    return 0;
}

void AddressSpaceGraphicsContext::perform(AddressSpaceDevicePingInfo *info) {
    uint64_t result;

    switch (static_cast<Command>(info->metadata)) {
    case Command::AllocOrGetOffset:
        if (!mBuffer) {
            mBuffer = sGraphicsBackingMemory->allocContextBuffer();
            initGraphicsContextRings(
                mBuffer,
                mToHost, mFromHost);
            mConsumerStatePtr =
                reinterpret_cast<ConsumerState*>(&(mToHost.ring->state));
        }
        result = sGraphicsBackingMemory->getOffsetIntoPhysmem(mBuffer);
        ASGFX_LOG("AllocOrGetOffset: Offset: 0x%llx", result);
        break;
    case Command::GetSize:
        result = AddressSpaceGraphicsContext::kContextAllocationSize;
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
    case Command::NotifyAvailable:
                                         {
                                             ThreadCommand cmd = ThreadCommand::Wakeup;
        mThreadMessages.send(cmd);
        mThreadReturnMessages.receive(&cmd);
                                         }
        result = 0;
        break;
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
        mCurrentConsumer = mConsumerCreateFunc(
            mToHost,
            mFromHost,
            [this] {
                return onUnavailableRead();
            });
        break;
    case Command::DestroyConsumer:
                fprintf(stderr, "%s: send exit cmd\n", __func__);
        mExiting = 1;
        mThreadMessages.send(ThreadCommand::Exit);
                fprintf(stderr, "%s: done send exit cmd\n", __func__);
        mConsumerDestroyFunc(mCurrentConsumer);
        mCurrentConsumer = 0;
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

void AddressSpaceGraphicsContext::save(base::Stream* stream) const {
}

bool AddressSpaceGraphicsContext::load(base::Stream* stream) {
    return true;
}

}  // namespace emulation
}  // namespace android
