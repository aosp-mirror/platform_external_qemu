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
#include "ChannelStream.h"

#include "android/base/containers/StaticMap.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/ring_buffer.h"
#include "android/base/system/System.h"

#include "FrameBuffer.h"
#include "OpenglRender/RenderChannel.h"

#define EMUGL_DEBUG_LEVEL  0
#include "emugl/common/debug.h"
#include "emugl/common/dma_device.h"
#include "emugl/common/vm_operations.h"

#include <iomanip>
#include <sstream>
#include <string>

#include <assert.h>
#include <memory.h>

#define CHANNEL_STREAM_DEBUG 0

#if CHANNEL_STREAM_DEBUG
#define D(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define D(fmt,...)
#endif

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::StaticLock;
using android::base::StaticMap;
using android::base::System;

namespace emugl {

static StaticLock sAllocatedChannelLock;
static LazyInstance<StaticMap<ChannelStream*, bool>> sAllocatedChannels =
    LAZY_INSTANCE_INIT;

using IoResult = RenderChannel::IoResult;

ChannelStream::ChannelStream(RenderChannelImpl* channel, size_t bufSize)
    : IOStream(bufSize), mChannel(channel) {
    mWriteBuffer.resize_noinit(bufSize);
}

ChannelStream::~ChannelStream() {
            // mHostReplies.send(0);
    AutoLock lock(sAllocatedChannelLock);
    sAllocatedChannels->erase(this);
}

void* ChannelStream::allocBuffer(size_t minSize) {
    if (mWriteBuffer.size() < minSize) {
        mWriteBuffer.resize_noinit(minSize);
    }
    return mWriteBuffer.data();
}

int ChannelStream::commitBuffer(size_t size) {
    D("committing %zu bytes from host", size);
    assert(size <= mWriteBuffer.size());
    uint64_t commit_start_us = System::get()->getHighResTimeUs();
    if (*mSharedMemoryCommandModePtr) {
        D("with ring");
        ring_buffer* fromHost = *mFromHostRingHandle;
        if (!fromHost) {
            fprintf(stderr, "%s: FATAL: did not init ring buffer!\n", __func__);
            abort();
        }
        ring_buffer_write_fully(fromHost, mFromHostRingBufferView, mWriteBuffer.data(), size);
    } else {
        D("with pipe");
        if (mWriteBuffer.isAllocated()) {
            mWriteBuffer.resize(size);
            mChannel->writeToGuest(std::move(mWriteBuffer));
        } else {
            mChannel->writeToGuest(
                    RenderChannel::Buffer(mWriteBuffer.data(), mWriteBuffer.data() + size));
        }
    }
    uint64_t commit_end_us = System::get()->getHighResTimeUs();

    mCommitBufferTime += (commit_end_us - commit_start_us);
    return size;
}

bool ChannelStream::printStats() {
    if (mReceivedBytes < 1000000ULL * 2ULL) return false;

    ring_buffer* toHost = *mToHostRingHandle;

    ring_buffer dummy;

    if (!toHost) {
        ring_buffer_init(&dummy);
        toHost = &dummy;
    }

    uint64_t bytesSentThisTime = 
        mReceivedBytes - mReceivedBytesSinceLast;

    uint64_t thisTime = System::get()->getHighResTimeUs();

    float intervalSec = (thisTime - mLastCheckTime) / 1000000.0f;
    float printSec = (thisTime - mLastPrintTime) / 1000000.0f;

    if (printSec > 0.99f) {
        fprintf(stderr, "%s: since last: l/y/s %lu %lu %lu. rate last: %f mb/s. spin receive time: %f s (implied live rate: %f mb/s) (rt side: %f s %f mb/s) commit time: %f s\n", __func__,
    toHost->read_live_count - mLastLive,
    toHost->read_yield_count - mLastYield,
    toHost->read_sleep_us_count - mLastSleep,
    (float)bytesSentThisTime / 1048576.0f / intervalSec,

    mSpinReceiveTime / 1000000.0f,
    (float)bytesSentThisTime / mSpinReceiveTime,
    mSpinReceiveTime2 / 1000000.0f,
    (float)bytesSentThisTime / mSpinReceiveTime2,

    mCommitBufferTime / 1000000.0f);
    mLastPrintTime = thisTime;
    }

    mLastRate = (float)bytesSentThisTime / 1048576.0f / intervalSec;

    mCommitBufferTime = 0;
    mReceivedBytesSinceLast = mReceivedBytes;
    mLastCheckTime = thisTime;

    mLastLive = toHost->read_live_count;
    mLastYield = toHost->read_yield_count;
    mLastSleep = toHost->read_sleep_us_count;

    mSpinReceiveTime = 0;
    mSpinReceiveTime2 = 0;
    return true;
}

bool ChannelStream::isHighTraffic() const {
    if (mReceivedBytes < 1000000ULL * 200ULL) return false;
    return mLastRate > 40.0f;
}

uint32_t ChannelStream::waitForGuestPingAndTrafficSize() {
    if (firstSharedMemoryRead()) {
        mLastReadUsingSharedMemory = true;
    }

    ring_buffer* toHost = *mToHostRingHandle;

    auto future = System::get()->getUnixTimeUs() + 1000000ULL;
    auto message = mGuestPings.timedReceive(future);

    if (message) {

    } else {
        mSlept = true;
        auto fb = FrameBuffer::getFB();
        if (fb) {
            fb->onGuestThreadIdle();
        }
        int message = 0;
        mGuestPings.receive(&message);
    }

    if (mSlept) {
        auto fb = FrameBuffer::getFB();
        if (fb) {
            fb->onGuestThreadWake();
        }
        mSlept = false;
    }


    // fprintf(stderr, "%s: received a message\n", __func__);
    toHost->state = RING_BUFFER_SYNC_PRODUCER_ACTIVE;

    uint64_t spinReceiveTime1_start_us = System::get()->getHighResTimeUs();
    bool exiting = false;

    if (mChannel->isStopped()) {
        // emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this]() { });
        fprintf(stderr, "%s: %p channel stopped\n", __func__, this);
        exiting = true;
    }

    if (!g_emugl_dma_get_host_addr(*mToHostRingAddrPtr)) {
        // emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this]() { });
        fprintf(stderr, "%s: %p channel stopped because dma mapping invalid\n", __func__, this);
        mChannel->stopFromHost();
        exiting = true;
    }

    if (exiting){
        toHost->state = RING_BUFFER_SYNC_CONSUMER_HANGING_UP;
        // unlockDma(*mToHostRingAddrPtr);
        return 0;
    }

    uint32_t len;
    ring_buffer_read_fully(toHost, mToHostRingBufferView, &len, sizeof(uint32_t));
    // fprintf(stderr, "%s: msg len: %u\n", __func__, len);
    if (!len) {
        toHost->state = RING_BUFFER_SYNC_CONSUMER_HANGING_UP;
        // unlockDma(*mToHostRingAddrPtr);
    }
    uint64_t spinReceiveTime1_end_us = System::get()->getHighResTimeUs();

    mSpinReceiveTime += (spinReceiveTime1_end_us - spinReceiveTime1_start_us);

    return len;
}

void ChannelStream::readFullyAndHangUp(uint8_t* dst, uint32_t len) {
    uint64_t spinReceiveTime2_start_us = System::get()->getHighResTimeUs();

    ring_buffer* toHost = *mToHostRingHandle;
    ring_buffer_read_fully(toHost, mToHostRingBufferView, dst, len);
    mReceivedBytes += len;

    uint64_t spinReceiveTime2_end_us = System::get()->getHighResTimeUs();

    mSpinReceiveTime += (spinReceiveTime2_end_us - spinReceiveTime2_start_us);

    toHost->state = RING_BUFFER_SYNC_CONSUMER_HANGING_UP;
    // unlockDma(*mToHostRingAddrPtr);
}

uint32_t ChannelStream::waitForGuestPingAndTrafficSizePageAddrs() {
    if (firstSharedMemoryRead()) {
        mLastReadUsingSharedMemory = true;
    }

    ring_buffer* toHost = *mToHostRingHandle;

    int nextTransferIndex = 0;

    uint64_t sleep_start_us = System::get()->getHighResTimeUs();
    mGuestPings.receive(&nextTransferIndex);
    uint64_t sleep_end_us = System::get()->getHighResTimeUs();

    toHost->state = RING_BUFFER_SYNC_PRODUCER_ACTIVE;

    bool exiting = false;

    if (nextTransferIndex == -1) {
        fprintf(stderr, "%s: %p channel stopped\n", __func__, this);
        exiting = true;
    }

    if (mChannel->isStopped()) {
        // emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this]() { });
        fprintf(stderr, "%s: %p channel stopped\n", __func__, this);
        exiting = true;
    }

    if (!g_emugl_dma_get_host_addr(*mToHostRingAddrPtr)) {
        // emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this]() { });
        fprintf(stderr, "%s: %p channel stopped because dma mapping invalid\n", __func__, this);
        mChannel->stopFromHost();
        exiting = true;
    }

    if (exiting){
        toHost->state = RING_BUFFER_SYNC_CONSUMER_HANGING_UP;
        mHostReplies.send(0);
        // unlockDma(*mToHostRingAddrPtr);
        return 0;
    }

    auto& x = mCurrentTransfers[nextTransferIndex];
    x.receiveSleepUs = sleep_end_us - sleep_start_us;
    mCurrentTransferIndex = nextTransferIndex;
    return x.xferLen;
}

void ChannelStream::readFullyAndHangUpPageAddrs(uint8_t* dst, uint32_t len) {
    auto& x = mCurrentTransfers[mCurrentTransferIndex];

    uint64_t spinReceiveTime2_start_us = System::get()->getHighResTimeUs();
    memcpy(dst, x.buffer.data(), len);
    uint64_t spinReceiveTime2_end_us = System::get()->getHighResTimeUs();
    x.spinReceiveTime2 = spinReceiveTime2_end_us - spinReceiveTime2_start_us;
    x.latencyEndUs = spinReceiveTime2_end_us;

    uint64_t latencyUs = x.latencyEndUs - x.latencyStartUs;

    if (latencyUs > 10000) {
        fprintf(stderr,
                "%s: **********high latency transfer: %f ms. spin recv 1/2 %f "
                "ms %f ms (total live %f ms). sleep for this xfer: %f ms channel lock time %f ms. live rates: %f mb/s, %f mb/s "
                "latency rate: %f mb/s\n",
                __func__, latencyUs / 1000.0f,
                x.spinReceiveTime1 / 1000.0f,
                x.spinReceiveTime2 / 1000.0f,
                x.spinReceiveTime1 / 1000.0f + x.spinReceiveTime2 / 1000.0f,
                x.receiveSleepUs / 1000.0f,
                x.allocedChannelLockTime / 1000.0f,
                1000000.0f * x.xferLen / 1048576.0f / (float)x.spinReceiveTime1,
                1000000.0f * x.xferLen / 1048576.0f / (float)x.spinReceiveTime1,
                1000000.0f * x.xferLen / 1048576.0f / (float)latencyUs);
    }

    mHostReplies.send(mCurrentTransferIndex);
    mSpinReceiveTime2 += (spinReceiveTime2_end_us - spinReceiveTime2_start_us);

}

const unsigned char* ChannelStream::readRaw(void* buf, size_t* inout_len) {
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<uint8_t*>(buf);

    if (*mSharedMemoryCommandModePtr) {
        if (!mLastReadUsingSharedMemory) {
            emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this](bool removing) {
                mLastPingTimeUs = System::get()->getUnixTimeUs();
            });
        }
        mLastReadUsingSharedMemory = true;
        D("Reading commands with ring");
        ring_buffer* toHost = *mToHostRingHandle;
        if (!toHost) {
            fprintf(stderr, "%s: FATAL: did not init ring buffer!\n", __func__);
            abort();
        }

