// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/qt/qt_setup.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

using namespace android::base;

TEST(androidQtSetupEnv, NoQtLibDir) {
    TestSystem testSys("/foo", 32);
    TestTempDir* testDir = testSys.getTempRoot();
    EXPECT_TRUE(testDir->makeSubDir("foo"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib64"));
    EXPECT_FALSE(androidQtSetupEnv(false));
    EXPECT_FALSE(androidQtSetupEnv(true));
}

TEST(androidQtSetupEnv, QtLibDir32Only) {
    TestSystem testSys("/foo", 32);
    TestTempDir* testDir = testSys.getTempRoot();
    EXPECT_TRUE(testDir->makeSubDir("foo"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib/qt"));
    EXPECT_TRUE(androidQtSetupEnv(false));
    EXPECT_FALSE(androidQtSetupEnv(true));
}

TEST(androidQtSetupEnv, QtLibDir64Only) {
    TestSystem testSys("/foo", 64);
    TestTempDir* testDir = testSys.getTempRoot();
    EXPECT_TRUE(testDir->makeSubDir("foo"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib64"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib64/qt"));
    EXPECT_FALSE(androidQtSetupEnv(false));
    EXPECT_TRUE(androidQtSetupEnv(true));
}

TEST(androidQtSetupEnv, QtLibDir32And64) {
    TestSystem testSys("/foo", 64);
    TestTempDir* testDir = testSys.getTempRoot();
    EXPECT_TRUE(testDir->makeSubDir("foo"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib/qt"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib64"));
    EXPECT_TRUE(testDir->makeSubDir("foo/lib64/qt"));

    EXPECT_TRUE(androidQtSetupEnv(false));
    EXPECT_TRUE(androidQtSetupEnv(true));
}
