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
#include "android/base/containers/BufferQueue.h"
#include "android/base/files/Stream.h"
#include "android/base/files/StreamSerializing.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"

#include <iterator>
#include <vector>
#include <utility>

#include <assert.h>
#include <stddef.h>

namespace emugl {

using BufferQueueBase = android::base::BufferQueue<RenderChannel::Buffer>;

// BufferQueue models a FIFO queue of RenderChannel::Buffer instances
// that can be used between two different threads. Note that it depends,
// for synchronization, on an external lock (passed as a reference in
// the BufferQueue constructor).
//
// This allows one to use multiple BufferQueue instances whose content
// are protected by a single lock.
class BufferQueue : public BufferQueueBase {
public:
    using Buffer = RenderChannel::Buffer;
    using IoResult = android::base::BufferQueueResult;

    // Constructor. |capacity| is the maximum number of Buffer instances in
    // the queue, and |lock| is a reference to an external lock provided by
    // the caller.
    BufferQueue(int capacity, android::base::Lock& lock)
        : BufferQueueBase(capacity, lock) {}

    // Changes the operation mode to snapshot or back. In snapshot mode
    // BufferQueue accepts all write requests and accumulates the data, but
    // returns error on all reads.
    void setSnapshotModeLocked(bool on) {
        mSnapshotMode = on;
        if (on && !mClosed) {
            wakeAllWaiters();
        }
    }

    // Try to send a buffer to the queue. On success, return IoResult::Ok
    // and moves |buffer| to the queue. On failure, return
    // IoResult::TryAgain if the queue was full, or IoResult::Error
    // if it was closed.
    // Note: in snapshot mode it never returns TryAgain, but grows the max
    //   queue size instead.
    virtual IoResult tryPushLocked(Buffer&& buffer) override {
        if (mClosed) {
            return IoResult::Error;
        }
        if (mCount >= (int)mBuffers.size()) {
            if (mSnapshotMode) {
                grow();
            } else {
                return IoResult::TryAgain;
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
        return IoResult::Ok;
    }

    // Push a buffer to the queue. This is a blocking call. On success,
    // move |buffer| into the queue and return IoResult::Ok. On failure,
    // return IoResult::Error meaning the queue was closed.
    virtual IoResult pushLocked(Buffer&& buffer) override {
        while (mCount == (int)mBuffers.size() && !mSnapshotMode) {
            if (mClosed) {
                return IoResult::Error;
            }
            mCanPush.wait(&mLock);
        }
        return tryPushLocked(std::move(buffer));
    }

    // Try to read a buffer from the queue. On success, moves item into
    // |*buffer| and return IoResult::Ok. On failure, return IoResult::Error
    // if the queue is empty and closed or in snapshot mode, and
    // IoResult::TryAgain if it is empty but not closed.
    virtual IoResult tryPopLocked(Buffer* buffer) override {
        if (mCount == 0) {
            return (mClosed || mSnapshotMode) ? IoResult::Error
                                              : IoResult::TryAgain;
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
        return IoResult::Ok;
    }

    // Pop a buffer from the queue. This is a blocking call. On success,
    // move item into |*buffer| and return IoResult::Ok. On failure,
    // return IoResult::Error to indicate the queue was closed or is in
    // snapshot mode.
    virtual IoResult popLocked(Buffer* buffer) override {
        while (mCount == 0 && !mSnapshotMode) {
            if (mClosed) {
                // Closed queue is empty.
                return IoResult::Error;
            }
            mCanPop.wait(&mLock);
        }
        return tryPopLocked(buffer);
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
        std::vector<Buffer> newBuffers;
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

private:
    bool mSnapshotMode = false;

    // This will force the same for RenderChannelImpl
    DISALLOW_COPY_ASSIGN_AND_MOVE(BufferQueue);
};

}  // namespace emugl
