// Copyright 2018 The Android Open Source Project
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
// limitations under the License.#pragma once
#include "AndroidBufferQueue.h"

#include "android/base/synchronization/MessageChannel.h"

using android::base::MessageChannel;

namespace aemu {

class AndroidBufferQueue::Impl {
public:
    Impl() = default;
    ~Impl() = default;
    MessageChannel<AndroidBufferQueue::Item,
                   AndroidBufferQueue::kCapacity> queue;
    MessageChannel<AndroidBufferQueue::Item,
                   AndroidBufferQueue::kCapacity> canceled;
};

AndroidBufferQueue::AndroidBufferQueue() {
    mImpl.reset(new AndroidBufferQueue::Impl());
}

AndroidBufferQueue::~AndroidBufferQueue() = default;

void AndroidBufferQueue::cancelBuffer(
    const AndroidBufferQueue::Item& item) {
    mImpl->canceled.send(item);
}

void AndroidBufferQueue::queueBuffer(
    const AndroidBufferQueue::Item& item) {
    mImpl->queue.send(item);
}

void AndroidBufferQueue::dequeueBuffer(
    AndroidBufferQueue::Item* outItem) {
    if (mImpl->canceled.tryReceive(outItem)) {
        return;
    }
    mImpl->queue.receive(outItem);
}

bool AndroidBufferQueue::try_dequeueBuffer(
    AndroidBufferQueue::Item* outItem) {
    if (mImpl->canceled.tryReceive(outItem)) return true;
    return mImpl->queue.tryReceive(outItem);
}

} // namespace aemu