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

ChannelStream::ChannelStream(std::shared_ptr<emugl::RenderingChannel> channel,
                             size_t bufSize)
    : IOStream(bufSize), mChannel(channel), mBuf(bufSize) {}

void* ChannelStream::allocBuffer(size_t minSize) {
    if (mBuf.size() < minSize) {
        mBuf.resize(minSize);
    }
    return mBuf.data();
}

int ChannelStream::commitBuffer(size_t size) {
    return writeFully(mBuf.data(), size);
}

const unsigned char* ChannelStream::readFully(void* buf, size_t len) {
    size_t size = mChannel->readFromGuest((char*)buf, len);
    if (size < len) {
        return nullptr;
    }
    return (const unsigned char*)buf;
}

const unsigned char* ChannelStream::read(void* buf, size_t* inout_len) {
    size_t size = mChannel->readFromGuest((char*)buf, *inout_len);
    if (size == 0) {
        return nullptr;
    }
    *inout_len = (int)size;
    return (const unsigned char*)buf;
}

int ChannelStream::writeFully(const void* buf, size_t len) {
    return mChannel->writeToGuest((const char*)buf, len) ? len : -1;
}

void ChannelStream::forceStop() {
    mChannel->stop();
}
