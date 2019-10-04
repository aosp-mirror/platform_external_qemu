// Copyright (C) 2019 The Android Open Source Project
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
#include "RingStream.h"

#include "android/base/system/System.h"

#define EMUGL_DEBUG_LEVEL  0
#include "emugl/common/debug.h"
#include "emugl/common/dma_device.h"

#include <assert.h>
#include <memory.h>

#define WRITE_BUFFER_INITIAL_SIZE 4096

using android::base::System;

namespace emugl {

static RingStream::Mode getModeFromEnv() {
    return RingStream::Mode::PtrsWithDataSameThread;
    // auto mode = System::getEnvironmentVariable("ANDROID_EMUGL_RING_STREAM_MODE");
    // if (mode == "ptrswithdata") return RingStream::Mode::PtrsWithData;
    // if (mode == "ptrswithdatasamethread") return RingStream::Mode::PtrsWithDataSameThread;
    // return RingStream::Mode::PtrsWithData;
}

RingStream::RingStream(
    struct ring_buffer_with_view toHost,
    struct ring_buffer_with_view fromHost,
    RingStream::OnUnavailableReadCallback onUnavailableReadFunc,
    size_t bufsize) :
        IOStream(bufsize),
        mToHost(toHost),
        mFromHost(fromHost),
        mOnUnavailableReadFunc(onUnavailableReadFunc),
        mFromGuest(8192, mLock),
        mFromGuestReadingThread([this] { threadFunc(); }) { mFromGuestReadingThread.start(); }

RingStream::RingStream(
    struct ring_buffer_with_view toHost,
    struct ring_buffer_with_view fromHost,
    struct ring_buffer_with_view toHostData,
    RingStream::OnUnavailableReadCallback onUnavailableReadFunc,
    RingStream::GetPtrAndSizeCallback getPtrAndSizeFunc,
    size_t bufsize) :
        IOStream(bufsize),
        mToHost(toHost),
        mFromHost(fromHost),
        mToHostData(toHostData),
        mOnUnavailableReadFunc(onUnavailableReadFunc),
        mGetPtrAndSizeFunc(getPtrAndSizeFunc),
        mMode(getModeFromEnv()),
        mFromGuest(8192, mLock),
        mFromGuestReadingThread([this] { threadFunc(); }) {
            if (mMode == Mode::PtrsWithDataSameThread) {
                fprintf(stderr, "%s: starting in same thread mode********************************************************************************\n", __func__);
            } else {
                mFromGuestReadingThread.start();
            }
        }

RingStream::~RingStream() {
    mFromGuestReadingThread.wait(); 
}

void* RingStream::allocBuffer(size_t minSize) {
    if (mWriteBuffer.size() < minSize) {
        mWriteBuffer.resize_noinit(minSize);
    }
    return mWriteBuffer.data();
}

int RingStream::commitBuffer(size_t size) {
    ring_buffer_write_fully(
        mFromHost.ring, &mFromHost.view,
        mWriteBuffer.data(), size);
    return size;
}

const unsigned char* RingStream::readRaw(void* buf, size_t* inout_len) {
    if (mMode == Mode::PtrsWithData) {
        return readRaw_differentThread(buf, inout_len);
    } else {
        return readRaw_sameThread(buf, inout_len);
    }
}

const unsigned char* RingStream::readRaw_sameThread(void* buf, size_t* inout_len) {
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<uint8_t*>(buf);
    // fprintf(stderr, "wanted %d bytes\n", (int)wanted);
    bool shouldExit = false;

    size_t guestTx = 0;
    size_t hostTx = 0;
    uint32_t ringAvailable = 0;
    uint32_t ringDataAvailable = 0;
    while (count < wanted) {
        if (mReadBufferLeft > 0) {
        auto start = System::get()->getHighResTimeUs();
            size_t avail = std::min<size_t>(wanted - count, mReadBufferLeft);
            memcpy(dst + count, mReadBuffer.data() + (mReadBuffer.size() - mReadBufferLeft) , avail);
            count += avail;
            mReadBufferLeft -= avail;
        auto end = System::get()->getHighResTimeUs();
        m_bufCopyOutTimeUs += end - start;
            continue;
        }

        // no read buffer left...
        if (count > 0) {  // There is some data to return.
            break;
        }

        auto start = System::get()->getHighResTimeUs();

        auto popResult = mFromGuest.tryPopSameThread(&mReadBuffer);
        if (popResult == RenderChannel::IoResult::Ok) {
            mReadBufferLeft = mReadBuffer.size();
            continue;
        }

        auto end = System::get()->getHighResTimeUs();
        m_bufPopTimeUs += end - start;

        if (shouldExit) {
            fprintf(stderr, "%s: xmits: %zu totalRecv: %zu\n", __func__, mXmits, mTotalRecv);
            return nullptr;
        }

        ringAvailable =
            ring_buffer_available_read(mToHost.ring, &mToHost.view);
        ringDataAvailable =
            ring_buffer_available_read(mToHostData.ring, &mToHostData.view);

        if (!ringAvailable && !ringDataAvailable) {
            // There is some data to return.
            if (count > 0) {
                ++mXmits;
                // fprintf(stderr, "%s: some data to return. %zu. 0x%llx\n", __func__, count, *(unsigned long long*)buf);
                // for (uint32_t i = 0; i < count; ++i) {
                //  fprintf(stderr, "%hhx ", *((char*)(buf) + i));
                // }
                // fprintf(stderr, "\n");
                break;
            }

            if (shouldExit) {
                fprintf(stderr, "%s: xmits: %zu totalRecv: %zu\n", __func__, mXmits, mTotalRecv);
                return nullptr;
            }

            auto start = System::get()->getHighResTimeUs();
            int unavailRes = mOnUnavailableReadFunc();

            if (unavailRes == -1) { // Stop streaming
                shouldExit = true;
            }
            auto end = System::get()->getHighResTimeUs();

            if (unavailRes == 0) {
                 m_bufUnavailableTimeUs += end - start;
            }

            if (unavailRes == 1) {
                 m_bufUnavailableSleepTimeUs += end - start;
            }
            continue;
        }

        enum ConsumerState {
            NeedNotify = 0,
            CanConsume = 1,
            CanConsumeButWillHangup = 2,
        };

        *(ConsumerState*)(&mToHost.ring->state) = ConsumerState::CanConsume;

        ringAvailable =
            ring_buffer_available_read(mToHost.ring, &mToHost.view);
        ringDataAvailable =
            ring_buffer_available_read(mToHostData.ring, &mToHostData.view);

        bool wasRingAvailable = ringAvailable != 0;

        while (ringAvailable && mFromGuest.canPushLocked()) {
            auto start = System::get()->getHighResTimeUs();
            if (mMode != Mode::PtrsWithDataSameThread) {
                fprintf(stderr, "%s: wrong mode\n", __func__);
                abort();
            }

            uint32_t xferCount = ringAvailable / 8;

            if (mOutstandingXfers.size() < xferCount) {
                mOutstandingXfers.resize(ringAvailable);
                mOutstandingXfersDecoded.resize(ringAvailable);
            }

            auto xfersPtr = mOutstandingXfers.data();
            auto decodedPtr = mOutstandingXfersDecoded.data();

            ring_buffer_copy_contents(
                mToHost.ring, &mToHost.view,
                ringAvailable,
                (uint8_t*)xfersPtr);

            size_t totalNeeded = 0;
            for (size_t i = 0; i < xferCount; ++i) {
                mGetPtrAndSizeFunc(
                    xfersPtr[i],
                    &((decodedPtr + i)->ptr),
                    &((decodedPtr + i)->size));
                totalNeeded += (decodedPtr + i)->size;
            }

            RenderChannel::Buffer buffer;
            buffer.resize_noinit(totalNeeded);
            size_t byteOff = 0;
            for (size_t i = 0; i < xferCount; ++i) {
                auto ptr = (decodedPtr + i)->ptr;
                auto size = (decodedPtr + i)->size;
                memcpy(buffer.data() + byteOff, ptr, size);
                byteOff += size;
            }
            ring_buffer_view_read(
                    mToHost.ring, &mToHost.view,
                    mOutstandingXfers.data(),
                    ringAvailable, 1);
            auto res = mFromGuest.tryPushSameThread(std::move(buffer));
            if (res != RenderChannel::IoResult::Ok) {
                fprintf(stderr, "%s: failed to push\n", __func__);
                abort();
            } else {
            }
            ringAvailable =
                ring_buffer_available_read(mToHost.ring, &mToHost.view);

            int spinCount = 5;
            while (!ringAvailable && spinCount > 0) {
                --spinCount;
                ringAvailable =
                    ring_buffer_available_read(mToHost.ring, &mToHost.view);
            }
            auto end = System::get()->getHighResTimeUs();
            m_bufPushTimeUs += end - start;
        }

        if (ringDataAvailable && mFromGuest.canPushLocked()) {
            auto start = System::get()->getHighResTimeUs();
            uint32_t wantToReadThisTime = mToHostData.ring->state;
            RenderChannel::Buffer buffer;
            buffer.resize_noinit(wantToReadThisTime);
            ring_buffer_read_fully(
                mToHostData.ring, &mToHostData.view,
                buffer.data(),
                wantToReadThisTime);
            auto res = mFromGuest.tryPushSameThread(std::move(buffer));
            if (res != RenderChannel::IoResult::Ok) {
                fprintf(stderr, "%s: failed to push\n", __func__);
                abort();
            }
            auto end = System::get()->getHighResTimeUs();
            m_bufPushTimeUs += end - start;
        }
    }

    *inout_len = count;
    ++mXmits;
    mTotalRecv += count;
    D("read %d bytes", (int)count);
    return (const unsigned char*)buf;
}

const unsigned char* RingStream::readRaw_differentThread(void* buf, size_t* inout_len) {
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<uint8_t*>(buf);
    D("wanted %d bytes", (int)wanted);
    while (count < wanted) {
        if (mReadBufferLeft > 0) {
            auto start = System::get()->getHighResTimeUs();
            size_t avail = std::min<size_t>(wanted - count, mReadBufferLeft);
            memcpy(dst + count,
                   mReadBuffer.data() + (mReadBuffer.size() - mReadBufferLeft),
                   avail);
            count += avail;
            mReadBufferLeft -= avail;
            auto end = System::get()->getHighResTimeUs();
            m_bufCopyOutTimeUs += end - start;
            continue;
        }
            bool blocking = (count == 0);
            auto start = System::get()->getHighResTimeUs();
        {
            android::base::AutoLock lock(mLock);
            if (blocking) {
                auto result = mFromGuest.popLocked(&mReadBuffer);
                if (result == RenderChannel::IoResult::Ok) {
                    mReadBufferLeft = mReadBuffer.size();
                    continue;
                } else {
                }
            } else {
                auto result = mFromGuest.tryPopLocked(&mReadBuffer);
                if (result == RenderChannel::IoResult::Ok) {
                    mReadBufferLeft = mReadBuffer.size();
                    continue;
                } else {
                }
            }
        }

        auto end = System::get()->getHighResTimeUs();
        m_bufPopTimeUs += end - start;

        if (count > 0) {  // There is some data to return.
            break;
        }
        // Result can only be IoResult::Error if |count| == 0
        // since |blocking| was true, it cannot be IoResult::TryAgain.
        assert(result == IoResult::Error);
        D("error while trying to read");
        return nullptr;
    }
    *inout_len = count;
    D("read %d bytes", (int)count);
    return (const unsigned char*)buf;
}

void* RingStream::getDmaForReading(uint64_t guest_paddr) {
    return g_emugl_dma_get_host_addr(guest_paddr);
}

void RingStream::unlockDma(uint64_t guest_paddr) {
    g_emugl_dma_unlock(guest_paddr);
}

int RingStream::writeFully(const void* buf, size_t len) {
    void* dstBuf = alloc(len);
    memcpy(dstBuf, buf, len);
    flush();
    return 0;
}

const unsigned char *RingStream::readFully( void *buf, size_t len) {
    fprintf(stderr, "%s: FATAL: not intended for use with RingStream\n", __func__);
    abort();
}

void RingStream::onSave(android::base::Stream* stream) {
    // Write only the data that's left in read buffer, but in the same format
    // as saveBuffer() does.
    stream->putBe32(mReadBufferLeft);
    stream->write(mReadBuffer.data() + mReadBuffer.size() - mReadBufferLeft,
                  mReadBufferLeft);
    android::base::saveBuffer(stream, mWriteBuffer);
}

unsigned char* RingStream::onLoad(android::base::Stream* stream) {
    android::base::loadBuffer(stream, &mReadBuffer);
    mReadBufferLeft = mReadBuffer.size();
    android::base::loadBuffer(stream, &mWriteBuffer);
    return reinterpret_cast<unsigned char*>(mWriteBuffer.data());
}

void RingStream::threadFunc() {
    fprintf(stderr, "%s: start\n", __func__);
    bool shouldExit = false;
    uint32_t readbackWaitCount = 0;
    uint32_t writeWaitCount = 0;
    const uint32_t writeWaitCountMax = 30;
    const uint32_t readbackWaitCountMax = 500;

    while (1) {
        uint32_t ringAvailable =
            ring_buffer_available_read(mToHost.ring, &mToHost.view);

        uint32_t ringDataAvailable = 0;

        if (mMode == Mode::PtrsWithData) {
            ringDataAvailable = ring_buffer_available_read(mToHostData.ring, &mToHostData.view);
        }

        if (!ringAvailable && !ringDataAvailable) {
            if (shouldExit) {
                android::base::AutoLock lock(mLock);
                fprintf(stderr, "%s: xmits: %zu totalRecv: %zu\n", __func__, mXmits, mTotalRecv);
                mFromGuest.closeLocked();
                return;
            }
           
            if (1 == mFromHost.ring->state &&
                (readbackWaitCount < readbackWaitCountMax)) {
                ring_buffer_yield();
                ++readbackWaitCount;
                continue;
            }

            // Heuristic: spin if the render thread is processing stuff still
            // android::base::AutoLock lock(mLock);
            if (mFromGuest.canPopLocked() || mRenderingActive) {
                ring_buffer_yield();
                continue;
            } else if (writeWaitCount < writeWaitCountMax) {
                ++writeWaitCount;
                ring_buffer_yield();
                continue;
            }

            int unavailRes = mOnUnavailableReadFunc();
            writeWaitCount = 0;
            readbackWaitCount = 0;

            if (unavailRes == -1) { // Stop streaming
                shouldExit = true;
            }
            continue;
        }

        enum ConsumerState {
            NeedNotify = 0,
            CanConsume = 1,
            CanConsumeButWillHangup = 2,
        };

        *(ConsumerState*)(&mToHost.ring->state) = ConsumerState::CanConsume;

        if (ringAvailable) {
            if (mMode == Mode::PtrsWithData) {
                std::vector<uint64_t> xfersOutstanding(ringAvailable / 8);
                std::vector<char*> ptrs(xfersOutstanding.size());
                std::vector<size_t> sizes(xfersOutstanding.size());
                ring_buffer_copy_contents(
                    mToHost.ring, &mToHost.view,
                    xfersOutstanding.size() * 8,
                    (uint8_t*)(xfersOutstanding.data()));
                size_t totalNeeded = 0;
                for (size_t i = 0; i < xfersOutstanding.size(); ++i) {
                    mGetPtrAndSizeFunc(xfersOutstanding[i], ptrs.data() + i, sizes.data() + i);
                    totalNeeded += sizes[i];
                }
                RenderChannel::Buffer buffer;
                buffer.resize_noinit(totalNeeded);
                size_t byteOff = 0;
                for (size_t i = 0; i < xfersOutstanding.size(); ++i) {
                    memcpy(buffer.data() + byteOff, ptrs[i], sizes[i]);
                    ring_buffer_view_read(
                            mToHost.ring, &mToHost.view,
                            xfersOutstanding.data(), 8, 1);
                    byteOff += sizes[i];
                }
                auto start = System::get()->getHighResTimeUs();
                android::base::AutoLock lock(mLock);
                mFromGuest.pushLocked(std::move(buffer));
                lock.unlock();
                auto end = System::get()->getHighResTimeUs();
                m_bufPushTimeUs += end - start;
                continue;
            } else {
                fprintf(stderr, "%s: unsupp\n", __func__);
                abort();
                // uint32_t wantToReadThisTime = todo < ringAvailable ? todo : ringAvailable;
                uint32_t wantToReadThisTime = ringAvailable;
                mReadBuffer.resize_noinit(wantToReadThisTime);
                ring_buffer_read_fully(
                    mToHost.ring, &mToHost.view, mReadBuffer.data(), wantToReadThisTime);
                mReadBufferLeft = wantToReadThisTime;
                continue;
            }
        }

        if (ringDataAvailable) {
            uint32_t wantToReadThisTime = mToHostData.ring->state;
            RenderChannel::Buffer buffer;
            buffer.resize_noinit(wantToReadThisTime);
            ring_buffer_read_fully(
                mToHostData.ring, &mToHostData.view,
                buffer.data(),
                wantToReadThisTime);
            auto start = System::get()->getHighResTimeUs();
            android::base::AutoLock lock(mLock);
            mFromGuest.pushLocked(std::move(buffer));
            lock.unlock();
                auto end = System::get()->getHighResTimeUs();
                m_bufPushTimeUs += end - start;
            continue;
        }
    }
}

void RingStream::printStats() {
    printf("RingStream::%s: push time %f ms pop time %f ms copyout %f ms unavail %f ms sleep %f ms\n", __func__,
            (float)m_bufPushTimeUs / 1000.0f,
            (float)m_bufPopTimeUs / 1000.0f,
            (float)m_bufCopyOutTimeUs / 1000.0f,
            (float)m_bufUnavailableTimeUs / 1000.0f,
            (float)m_bufUnavailableSleepTimeUs / 1000.0f);
    m_bufPushTimeUs = 0;
    m_bufPopTimeUs = 0;
    m_bufCopyOutTimeUs = 0;
    m_bufUnavailableTimeUs = 0;
    m_bufUnavailableSleepTimeUs = 0;
}
}  // namespace emugl
