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
#include "android/emulation/control/logcat/RingStreambuf.h"

namespace android {
namespace emulation {
namespace control {

using namespace base;

RingStreambuf::RingStreambuf(int capacity) {
    mRingBuffer.resize(capacity + 1);
}

std::streamsize RingStreambuf::xsputn(const char* s, std::streamsize n) {
    AutoLock lock(mLock);
    std::streamsize written = 0;
    for (; written < n; written++) {
        mRingBuffer[mWrite] = s[written];
        mWrite = (mWrite + 1) % mRingBuffer.capacity();
        mWriteOffset++;
        // Okay, we have overwritten the last one. Time to increment mRead
        if (mRead == mWrite) {
            mRead = (mRead + 1) % mRingBuffer.capacity();
        }
    }

    mCanRead.signal();
    return written;
}

int RingStreambuf::overflow(int c) {
    return EOF;
}

std::streamsize RingStreambuf::showmanyc() {
    if (mWrite < mRead) {
        return mWrite + mRingBuffer.capacity() - mRead;
    }
    return mWrite - mRead;
}

std::streamsize RingStreambuf::xsgetn(char* s, std::streamsize n) {
    AutoLock lock(mLock);
    mCanRead.wait(&mLock, [this]() { return mRead != mWrite; });
    std::streamsize toRead = std::min(showmanyc(), n);
    for (int i = 0; i < toRead; i++) {
        *s = mRingBuffer[mRead];
        mRead = (mRead + 1) % mRingBuffer.capacity();
        s++;
    }
    return toRead;
}

int RingStreambuf::underflow() {
    AutoLock lock(mLock);
    mCanRead.wait(&mLock, [this]() { return mRead != mWrite; });
    return mRingBuffer[mRead];
};

int RingStreambuf::uflow() {
    AutoLock lock(mLock);
    mCanRead.wait(&mLock, [this]() { return mRead != mWrite; });
    int val = mRingBuffer[mRead];
    mRead = (mRead + 1) % mRingBuffer.capacity();
    return val;
}

std::pair<int, std::string> RingStreambuf::bufferAtOffset(
        uint64_t offset,
        System::Duration timeoutMs) {
    AutoLock lock(mLock);
    std::string res;
    while (offset >= mWriteOffset) {
        System::Duration waitUntilUs =
                System::get()->getUnixTimeUs() + timeoutMs * 1000;
        if (!mCanRead.timedWait(&mLock, waitUntilUs)) {
            return std::make_pair(mWriteOffset, res);
        }
    }

    uint64_t startOffset = mWriteOffset - showmanyc();
    // Check how many to skip..
    uint64_t skip = std::max(startOffset, offset) - startOffset;
    uint64_t read = (mRead + skip) % mRingBuffer.capacity();
    while (read != mWrite) {
        res += mRingBuffer[read];
        read = (read + 1) % mRingBuffer.capacity();
    }

    return std::make_pair(startOffset + skip, res);
}

}  // namespace control
}  // namespace emulation
}  // namespace android