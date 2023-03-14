// Copyright (C) 2019 The Android Open Source Project
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

#include <gtest/gtest.h>  // for SuiteApiResolver, TestInfo (ptr only), Message
#include <thread>         // for thread
#include <unordered_map>  // for unordered_map
#include <utility>        // for pair

namespace android {
namespace emulation {
namespace control {

static int sRegistrySize = 0;

static std::unordered_map<void*, Callback> sReceivers{};
void fake_add_cb(Callback cb, void* opaque) {
    sRegistrySize++;
    sReceivers[opaque] = cb;
}
void fake_remove_cb(void* opaque) {
    sRegistrySize--;
    sReceivers.erase(opaque);
}

void notify_all() {
    for (auto fn : sReceivers) {
        fn.second(fn.first);
    }
}

class EventWaiterTest : public ::testing::Test {
protected:
    void SetUp() override {
        sRegistrySize = 0;
        sReceivers.clear();
    }
};

TEST_F(EventWaiterTest, add_and_removes) {
    EXPECT_EQ(sRegistrySize, 0);
    {
        EventWaiter tst(fake_add_cb, fake_remove_cb);
        EXPECT_EQ(sRegistrySize, 1);
    }
    EXPECT_EQ(sRegistrySize, 0);
}

TEST_F(EventWaiterTest, times_out) {
    EventWaiter tst(fake_add_cb, fake_remove_cb);
    EXPECT_EQ(0, tst.next(std::chrono::milliseconds(2)));
}

TEST_F(EventWaiterTest, wake_up_for_event) {
    EventWaiter tst(fake_add_cb, fake_remove_cb);
    auto test = std::thread([]() {
        for (int i = 0; i < 100; i++)
            notify_all();
    });

    // Wait until we have fired a lot of events.
    test.join();

    // At least one event should be available..
    EXPECT_GT(tst.next(std::chrono::milliseconds(10)), 1);
}

TEST_F(EventWaiterTest, times_out_not_enough) {
    EventWaiter tst(fake_add_cb, fake_remove_cb);
    bool completed = false;
    auto test = std::thread([&completed]() {
        for (int i = 0; i < 100; i++)
            notify_all();
        completed = true;
    });


    test.join();
    EXPECT_TRUE(completed);

    auto start = std::chrono::system_clock::now();
    // We are waiting for >100..
    EXPECT_EQ(0, tst.next(100, std::chrono::milliseconds(10)));

    // Make sure we waited at least 10ms.
    auto end = std::chrono::system_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_GE(milliseconds.count(), 10);
}

TEST_F(EventWaiterTest, no_callback_no_crash) {
    {
        EventWaiter wt;
    }
    SUCCEED();
}

TEST_F(EventWaiterTest, available_no_block) {
    // Test that you can do the following to not track all events:
    // int curr = 0;
    // curr += wt.next(curr, timeout);
    EventWaiter wt;
    wt.newEvent();
    EXPECT_EQ(1, wt.next(0, std::chrono::milliseconds(5)));
    wt.newEvent();
    EXPECT_EQ(1, wt.next(1, std::chrono::milliseconds(5)));
}

}  // namespace control
}  // namespace emulation
}  // namespace android
