// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/system/System.h"

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/String.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <unistd.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android {
namespace base {

static void make_subfile(const String& dir, const char* file) {
    String path = dir;
    path.append("/");
    path.append(file);
    int fd = ::open(path.c_str(), O_WRONLY|O_CREAT, 0755);
    EXPECT_GE(fd, 0) << "Path: " << path.c_str();
    ::close(fd);
}

TEST(System, get) {
    System* sys1 = System::get();
    EXPECT_TRUE(sys1);

    System* sys2 = System::get();
    EXPECT_EQ(sys1, sys2);
}

TEST(System, getProgramDirectory) {
    String dir = System::get()->getProgramDirectory();
    EXPECT_FALSE(dir.empty());
    LOG(INFO) << "Program directory: [" << dir.c_str() << "]";
}

TEST(System, getHomeDirectory) {
    String dir = System::get()->getHomeDirectory();
    EXPECT_FALSE(dir.empty());
    LOG(INFO) << "Home directory: [" << dir.c_str() << "]";
}

TEST(System, getAppDataDirectory) {
    String dir = System::get()->getAppDataDirectory();
#if defined(__linux__)
    EXPECT_TRUE(dir.empty());
#else
    // Mac OS X, Microsoft Windows
    EXPECT_FALSE(dir.empty());
#endif // __linux__
    LOG(INFO) << "AppData directory: [" << dir.c_str() << "]";
}

// Tests case where program directory == launcher directory (QEMU1)
TEST(TestSystem, getDirectory) {
    const char kLauncherDir[] = "/foo/bar";
    const char  kHomeDir[]    = "/mama/papa";
#if defined(__linux__)
    const char *kAppDataDir   = "";
#else
    // Mac OS X, Microsoft Windows
    const char  kAppDataDir[] = "/lala/kaka";
#endif // __linux__
    TestSystem testSys(kLauncherDir, 32, kHomeDir, kAppDataDir);
    String ldir = System::get()->getLauncherDirectory();
    EXPECT_STREQ(kLauncherDir, ldir.c_str());
    String pdir = System::get()->getProgramDirectory();
    EXPECT_STREQ(kLauncherDir, pdir.c_str());
    String hdir = System::get()->getHomeDirectory();
    EXPECT_STREQ(kHomeDir, hdir.c_str());
    String adir = System::get()->getAppDataDirectory();
#if defined(__linux__)
    EXPECT_TRUE(adir.empty());
#else
    // Mac OS X, Microsoft Windows
    EXPECT_STREQ(kAppDataDir, adir.c_str());
#endif  // __linux__
}

// Tests case where program directory is a subdirectory of launcher directory
// (QEMU2)
TEST(TestSystem, getDirectoryProgramDir) {
    const char kLauncherDir[] = "/foo/bar";
    const char kProgramDir[] = "qemu/os-arch";
    TestSystem testSys(kLauncherDir, 32, "/home", "/app");
    testSys.setProgramDir(kProgramDir);

    String ldir = System::get()->getLauncherDirectory();
    EXPECT_STREQ(kLauncherDir, ldir.c_str());
    String pdir = System::get()->getProgramDirectory();
#ifdef _WIN32
    EXPECT_STREQ("/foo/bar\\qemu/os-arch", pdir.c_str());
#else
    EXPECT_STREQ("/foo/bar/qemu/os-arch", pdir.c_str());
#endif
}

TEST(System, getHostBitness) {
    int hostBitness = System::get()->getHostBitness();
    LOG(INFO) << "Host bitness: " << hostBitness;
    EXPECT_TRUE(hostBitness == 32 || hostBitness == 64);

    {
        TestSystem sysTest("/foo", 32);
        EXPECT_EQ(32, System::get()->getHostBitness());
    }
    {
        TestSystem sysTest("/foo64", 64);
        EXPECT_EQ(64, System::get()->getHostBitness());
    }
}

TEST(System, getProgramBitness) {
    const int kExpected = (sizeof(void*) == 8) ? 64 : 32;
    EXPECT_EQ(kExpected, System::get()->getProgramBitness());
}

TEST(System, scandDirEntries) {
    static const char* const kExpected[] = {
        "fifth", "first", "fourth", "second", "sixth", "third"
    };
    static const char* const kInput[] = {
        "first", "second", "third", "fourth", "fifth", "sixth"
    };
    const size_t kCount = ARRAYLEN(kInput);

    TestTempDir myDir("scanDirEntries");
    for (size_t n = 0; n < kCount; ++n) {
        make_subfile(myDir.path(), kInput[n]);
    }

    StringVector entries = System::get()->scanDirEntries(myDir.path());

    EXPECT_EQ(kCount, entries.size());
    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_STREQ(kExpected[n], entries[n].c_str()) << "#" << n;
    }
}

TEST(System, envGetAndSet) {
    System* sys = System::get();
    const char kVarName[] = "FOO_BAR_TESTING_STUFF";
    const char kVarValue[] = "SomethingCompletelyRandomForYou!";

    EXPECT_FALSE(sys->envTest(kVarName));
    EXPECT_STREQ("", sys->envGet(kVarName).c_str());
    sys->envSet(kVarName, kVarValue);
    EXPECT_TRUE(sys->envTest(kVarName));
    EXPECT_STREQ(kVarValue, sys->envGet(kVarName).c_str());
    sys->envSet(kVarName, NULL);
    EXPECT_FALSE(sys->envTest(kVarName));
    EXPECT_STREQ("", sys->envGet(kVarName).c_str());
}

TEST(System, scanDirEntriesWithFullPaths) {
    static const char* const kExpected[] = {
        "fifth", "first", "fourth", "second", "sixth", "third"
    };
    static const char* const kInput[] = {
        "first", "second", "third", "fourth", "fifth", "sixth"
    };
    const size_t kCount = ARRAYLEN(kInput);

    TestTempDir myDir("scanDirEntriesFull");
    for (size_t n = 0; n < kCount; ++n) {
        make_subfile(myDir.path(), kInput[n]);
    }

    StringVector entries = System::get()->scanDirEntries(myDir.path(), true);

    EXPECT_EQ(kCount, entries.size());
    for (size_t n = 0; n < kCount; ++n) {
        String expected(myDir.path());
        expected = PathUtils::addTrailingDirSeparator(expected);
        expected += kExpected[n];
        EXPECT_STREQ(expected.c_str(), entries[n].c_str()) << "#" << n;
    }
}

TEST(System, isRemoteSession) {
    String sessionType;
    bool isRemote = System::get()->isRemoteSession(&sessionType);
    if (isRemote) {
        LOG(INFO) << "Remote session type [" << sessionType.c_str() << "]";
    } else {
        LOG(INFO) << "Local session type";
    }
}

TEST(System, addLibrarySearchDir) {
    TestSystem testSys("/foo/bar", 32);
    TestTempDir* testDir = testSys.getTempRoot();
    ASSERT_TRUE(testDir->makeSubDir("lib"));
    testSys.addLibrarySearchDir("lib");
}

TEST(System, findBundledExecutable) {
#ifdef _WIN32
    static const char kProgramFile[] = "myprogram.exe";
#else
    static const char kProgramFile[] = "myprogram";
#endif

    TestSystem testSys("/foo", System::kProgramBitness);
    TestTempDir* testDir = testSys.getTempRoot();
    ASSERT_TRUE(testDir->makeSubDir("foo"));

    StringVector pathList;
    pathList.push_back(String("foo"));
    pathList.push_back(String(System::kBinSubDir));
    ASSERT_TRUE(testDir->makeSubDir(PathUtils::recompose(pathList).c_str()));

    pathList.push_back(String(kProgramFile));
    String programPath = PathUtils::recompose(pathList);
    make_subfile(testDir->path(), programPath.c_str());

    String path = testSys.findBundledExecutable("myprogram");
    String expectedPath("/");
    expectedPath += programPath;
    EXPECT_STREQ(expectedPath.c_str(), path.c_str());

    path = testSys.findBundledExecutable("otherprogram");
    EXPECT_FALSE(path.size());
}

TEST(System, getProcessTimes) {
    const System::Times times1 = System::get()->getProcessTimes();
    const System::Times times2 = System::get()->getProcessTimes();
    ASSERT_GE(times2.userMs, times1.userMs);
    ASSERT_GE(times2.systemMs, times1.systemMs);
}

TEST(System, getUnixTime) {
    const time_t curTime = time(NULL);
    const time_t time1 = System::get()->getUnixTime();
    const time_t time2 = System::get()->getUnixTime();
    ASSERT_GE(time1, curTime);
    ASSERT_GE(time2, time1);
}

}  // namespace base
}  // namespace android
