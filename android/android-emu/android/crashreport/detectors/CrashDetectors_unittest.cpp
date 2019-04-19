// Copyright 2019 The Android Open Source Project
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

#include "android/crashreport/detectors/CrashDetectors.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

using namespace android::base;
using namespace android::crashreport;

class AlwaysCrash : public StatefulHangdetector {
public:
    bool check() { return true; }
};

TEST(CrashDetectorsTest, timeoutProperly) {
    TestSystem testSys("foo", 64);
    TimedHangDetector t(15, new AlwaysCrash());
    EXPECT_FALSE(t.check());
    testSys.setUnixTimeUs(1000000);
    EXPECT_TRUE(t.check());
}

TEST(CrashDetectorsTest, heartBeatDetector_nobeatIsFine) {
    // Some images will never send a heart beat.
    TestSystem testSys("foo", 64);
    int booted = 0;
    int beatCount = 0;
    HeartBeatDetector hb([&beatCount]{return beatCount;}, &booted);
    EXPECT_FALSE(hb.check());
    booted = 1;

    // First time boot so should be ok
    EXPECT_FALSE(hb.check());

    // No heartbeat as the image doesn't provide one..
    EXPECT_FALSE(hb.check());
}

TEST(CrashDetectorsTest, heartBeatDetector_detectsBootFailure) {
    TestSystem testSys("foo", 64);
    int booted = 0;
    int beatCount = 1;
    HeartBeatDetector hb([&beatCount]{return beatCount;}, &booted);
    EXPECT_FALSE(hb.check());
    booted = 1;

    // First time boot so should be ok
    EXPECT_FALSE(hb.check());

    beatCount++;
    // Heartbeat.. no problem!
    EXPECT_FALSE(hb.check());
    // No heartbeat.. trouble!
    EXPECT_TRUE(hb.check());
}
