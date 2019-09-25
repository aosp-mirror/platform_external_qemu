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

#include "emugl/common/crash_reporter.h"
#include "emugl/common/debug.h"
#include "emugl/common/dma_device.h"

#include <assert.h>
#include <memory.h>

using android::base::System;

namespace emugl {

static bool getBenchmarkEnabledFromEnv() {
    auto threadEnabled =
        System::getEnvironmentVariable("ANDROID_EMUGL_RENDERTHREAD_STATS");
    if (threadEnabled == "1") return true;
    return false;
}

RingStream::RingStream(
    struct asg_context context,
    android::emulation::asg::ConsumerCallbacks callbacks,
    size_t bufsize) :
    IOStream(bufsize),
    mContext(context),
    mCallbacks(callbacks) { }
RingStream::~RingStream() = default;

int RingStream::getNeededFreeTailSize() const {
    return mContext.ring_config->flush_interval;
}

void* RingStream::allocBuffer(size_t minSize) {
    if (mWriteBuffer.size() < minSize) {
        mWriteBuffer.resize_noinit(minSize);
    }
    return mWriteBuffer.data();
}

int RingStream::commitBuffer(size_t size) {
    return static_cast<int>(
        ring_buffer_write_fully_with_abort(
            mContext.from_host_large_xfer.ring,
            &mContext.from_host_large_xfer.view,
            mWriteBuffer.data(), size,
            1, &mContext.ring_config->in_error));
}

const unsigned char* RingStream::readRaw(void* buf, size_t* inout_len) {
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<char*>(buf);
    bool shouldExit = false;

    size_t guestTx = 0;
    size_t hostTx = 0;
    uint32_t ringAvailable = 0;
    uint32_t ringLargeXferAvailable = 0;

    while (count < wanted) {

        // no read buffer left...
        if (count > 0) {  // There is some data to return.
            break;
        }

        if (shouldExit) {
            fprintf(stderr, "%s: xmits: %zu totalRecv: %zu\n", __func__, mXmits, mTotalRecv);
            return nullptr;
        }

        ringAvailable =
            ring_buffer_available_read(mContext.to_host, 0);
        ringLargeXferAvailable =
            ring_buffer_available_read(
                mContext.to_host_large_xfer.ring,
                &mContext.to_host_large_xfer.view);

        auto current = dst + count;
        auto ptrEnd = dst + wanted;

        if (ringAvailable) {
            uint32_t transferMode =
                mContext.ring_config->transfer_mode;
            switch (transferMode) {
                case 1:
                    type1Read(ringAvailable, &count, &current, ptrEnd);
                    break;
                case 2:
                    type2Read(ringAvailable, &count, &current, ptrEnd);
                    break;
                case 3:
                    emugl::emugl_crash_reporter(
                        "Guest should never set to "
                        "transfer mode 3 with ringAvailable != 0\n");
                default:
                    emugl::emugl_crash_reporter(
                        "Unknown transfer mode %u\n",
                        transferMode);
                    break;
            }
        } else if (ringLargeXferAvailable) {
            type3Read(ringLargeXferAvailable,
                      &count, &current, ptrEnd);
        } else {
            if (shouldExit) {
                fprintf(stderr, "%s: exit. xmits: %zu totalRecv: %zu\n", __func__, mXmits, mTotalRecv);
                return nullptr;
            }
            if (-1 == mCallbacks.onUnavailableRead()) {
                shouldExit = true;
            }
            continue;
        }
    }

    *inout_len = count;
    ++mXmits;
    mTotalRecv += count;
    D("read %d bytes", (int)count);
    return (const unsigned char*)buf;
}

void RingStream::type1Read(
    uint32_t available,
    size_t* count, char** current, const char* ptrEnd) {

    uint32_t xferTotal = available / sizeof(struct asg_type1_xfer);

    if (mType1Xfers.size() < xferTotal) {
        mType1Xfers.resize(xferTotal * 2);
    }

    auto xfersPtr = mType1Xfers.data();

    ring_buffer_copy_contents(
        mContext.to_host, 0, available, (uint8_t*)xfersPtr);

    struct asg_type1_xfer forRead;
    (void)forRead;

    for (uint32_t i = 0; i < xferTotal; ++i) {

        if (*current + xfersPtr[i].size > ptrEnd) return;

        const char* src = mContext.buffer + xfersPtr[i].offset;
        memcpy(*current, src, xfersPtr[i].size);

        mContext.ring_config->host_consumed_pos =
            (src) - mContext.buffer;

        ring_buffer_advance_read(
            mContext.to_host, sizeof(struct asg_type1_xfer), 1);

        *current += xfersPtr[i].size;
        *count += xfersPtr[i].size;
    }
}

void RingStream::type2Read(
    uint32_t available,
    size_t* count, char** current,const char* ptrEnd) {

    uint32_t xferTotal = available / sizeof(struct asg_type2_xfer);

    if (mType2Xfers.size() < xferTotal) {
        mType2Xfers.resize(xferTotal * 2);
    }

    auto xfersPtr = mType2Xfers.data();

    ring_buffer_copy_contents(
        mContext.to_host, 0, available, (uint8_t*)xfersPtr);

    for (uint32_t i = 0; i < xferTotal; ++i) {

        if (*current + xfersPtr[i].size > ptrEnd) return;

        const char* src =
            mCallbacks.getPtr(xfersPtr[i].physAddr);

        memcpy(*current, src, xfersPtr[i].size);

        ring_buffer_advance_read(
            mContext.to_host, sizeof(struct asg_type1_xfer), 1);

        *current += xfersPtr[i].size;
        *count += xfersPtr[i].size;
    }
}

void RingStream::type3Read(
    uint32_t available,
    size_t* count, char** current, const char* ptrEnd) {

    uint32_t xferTotal = mContext.ring_config->transfer_size;
    uint32_t maxCanRead = ptrEnd - *current;
    uint32_t actuallyRead = std::min(xferTotal, maxCanRead);

    ring_buffer_read_fully_with_abort(
        mContext.to_host_large_xfer.ring,
        &mContext.to_host_large_xfer.view,
        *current, actuallyRead,
        1, &mContext.ring_config->in_error);

    *current += actuallyRead;
    *count += actuallyRead;
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
    // TODO
    // Write only the data that's left in read buffer, but in the same format
    // as saveBuffer() does.
    // stream->putBe32(mReadBufferLeft);
    // stream->write(mReadBuffer.data() + mReadBuffer.size() - mReadBufferLeft,
    //               mReadBufferLeft);
    // android::base::saveBuffer(stream, mWriteBuffer);
}

unsigned char* RingStream::onLoad(android::base::Stream* stream) {
    // TODO
    // android::base::loadBuffer(stream, &mReadBuffer);
    // mReadBufferLeft = mReadBuffer.size();
    // android::base::loadBuffer(stream, &mWriteBuffer);
    // return reinterpret_cast<unsigned char*>(mWriteBuffer.data());
    return nullptr;
}

}  // namespace emugl
