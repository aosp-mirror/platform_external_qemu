// Copyright 2020 The Android Open Source Project
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

#include "android/base/synchronization/Event.h"

#include "android/base/testing/TestThread.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

// Test the basic case where timed wait times out correctly
TEST(Event, Timeout) {
    Event e;
    EXPECT_FALSE(e.timedWait(0));
    EXPECT_FALSE(e.timedWait(System::get()->getUnixTimeUs()));
}

// Test the basic case where event can be signaled from the same thread thread
TEST(Event, SameThread) {
    Event e;
    e.signal();
    EXPECT_TRUE(e.timedWait(System::get()->getUnixTimeUs()));
}

void* testThreadFunc(void* data) {
    Event* event = static_cast<Event*>(data);
    event->signal();
    return 0;
}

// Test the basic case where event can be signaled from another thread
TEST(Event, TwoThreads) {
    Event e;

    auto thread = std::unique_ptr<TestThread>(
            new TestThread(testThreadFunc, &e));

    e.wait();

    thread->join();
}

// Test signaling from more than one thread at the same time
TEST(Event, ThreeThreads) {
    Event e;

    auto thread1 = std::unique_ptr<TestThread>(
            new TestThread(testThreadFunc, &e));
    auto thread2 = std::unique_ptr<TestThread>(
            new TestThread(testThreadFunc, &e));

    e.wait();

    thread1->join();
    thread2->join();
}

}  // namespace base
}  // namespace android
