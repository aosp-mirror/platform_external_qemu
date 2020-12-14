// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/testing/TestEvent.h"

#include <gtest/gtest.h>
#include <gtest/gtest-spi.h>

#include <thread>

TEST(TestEvent, Simple) {
    TestEvent event;
    EXPECT_FALSE(event.isSignaled());

    event.signal();
    EXPECT_TRUE(event.isSignaled());
    event.wait();
    EXPECT_FALSE(event.isSignaled());
}

TEST(TestEvent, MultipleSignals) {
    TestEvent event;

    event.signal();
    event.signal();
    event.wait();
    EXPECT_TRUE(event.isSignaled());
    event.wait();
    EXPECT_FALSE(event.isSignaled());
}

TEST(TestEvent, Reset) {
    TestEvent event;

    event.signal();
    event.signal();
    event.reset();
    EXPECT_FALSE(event.isSignaled());
}

TEST(TestEvent, Timeout) {
    EXPECT_FATAL_FAILURE(
            {
                TestEvent event;
                event.wait(0);
            },
            "timed out");
}

TEST(TestEvent, Multithreaded) {
    TestEvent event;
    std::thread worker([&event]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        event.signal();
    });

    event.wait();
    worker.join();
}
