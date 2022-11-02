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

#include "android/avd/info.h"                               // for AvdInfo
#include "aemu/base/Debug.h"                             // for IsDebugge...
#include "aemu/base/Log.h"                               // for base

#include <string_view>

#include "aemu/base/async/ThreadLooper.h"                // for ThreadLooper
#include "android/cmdline-definitions.h"                    // for AndroidOp...
#include "android/console.h"                                // for getConsol...
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
                      android::base::AutoLock lock(mLock);
                      mHangDetected = true;
                      mHangCondition.signalAndUnlock(&lock);
                  },
                  {
                          .hangLoopIterationTimeoutMs = 100,
                          .taskProcessingTimeoutMs = 10,
                          .hangCheckTimeoutMs = 10,
                  }) {}

    void TearDown() override { mHangDetector.stop(); }

    void waitUntilUnlocked() {
        android::base::AutoLock lock(mLock);
        auto waitUntil = System::get()->getUnixTimeUs() + kMaxBlockingTimeUs;
        while (!mHangDetected && System::get()->getUnixTimeUs() < waitUntil) {
            mHangCondition.timedWait(&mLock, waitUntil);
        }
        EXPECT_TRUE(mHangDetected);
    }

protected:
    const System::Duration kMaxBlockingTimeUs = 30 * 1000 * 1000;
    Lock mLock;
    android::base::ConditionVariable mHangCondition;
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