        D("%p %p want %zu. ring write/read positions: %u %u", this,
          toHost, wanted, toHost->write_pos, toHost->read_pos);

        while (count < wanted) {
            bool blocking = (count == 0);
            uint32_t leftToRead = wanted - count;
            if (blocking) {
                uint32_t avail = ring_buffer_available_read(toHost, mToHostRingBufferView);
                avail = avail > leftToRead ? leftToRead : avail;
                if (avail) {
                    uint32_t* forPrinting = (uint32_t*)(dst + count);
                    ring_buffer_read_fully(toHost, mToHostRingBufferView, dst + count, avail);
                    count += avail;
                    leftToRead = wanted - count;
                } else {
                    uint32_t smallWanted = (leftToRead > 4) ? 4 : leftToRead;
                    bool exiting = false;

                    uint64_t wait = 16000;

                    if (mLastRate > 30.0f) {
                        wait = 0;
                    }

                    while (!ring_buffer_wait_read(toHost, mToHostRingBufferView, smallWanted, wait)) {
                        if (mChannel->isStopped()) {
                            emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this](bool remove) { });
                            // fprintf(stderr, "%s: %p channel stopped\n", __func__, this);
                            exiting = true;
                            break;
                        }
                        if (!g_emugl_dma_get_host_addr(*mToHostRingAddrPtr)) {
                            emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this](bool remove) { });
                            // fprintf(stderr, "%s: %p channel stopped because dma mapping invalid\n", __func__, this);
                            mChannel->stopFromHost();
                            exiting = true;
                            break;
                        }
                    }
                    if (!exiting) {
                        uint32_t avail = ring_buffer_available_read(toHost, mToHostRingBufferView);
                        avail = avail > leftToRead ? leftToRead : avail;
                        ring_buffer_read_fully(toHost, mToHostRingBufferView, dst + count,
                                               avail);
                        count += avail;
                        leftToRead = wanted - count;
                    } else {
                        break;
                    }
                }
            } else {
                uint32_t avail = ring_buffer_available_read(toHost, mToHostRingBufferView);
                if (!avail) {
                    break;
                }
                avail = avail > leftToRead ? leftToRead : avail;
                ring_buffer_read_fully(toHost, mToHostRingBufferView, dst + count,
                                       avail);
                count += avail;
                leftToRead = wanted - count;
            }

            if (count > 0 ||
                count == wanted ||
                !ring_buffer_available_read(toHost, mToHostRingBufferView)) {
                break;
            }
        }
    } else {

        D("%p using pipe", this);

        if (mLastReadUsingSharedMemory) {
            // fprintf(stderr, "%p Ended shared mem commands********************************************************************************\n", this);
            ring_buffer* toHost = *mToHostRingHandle;
            ring_buffer_consumer_hung_up(toHost);
            mLastReadUsingSharedMemory = false;
            mHostReplies.send(0);
        }

        D("wanted %d bytes", (int)wanted);
        while (count < wanted) {
            if (mReadBufferLeft > 0) {
                size_t avail = std::min<size_t>(wanted - count, mReadBufferLeft);
                memcpy(dst + count,
                       mReadBuffer.data() + (mReadBuffer.size() - mReadBufferLeft),
                       avail);
                count += avail;
                mReadBufferLeft -= avail;
                continue;
            }
            bool blocking = (count == 0);
            auto result = mChannel->readFromGuest(&mReadBuffer, blocking);
            D("readFromGuest() returned %d, size %d", (int)result, (int)mReadBuffer.size());
            if (result == IoResult::Ok) {
                mReadBufferLeft = mReadBuffer.size();
                continue;
            }
            if (count > 0) {  // There is some data to return.
                break;
            }
            // Result can only be IoResult::Error if |count| == 0
            // since |blocking| was true, it cannot be IoResult::TryAgain.
            assert(result == IoResult::Error);
            D("error while trying to read");
            return nullptr;
        }
    }

    *inout_len = count;
    mReceivedBytes += count;
    D("read %d bytes", (int)count);
    return (const unsigned char*)buf;
}

