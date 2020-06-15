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
#pragma once

#include <atomic>  // for atomic
#include <chrono>  // for milliseconds
#include <condition_variable>
#include <cstdint>  // for uint64_t
#include <mutex>    // for condition_variable, mutex

namespace android {
namespace emulation {
namespace control {

using Callback = void (*)(void* opaque);
using RegisterCallback = void (*)(Callback frameAvailable, void* opaque);
using RemoveCallback = void (*)(void* opaque);

// An EventWaiter can be used to block and wait for events that are coming from
// qemu. The QEMU engine that produces events must provide the following C
// interface:
//
// - A RegisterCallback function, which can be used to register a Callback. The
//   Callback should be invoked when a new event is available. Registration is
//   identified by the opaque pointer.
// - A RemoveCallback function, that can be used to unregister a Callback
//   identified by the opaque pointer. No events should be delivered after the
//   return of this function. You will see crashes if you do.
//
// Typical usage would be something like:
//
// int cnt = 0;
// EventWaiter frameEvent(&gpu_register_shared_memory_callback,
// &gpu_unregister_shared_memory_callback);
//
// while(cnt < 10) {
//    if (frameEvent.next(std::chrono::seconds(1)) > 0)
//      handleEvent(cnt++);
// }
//
// If you wish to use the EventWaiter without the registration mechanism
// you can use the default constructor. Call the "newEvent" method to
// notify listeners of a newEvent.
class EventWaiter {
public:
    EventWaiter() = default;
    EventWaiter(RegisterCallback add, RemoveCallback remove);
    ~EventWaiter();

    // Block and wait at most timeout milliseconds until the next event
    // arrives. returns the number of missed events. Note, that this is
    // the least number of events you missed, you might have missed more.
    //
    // |afterEvent| Wait until we have received event number x+1.
    // |timeout|  Maximimum time to wait in milliseconds.
    uint64_t next(uint64_t afterEvent, std::chrono::milliseconds timeout);

    // Block and wait at most timeout milliseconds until the next event
    // arrives. Returns the number of missed events.
    //
    // This convenience method is not thread safe, and might not accurately
    // report events if multiple threads are using the same waiter.
    //
    // |timeout|  Maximimum time to wait in milliseconds.
    uint64_t next(std::chrono::milliseconds timeout);

    // The number of events received by the waiter. This is a monotonically
    // increasing number.
    uint64_t current() const;

    // Call this when a new event has arrived.
    void newEvent();
private:
    static void callbackForwarder(void* opaque);

    std::mutex mStreamLock;
    std::condition_variable mCv;
    uint64_t mEventCounter{0};
    std::atomic<uint64_t> mLastEvent{0};
    RemoveCallback mRemove;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
