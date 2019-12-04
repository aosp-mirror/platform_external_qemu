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
#include "android/base/files/QueueStreambuf.h"

#include <algorithm>  // for max, min
#include <cassert>
#include <iostream>  // for streamsize

namespace android {
namespace base {

QueueStreambuf::QueueStreambuf(uint32_t capacity,
                               ReclaimFunction reclaim_strategy)
    : mReclaim(reclaim_strategy) {
    mQueuebuffer.reserve(capacity);
}

std::streamsize QueueStreambuf::xsputn(const char* s, std::streamsize n) {
    if (!mOpen) {
        return 0;
    }
    AutoLock lock(mLock);
    mReadOffset = mReclaim(mReadOffset, mQueuebuffer);
    mQueuebuffer.insert(mQueuebuffer.end(), s, s + n);
    mCanRead.signal();
    return n;
}

QueueStreambuf* QueueStreambuf::close() {
    if (!mOpen)
        return this;

    mOpen = false;
    AutoLock lock(mLock);
    mCanRead.signal();
    return this;
}

bool QueueStreambuf::is_open() {
    return mOpen;
}

std::streamsize QueueStreambuf::showmanyc() {
    return mQueuebuffer.size() - mReadOffset;
}

size_t QueueStreambuf::reclaim(size_t offset, CharVec& vect) {
    // Cleanup if we use up to much buffer, we cleanup if we read more than half
    // of the capacity.
    if (offset > (vect.capacity() / 2)) {
        vect.erase(vect.begin(), vect.begin() + offset);
        return 0;
    }
    return offset;
}

std::streamsize QueueStreambuf::xsgetn(char* s, std::streamsize n) {
    AutoLock lock(mLock);
    while (mOpen && (mQueuebuffer.size() - mReadOffset) < n)
        mCanRead.wait(&mLock);
    assert(mReadOffset <= mQueuebuffer.size());
    auto read =
            std::min<std::streamsize>(n, (mQueuebuffer.size() - mReadOffset));
    memcpy(s, mQueuebuffer.data() + mReadOffset, read);
    mReadOffset += read;
    return read;
}

}  // namespace base
}  // namespace android