// Copyright 2015 The Android Open Source Project
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

#include "android/emulation/ConfigDirs.h"

#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

using android::ConfigDirs;
using namespace android::base;

TEST(ConfigDirs, getUserDirectoryDefault) {
    TestSystem sys("/bin", 32, "/myhome");
    static const char kExpected[] =
#ifdef _WIN32
            "/myhome\\.android";
#else
            "/myhome/.android";
#endif
    EXPECT_STREQ(kExpected, ConfigDirs::getUserDirectory().c_str());
}

TEST(ConfigDirs, getUserDirectoryWithAndroidSdkHome) {
    TestSystem sys("/bin", 32, "/myhome");
    sys.envSet("ANDROID_SDK_HOME", "/android-sdk");
    static const char kExpected[] =
#ifdef _WIN32
            "/android-sdk\\.android";
#else
            "/android-sdk/.android";
#endif
    EXPECT_STREQ(kExpected, ConfigDirs::getUserDirectory().c_str());
}

TEST(ConfigDirs, getUserDirectoryWithAndroidEmulatorHome) {
    TestSystem sys("/bin", 32, "/myhome");
    sys.envSet("ANDROID_EMULATOR_HOME", "/android/home");
    EXPECT_STREQ("/android/home", ConfigDirs::getUserDirectory().c_str());
}
