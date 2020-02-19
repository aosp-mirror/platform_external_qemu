// Copyright (C) 2020 The Android Open Source Project
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
#include "android/emulation/control/EmulatorAdvertisement.h"

#include <gtest/gtest.h>                       // for AssertionResult, Message
#include <chrono>                              // for operator+, operator<
#include <fstream>                             // for ofstream, operator<<
#include <functional>                          // for function, __base
#include <vector>                              // for vector

#include "android/base/EnumFlags.h"            // for operator|
#include "android/base/Log.h"                  // for LogStream, LOG, LogMes...
#include "android/base/Optional.h"             // for Optional
#include "android/base/StringView.h"           // for StringView
#include "android/base/files/PathUtils.h"      // for pj, PathUtils (ptr only)
#include "android/base/system/System.h"        // for System, System::Pid
#include "android/base/testing/TestTempDir.h"  // for TestTempDir
#include "android/emulation/ConfigDirs.h"      // for ConfigDirs

#ifndef _WIN32
#include <sys/wait.h>                          // for waitpid
#endif

using android::ConfigDirs;
using android::base::Optional;
using android::base::PathUtils;
using android::base::pj;
using android::base::System;
using android::base::TestTempDir;
using namespace std::chrono;

/* Tests rely on non-existing pids, linux declares a max for 4 million. All os'es recycle pids, so
unlikely to hit this limit.
 */
const int PID_MAX_LIMIT = 4 * 1024 * 1024;

namespace android {
namespace emulation {
namespace control {

class EmulatorAdvertisementTest : public testing::Test {
protected:
    EmulatorAdvertisementTest() {}

    void write_hello(std::string fname) {
        std::ofstream hello(fname, std::ios_base::binary);
        hello << "hello=world";
        hello.close();
    }

    Optional<System::Pid> launchInBackground(std::string dir = "") {
        std::string executable =
                System::get()->findBundledExecutable(kDiscovery);
        std::vector<std::string> cmdArgs{executable};
        if (!dir.empty()) {
            cmdArgs.push_back(dir);
        }
        System::Pid exePid = -1;
        std::string invoke = "";
        for (auto arg : cmdArgs) {
            invoke += arg + " ";
        }

        LOG(INFO) << "Launching: " << invoke << "&";
        if (!System::get()->runCommand(cmdArgs,
                                       System::RunOptions::DontWait |
                                               System::RunOptions::ShowOutput,
                                       System::kInfinite, nullptr, &exePid)) {
            // Failed to start Mission abort!
            LOG(INFO) << "Failed to start " << invoke;
            return {};
        }

        return exePid;
    }

    bool waitForPred(std::function<bool()> pred) {
        auto maxWait = system_clock::now() + kPauseForLaunch;
        while (system_clock::now() < maxWait && !pred()) {
            System::get()->sleepMs(kWaitIntervalMs);
        }
        return pred();
    }

    bool waitForPid(std::string dir, int pid) {
        auto path = pj(dir, "pid_" + std::to_string(pid) + ".ini");
        return waitForPred(
                [&path]() { return System::get()->pathIsFile(path); });
    }

    TestTempDir mTempDir{"DiscoveryTest"};

    const std::string kDiscovery = "studio_discovery_tester";
    const seconds kPauseForLaunch{10};
    const int kWaitIntervalMs = 5;
};

TEST_F(EmulatorAdvertisementTest, cleans_two_files) {
    // PIDs are positive numbers ;-)
    int pid = PID_MAX_LIMIT + 10;
    std::string hi = pj(mTempDir.path(), "pid_" + std::to_string(pid++) + ".ini");
    std::string hi2 = pj(mTempDir.path(), "pid_" + std::to_string(pid++) + ".ini");
    write_hello(hi);
    write_hello(hi2);
    EmulatorAdvertisement testCfg({}, mTempDir.path());
    EXPECT_EQ(2, testCfg.garbageCollect());
};

TEST_F(EmulatorAdvertisementTest, writes_a_dictionary) {
    EmulatorAdvertisement testCfg({{"hello", "world"}}, mTempDir.path());

    EXPECT_FALSE(System::get()->pathExists(testCfg.location()));
    testCfg.write();
    EXPECT_TRUE(System::get()->pathExists(testCfg.location()));

    std::ifstream rstream(testCfg.location());
    std::string data;
    rstream >> data;
    EXPECT_EQ("hello=world", data);
};


TEST_F(EmulatorAdvertisementTest, delete_removes_file) {
   EmulatorAdvertisement testCfg({{"hello", "world"}}, mTempDir.path());
    testCfg.write();
    EXPECT_TRUE(System::get()->pathExists(testCfg.location()));
    testCfg.remove();
    EXPECT_FALSE(System::get()->pathExists(testCfg.location()));
}


TEST_F(EmulatorAdvertisementTest, system_wide_share_file_exists) {
    EmulatorAdvertisement testCfg({{"hello", "world"}});
    testCfg.write();
    EXPECT_TRUE(System::get()->pathExists(testCfg.location()));

    // Let's manually erase this file..
    System::get()->deleteFile(testCfg.location());
}

#ifndef AEMU_GFXSTREAM_BACKEND

TEST_F(EmulatorAdvertisementTest, pid_file_is_written) {
    auto pid = launchInBackground(mTempDir.path());
    EXPECT_TRUE(pid);
    if (pid) {
        EXPECT_TRUE(waitForPid(mTempDir.path(), *pid));
        System::get()->killProcess(*pid);
    }
}

TEST_F(EmulatorAdvertisementTest, pid_file_is_cleaned_in_shared_dir) {
    auto pid = launchInBackground();
    EXPECT_TRUE(pid);
    if (pid) {
        EmulatorAdvertisement discovery({});

        // Clean-up all dead processes we know of..
        discovery.garbageCollect();

        EXPECT_TRUE(waitForPid(ConfigDirs::getDiscoveryDirectory(), *pid));
        System::get()->killProcess(*pid);
#ifndef _WIN32
        // On linux we will have lingering zombies.
        int wstatus = 0;
        waitpid(*pid, &wstatus, 0);
#endif

        // We wrote one discovery file, the pid got killed, so at least one
        // file should be cleaned up.  In theory can fail if you exit
        // a running emulator at the same time. (Note this is not possible on
        // build bots.)
        bool collected = false;
        EXPECT_TRUE(waitForPred([&discovery, &collected] {
            collected = collected || discovery.garbageCollect() > 0;
            return collected;
        }));
    }
}

#endif

}  // namespace control
}  // namespace emulation
}  // namespace android
