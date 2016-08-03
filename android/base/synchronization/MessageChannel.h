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

#pragma once

#include "android/base/Optional.h"
#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"

#include <utility>
#include <stddef.h>

namespace android {
namespace base {

// Base non-templated class used to reduce the amount of template
// specialization.
class MessageChannelBase {
public:
    // Get the current channel size
    size_t size() const {
        android::base::AutoLock lock(mLock);
        return mCount;
    }

    // Abort the currently pending operations and don't allow any other ones
    void stop();

    // Check if the channel is stopped.
    bool isStopped() const {
        android::base::AutoLock lock(mLock);
        return isStoppedLocked();
    }

protected:
    // Constructor. |capacity| is the buffer capacity in messages.
    MessageChannelBase(size_t capacity);

    // Destructor.
    ~MessageChannelBase() = default;

    // Call this method in the sender thread before writing a new message.
    // This returns the position of the available slot in the message array
    // where to copy the new fixed-size message. After the copy, call
    // afterWrite().
    // If the channel is stopped, return value is undefined.
    size_t beforeWrite();

    // To be called after beforeWrite() and copying a new fixed-size message
    // into the array. This signal the receiver thread that there is a new
    // incoming message.
    void afterWrite();

    // Call this method in the receiver thread before reading a new message.
    // This returns the position in the message array where the new message
    // can be read. Caller must process the message, then call afterRead().
    // If the channel is stopped, return value is undefined.
    size_t beforeRead();

    // To be called in the receiver thread after beforeRead() and processing
    // the corresponding message.
    void afterRead();

    // A version of isStopped() that doesn't lock the channel but expects it
    // to be locked by the caller.
    bool isStoppedLocked() const { return mStopped; }

private:
    size_t mPos = 0;
    size_t mCount = 0;
    size_t mCapacity;
    bool mStopped = false;
    mutable Lock mLock;     // Mutable to allow const members to lock it.
    ConditionVariable mCanRead;
    ConditionVariable mCanWrite;
};

// Helper class used to implement an uni-directional IPC channel between
// two threads. The channel can be used to send fixed-size messages of type
// |T|, with an internal buffer size of |CAPACITY| items. All calls are
// blocking.
//
// Usage is pretty straightforward:
//
//   - From the sender thread, call send(msg);
//   - From the receiver thread, call receive(&msg);
//   - If you want to stop the IPC, call stop();
template <typename T, size_t CAPACITY>
class MessageChannel : public MessageChannelBase {
public:
    MessageChannel() : MessageChannelBase(CAPACITY) {}

    bool send(const T& msg) {
        size_t pos = beforeWrite();
        bool res = !isStoppedLocked();
        if (res) {
            mItems[pos] = msg;
        }
        afterWrite();
        return res;
    }

    bool send(T&& msg) {
        size_t pos = beforeWrite();
        bool res = !isStoppedLocked();
        if (res) {
            mItems[pos] = std::move(msg);
        }
        afterWrite();
        return res;
    }

    bool receive(T* msg) {
        size_t pos = beforeRead();
        bool res = !isStoppedLocked();
        if (res) {
            *msg = std::move(mItems[pos]);
        }
        afterRead();
        return res;
    }

    Optional<T> receive() {
        size_t pos = beforeRead();
        if (!isStoppedLocked()) {
            Optional<T> msg(std::move(mItems[pos]));
            afterRead();
            return msg;
        } else {
            afterRead();
            return {};
        }
    }

    constexpr size_t capacity() const { return CAPACITY; }

private:
    T mItems[CAPACITY];
};

}  // namespace base
}  // namespace android