void* ChannelStream::getDmaForReading(uint64_t guest_paddr) {
    return g_emugl_dma_get_host_addr(guest_paddr);
}

void ChannelStream::unlockDma(uint64_t guest_paddr) {
    g_emugl_dma_unlock(guest_paddr);
}

void ChannelStream::forceStop() {
    mChannel->stopFromHost();
}

int ChannelStream::writeFully(const void* buf, size_t len) {
    void* dstBuf = alloc(len);
    memcpy(dstBuf, buf, len);
    flush();
    return 0;
}

const unsigned char *ChannelStream::readFully( void *buf, size_t len) {
    fprintf(stderr, "%s: FATAL: not intended for use with ChannelStream\n", __func__);
    abort();
}

void ChannelStream::enableSharedMemoryPings() {
    mChannel->registerStopCallback([this]() {
        mGuestPings.send(0);
    });

#define MAX_CONCURRENT_TRANSFERS 4
    mCurrentTransfers.resize(MAX_CONCURRENT_TRANSFERS);
    for (int i = 0; i < MAX_CONCURRENT_TRANSFERS; ++i) {
        mHostReplies.send(i);
    }

    // struct sched_param sp;
    // memset(&sp, 0, sizeof(struct sched_param));
    // sp.sched_priority=0;
    // if (pthread_setschedparam(pthread_self(), SCHED_RR, &sp)  == -1) {
        // printf("Failed to change priority.\n");
    // }

    sAllocatedChannels->set(this, true);
    emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this](bool remove) {
        uint64_t allocedChannelLock_start_us = System::get()->getHighResTimeUs();
        AutoLock lockAlloced(sAllocatedChannelLock);
        uint64_t allocedChannelLock_end_us = System::get()->getHighResTimeUs();

        auto alloced = sAllocatedChannels->get(this);
        if (!alloced || !(*alloced)) {
            // fprintf(stderr, "%s: pinged unalloced channel\n", __func__);
            return;
        }


        mGuestPings.send(0);
/*
        if (remove) {
            mGuestPings.send(-1);
            return;
        }

        int nextTransferIndex;
        uint64_t spinReceiveTime1_start_us = System::get()->getHighResTimeUs();
        mHostReplies.receive(&nextTransferIndex);

        auto& x = mCurrentTransfers[nextTransferIndex];

        x.allocedChannelLockTime = (allocedChannelLock_end_us - allocedChannelLock_start_us);
        x.latencyStartUs = spinReceiveTime1_start_us;

        ring_buffer_read_fully(*mToHostRingHandle, mToHostRingBufferView,
                               &x.numPages, sizeof(uint32_t));
        ring_buffer_read_fully(*mToHostRingHandle, mToHostRingBufferView,
                               &x.firstPageOffset,
                               sizeof(uint64_t));
        ring_buffer_read_fully(*mToHostRingHandle, mToHostRingBufferView,
                               &x.lastPageSize,
                               sizeof(uint64_t));
        x.xferLen = 0;

        for (uint32_t i = 0; i < x.numPages; ++i) {
            if (i == 0 && x.numPages == 1) {
                x.xferLen += x.lastPageSize - x.firstPageOffset;
            } else if (i == 0) {
                x.xferLen += 4096 - x.firstPageOffset;
            } else if (i == x.numPages - 1) {
                x.xferLen += x.lastPageSize;
            } else {
                x.xferLen += 4096;
            }
        }

        if (x.pages.size() < x.numPages) {
            x.pages.resize(x.numPages * 2);
        }

        for (uint32_t i = 0; i < x.numPages; ++i) {
            ring_buffer_read_fully(*mToHostRingHandle, mToHostRingBufferView,
                                   &x.pages[i],
                                   sizeof(uint64_t));
        }

        const auto& vmOps = get_emugl_vm_operations();

        if (x.xferLen > x.buffer.size()) {
            x.buffer.resize(x.xferLen * 2);
        }

        uint8_t* current_dst = x.buffer.data();

        for (uint32_t i = 0; i < x.numPages; ++i) {
            uint8_t* pageHva = vmOps.gpa2hva(x.pages[i]);
            // fprintf(stderr, "%s: %d: page 0x%llx hva %p\n", __func__, i,
            // pageHva, x.pages[i]);
            if (i == 0 && x.numPages == 1) {
                memcpy(current_dst, pageHva + x.firstPageOffset, x.xferLen);
            } else if (i == 0) {
                memcpy(current_dst, pageHva + x.firstPageOffset,
                       4096 - x.firstPageOffset);
                current_dst += 4096 - x.firstPageOffset;
            } else if (i == x.numPages - 1) {
                memcpy(current_dst, pageHva, x.lastPageSize);
            } else {
                memcpy(current_dst, pageHva, 4096);
                current_dst += 4096;
            }
        }

        mReceivedBytes += x.xferLen;

        mGuestPings.send(nextTransferIndex);
        uint64_t spinReceiveTime1_end_us = System::get()->getHighResTimeUs();
        x.spinReceiveTime1 = (spinReceiveTime1_end_us - spinReceiveTime1_start_us);
        mSpinReceiveTime += x.spinReceiveTime1;*/
    });
}

