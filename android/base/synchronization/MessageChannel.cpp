// Copyright 2014 The Android Open Source Project
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

#include "android/base/synchronization/MessageChannel.h"

namespace android {
namespace base {

MessageChannelBase::MessageChannelBase(size_t capacity) : mCapacity(capacity) {}

void MessageChannelBase::stop() {
    android::base::AutoLock lock(mLock);
    mStopped = true;
    mCount.store(0, std::memory_order_release);
    mCanRead.broadcast();
    mCanWrite.broadcast();
}

size_t MessageChannelBase::beforeWrite() {
    mLock.lock();
    while (mCount.load(std::memory_order_relaxed) >= mCapacity && !mStopped) {
        mCanWrite.wait(&mLock);
    }
    // Return value is undefined if stopped, so let's save a branch and skip the
    // check for it.
    size_t result = mPos + mCount.load(std::memory_order_relaxed);
    if (result >= mCapacity) {
        result -= mCapacity;
    }
    return result;
}

Optional<size_t> MessageChannelBase::beforeTryWrite() {
    mLock.lock();

    const auto count = mCount.load(std::memory_order_relaxed);
    if (count >= mCapacity || mStopped) {
        return {};
    }
    size_t result = mPos + count;
    if (result >= mCapacity) {
        result -= mCapacity;
    }
    return result;
}

void MessageChannelBase::afterWrite(bool success) {
    if (success) {
        mCount.fetch_add(1, std::memory_order_relaxed);
    }
    mCanRead.signalAndUnlock(&mLock);
}

size_t MessageChannelBase::beforeRead() {
    mLock.lock();
    while (mCount.load(std::memory_order_relaxed) == 0 && !mStopped) {
        mCanRead.wait(&mLock);
    }
    return mPos; // return value is undefined if stopped, so let's save a branch
}

Optional<size_t> MessageChannelBase::beforeTryRead() {
    mLock.lock();

    const auto count = mCount.load(std::memory_order_relaxed);
    if (count == 0 || mStopped) {
        return {};
    }
    return mPos;
}

void MessageChannelBase::afterRead(bool success) {
    if (success) {
        if (++mPos == mCapacity) {
            mPos = 0U;
        }
        mCount.fetch_sub(1, std::memory_order_relaxed);
    }
    mCanWrite.signalAndUnlock(&mLock);
}

}  // namespace base
}  // namespace android
