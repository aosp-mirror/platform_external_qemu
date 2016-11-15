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

#include "OpenglRender/RenderChannel.h"

#define EMUGL_DEBUG_LEVEL  0
#include "emugl/common/debug.h"

#include <assert.h>
#include <memory.h>

namespace emugl {

using IoResult = RenderChannel::IoResult;

ChannelStream::ChannelStream(std::shared_ptr<RenderChannelImpl> channel,
                             size_t bufSize)
    : IOStream(bufSize), mChannel(channel) {
    mBuf.resize_noinit(bufSize);
}

void* ChannelStream::allocBuffer(size_t minSize) {
    if (mBuf.size() < minSize) {
        mBuf.resize_noinit(minSize);
    }
    return mBuf.data();
}

int ChannelStream::commitBuffer(size_t size) {
    assert(size <= mBuf.size());
    if (mBuf.isAllocated()) {
        mBuf.resize(size);
        mChannel->writeToGuest(std::move(mBuf));
    } else {
        mChannel->writeToGuest(
                RenderChannel::Buffer(mBuf.data(), mBuf.data() + size));
    }
    return size;
}

const unsigned char* ChannelStream::read(void* buf, size_t* inout_len) {
    size_t wanted = *inout_len;
    size_t count = 0U;
    auto dst = static_cast<uint8_t*>(buf);
    D("wanted %d bytes", (int)wanted);
    while (wanted > 0) {
        if (mReadBufferLeft > 0) {
            size_t avail = std::min<size_t>(wanted, mReadBufferLeft);
            memcpy(dst,
                   mReadBuffer.data() + (mReadBuffer.size() - mReadBufferLeft),
                   avail);
            count += avail;
            dst += avail;
            mReadBufferLeft -= avail;
            wanted -= avail;
            continue;
        }
        bool blocking = (count == 0);
        auto result = mChannel->readFromGuest(&mReadBuffer, blocking);
        D("readFromGuest() returned %d, size %d", (int)result, (int)mReadBuffer.size());
        if (result == IoResult::Ok) {
            mReadBufferLeft = mReadBuffer.size();
            continue;
        }
        if (count > 0) {
            break;
        }
        if (result == IoResult::Error) {
            D("error while trying to read");
            *inout_len = count;
            return nullptr;
        }
        mReadBufferLeft = mReadBuffer.size();
    }
    *inout_len = count;
    D("read %d bytes", (int)count);
    return (const unsigned char*)buf;
}

void ChannelStream::forceStop() {
    mChannel->stopFromHost();
}

}  // namespace emugl
