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

#define EMUGL_DEBUG_LEVEL  0
#include "emugl/common/debug.h"
#include "emugl/common/dma_device.h"

#include <assert.h>
#include <memory.h>

#define WRITE_BUFFER_INITIAL_SIZE 4096

namespace emugl {

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
        mMode(Mode::PtrsWithData),
        mFromGuest(8192, mLock),
        mFromGuestReadingThread([this] { threadFunc(); }) { mFromGuestReadingThread.start(); }

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
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<uint8_t*>(buf);
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

const unsigned char* RingStream::readRaw_samethread(void* buf, size_t* inout_len) {
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<uint8_t*>(buf);
    // fprintf(stderr, "wanted %d bytes\n", (int)wanted);
    bool shouldExit = false;

    size_t guestTx = 0;
    size_t hostTx = 0;
    while (count < wanted) {
        if (mReadBufferLeft > 0) {
            // fprintf(stderr, "%s: copy out @ dst off %zu rb off %zu\n", __func__,
            //         count,
            //         (mReadBuffer.size() - mReadBufferLeft));
            size_t avail = std::min<size_t>(wanted - count, mReadBufferLeft);
            memcpy(dst + count, mReadBuffer.data() + (mReadBuffer.size() - mReadBufferLeft) , avail);
            count += avail;
            mReadBufferLeft -= avail;

            // if (count == wanted) {
            //     fprintf(stderr, "%s: done\n", __func__);
            // }
            continue;
        }

        uint32_t ringAvailable =
            ring_buffer_available_read(mToHost.ring, &mToHost.view);

        uint32_t ringDataAvailable = 0;

        if (mMode == Mode::PtrsWithData) {
            ringDataAvailable = ring_buffer_available_read(mToHostData.ring, &mToHostData.view);
        }

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

            int unavailRes = mOnUnavailableReadFunc();

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
            uint32_t todo = wanted - count;
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
                mReadBuffer.resize_noinit(totalNeeded);
                size_t byteOff = 0;
                for (size_t i = 0; i < xfersOutstanding.size(); ++i) {
                    memcpy(mReadBuffer.data() + byteOff, ptrs[i], sizes[i]);
                    // fprintf(stderr, "%s: one from %p:\n", __func__, ptrs[i]);
                    // for (size_t j = 0; j < sizes[i]; ++j) {
                        // fprintf(stderr, "%hhx ", ptrs[i][j]);
                    // }
                    // fprintf(stderr, "\n");
                    byteOff += sizes[i];
                }
                mReadBufferLeft = mReadBuffer.size();
                ring_buffer_read_fully(
                    mToHost.ring, &mToHost.view,
                    xfersOutstanding.data(),
                    xfersOutstanding.size() * 8);
            // j// jif (todo < mReadBufferLeft) {
                // jfprintf(stderr, "%s: exceed. todo: %u left: %zu\n", __func__, todo, mReadBufferLeft);
            // j}

        // fprintf(stderr, "%s: ring delivery %zu\n", __func__, mReadBuffer.size());
        // for (uint32_t i = 0; i < mReadBuffer.size(); ++i) {
        //  fprintf(stderr, "%hhx ", *((char*)(mReadBuffer.data()) + i));
        // }
        // fprintf(stderr, "\n");
                continue;
            } else {
                uint32_t todo = wanted - count;
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
            // fprintf(stderr, "%s: ring data avail\n", __func__);
            uint32_t wantToReadThisTime = ringDataAvailable;
            mReadBuffer.resize_noinit(wantToReadThisTime);
            ring_buffer_read_fully(
                mToHostData.ring, &mToHostData.view,
                mReadBuffer.data(),
                wantToReadThisTime);
            mReadBufferLeft = mReadBuffer.size();
            continue;
        }

    }

    *inout_len = count;
    ++mXmits;
    mTotalRecv += count;
    // fprintf(stderr, "read %d bytes\n", (int)count);
    D("read %d bytes", (int)count);
       //          fprintf(stderr, "%s: some data to return. %zu. 0x%llx\n", __func__, count, *(unsigned long long*)buf);
       //          for (uint32_t i = 0; i < count; ++i) {
       //           fprintf(stderr, "%hhx ", *((char*)(buf) + i));
       //          }
       //          fprintf(stderr, "\n");
    return (const unsigned char*)buf;
}

