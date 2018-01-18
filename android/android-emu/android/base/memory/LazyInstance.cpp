// Copyright (C) 2014 The Android Open Source Project
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

#include "android/base/memory/LazyInstance.h"
#include "android/base/threads/Thread.h"

namespace android {
namespace base {
namespace internal {

bool LazyInstanceState::inNoObjectState() const {
    const auto state = mState.load(std::memory_order_acquire);
    return state == State::Init || state == State::Destroying;
}

template <LazyInstanceState::State start,
          LazyInstanceState::State intermediate,
          LazyInstanceState::State end>
static bool checkAndTranformState(
        std::atomic<LazyInstanceState::State>* state) {
    for (;;) {
        auto current = start;
        if (state->compare_exchange_strong(current, intermediate,
                                           std::memory_order_acquire,
                                           std::memory_order_acquire)) {
            // The object was in the expected |start| state, so we're done here
            // and return that the action needs to be done.
            return true;
        }

        while (current != end && current != start) {
            Thread::yield();
            current = state->load(std::memory_order_acquire);
        }

        // |end| -> object is in final state, nothing to do.
        if (current == end) {
            return false;
        }

        // |start| -> some other thread has undone all progress; we need to
        // retry the whole thing again.
    }
}

bool LazyInstanceState::needConstruction() {
    if (mState.load(std::memory_order_acquire) == State::Done) {
        return false;
    }
    return checkAndTranformState<State::Init, State::Constructing, State::Done>(
            &mState);
}

void LazyInstanceState::doneConstructing() {
    mState.store(State::Done, std::memory_order_release);
}

bool LazyInstanceState::needDestruction() {
    return checkAndTranformState<State::Done, State::Destroying, State::Init>(
            &mState);
}

void LazyInstanceState::doneDestroying() {
    mState.store(State::Init, std::memory_order_release);
}

}  // namespace internal
}  // namespace base
}  // namespace android
