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
#pragma once

#include "OpenglRender/RenderChannel.h"
#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"

#include <stddef.h>

namespace emugl {

// BufferQueue models a FIFO queue of RenderChannel::Buffer instances.
// When used by multiple threads, synchronization must be performed
// by the caller. See SyncBufferQueue for a version that can be used
// with multiple threads.
class BufferQueue {
public:
    using IoResult = RenderChannel::IoResult;
    using Buffer = RenderChannel::Buffer;

    BufferQueue(size_t capacity)
        : mCapacity(capacity), mBuffers(new Buffer[capacity]) {}

    ~BufferQueue() {
        delete [] mBuffers;
    }

    // Return the number of items in the queue.
    size_t size() const { return mCount; }

    // Return true iff the queue has been closed. Remaining items can
    // be read from its, but it won't be possible to add new items to it.
    bool closed() const { return mClosed; }

    // Return true iff one can send a buffer to the queue, i.e. if it
    // is not full.
    bool canPush() const { return !mClosed && (mCount < mCapacity); }

    // Return true iff one can receive a buffer from the queue, i.e. if
    // it is not empty.
    bool canPop() const { return mCount > 0U; }

    // Try to send a buffer to the queue. On success, return IoResult::Ok
    // and moves |buffer| to the queue. On failure, return
    // IoResult::TryAgain if the queue was full, or IoResult::Error
    // if it was closed.
    IoResult tryPush(Buffer&& buffer) {
        if (mClosed) {
            return IoResult::Error;
        }
        if (mCount >= mCapacity) {
            return IoResult::TryAgain;
        }
        size_t pos = mPos + mCount;
        if (pos >= mCapacity) {
            pos -= mCapacity;
        }
        mBuffers[pos] = std::move(buffer);
        mCount++;
        return IoResult::Ok;
    }

    // Try to read a buffer from the queue. On success, moves item into
    // |*buffer| and return IoResult::Ok. On failure, return IoResult::Error
    // if the queue is empty and closed, and IoResult::TryAgain if it is
    // empty but not close.
    IoResult tryPop(Buffer* buffer) {
        if (mCount == 0) {
            return mClosed ? IoResult::Error : IoResult::TryAgain;
        }
        *buffer = std::move(mBuffers[mPos]);
        size_t pos = mPos + 1;
        if (pos >= mCapacity) {
            pos -= mCapacity;
        }
        mPos = pos;
        mCount--;
        return IoResult::Ok;
    }

    void closeAndClear(bool clear) {
        mClosed = true;

        if (clear) {
            while (mCount > 0) {
                mBuffers[mPos].clear();
                mPos ++;
                if (mPos == mCapacity) {
                    mPos = 0;
                }
                mCount--;
            }
        }
    }

protected:
    size_t mPos = 0;
    size_t mCount = 0;
    size_t mCapacity = 0;
    bool mClosed = false;
    Buffer* mBuffers = nullptr;

    // This will force the same for SyncBufferQueue and RenderChannelImpl
    DISALLOW_COPY_ASSIGN_AND_MOVE(BufferQueue);
};

// A BufferQueue instance that provides synchronous/blocking push/pop
// functions. Note that these take a *reference* to a lock that protects
// other data as well! This allows one to use several SyncBufferQueue
// instances protected by the same lock.
class SyncBufferQueue final : public BufferQueue {
    using ConditionVariable = android::base::ConditionVariable;
    using Lock = android::base::Lock;
    using AutoLock = android::base::AutoLock;

public:
    // Constructor, notice
    SyncBufferQueue(size_t capacity, android::base::Lock& lock)
        : BufferQueue(capacity), mLock(lock) {}

    // Try to send a buffer to the queue. On success, return IoResult::Ok
    // and moves |buffer| to the queue. On failure, return
    // IoResult::TryAgain if the queue was full, or IoResult::Error
    // if it was closed. Assumes |mLock| is held.
    IoResult tryPushLocked(Buffer&& buffer) {
        if (mClosed) {
            return IoResult::Error;
        }
        IoResult result = BufferQueue::tryPush(std::move(buffer));
        if (result == IoResult::Ok && mCount == 1) {
            mCanPop.signal();
        }
        return result;
    }

    // Try to read a buffer from the queue. On success, moves item into
    // |*buffer| and return IoResult::Ok. On failure, return IoResult::Error
    // if the queue is empty and closed, and IoResult::TryAgain if it is
    // empty but not close. Assumes |mLock| is held.
    IoResult tryPopLocked(Buffer* buffer) {
        IoResult result = BufferQueue::tryPop(buffer);
        if (result == IoResult::Ok && mCount + 1U == mCapacity) {
            mCanPush.signal();
        }
        return result;
    }

    // Push a buffer to the queue. This is a blocking call. On success,
    // move |buffer| into the queue and return IoResult::Ok. On failure,
    // return IoResult::Error meaning the queue was stopped by the other
    // side. Assumes |mLock| is held.
    IoResult pushLocked(Buffer&& buffer) {
        while (!canPush()) {
            if (mClosed) {
                return IoResult::Error;
            }
            mCanPush.wait(&mLock);
        }
        IoResult result = BufferQueue::tryPush(std::move(buffer));
        if (result == IoResult::Ok && mCount == 1) {
            mCanPop.signal();
        }
        return result;
    }

    // Pop a buffer from the queue. This is a blocking call. On success,
    // move item into |*buffer| and return IoResult::Ok. On failure,
    // return IoResult::Error to indicate the queue was stopped by the
    // other side. Assumes |mLock| is held.
    IoResult popLocked(Buffer* buffer) {
        while (!canPop()) {
            if (mClosed) {
                // Closed queue is empty.
                return IoResult::Error;
            }
            mCanPop.wait(&mLock);
        }
        IoResult result = BufferQueue::tryPop(buffer);
        if (result == IoResult::Ok && mCount + 1U == mCapacity) {
            mCanPush.signal();
        }
        return result;
    }

    // Close the queue, it is no longer possible to push new items
    // to it (i.e. push() will always return IoResult::Error), or to
    // read from an empty queue (i.e. pop() will always return
    // IoResult::Error once the queue becomes empty).
    void closeLocked() { doCloseLocked(false); }

    // Same as close() but also clears all items in the queue.
    void closeAndClearLocked() { doCloseLocked(true); }

private:
    void doCloseLocked(bool clear) {
        BufferQueue::closeAndClear(clear);
        // Wake any potential waiters.
        if (mCount == mCapacity) {
            mCanPush.broadcast();
        }
        if (mCount == 0) {
            mCanPop.broadcast();
        }
    }

    Lock& mLock;
    ConditionVariable mCanPush;
    ConditionVariable mCanPop;
};

}  // namespace emugl