void ChannelStream::setSharedMemoryCommandInfo(
    bool* modePtr,
    uint64_t* toHostRingAddr,
    uint64_t* fromHostRingAddr,
    ring_buffer** toHostRingHandle,
    ring_buffer** fromHostRingHandle,
    ring_buffer_view* toHostRingBufferView,
    ring_buffer_view* fromHostRingBufferView) {

    mSharedMemoryCommandModePtr = modePtr; 

    mToHostRingAddrPtr = toHostRingAddr;
    mFromHostRingAddrPtr = fromHostRingAddr;
    mToHostRingHandle = toHostRingHandle;
    mFromHostRingHandle = fromHostRingHandle;

    mToHostRingBufferView = toHostRingBufferView;
    mFromHostRingBufferView = fromHostRingBufferView;
}

void ChannelStream::onSave(android::base::Stream* stream) {
    // Write only the data that's left in read buffer, but in the same format
    // as saveBuffer() does.
    stream->putBe32(mReadBufferLeft);
    stream->write(mReadBuffer.data() + mReadBuffer.size() - mReadBufferLeft,
                  mReadBufferLeft);
    android::base::saveBuffer(stream, mWriteBuffer);
}

unsigned char* ChannelStream::onLoad(android::base::Stream* stream) {
    android::base::loadBuffer(stream, &mReadBuffer);
    mReadBufferLeft = mReadBuffer.size();
    android::base::loadBuffer(stream, &mWriteBuffer);
    return reinterpret_cast<unsigned char*>(mWriteBuffer.data());
}

}  // namespace emugl
