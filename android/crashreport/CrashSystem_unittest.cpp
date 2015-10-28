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

#include "android/crashreport/CrashSystem.h"

#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

using namespace android::base;
using namespace android::crashreport;

// TODO fix validatePaths_nonWritableHomeDir
// TODO windows tests

void makeCaBundle(TestTempDir* rootDir) {
    EXPECT_TRUE(rootDir->makeSubDir("progdir/lib"));
    EXPECT_TRUE(rootDir->makeSubFile("progdir/lib/ca-bundle.pem"));
}

void makeCrashService(TestTempDir* rootDir) {
#ifdef _WIN32
    EXPECT_TRUE(rootDir->makeSubFile("progdir/emulator-crash-service.exe"));
#else
    EXPECT_TRUE(rootDir->makeSubFile("progdir/emulator-crash-service"));
#endif
}

void makeAndroidHome(TestTempDir* rootDir) {
    EXPECT_TRUE(rootDir->makeSubDir("homedir/.android"));
}

void makeBreakpadHome(TestTempDir* rootDir) {
    makeAndroidHome(rootDir);
    EXPECT_TRUE(rootDir->makeSubDir("homedir/.android/breakpad"));
}

#define BUILD_TESTSYSTEM32                                     \
    TestSystem testsys("/progdir", 32, "/homedir", "/appdir"); \
    TestTempDir* testDir = testsys.getTempRoot();              \
    EXPECT_TRUE(testDir->makeSubDir("progdir"));               \
    EXPECT_TRUE(testDir->makeSubDir("homedir"));               \
    EXPECT_TRUE(testDir->makeSubDir("appdir"));

TEST(CrashSystem, getCrashDirectory) {
    BUILD_TESTSYSTEM32
    makeBreakpadHome(testDir);
    std::string tmp1 = CrashSystem::get()->getCrashDirectory();
#ifdef _WIN32
    EXPECT_STREQ(tmp1.c_str(), "/homedir\\.android\\breakpad");
#else
    EXPECT_STREQ(tmp1.c_str(), "/homedir/.android/breakpad");
#endif
}

TEST(CrashSystem, getCaBundlePath) {
    BUILD_TESTSYSTEM32
    makeCaBundle(testDir);
    std::string tmp1 = CrashSystem::get()->getCaBundlePath();
#ifdef _WIN32
    EXPECT_STREQ(tmp1.c_str(), "/progdir\\lib\\ca-bundle.pem");
#else
    EXPECT_STREQ(tmp1.c_str(), "/progdir/lib/ca-bundle.pem");
#endif
}

TEST(CrashSystem, getCrashServicePath) {
    BUILD_TESTSYSTEM32
    makeCrashService(testDir);
    std::string tmp1 = CrashSystem::get()->getCrashServicePath();
#ifdef _WIN32
    EXPECT_STREQ(tmp1.c_str(), "/progdir\\emulator-crash-service.exe");
#else
    EXPECT_STREQ(tmp1.c_str(), "/progdir/emulator-crash-service");
#endif
}

TEST(CrashSystem, getCrashServiceCmdLine) {
    BUILD_TESTSYSTEM32
    std::string pipe("pipeval");
    std::string proc("procval");
    ::android::base::StringVector tmp1 =
            CrashSystem::get()->getCrashServiceCmdLine(pipe, proc);
    EXPECT_EQ(tmp1.size(), 5);
#ifdef _WIN32
    EXPECT_STREQ(tmp1[0].c_str(), "/progdir\\emulator-crash-service.exe");
#else
    EXPECT_STREQ(tmp1[0].c_str(), "/progdir/emulator-crash-service");
#endif
    EXPECT_STREQ(tmp1[1].c_str(), "-pipe");
    EXPECT_STREQ(tmp1[2].c_str(), pipe.c_str());
    EXPECT_STREQ(tmp1[3].c_str(), "-ppid");
    EXPECT_STREQ(tmp1[4].c_str(), proc.c_str());
}

TEST(CrashSystem, validatePaths_ValidPaths) {
    BUILD_TESTSYSTEM32
    makeCaBundle(testDir);
    makeCrashService(testDir);
    makeBreakpadHome(testDir);
    EXPECT_TRUE(CrashSystem::get()->validatePaths());
}

TEST(CrashSystem, validatePaths_NoCrashServiceFile) {
    BUILD_TESTSYSTEM32
    makeCaBundle(testDir);
    makeBreakpadHome(testDir);
    EXPECT_FALSE(CrashSystem::get()->validatePaths());
}

TEST(CrashSystem, validatePaths_CrashServiceIsDir) {
    BUILD_TESTSYSTEM32
#ifdef _WIN32
    EXPECT_TRUE(testDir->makeSubDir("progdir/emulator-crash-service.exe"));
#else
    EXPECT_TRUE(testDir->makeSubDir("progdir/emulator-crash-service"));
#endif
    makeBreakpadHome(testDir);
    EXPECT_FALSE(CrashSystem::get()->validatePaths());
}

TEST(CrashSystem, validatePaths_NoCaBundle) {
    BUILD_TESTSYSTEM32
    makeCrashService(testDir);
    makeBreakpadHome(testDir);
    EXPECT_FALSE(CrashSystem::get()->validatePaths());
}

TEST(CrashSystem, validatePaths_CaBundleIsDir) {
    BUILD_TESTSYSTEM32
    EXPECT_TRUE(testDir->makeSubDir("progdir/lib"));
    EXPECT_TRUE(testDir->makeSubDir("progdir/lib/ca-bundle.pem"));
    makeCrashService(testDir);
    makeBreakpadHome(testDir);
    EXPECT_FALSE(CrashSystem::get()->validatePaths());
}

TEST(CrashSystem, validatePaths_crashDirIsFile) {
    BUILD_TESTSYSTEM32
    makeCaBundle(testDir);
    makeCrashService(testDir);
    makeAndroidHome(testDir);
    EXPECT_TRUE(testDir->makeSubFile("homedir/.android/breakpad"));
    EXPECT_FALSE(CrashSystem::get()->validatePaths());
}

//  This test is a false pass.  Currently validatePaths calls a
//    utils/paths::path_mkdir_if_needed function to create the breakpad
//    directory if not already existing.
//  This function does not use the System/TestSystem infrastructure and does
//    ignores the testroot, instead trying to mkdir at the system root "/".
//    This has the same result as if the HOME directory path was not writable.
//  TEST(CrashSystem, validatePaths_nonWritableHomeDir) {
//       BUILD_TESTSYSTEM32
//       makeCaBundle(testDir);
//       makeCrashService(testDir);
//       EXPECT_FALSE(CrashSystem::get()->validatePaths());
//}
