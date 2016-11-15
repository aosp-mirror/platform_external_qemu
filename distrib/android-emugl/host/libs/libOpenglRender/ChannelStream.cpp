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

#include <assert.h>

namespace emugl {

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
                ChannelBuffer(mBuf.data(), mBuf.data() + size));
    }
    return size;
}

const unsigned char* ChannelStream::read(void* buf, size_t* inout_len) {
    size_t size = mChannel->readFromGuest((char*)buf, *inout_len, true);
    if (size == 0) {
        return nullptr;
    }
    *inout_len = size;
    return (const unsigned char*)buf;
}

void ChannelStream::forceStop() {
    mChannel->stop();
}

}  // namespace emugl
