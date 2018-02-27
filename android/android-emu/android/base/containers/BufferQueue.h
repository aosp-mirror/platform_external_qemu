// Copyright (C) 2018 The Android Open Source Project
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
#pragma once

#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"

#include <vector>

namespace android {
namespace base {

enum class BufferQueueResult {
    Ok = 0,
    TryAgain = 1,
    Error = 2,
};

// BufferQueue models a FIFO queue of <T> instances
// that can be used between two different threads. Note that it depends,
// for synchronization, on an external lock (passed as a reference in
// the BufferQueue constructor).
//
// This allows one to use multiple BufferQueue instances whose content
// are protected by a single lock.
template <class T>
class BufferQueue {
    using ConditionVariable = android::base::ConditionVariable;
    using Lock = android::base::Lock;
    using AutoLock = android::base::AutoLock;

public:
    using value_type = T;

    // Constructor. |capacity| is the maximum number of Buffer instances in
    // the queue, and |lock| is a reference to an external lock provided by
    // the caller.
    explicit BufferQueue(int capacity, android::base::Lock& lock)
        : mBuffers(capacity), mLock(lock) {}

    // Return true iff one can send a buffer to the queue, i.e. if it
    // is not full or it would grow anyway.
    virtual bool canPushLocked() const {
        return !mClosed && mCount < (int)mBuffers.size();
    }

    // Return true iff one can receive a buffer from the queue, i.e. if
    // it is not empty.
    virtual bool canPopLocked() const { return mCount > 0; }

    // Return true iff the queue is closed.
    virtual bool isClosedLocked() const { return mClosed; }

    // Try to send a buffer to the queue. On success, return
    // BufferQueueResult::Ok and moves |buffer| to the queue. On failure, return
    // BufferQueueResult::TryAgain if the queue was full, or
    // BufferQueueResult::Error if it was closed.
    virtual BufferQueueResult tryPushLocked(T&& buffer) {
        if (mClosed) {
            return BufferQueueResult::Error;
        }
        if (mCount >= (int)mBuffers.size()) {
            return BufferQueueResult::TryAgain;
        }
        int pos = mPos + mCount;
        if (pos >= (int)mBuffers.size()) {
            pos -= mBuffers.size();
        }
        mBuffers[pos] = std::move(buffer);
        if (mCount++ == 0) {
            mCanPop.signal();
        }
        return BufferQueueResult::Ok;
    }

    // Push a buffer to the queue. This is a blocking call. On success,
    // move |buffer| into the queue and return BufferQueueResult::Ok. On
    // failure, return BufferQueueResult::Error meaning the queue was closed.
    virtual BufferQueueResult pushLocked(T&& buffer) {
        while (mCount == (int)mBuffers.size()) {
            if (mClosed) {
                return BufferQueueResult::Error;
            }
            mCanPush.wait(&mLock);
        }
        return tryPushLocked(std::move(buffer));
    }

    // Try to read a buffer from the queue. On success, moves item into
    // |*buffer| and return BufferQueueResult::Ok. On failure, return
    // BufferQueueResult::TryAgain if the queue is empty or
    // BufferQueueResult::Error if closed.
    virtual BufferQueueResult tryPopLocked(T* buffer) {
        if (mCount == 0) {
            return mClosed ? BufferQueueResult::Error
                           : BufferQueueResult::TryAgain;
        }
        *buffer = std::move(mBuffers[mPos]);
        int pos = mPos + 1;
        if (pos >= (int)mBuffers.size()) {
            pos -= mBuffers.size();
        }
        mPos = pos;
        if (mCount-- == (int)mBuffers.size()) {
            mCanPush.signal();
        }
        return BufferQueueResult::Ok;
    }

    // Pop a buffer from the queue. This is a blocking call. On success,
    // move item into |*buffer| and return BufferQueueResult::Ok. On failure,
    // return BufferQueueResult::TryAgain to indicate the queue was closed or is
    // in snapshot mode.
    virtual BufferQueueResult popLocked(T* buffer) {
        while (mCount == 0) {
            if (mClosed) {
                // Closed queue is empty.
                return BufferQueueResult::Error;
            }
            mCanPop.wait(&mLock);
        }
        return tryPopLocked(buffer);
    }

    // Close the queue, it is no longer possible to push new items
    // to it (i.e. push() will always return BufferQueueResult::Error), or to
    // read from an empty queue (i.e. pop() will always return
    // BufferQueueResult::Error once the queue becomes empty).
    virtual void closeLocked() {
        mClosed = true;
        wakeAllWaiters();
    }

protected:
    virtual void wakeAllWaiters() {
        if (mCount == (int)mBuffers.size()) {
            mCanPush.broadcast();
        }
        if (mCount == 0) {
            mCanPop.broadcast();
        }
    }

protected:
    int mPos = 0;
    int mCount = 0;
    bool mClosed = false;
    std::vector<T> mBuffers;

    Lock& mLock;
    ConditionVariable mCanPush;
    ConditionVariable mCanPop;

    // This will force the same for RenderChannelImpl
    DISALLOW_COPY_ASSIGN_AND_MOVE(BufferQueue);
};

}  // namespace base
}  // namespace android
