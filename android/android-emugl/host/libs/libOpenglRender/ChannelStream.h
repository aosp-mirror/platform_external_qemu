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
#include "android/base/synchronization/MessageChannel.h"

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

    void enableSharedMemoryPings();

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

    uint32_t waitForGuestPingAndTrafficSizePageAddrs();
    void readFullyAndHangUpPageAddrs(uint8_t* dst, uint32_t len);

    void setCmdLatency(uint64_t time);

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

    struct PageAddrTransfer {
        uint32_t numPages;
        uint64_t firstPageOffset;
        uint64_t lastPageSize;
        uint32_t xferLen;
        uint64_t spinReceiveTime1;
        uint64_t spinReceiveTime2;
        uint64_t latencyStartUs;
        uint64_t latencyEndUs;
        uint64_t receiveSleepUs;
        uint64_t allocedChannelLockTime;
        std::vector<uint64_t> pages;
        std::vector<uint8_t> buffer;
    };
    std::vector<PageAddrTransfer> mCurrentTransfers;

    android::base::Lock mLock;
    android::base::ConditionVariable mCv;
    android::base::MessageChannel<int, 1024> mGuestPings;
    android::base::MessageChannel<int, 1024> mHostReplies;
    uint64_t mLastPingTimeUs = 0;
    bool mSleeping = true;

#define PING_STATE_IDLE 0
#define PING_STATE_GOT_XFER_INFO 1

    int mPingState = PING_STATE_IDLE;
    int mCurrentTransferIndex = 0;


    uint64_t mLastCheckTime = 0;
    uint64_t mLastPrintTime = 0;
    uint64_t mReceivedBytes = 0;
    uint64_t mReceivedBytesSinceLast = 0;
    uint64_t mLastLive = 0;
    uint64_t mLastYield = 0;
    uint64_t mLastSleep = 0;
    uint64_t mCommitBufferTime = 0;
    uint64_t mSpinReceiveTime = 0;
    uint64_t mSpinReceiveTime2 = 0;

    float mLastRate = 0.0f;

    bool mSlept = false;

};

}  // namespace emugl
