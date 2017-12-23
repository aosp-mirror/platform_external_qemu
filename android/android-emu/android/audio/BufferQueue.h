// Copyright (C) 2017 The Android Open Source Project
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

#include "android/base/containers/CircularBuffer.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"

namespace android {
namespace audio {

using android::base::AutoLock;
using android::base::CircularBuffer;
using android::base::ConditionVariable;
using android::base::Lock;

// This class is useful if you have multiple producers and one consumer,
// where there are a lot of memory allocations and releases. This class
// will pre-allocate a fixed amount of memory that cannot be changed once
// allocated and stores it in a circular buffer. The producer will write into
// the oldest slot, and the consumer will read the oldest data inserted into the
// queue.
//
// Note: A producer can overwrite the oldest buffer, regardless of whether
// the consumer has used it or not. The unused buffer is simply thrown
// away.
template <class T, int N>
class BufferQueue {
    static_assert(N > 0, "BufferQueue must have N > 0");

public:
    using value_type = T*;
    static constexpr int capacity = N;

    template <class... Args>
    BufferQueue(Lock& lock, Args&&... args)
        : mLock(lock), mBuffers(CircularBuffer<T*>(N)) {
        // Pre-allocate the buffers
        for (int i = 0; i < N; ++i) {
            mBuffers.push_back(new T(std::forward<Args>(args)...));
        }
    }
    // PRODUCER calls
    // Get an empty buffer from the queue.
    T* dequeueBuffer() {
        AutoLock lock(mLock);
        T* t = mBuffers.front();
        mBuffers.pop_front();
        return t;
    }

    // Push a buffer with data into the queue.
    void queueBuffer(T* t) {
        assert(t);
        AutoLock lock(mLock);
        mBuffers.push_back(t);

        // Consumer may be waiting on data.
        if (mConsumerIsWaiting) {
            mConsumerIsWaiting = false;
            mCond.signalAndUnlock(&lock);
        }
    }

    // CONSUMER calls
    // Get the oldest buffer with data.
    T* acquireBuffer() {
        AutoLock lock(mLock);
        T* t = mBuffers.front();
        mBuffers.pop_front();
        return t;
    }

    // Get the oldest buffer if |cond| is satisfied. If |cond| is
    // false, then wait for a signal from queueBuffer and then
    // recheck |cond|.
    T* acquireBuffer(std::function<bool(T*)> cond) {
        AutoLock lock(mLock);

        while (!cond(mBuffers.front())) {
            mConsumerIsWaiting = true;
            do {
                // consumerIsWaiting reset by queueBuffer()
                mCond.wait(&lock);
            } while (mConsumerIsWaiting);
        }

        T* t = mBuffers.front();
        mBuffers.pop_front();
        return t;
    }

    // Release the buffer back for reuse.
    void releaseBuffer(T* t) {
        assert(t);
        AutoLock lock(mLock);
        mBuffers.push_back(t);
    }

    size_t size() { return mBuffers.size(); }

    bool empty() { return mBuffers.empty(); }

    // Destructor
    virtual ~BufferQueue() {
        AutoLock lock(mLock);
        assert(mBuffers.size() == N);
        while (!mBuffers.empty()) {
            delete mBuffers.back();
            mBuffers.pop_back();
        }
    }

private:
    Lock& mLock;
    ConditionVariable mCond;
    CircularBuffer<T*> mBuffers;
    bool mConsumerIsWaiting = false;

    DISALLOW_COPY_ASSIGN_AND_MOVE(BufferQueue);
};

}  // namespace audio
}  // namespace android
