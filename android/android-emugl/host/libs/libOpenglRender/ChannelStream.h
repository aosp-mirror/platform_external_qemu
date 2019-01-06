// Copyright (C) 2016 The Android Open Source Project
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
#pragma once

#include "android/base/synchronization/Lock.h"

#include "OpenglRender/IOStream.h"
#include "RenderChannelImpl.h"

#include <memory>

struct ring_buffer;
struct ring_buffer_view;

namespace emugl {

// An IOStream instance that can be used by the host RenderThread to
// wrap a RenderChannelImpl channel.
class ChannelStream final : public IOStream {
public:
    ChannelStream(RenderChannelImpl* channel, size_t bufSize);
    ~ChannelStream();

    void forceStop();
    int writeFully(const void* buf, size_t len) override;
    const unsigned char *readFully( void *buf, size_t len) override;

    void setSharedMemoryCommandInfo(
        bool* modePtr,
        uint64_t* toHostRingAddr,
        uint64_t* fromHostRingAddr,
        ring_buffer** toHostRingHandle,
        ring_buffer** fromHostRingHandle,
        ring_buffer_view* toHostRingBufferView,
        ring_buffer_view* fromHostRingBufferView);

    bool printStats();

    bool isHighTraffic() const;

    bool isSharedMemory() {
        return *mSharedMemoryCommandModePtr;
    }

    bool firstSharedMemoryRead() {
        return isSharedMemory() && !mLastReadUsingSharedMemory;
    }

    uint32_t waitForGuestPingAndTrafficSize();
    void readFullyAndHangUp(uint8_t* dst, uint32_t len);

protected:
    virtual void* allocBuffer(size_t minSize) override final;
    virtual int commitBuffer(size_t size) override final;
    virtual const unsigned char* readRaw(void* buf, size_t* inout_len)
            override final;
    virtual void* getDmaForReading(uint64_t guest_paddr) override final;
    virtual void unlockDma(uint64_t guest_paddr) override final;

    void onSave(android::base::Stream* stream) override;
    unsigned char* onLoad(android::base::Stream* stream) override;

private:
    RenderChannelImpl* mChannel;
    RenderChannel::Buffer mWriteBuffer;
    RenderChannel::Buffer mReadBuffer;
    size_t mReadBufferLeft = 0;

    bool* mSharedMemoryCommandModePtr = nullptr;
    uint64_t* mToHostRingAddrPtr = nullptr;
    uint64_t* mFromHostRingAddrPtr = nullptr;
    ring_buffer** mToHostRingHandle = nullptr;
    ring_buffer** mFromHostRingHandle = nullptr;
    ring_buffer_view* mToHostRingBufferView = nullptr;
    ring_buffer_view* mFromHostRingBufferView = nullptr;
    bool mLastReadUsingSharedMemory = false;

    android::base::Lock mLock;
    android::base::ConditionVariable mCv;
    uint64_t mLastPingTimeUs = 0;
    bool mSleeping = true;

    uint64_t mLastCheckTime = 0;
    uint64_t mLastPrintTime = 0;
    uint64_t mReceivedBytes = 0;
    uint64_t mReceivedBytesSinceLast = 0;
    uint64_t mLastLive = 0;
    uint64_t mLastYield = 0;
    uint64_t mLastSleep = 0;
    uint64_t mCommitBufferTime = 0;
    float mLastRate = 0.0f;
};

}  // namespace emugl
