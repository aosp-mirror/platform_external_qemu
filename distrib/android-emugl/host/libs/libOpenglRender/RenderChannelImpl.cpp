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
#include "RenderChannelImpl.h"

#include <algorithm>
#include <utility>

#include <assert.h>
#include <string.h>

namespace emugl {

RenderChannelImpl::RenderChannelImpl(std::shared_ptr<RendererImpl> renderer)
    : mRenderer(std::move(renderer)), mState(State::Empty) {
    assert(mState.is_lock_free()); // rethink it if this fails - we've already
                                   // got a lock, use it instead.
}

void RenderChannelImpl::setEventCallback(
        RenderChannelImpl::EventCallback callback) {
    mOnEvent = std::move(callback);

    // Reset the current state to make sure the new subscriber gets it.
    mState.store(State::Empty, std::memory_order_release);
    onEvent(true);
}

bool RenderChannelImpl::write(ChannelBuffer&& buffer) {
    const bool res = mFromGuest.send(std::move(buffer));
    onEvent(true);
    return res;
}

bool RenderChannelImpl::read(ChannelBuffer* buffer, CallType type) {
    if (type == CallType::Nonblocking && mToGuest.size() == 0) {
        return false;
    }
    const bool res = mToGuest.receive(buffer);
    onEvent(true);
    return res;
}

void RenderChannelImpl::stop() {
    stop(true);
}

void RenderChannelImpl::forceStop() {
    stop(false);
}

void RenderChannelImpl::stop(bool byGuest) {
    if (mStopped) {
        return;
    }
    mStopped = true;
    mFromGuest.stop();
    mToGuest.stop();
    onEvent(byGuest);
}

bool RenderChannelImpl::isStopped() const {
    return mStopped;
}

void RenderChannelImpl::writeToGuest(ChannelBuffer&& buf) {
    mToGuest.send(std::move(buf));
    onEvent(false);
}

bool RenderChannelImpl::readFromGuest(ChannelBuffer* buf) {
    assert(buf);

    const bool res = !mStopped && mFromGuest.receive(buf);
    onEvent(false);
    return res;
}

void RenderChannelImpl::onEvent(bool byGuest) {
    if (!mOnEvent) {
        return;
    }

    // We need to make sure that the state returned from calcState() remains
    // valid when we write it into mState; this means we need to lock both
    // calcState() and mState assignment with the same lock - otherwise it is
    // possible (and it happened before the lock was added) that some other
    // thread would overwrite a newer state with its older value, e.g.:
    //  - thread 1 reads the last message from |mToGuest|
    //  - thread 1 enters onEvent()
    //  - thread 1 calculates state 1 = "can't read", switches out
    //  -   thread 2 writes some data into |mToGuest|
    //  -   thread 2 enters onEvent()
    //  -   thread 2 calculates state 2 = "can read"
    //  -   thread 2 gets the lock
    //  -   thread 2 updates |mState| to "can read", unlocks, switches out
    //  - thread 1 gets the lock
    //  - thread 1 overwrites |mState| with state 1 = "can't read"
    //  - thread 1 unlocks and calls the |mOnEvent| callback.
    //
    // The result is that state 2 = "can read" is completely lost - callback
    // is never called when |mState| is "can read".
    //
    // But if the whole block of code is locked, threads can't overwrite newer
    // |mState| with some older value, and the described situation would never
    // happen.
    android::base::AutoLock lock(mStateLock);
    const State newState = calcState();
    if (mState != newState) {
        mState = newState;
        lock.unlock();

        mOnEvent(newState,
                 byGuest ? EventSource::Client : EventSource::RenderChannel);
    }
}

RenderChannel::State RenderChannelImpl::calcState() const {
    State state = State::Empty;
    if (mStopped) {
        state = State::Stopped;
    } else {
        if (mFromGuest.size() < mFromGuest.capacity()) {
            state |= State::CanWrite;
        }
        if (mToGuest.size() > 0) {
            state |= State::CanRead;
        }
    }
    return state;
}

}  // namespace emugl
