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
    mCount = 0;
    mCanRead.signal();
    mCanWrite.signal();
}

size_t MessageChannelBase::beforeWrite() {
    mLock.lock();
    while (mCount >= mCapacity && !mStopped) {
        mCanWrite.wait(&mLock);
    }
    if (mStopped) {
        return 0; // just anything
    }

    size_t result = mPos + mCount;
    if (result >= mCapacity) {
        result -= mCapacity;
    }
    return result;
}

void MessageChannelBase::afterWrite() {
    if (!mStopped) {
        mCount++;
        mCanRead.signal();
    }
    mLock.unlock();
}

size_t MessageChannelBase::beforeRead() {
    mLock.lock();
    while (mCount == 0 && !mStopped) {
        mCanRead.wait(&mLock);
    }
    return mPos; // return value is undefined if stopped, so let's save a branch
}

void MessageChannelBase::afterRead() {
    if (!mStopped) {
        if (++mPos == mCapacity) {
            mPos = 0U;
        }
        mCount--;
        mCanWrite.signal();
    }
    mLock.unlock();
}

}  // namespace base
}  // namespace android