const unsigned char* RingStream::readRaw_prev(void* buf, size_t* inout_len) {
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<uint8_t*>(buf);
    // fprintf(stderr, "wanted %d bytes\n", (int)wanted);
    bool shouldExit = false;

    size_t guestTx = 0;
    size_t hostTx = 0;
    while (count < wanted) {
        if (mReadBufferLeft > 0) {
            // fprintf(stderr, "%s: copy out @ dst off %zu rb off %zu\n", __func__,
            //         count,
            //         (mReadBuffer.size() - mReadBufferLeft));
            size_t avail = std::min<size_t>(wanted - count, mReadBufferLeft);
            memcpy(dst + count,
                   mReadBuffer.data() + (mReadBuffer.size() - mReadBufferLeft),
                   avail);
            count += avail;
            mReadBufferLeft -= avail;
            continue;
        }

        uint32_t ringAvailable =
            ring_buffer_available_read(mToHost.ring, &mToHost.view);

        // if (mToHost.ring->seqno != mToHost.ring->seqnohost) {
        //     while (!ringAvailable) {
        //         ringAvailable = ring_buffer_available_read(mToHost.ring, &mToHost.view);
        //         sched_yield();
        //     }
        // }

        // uint32_t spins = 100;

        // while (!ringAvailable && spins > 0) {
        //  ringAvailable =
        //     ring_buffer_available_read(mToHost.ring, &mToHost.view);
        //  sched_yield();
        //     --spins;
        // }

            // There is some data to return.
        // if (count > 65536) {
        //     ++mXmits;
        //     // fprintf(stderr, "%s: some data to return. %zu. 0x%llx\n", __func__, count, *(unsigned long long*)buf);
        //     // for (uint32_t i = 0; i < count; ++i) {
        //     //     fprintf(stderr, "0x%x ", *((char*)(buf) + i));
        //     // }
        //     // fprintf(stderr, "\n");
        //     break;
        // }

        // Spin a little bit more
        if (!ringAvailable) {
            // There is some data to return.
            if (count > 0) {
                ++mXmits;
                // fprintf(stderr, "%s: some data to return. %zu. 0x%llx\n", __func__, count, *(unsigned long long*)buf);
                // for (uint32_t i = 0; i < count; ++i) {
                //     fprintf(stderr, "0x%x ", *((char*)(buf) + i));
                // }
                // fprintf(stderr, "\n");
                break;
            }

            if (shouldExit) {
                fprintf(stderr, "%s: xmits: %zu totalRecv: %zu\n", __func__, mXmits, mTotalRecv);
                return nullptr;
            }

            int unavailRes = mOnUnavailableReadFunc();

            if (unavailRes == -1) { // Stop streaming
                shouldExit = true;
            }
            continue;
        }

        uint32_t todo = wanted - count;
        uint32_t wantToReadThisTime = todo < ringAvailable ? todo : ringAvailable;

        // fprintf(stderr, "%s: want this time: %u\n", __func__, wantToReadThisTime);

        enum ConsumerState {
            NeedNotify = 0,
            CanConsume = 1,
            CanConsumeButWillHangup = 2,
        };

        *(ConsumerState*)(&mToHost.ring->state) = ConsumerState::CanConsume;

        mReadBuffer.resize_noinit(wantToReadThisTime);

        ring_buffer_read_fully(
            mToHost.ring, &mToHost.view, mReadBuffer.data(), wantToReadThisTime);

        // mToHost.ring->txsizehost += wantToReadThisTime;
        // if (mToHost.ring->txsizehost == mToHost.ring->txsize) {
        //     // fprintf(stderr, "%s: got entire transaction of %u bytes\n", __func__,
        //     //         mToHost.ring->txsize);
        //     mToHost.ring->txsizehost = 0;
        //     ++mToHost.ring->seqnohost;
        // }

        mReadBufferLeft = wantToReadThisTime;
        // fprintf(stderr, "%s: ring delivery\n", __func__);
        // for (uint32_t i = 0; i < mReadBuffer.size(); ++i) {
            // fprintf(stderr, "0x%x ", *((char*)(mReadBuffer.data()) + i));
        // }
        // fprintf(stderr, "\n");
    }

    *inout_len = count;
    ++mXmits;
    mTotalRecv += count;
    // fprintf(stderr, "read %d bytes\n", (int)count);
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
           
            // Heuristic: spin if the render thread is processing stuff still
            // android::base::AutoLock lock(mLock);
            if (mFromGuest.canPopLocked()) {
                ring_buffer_yield();
                continue;
            }

            if (1 == mFromHost.ring->state &&
                (readbackWaitCount < readbackWaitCountMax)) {
                ring_buffer_yield();
                ++readbackWaitCount;
                continue;
            }

            int unavailRes = mOnUnavailableReadFunc();

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
                    byteOff += sizes[i];
                }
                ring_buffer_read_fully(
                    mToHost.ring, &mToHost.view,
                    xfersOutstanding.data(),
                    xfersOutstanding.size() * 8);

                android::base::AutoLock lock(mLock);
                mFromGuest.pushLocked(std::move(buffer));
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
            uint32_t wantToReadThisTime = ringDataAvailable;
            RenderChannel::Buffer buffer;
            buffer.resize_noinit(wantToReadThisTime);
            ring_buffer_read_fully(
                mToHostData.ring, &mToHostData.view,
                buffer.data(),
                wantToReadThisTime);
            android::base::AutoLock lock(mLock);
            mFromGuest.pushLocked(std::move(buffer));
            continue;
        }
    }
}

}  // namespace emugl
