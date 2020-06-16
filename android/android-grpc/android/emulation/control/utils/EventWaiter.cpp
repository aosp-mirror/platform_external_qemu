// Copyright (C) 2020 The Android Open Source Project
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
#include "android/emulation/control/utils/EventWaiter.h"

namespace android {
namespace emulation {
namespace control {

EventWaiter::EventWaiter(RegisterCallback add, RemoveCallback remove)
    : mRemove(remove) {
    add(&EventWaiter::callbackForwarder, this);
}

EventWaiter::~EventWaiter() {
    mRemove(this);
}

uint64_t EventWaiter::next(std::chrono::milliseconds timeout_ms) {
    uint64_t missed = next(mLastEvent, timeout_ms);
    mLastEvent = current();
    return missed;
}

uint64_t EventWaiter::current() const {
    return mEventCounter;
}

uint64_t EventWaiter::next(uint64_t afterEvent,
                           std::chrono::milliseconds timeout_ms) {
    std::unique_lock<std::mutex> lock(mStreamLock);
    auto future = std::chrono::system_clock::now() + timeout_ms;
    bool timedOut = !mCv.wait_until(
            lock, future, [=]() { return mEventCounter > afterEvent; });
    (void)timedOut;

    return afterEvent > mEventCounter ? 0 : mEventCounter - afterEvent;
}

void EventWaiter::newEvent() {
    std::unique_lock<std::mutex> lock(mStreamLock);
    mEventCounter++;
    mCv.notify_all();
}

void EventWaiter::callbackForwarder(void* opaque) {
    reinterpret_cast<EventWaiter*>(opaque)->newEvent();
}

}  // namespace control
}  // namespace emulation
}  // namespace android
