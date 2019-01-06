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

#include "OpenglRender/RenderChannel.h"

#define EMUGL_DEBUG_LEVEL  0
#include "emugl/common/debug.h"
#include "emugl/common/dma_device.h"

#include <assert.h>
#include <memory.h>

namespace emugl {

using IoResult = RenderChannel::IoResult;

ChannelStream::ChannelStream(RenderChannelImpl* channel, size_t bufSize)
    : IOStream(bufSize), mChannel(channel) {
    mWriteBuffer.resize_noinit(bufSize);
}

void* ChannelStream::allocBuffer(size_t minSize) {
    if (mWriteBuffer.size() < minSize) {
        mWriteBuffer.resize_noinit(minSize);
    }
    return mWriteBuffer.data();
}

int ChannelStream::commitBuffer(size_t size) {
    fprintf(stderr, "%s: committing %zu bytes form host\n", __func__, size);
    assert(size <= mWriteBuffer.size());
    if (*mSharedMemoryCommandModePtr) {
    fprintf(stderr, "%s: with ring\n", __func__);
        ring_buffer* fromHost = *mFromHostRingHandle;
        if (!fromHost) {
            fprintf(stderr, "%s: FATAL: did not init ring buffer!\n", __func__);
            abort();
        }
        // if (mWriteBuffer.isAllocated()) {
            // ring_buffer_write_fully(fromHost, nullptr, mWriteBuffer.data(), size);
        // } else {
            ring_buffer_write_fully(fromHost, nullptr, mWriteBuffer.data(), size);
        // }
    } else {
    fprintf(stderr, "%s: with pipe\n", __func__);
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
        fprintf(stderr, "%s: should be using shared mem to read this\n", __func__);
        ring_buffer* toHost = *mToHostRingHandle;
        if (!toHost) {
            fprintf(stderr, "%s: FATAL: did not init ring buffer!\n", __func__);
            abort();
        }

        fprintf(stderr, "%s: %p want %zu ring write %u read %u\n", __func__,
        toHost, 
        wanted, toHost->write_pos, toHost->read_pos);

        while (count < wanted) {
            uint32_t avail = ring_buffer_available_read(toHost, nullptr);
            if (!avail) {
                fprintf(stderr, "%s: none available\n", __func__);
                uint32_t smallWanted = 8;
                if (ring_buffer_wait_read(toHost, nullptr, smallWanted, -1)) {
                    ring_buffer_read_fully(toHost, nullptr, dst + count, smallWanted);
                    fprintf(stderr, "%s: got 8 bytes\n", __func__);
                    count += 8;
                } else {
                    fprintf(stderr, "%s: fell through\n", __func__);
                //     // Hang up, reading whatever got in the middle
                //     if (!ring_buffer_consumer_hangup(toHost)) {
                //         ring_buffer_consumer_wait_producer_idle(toHost);
                //         while (ring_buffer_can_read(toHost, 1)) {
                //             ring_buffer_read_fully(
                //                 toHost, nullptr, dst + count, 1);
                //             ++count;
                //         }
                //     }
                //     ring_buffer_consumer_hung_up(toHost);
                //     fprintf(stderr, "%s: hang up with %zu bytes\n", __func__, count);
                }
            } else {
                ring_buffer_read_fully(toHost, nullptr, dst + count, avail);
                count += avail;
                fprintf(stderr, "%s: read with %zu bytes\n", __func__, avail);
            }

            if (count > 0) {
                break;
            }
        }
    } else {
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
    ring_buffer** toHostRingHandle,
    ring_buffer** fromHostRingHandle) {

    mSharedMemoryCommandModePtr = modePtr; 
    mToHostRingHandle = toHostRingHandle;
    mFromHostRingHandle = fromHostRingHandle;
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
