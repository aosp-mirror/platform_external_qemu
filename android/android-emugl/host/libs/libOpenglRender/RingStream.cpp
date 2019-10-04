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
        mOnUnavailableReadFunc(onUnavailableReadFunc) { }

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
    // fprintf(stderr, "wanted %d bytes\n", (int)wanted);
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

        // There is some data to return.
        if (count > 0) {
            // fprintf(stderr, "%s: some data to return. %zu. 0x%llx\n", __func__, count, *(unsigned long long*)buf);
            // for (uint32_t i = 0; i < count; ++i) {
            //     fprintf(stderr, "0x%x ", *((char*)(buf) + i));
            // }
            // fprintf(stderr, "\n");
            break;
        }

        uint32_t ringAvailable =
            ring_buffer_available_read(mToHost.ring, &mToHost.view);

        if (!ringAvailable) {
            int unavailRes = mOnUnavailableReadFunc();
            if (unavailRes == -1) { // Stop streaming
                return nullptr;
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

        mReadBufferLeft = wantToReadThisTime;
        // fprintf(stderr, "%s: ring delivery\n", __func__);
        for (uint32_t i = 0; i < mReadBuffer.size(); ++i) {
            // fprintf(stderr, "0x%x ", *((char*)(mReadBuffer.data()) + i));
        }
        // fprintf(stderr, "\n");
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

}  // namespace emugl
