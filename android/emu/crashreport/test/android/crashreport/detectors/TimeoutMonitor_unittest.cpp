// Copyright 2024 The Android Open Source Project
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
#include "android/crashreport/detectors/TimeoutMonitor.h"

#include <gtest/gtest.h>

namespace android {
namespace crashreport {

void simulateWork(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

TEST(TimeoutMonitorTests, BasicTimeout) {
    bool crash_handler_called = false;
    auto crash_handler = [&crash_handler_called]() {
        crash_handler_called = true;
    };

    TimeoutMonitor monitor(std::chrono::milliseconds(1), crash_handler);

    // Simulate work that exceeds the timeout
    simulateWork(std::chrono::milliseconds(200));

    EXPECT_TRUE(crash_handler_called);
}

TEST(TimeoutMonitorTests, NoTimeout) {
    bool crash_handler_called = false;
    auto crash_handler = [&crash_handler_called]() {
        crash_handler_called = true;
    };

    TimeoutMonitor monitor(std::chrono::milliseconds(200), crash_handler);

    // Work that finishes within the limit
    simulateWork(std::chrono::milliseconds(1));

    EXPECT_FALSE(crash_handler_called);
}

TEST(TimeoutMonitorTests, TimeoutDoesNotBlock) {
    bool crash_handler_called = false;
    auto crash_handler = [&crash_handler_called]() {
        crash_handler_called = true;
    };

    auto start_time = std::chrono::steady_clock::now();

    {
        TimeoutMonitor monitor(std::chrono::milliseconds(500), crash_handler);
        // Work that finishes within the limit
        simulateWork(std::chrono::milliseconds(1));
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // The monitor should properly clean up in a timely fashion.
    EXPECT_LT(duration.count(), 500);
}

}  // namespace crashreport
}  // namespace android