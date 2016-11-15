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

#include <memory>

#include <stddef.h>

namespace emugl {

// BufferQueue models a FIFO queue of RenderChannel::Buffer instances
// that can be used between two different threads. Note that it depends,
// for synchronization, on an external lock (passed as a reference in
// the BufferQueue constructor).
//
// This allows one to use multiple BufferQueue instances whose content
// are protected by a single lock.
class BufferQueue {
    using ConditionVariable = android::base::ConditionVariable;
    using Lock = android::base::Lock;
    using AutoLock = android::base::AutoLock;

public:
    using IoResult = RenderChannel::IoResult;
    using Buffer = RenderChannel::Buffer;

    // Constructor. |capacity| is the maximum number of Buffer instances in
    // the queue, and |lock| is a reference to an external lock provided by
    // the caller.
    BufferQueue(size_t capacity, android::base::Lock& lock)
        : mCapacity(capacity), mBuffers(new Buffer[capacity]), mLock(lock) {}

    // Return true iff the queue has been closed. Remaining items can
    // be read from its, but it won't be possible to add new items to it.
    // bool closed() const { return mClosed; }

    // Return true iff one can send a buffer to the queue, i.e. if it
    // is not full.
    bool canPushLocked() const { return !mClosed && (mCount < mCapacity); }

    // Return true iff one can receive a buffer from the queue, i.e. if
    // it is not empty.
    bool canPopLocked() const { return mCount > 0U; }

    // Return true iff the queue is closed.
    bool isClosedLocked() const { return mClosed; }

    // Try to send a buffer to the queue. On success, return IoResult::Ok
    // and moves |buffer| to the queue. On failure, return
    // IoResult::TryAgain if the queue was full, or IoResult::Error
    // if it was closed.
    IoResult tryPushLocked(Buffer&& buffer) {
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
        if (mCount++ == 0) {
            mCanPop.signal();
        }
        return IoResult::Ok;
    }

    // Push a buffer to the queue. This is a blocking call. On success,
    // move |buffer| into the queue and return IoResult::Ok. On failure,
    // return IoResult::Error meaning the queue was closed.
    IoResult pushLocked(Buffer&& buffer) {
        while (mCount == mCapacity) {
            if (mClosed) {
                return IoResult::Error;
            }
            mCanPush.wait(&mLock);
        }
        return tryPushLocked(std::move(buffer));
    }

    // Try to read a buffer from the queue. On success, moves item into
    // |*buffer| and return IoResult::Ok. On failure, return IoResult::Error
    // if the queue is empty and closed, and IoResult::TryAgain if it is
    // empty but not close.
    IoResult tryPopLocked(Buffer* buffer) {
        if (mCount == 0) {
            return mClosed ? IoResult::Error : IoResult::TryAgain;
        }
        *buffer = std::move(mBuffers[mPos]);
        size_t pos = mPos + 1;
        if (pos >= mCapacity) {
            pos -= mCapacity;
        }
        mPos = pos;
        if (mCount-- == mCapacity) {
            mCanPush.signal();
        }
        return IoResult::Ok;
    }

    // Pop a buffer from the queue. This is a blocking call. On success,
    // move item into |*buffer| and return IoResult::Ok. On failure,
    // return IoResult::Error to indicate the queue was closed.
    IoResult popLocked(Buffer* buffer) {
        while (mCount == 0) {
            if (mClosed) {
                // Closed queue is empty.
                return IoResult::Error;
            }
            mCanPop.wait(&mLock);
        }
        return tryPopLocked(buffer);
    }

    // Close the queue, it is no longer possible to push new items
    // to it (i.e. push() will always return IoResult::Error), or to
    // read from an empty queue (i.e. pop() will always return
    // IoResult::Error once the queue becomes empty).
    void closeLocked() {
        mClosed = true;

        // Wake any potential waiters.
        if (mCount == mCapacity) {
            mCanPush.broadcast();
        }
        if (mCount == 0) {
            mCanPop.broadcast();
        }
    }

private:
    size_t mCapacity = 0;
    size_t mPos = 0;
    size_t mCount = 0;
    bool mClosed = false;
    std::unique_ptr<Buffer[]> mBuffers;

    Lock& mLock;
    ConditionVariable mCanPush;
    ConditionVariable mCanPop;

    // This will force the same for SyncBufferQueue and RenderChannelImpl
    DISALLOW_COPY_ASSIGN_AND_MOVE(BufferQueue);
};

}  // namespace emugl
