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

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"
#include "android/base/async/ThreadLooper.h"

#include <gtest/gtest.h>

using namespace android::base;
using namespace android::crashreport;

class TimeTestSystem : public android::base::TestSystem {
 public:
  TimeTestSystem() : TestSystem("/", System::kProgramBitness)  { }

  // Override the clock so that it (usually) moves forward for each call.
  virtual WallDuration getHighResTimeUs() const override {
     return android::base::TestSystem::getHighResTimeUs() + (mCount++ * mStep);
  }

  mutable int mCount{0};
  const Duration mStep{60 * 60 * 1000};

};

TEST(HangDetector, PredicateTriggersHang) {
   TimeTestSystem test;
   bool hangDetected = false;
   HangDetector detect([&hangDetected](StringView msg) { hangDetected = true; });

   EXPECT_FALSE(hangDetected);
   detect.addPredicateCheck([]{ return true; }, "Always dead");
   while(test.mCount < 100 && !hangDetected) { Thread::sleepMs(1); }
   EXPECT_TRUE(hangDetected);
   detect.stop();
}

TEST(HangDetector, BlockedLooperTriggersHang) {
   TimeTestSystem test;
   bool hangDetected = false;
   HangDetector detect([&hangDetected](StringView msg) { hangDetected = true; });
   auto looper =  ThreadLooper::get();

   EXPECT_FALSE(hangDetected);
   detect.addWatchedLooper(looper);
   while(test.mCount < 100 && !hangDetected) { Thread::sleepMs(1); }
   EXPECT_TRUE(hangDetected);
   detect.stop();
}
