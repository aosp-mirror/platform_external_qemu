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

#include "android/base/files/PathUtils.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>

using android::ConfigDirs;
using namespace android::base;

#ifdef _WIN32
#define SLASH "\\"
#else
#define SLASH "/"
#endif

TEST(ConfigDirs, getUserDirectoryDefault) {
    TestSystem sys("/bin", 32, "/myhome");
    static const char kExpected[] = "/myhome" SLASH ".android";
    EXPECT_STREQ(kExpected, ConfigDirs::getUserDirectory().c_str());
}

TEST(ConfigDirs, getUserDirectoryWithAndroidSdkHome) {
    TestSystem sys("/bin", 32, "/myhome");
    sys.envSet("ANDROID_SDK_HOME", "/android-sdk");
    EXPECT_STREQ("/android-sdk", ConfigDirs::getUserDirectory().c_str());

    sys.getTempRoot()->makeSubDir("android-sdk");
    sys.getTempRoot()->makeSubDir("android-sdk/.android");
    EXPECT_STREQ(PathUtils::join("/android-sdk", ".android").c_str(),
                 ConfigDirs::getUserDirectory().c_str());
}

TEST(ConfigDirs, getUserDirectoryWithAndroidEmulatorHome) {
    TestSystem sys("/bin", 32, "/myhome");
    sys.envSet("ANDROID_EMULATOR_HOME", "/android/home");
    EXPECT_STREQ("/android/home", ConfigDirs::getUserDirectory().c_str());
}


TEST(ConfigDirs, getSdkRootDirectory) {
    TestSystem sys("", 32, "/myhome");
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk/platform-tools"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk/platforms"));
    ASSERT_TRUE(sys.pathIsDir("Sdk"));

    sys.envSet("ANDROID_SDK_ROOT", "Sdk");
    EXPECT_STREQ("Sdk", ConfigDirs::getSdkRootDirectory().c_str());

    sys.envSet("ANDROID_SDK_ROOT", "\"Sdk\"");
    EXPECT_STREQ("Sdk", ConfigDirs::getSdkRootDirectory().c_str());

    sys.envSet("ANDROID_SDK_ROOT", "");
    EXPECT_STRNE("Sdk", ConfigDirs::getSdkRootDirectory().c_str());

    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk2"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk2/platform-tools"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Sdk2/platforms"));
    ASSERT_TRUE(sys.pathIsDir("Sdk2"));

    // ANDROID_HOME should take precedence over ANDROID_SDK_ROOT
    sys.envSet("ANDROID_HOME", "Sdk2");
    EXPECT_STREQ("Sdk2", ConfigDirs::getSdkRootDirectory().c_str());

    // Bad ANDROID_HOME falls back to ANDROID_SDK_ROOT
    sys.envSet("ANDROID_HOME", "bogus");
    sys.envSet("ANDROID_SDK_ROOT", "Sdk");
    EXPECT_STREQ("Sdk", ConfigDirs::getSdkRootDirectory().c_str());
}


TEST(ConfigDirs, getAvdRootDirectory) {
    TestSystem sys("", 32, "/myhome");
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_1"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_1/.android"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_1/.android/avd"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_2"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_2/.android"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_2/.android/avd"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_3"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_3/.android"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_3/.android/avd"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_4"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_4/.android"));
    ASSERT_TRUE(sys.getTempRoot()->makeSubDir("Area_4/.android/avd"));
    ASSERT_TRUE(sys.pathIsDir("Area_1/.android/avd"));
    ASSERT_TRUE(sys.pathIsDir("Area_2/.android/avd"));
    ASSERT_TRUE(sys.pathIsDir("Area_3/.android/avd"));
    ASSERT_TRUE(sys.pathIsDir("Area_4/.android/avd"));

    // Order of precedence is
    //   ANDROID_AVD_HOME
    //   ANDROID_SDK_HOME
    //   USER_HOME or HOME
    //   ANDROID_EMULATOR_HOME

    sys.envSet("ANDROID_AVD_HOME", "Area_1/.android/avd");
    sys.envSet("ANDROID_SDK_HOME", "Area_2");
    sys.envSet("USER_HOME", "Area_3");
    sys.envSet("ANDROID_EMULATOR_HOME", "Area_4/.android");
    EXPECT_STREQ("Area_1/.android/avd", ConfigDirs::getAvdRootDirectory().c_str());

    sys.envSet("ANDROID_AVD_HOME", "bogus");
    EXPECT_STREQ("Area_2" SLASH ".android" SLASH "avd", ConfigDirs::getAvdRootDirectory().c_str());

    sys.envSet("ANDROID_SDK_HOME", "bogus");
    EXPECT_STREQ("Area_3" SLASH ".android" SLASH "avd", ConfigDirs::getAvdRootDirectory().c_str());

    sys.envSet("USER_HOME", "bogus");
    EXPECT_STREQ("Area_4/.android" SLASH "avd", ConfigDirs::getAvdRootDirectory().c_str());
}
