// Copyright 2021 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expresso or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "VsyncThread.h"

#include <gtest/gtest.h>

#include <unistd.h>

// Tests scheduling some basic task, and that they are processed
// before the thread completely exits.
TEST(VsyncThread, Basic) {
    const uint64_t kOneSecondNs = 1000000000ULL;
    const uint64_t k60HzPeriodNs = kOneSecondNs / 60ULL;

    for (int i = 0; i < 2; ++i) {
        uint64_t count = 0;

        {
            VsyncThread thread(k60HzPeriodNs);
            EXPECT_EQ(k60HzPeriodNs, thread.getPeriod());
            for (int j = 0; j < 2; ++j) {
                thread.schedule([&count](uint64_t currentCount) {
                    ++count;
                });
            }
        }

        EXPECT_EQ(2, count);
    }
}

// Tests scheduling tasks and changing the period.
TEST(VsyncThread, ChangePeriod) {
    const uint64_t kOneSecondNs = 1000000000ULL;
    const uint64_t k60HzPeriodNs = kOneSecondNs / 60ULL;
    const uint64_t k144HzPeriodNs = kOneSecondNs / 144ULL;

    for (int i = 0; i < 2; ++i) {
        uint64_t count = 0;

        {
            VsyncThread thread(k60HzPeriodNs);
            EXPECT_EQ(k60HzPeriodNs, thread.getPeriod());
            thread.schedule([&count](uint64_t currentCount) {
                ++count;
            });
            thread.setPeriod(k144HzPeriodNs);
            thread.schedule([&count](uint64_t currentCount) {
                ++count;
            });
        }

        EXPECT_EQ(2, count);
    }
}
