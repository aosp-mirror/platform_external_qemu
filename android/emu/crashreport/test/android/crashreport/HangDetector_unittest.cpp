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
// limitations under the License.

#include "android/crashreport/HangDetector.h"

#include <assert.h>       // for assert
#include <gtest/gtest.h>  // for SuiteApiR...
#include <stdio.h>        // for printf

#include "aemu/base/Debug.h"  // for IsDebugge...
#include "aemu/base/Log.h"    // for base

#include <string_view>

#include "aemu/base/async/ThreadLooper.h"                   // for ThreadLooper
#include "android/emulation/control/AndroidAgentFactory.h"  // for injectCon...
#include "android/emulation/control/globals_agent.h"        // for QAndroidG...
#include "android/utils/debug.h"                            // for dinfo
#include "gtest/gtest_pred_impl.h"                          // for Assertion...

struct AvdInfo;

using namespace android::base;
using namespace android::crashreport;

class HangDetectorTest : public ::testing::Test {
public:
    HangDetectorTest()
        : mHangDetector(
                  [this](std::string_view msg) {
                      std::unique_lock<std::mutex> l(mLock);
                      mHangDetected = true;
                      mHangCondition.notify_one();
                  },
                  {
                          .hangLoopIterationTimeout = std::chrono::milliseconds(100),
                          .hangCheckTimeout = std::chrono::milliseconds(10),
                  }) {}

    void TearDown() override { mHangDetector.stop(); }

    void waitUntilUnlocked() {
        std::unique_lock<std::mutex> l(mLock);
        auto waitUntil = std::chrono::high_resolution_clock::now() +
                kMaxBlockingTime;
        while (!mHangDetected &&
               std::chrono::high_resolution_clock::now() < waitUntil) {
            mHangCondition.wait_until(l, waitUntil);
        }
        EXPECT_TRUE(mHangDetected);
    }

protected:
    const std::chrono::seconds kMaxBlockingTime = std::chrono::seconds(30);
    std::mutex mLock;
    std::condition_variable mHangCondition;
    bool mHangDetected{false};
    HangDetector mHangDetector;
};

TEST_F(HangDetectorTest, PredicateTriggersHang) {
    if (android::base::IsDebuggerAttached()) {
        printf("This test cannot be run under a debugger.");
        return;
    }
    mHangDetector.addPredicateCheck([] { return true; }, "Always dead");
    waitUntilUnlocked();
}

// Flaky failure to detect hang on Mac.
TEST_F(HangDetectorTest, DISABLED_BlockedLooperTriggersHang) {
    // Note that the looper is not running!
    auto looper = ThreadLooper::get();
    mHangDetector.addWatchedLooper(looper);
    waitUntilUnlocked();
}
