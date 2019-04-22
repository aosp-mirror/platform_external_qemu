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

#include "android/base/Compiler.h"
#include "android/base/files/Stream.h"
#include "android/base/files/StreamSerializing.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"

#include <iterator>
#include <vector>
#include <utility>

#include <assert.h>
#include <stddef.h>

namespace android {
namespace base {

// Values corresponding to the result of BufferQueue operations.
// |Ok| means everything went well.
// |TryAgain| means the operation could not be performed and should be
// tried later.
// |Error| means an error happened (i.e. the BufferQueue is closed).
// |Timeout| means that an item could not be popped in time.
enum class BufferQueueResult {
    Ok = 0,
    TryAgain = 1,
    Error = 2,
    Timeout = 3,
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

    // Constructor. |capacity| is the maximum number of T instances in
    // the queue, and |lock| is a reference to an external lock provided by
    // the caller.
    BufferQueue(int capacity, android::base::Lock& lock)
        : mBuffers(capacity), mLock(lock) {}

    // Return true iff one can send a buffer to the queue, i.e. if it
    // is not full or it would grow anyway.
    bool canPushLocked() const { return !mClosed && mCount < (int)mBuffers.size(); }

    // Return true iff one can receive a buffer from the queue, i.e. if
    // it is not empty.
    bool canPopLocked() const { return mCount > 0; }

    // Return true iff the queue is closed.
    bool isClosedLocked() const { return mClosed; }

    // Changes the operation mode to snapshot or back. In snapshot mode
    // BufferQueue accepts all write requests and accumulates the data, but
    // returns error on all reads.
    void setSnapshotModeLocked(bool on) {
        mSnapshotMode = on;
        if (on && !mClosed) {
            wakeAllWaiters();
        }
    }

    // Try to send a buffer to the queue. On success, return BufferQueueResult::Ok
    // and moves |buffer| to the queue. On failure, return
    // BufferQueueResult::TryAgain if the queue was full, or BufferQueueResult::Error
    // if it was closed.
    // Note: in snapshot mode it never returns TryAgain, but grows the max
    //   queue size instead.
    BufferQueueResult tryPushLocked(T&& buffer) {
        if (mClosed) {
            return BufferQueueResult::Error;
        }
        if (mCount >= (int)mBuffers.size()) {
            if (mSnapshotMode) {
                grow();
            } else {
                return BufferQueueResult::TryAgain;
            }
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
    // move |buffer| into the queue and return BufferQueueResult::Ok. On failure,
    // return BufferQueueResult::Error meaning the queue was closed.
    BufferQueueResult pushLocked(T&& buffer) {
        while (mCount == (int)mBuffers.size() && !mSnapshotMode) {
            if (mClosed) {
                return BufferQueueResult::Error;
            }
            mCanPush.wait(&mLock);
        }
        return tryPushLocked(std::move(buffer));
    }

    // Try to read a buffer from the queue. On success, moves item into
    // |*buffer| and return BufferQueueResult::Ok. On failure, return BufferQueueResult::Error
    // if the queue is empty and closed or in snapshot mode, and
    // BufferQueueResult::TryAgain if it is empty but not closed.
    BufferQueueResult tryPopLocked(T* buffer) {
        if (mCount == 0) {
            return (mClosed || mSnapshotMode) ? BufferQueueResult::Error
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
    // return BufferQueueResult::Error to indicate the queue was closed or is in
    // snapshot mode.
    BufferQueueResult popLocked(T* buffer) {
        while (mCount == 0 && !mSnapshotMode) {
            if (mClosed) {
                // Closed queue is empty.
                return BufferQueueResult::Error;
            }
            mCanPop.wait(&mLock);
        }
        return tryPopLocked(buffer);
    }

    // Pop a buffer from the queue. This is a blocking call. On success,
    // move item into |*buffer| and return BufferQueueResult::Ok. On failure,
    // return BufferQueueResult::Error to indicate the queue was closed or is in
    // snapshot mode. Returns BufferQueueResult::Timeout if we waited passed
    // waitUntilUs.
    BufferQueueResult popLockedBefore(T* buffer, System::Duration waitUntilUs) {
        while (mCount == 0 && !mSnapshotMode) {
            if (mClosed) {
                // Closed queue is empty.
                return BufferQueueResult::Error;
            }
            if (!mCanPop.timedWait(&mLock, waitUntilUs)) {
                return BufferQueueResult::Timeout;
            }

        }
        return tryPopLocked(buffer);
    }

    // Close the queue, it is no longer possible to push new items
    // to it (i.e. push() will always return BufferQueueResult::Error), or to
    // read from an empty queue (i.e. pop() will always return
    // BufferQueueResult::Error once the queue becomes empty).
    void closeLocked() {
        mClosed = true;
        wakeAllWaiters();
    }

    // Save to a snapshot file
    void onSaveLocked(android::base::Stream* stream) {
        stream->putByte(mClosed);
        if (!mClosed) {
            stream->putBe32(mCount);
            for (int i = 0; i < mCount; i++) {
                android::base::saveBuffer(
                        stream, mBuffers[(i + mPos) % mBuffers.size()]);
            }
        }
    }

    bool onLoadLocked(android::base::Stream* stream) {
        mClosed = stream->getByte();
        if (!mClosed) {
            mCount = stream->getBe32();
            if ((int)mBuffers.size() < mCount) {
                mBuffers.resize(mCount);
            }
            mPos = 0;
            for (int i = 0; i < mCount; i++) {
                if (!android::base::loadBuffer(stream, &mBuffers[i])) {
                    return false;
                }
            }
        }
        return true;
    }

private:
    void grow() {
        assert(mCount == (int)mBuffers.size());
        std::vector<T> newBuffers;
        newBuffers.reserve(mBuffers.size() * 2);
        newBuffers.insert(newBuffers.end(),
                          std::make_move_iterator(mBuffers.begin() + mPos),
                          std::make_move_iterator(
                                  mBuffers.begin() +
                                  std::min<int>(mPos + mCount, mBuffers.size())));
        newBuffers.insert(
                newBuffers.end(), std::make_move_iterator(mBuffers.begin()),
                std::make_move_iterator(mBuffers.begin() +
                                        (mPos + mCount) % mBuffers.size()));
        mBuffers = std::move(newBuffers);
        mBuffers.resize(mBuffers.capacity());
        mPos = 0;
    }

    void wakeAllWaiters() {
        if (mCount == (int)mBuffers.size()) {
            mCanPush.broadcast();
        }
        if (mCount == 0) {
            mCanPop.broadcast();
        }
    }

private:
    int mPos = 0;
    int mCount = 0;
    bool mClosed = false;
    bool mSnapshotMode = false;
    std::vector<T> mBuffers;

    Lock& mLock;
    ConditionVariable mCanPush;
    ConditionVariable mCanPop;

    DISALLOW_COPY_ASSIGN_AND_MOVE(BufferQueue);
};

}  // namespace base
}  // namespace android
