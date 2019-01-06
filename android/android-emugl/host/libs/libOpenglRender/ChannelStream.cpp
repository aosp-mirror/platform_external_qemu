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

#include "android/base/ring_buffer.h"
#include "android/emulation/FaultHandler.h"

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
using android::base::Lock;

namespace emugl {

using IoResult = RenderChannel::IoResult;

ChannelStream::ChannelStream(RenderChannelImpl* channel, size_t bufSize)
    : IOStream(bufSize), mChannel(channel) {
    mWriteBuffer.resize_noinit(bufSize);
    android::FaultHandler* faultHandler =
        (android::FaultHandler*)get_emugl_vm_operations().getFaultHandler();
    mSupportsFaultHandling =
            faultHandler && faultHandler->supportsFaultHandling();
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
    return size;
}

const unsigned char* ChannelStream::readRaw(void* buf, size_t* inout_len) {
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<uint8_t*>(buf);

    if (*mSharedMemoryCommandModePtr) {
        if (!mLastReadUsingSharedMemory) {
            if (mSupportsFaultHandling) {
                android::FaultHandler* faultHandler =
                        (android::FaultHandler*)get_emugl_vm_operations()
                                .getFaultHandler();
                mFaultHandler = faultHandler->addFaultHandler(
                        *mToHostRingAddrPtr, 4096, [this]() {
                            AutoLock lock(mGuestWaitingLock);
                            mGuestTouchedRing = true;
                            mGuestWaitingCv.signalAndUnlock(&mGuestWaitingLock);
                        });
            }
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
                    bool exiting = false;
                    bool tryAgain = false;
                    // while (!ring_buffer_view_can_read(toHost, mToHostRingBufferView, 4)) {
                    while (!ring_buffer_wait_read(toHost, mToHostRingBufferView, 1, 4000)) {
                        if (mSupportsFaultHandling) {
                            android::FaultHandler* faultHandler =
                                    (android::FaultHandler*)get_emugl_vm_operations()
                                            .getFaultHandler();
                            AutoLock lock(mGuestWaitingLock);
                            faultHandler->setupFault(mFaultHandler);

                            uint32_t avail = ring_buffer_available_read(toHost, mToHostRingBufferView);
                            if (avail) {
                                avail = avail > leftToRead ? leftToRead : avail;
                                ring_buffer_read_fully(toHost, mToHostRingBufferView, dst + count, avail);
                                count += avail;
                                leftToRead = wanted - count;
                                faultHandler->teardownFault(mFaultHandler);
                                break;
                            }

                            mGuestTouchedRing = false;
                            while (!mGuestTouchedRing) {
                                mGuestWaitingCv.wait(&mGuestWaitingLock);
                            }

                            tryAgain = true;
                        }

                        if (mChannel->isStopped()) {
                            fprintf(stderr, "%s: %p channel stopped\n", __func__, this);
                            android::FaultHandler* faultHandler =
                                    (android::FaultHandler*)get_emugl_vm_operations()
                                            .getFaultHandler();
                            faultHandler->removeFaultHandler(mFaultHandler);
                            exiting = true;
                            break;
                        }

                        if (!g_emugl_dma_get_host_addr(*mToHostRingAddrPtr)) {
                            fprintf(stderr, "%s: %p channel stopped because dma mapping invalid\n", __func__, this);
                            android::FaultHandler* faultHandler =
                                    (android::FaultHandler*)get_emugl_vm_operations()
                                            .getFaultHandler();
                            faultHandler->removeFaultHandler(mFaultHandler);
                            mChannel->stopFromHost();
                            exiting = true;
                            break;
                        }
                    }

                    if (exiting) {
                        break;
                    } else {
                        uint32_t avail = ring_buffer_available_read(toHost, mToHostRingBufferView);
                        avail = avail > leftToRead ? leftToRead : avail;
                        if (avail) {
                            ring_buffer_read_fully(toHost, mToHostRingBufferView, dst + count,
                                                   avail);
                            count += avail;
                            leftToRead = wanted - count;
                        }
                    }
                }
            }

            if (count > 0) {
                break;
            }
        }
    } else {

        D("%p using pipe", this);

        if (mLastReadUsingSharedMemory) {
            ring_buffer* toHost = *mToHostRingHandle;
            ring_buffer_consumer_hung_up(toHost);
            android::FaultHandler* faultHandler =
                    (android::FaultHandler*)get_emugl_vm_operations()
                            .getFaultHandler();
            faultHandler->removeFaultHandler(mFaultHandler);
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
