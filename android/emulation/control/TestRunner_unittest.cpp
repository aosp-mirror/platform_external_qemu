// Copyright (C) 2016 The Android Open Source Project
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

#include "android/emulation/control/TestRunner.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

#include <fstream>
#include <string>
#include <gtest/gtest.h>

using android::base::PathUtils;
using android::base::System;
using android::emulation::TestRunner;

// Tests ctor/dtor, which will also start/stop
// the test runner thread.
TEST(TestRunner, constructDestruct) {
    TestRunner* testRunner = new TestRunner();
    EXPECT_TRUE(testRunner != NULL);
    delete testRunner;
}

TEST(TestRunner, outFilePath) {
    std::string testOutFile("ConsoleTestRunnerTestFile.txt");

    System* sys = System::get();
    std::string testPath = PathUtils::join(sys->getTempDir(),
                                           testOutFile);

    TestRunner* testRunner = new TestRunner(testOutFile);
    EXPECT_TRUE(testRunner != NULL);
    std::string testOutPath = testRunner->queryTestResultLocation();
    delete testRunner;

    EXPECT_TRUE(testOutPath == testPath);
    EXPECT_TRUE(sys->pathExists(testPath));
    EXPECT_TRUE(sys->deleteFile(testPath));
}

TEST(TestRunner, basicRun) {
    std::string testOutFile("ConsoleTestRunnerTestFile.txt");

    System* sys = System::get();
    std::string testPath = PathUtils::join(sys->getTempDir(),
                                           testOutFile);

    TestRunner* testRunner = new TestRunner(testOutFile);
    EXPECT_TRUE(testRunner != NULL);
    testRunner->initSelfTests();

    std::string testOutPath = testRunner->queryTestResultLocation();

    testRunner->runTest("basicMultiThread");

    delete testRunner;

    EXPECT_TRUE(testOutPath == testPath);
    EXPECT_TRUE(sys->pathExists(testPath));
    EXPECT_TRUE(sys->deleteFile(testPath));
}
