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

#include "OpenglRender/RenderChannel.h"

#define EMUGL_DEBUG_LEVEL  0
#include "emugl/common/debug.h"
#include "emugl/common/dma_device.h"

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
    if (mReceivedBytes < 1000000ULL * 50ULL) return false;

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
        fprintf(stderr, "%s: since last: l/y/s %lu %lu %lu. rate last: %f mb/s. spin receive time: %f s (implied live rate: %f mb/s) commit time: %f s\n", __func__,
    toHost->read_live_count - mLastLive,
    toHost->read_yield_count - mLastYield,
    toHost->read_sleep_us_count - mLastSleep,
    (float)bytesSentThisTime / 1048576.0f / intervalSec,

    mSpinReceiveTime / 1000000.0f,
    (float)bytesSentThisTime / mSpinReceiveTime,

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

    int message = 0;
    mGuestPings.receive(&message);
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

const unsigned char* ChannelStream::readRaw(void* buf, size_t* inout_len) {
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<uint8_t*>(buf);

    if (*mSharedMemoryCommandModePtr) {
        if (!mLastReadUsingSharedMemory) {
            emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this]() {
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
                            emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this]() { });
                            // fprintf(stderr, "%s: %p channel stopped\n", __func__, this);
                            exiting = true;
                            break;
                        }
                        if (!g_emugl_dma_get_host_addr(*mToHostRingAddrPtr)) {
                            emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this]() { });
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
            fprintf(stderr, "%p Ended shared mem commands********************************************************************************\n", this);
            ring_buffer* toHost = *mToHostRingHandle;
            ring_buffer_consumer_hung_up(toHost);
            mLastReadUsingSharedMemory = false;
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
    // fprintf(stderr, "%s: FATAL: not intended for use with ChannelStream\n", __func__);
    abort();
}

void ChannelStream::enableSharedMemoryPings() {
    sAllocatedChannels->set(this, true);
    emugl::g_emugl_dma_register_ping_callback(*mToHostRingAddrPtr, [this]() {
        AutoLock lockAlloced(sAllocatedChannelLock);
        auto alloced = sAllocatedChannels->get(this);
        if (!alloced || !(*alloced)) {
            // fprintf(stderr, "%s: pinged unalloced channel\n", __func__);
            return;
        }

        int message = 0;
        mGuestPings.send(message);
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
