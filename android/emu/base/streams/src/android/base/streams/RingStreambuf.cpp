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
#include "aemu/base/streams/RingStreambuf.h"

#include <string.h>   // for memcpy
#include <algorithm>  // for max, min

namespace android {
namespace base {
namespace streams {

// See https://jameshfisher.com/2018/03/30/round-up-power-2 for details
static uint64_t next_pow2(uint64_t x) {
    return x == 1 ? 1 : 1 << (64 - __builtin_clzl(x - 1));
}
RingStreambuf::RingStreambuf(uint32_t capacity, milliseconds timeout)
    : mTimeout(timeout) {
    uint64_t cap = next_pow2(capacity + 1);
    mRingbuffer.resize(cap);
}

void RingStreambuf::close() {
    {
        std::unique_lock<std::mutex> lock(mLock);
        mTimeout = std::chrono::milliseconds(0);
        mClosed = true;
    }
    mCanRead.notify_all();
}

std::streamsize RingStreambuf::xsputn(const char* s, std::streamsize n) {
    // Usually n >> 1..
    mLock.lock();
    std::streamsize capacity = mRingbuffer.capacity();
    std::streamsize maxWrite = std::max(n, capacity);

    if (mClosed) {
        mLock.unlock();
        return 0;
    }

    // Case 1: It doesn't fit in the ringbuffer
    if (n >= capacity) {
        // We are overwriting everything, so let's just reset it all.
        memcpy(mRingbuffer.data(), s + n - capacity, capacity);
        mHead = capacity;
        mTail = 0;
        mHeadOffset += n;
        mLock.unlock();
        mCanRead.notify_all();
        return n;
    }

    // Case 2, it fits in the ringbuffer.
    // Case 2a: We are going over the edge of the buffer.

    // Check to see if we have to update the tail, we are checking
    // the case where the head is moving over the tail.
    bool updateTail = (mHead < mTail && mTail <= mHead + n);

    // We are getting overwritten from the end..
    if (mHead + n > capacity) {
        // Write up until the end of the buffer.
        std::streamsize bytesUntilTheEnd = capacity - mHead;
        memcpy(mRingbuffer.data() + mHead, s, bytesUntilTheEnd);

        // Write he remaining bytes from the start of the buffer.
        memcpy(mRingbuffer.data(), s + bytesUntilTheEnd, n - bytesUntilTheEnd);
        mHead = n - bytesUntilTheEnd;

        // We are checking the case where the tail got overwritten from the
        // front.
        updateTail |= mTail <= mHead;
    } else {
        // Case 2b: We are not falling off the edge of the world.
        memcpy(mRingbuffer.data() + mHead, s, n);
        mHead = (mHead + n) & (capacity - 1);

        // Check the corner case where we flipped to pos 0.
        updateTail |= mHead == mTail;
    }
    if (updateTail)
        mTail = (mHead + 1) & (capacity - 1);
    mHeadOffset += n;
    mLock.unlock();
    mCanRead.notify_all();
    return n;
}

int RingStreambuf::overflow(int c) {
    return EOF;
}

std::streamsize RingStreambuf::waitForAvailableSpace(std::streamsize n) {
    std::unique_lock<std::mutex> lock(mLock);
    mCanRead.wait_for(lock, mTimeout,
                      [this, n]() { return showmanyw() >= n || mClosed; });
    return showmanyw();
}

std::streamsize RingStreambuf::showmanyw() {
    return mRingbuffer.capacity() - 1 - showmanyc();
}

std::streamsize RingStreambuf::showmanyc() {
    // Note that:
    // Full state is mHead + 1 == mTail
    // Empty state is mHead == mTail
    if (mHead < mTail) {
        return mHead + mRingbuffer.capacity() - mTail;
    }
    return mHead - mTail;
}

std::streamsize RingStreambuf::xsgetn(char* s, std::streamsize n) {
    std::unique_lock<std::mutex> lock(mLock);
    if (!mCanRead.wait_for(lock, mTimeout,
                           [this]() { return mTail != mHead; })) {
        return 0;
    }
    std::streamsize toRead = std::min(showmanyc(), n);
    std::streamsize capacity = mRingbuffer.capacity();
    // 2 Cases:
    // We are falling over the edge, or not:
    if (mTail + toRead > capacity) {
        // We wrap around
        std::streamsize bytesUntilTheEnd = capacity - mTail;
        memcpy(s, mRingbuffer.data() + mTail, bytesUntilTheEnd);
        memcpy(s + bytesUntilTheEnd, mRingbuffer.data(),
               toRead - bytesUntilTheEnd);
    } else {
        // We don't
        memcpy(s, mRingbuffer.data() + mTail, toRead);
    }
    mTail = (mTail + toRead) & (capacity - 1);
    return toRead;
}

int RingStreambuf::underflow() {
    std::unique_lock<std::mutex> lock(mLock);
    if (!mCanRead.wait_for(lock, mTimeout,
                           [this]() { return mTail != mHead || mClosed; })) {
        return traits_type::eof();
    }
    if (mClosed && mTail == mHead) {
        return traits_type::eof();
    }
    return mRingbuffer[mTail];
};

int RingStreambuf::uflow() {
    std::unique_lock<std::mutex> lock(mLock);
    if (!mCanRead.wait_for(lock, mTimeout,
                           [this]() { return mTail != mHead || mClosed; })) {
        return traits_type::eof();
    }
    if (mClosed && mTail == mHead) {
        // [[unlikely]]
        return traits_type::eof();
    }

    int val = mRingbuffer[mTail];
    mTail = (mTail + 1) & (mRingbuffer.capacity() - 1);
    return val;
}

std::pair<int, std::string> RingStreambuf::bufferAtOffset(
        std::streamsize offset,
        milliseconds timeoutMs) {
    std::unique_lock<std::mutex> lock(mLock);
    std::string res;
    if (!mCanRead.wait_for(lock, timeoutMs,
                           [offset, this]() { return offset < mHeadOffset; })) {
        return std::make_pair(mHeadOffset, res);
    }

    // Prepare the outgoing buffer.
    std::streamsize capacity = mRingbuffer.capacity();
    std::streamsize toRead = showmanyc();
    std::streamsize startOffset = mHeadOffset - toRead;
    std::streamsize skip = std::max(startOffset, offset) - startOffset;

    // Let's find the starting point where we should be reading.
    uint16_t read = (mTail + skip) & (capacity - 1);

    // We are looking for an offset that is in the future...
    // Return the current start offset, without anything
    if (skip > toRead) {
        return std::make_pair(mHeadOffset, res);
    }

    // Actual size of bytes we are going to read.
    toRead -= skip;

    // We are falling over the edge, or not:
    res.reserve(toRead);
    if (read + toRead > capacity) {
        // We wrap around
        std::streamsize bytesUntilTheEnd = capacity - read;
        res.assign(mRingbuffer.data() + read, bytesUntilTheEnd);
        res.append(mRingbuffer.data(), toRead - bytesUntilTheEnd);
    } else {
        // We don't fall of the cliff..
        res.assign(mRingbuffer.data() + read, toRead);
    }

    return std::make_pair(startOffset + skip, res);
}

}  // namespace streams
}  // namespace base
}  // namespace android
