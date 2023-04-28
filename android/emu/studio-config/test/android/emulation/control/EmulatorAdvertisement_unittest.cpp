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

#include <gtest/gtest.h>
#include <chrono>
#include <fstream>
#include <functional>
#include <optional>
#include <vector>

#include "aemu/base/EnumFlags.h"
#include "aemu/base/Log.h"

#include "aemu/base/files/IniFile.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/process/Command.h"
#include "aemu/base/sockets/ScopedSocket.h"
#include "aemu/base/sockets/SocketUtils.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestTempDir.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/path.h"

#ifndef _WIN32
#include <sys/wait.h>
#endif

using android::ConfigDirs;
using android::base::Optional;
using android::base::PathUtils;
using android::base::pj;
using android::base::System;
using android::base::TestTempDir;
using namespace std::chrono;
using android::base::Command;

/* Tests rely on non-existing pids, linux declares a max for 4 million. All
os'es recycle pids, so unlikely to hit this limit.
 */
const int PID_MAX_LIMIT = 4 * 1024 * 1024;

namespace android {
namespace emulation {
namespace control {

class EmulatorAdvertisementTest : public testing::Test {
protected:
    EmulatorAdvertisementTest() {}

    void TearDown() override {
        if (mTesterProc) {
            LOG(INFO) << "Terminating tester.";
            mTesterProc->terminate();
        }
    }

    void write_hello(std::string fname) {
        std::ofstream hello(fname, std::ios_base::binary);
        hello << "hello=world";
        hello.close();
    }

    std::optional<android::base::Pid> launchInBackground(std::string dir = "",
                                                         int sleepMs = 100) {
        std::string executable =
                System::get()->findBundledExecutable(kDiscovery);
        std::vector<std::string> cmdArgs{executable};
        auto cmd = Command::create({executable, "--sleep", std::to_string(sleepMs) + "ms"});
        if (!dir.empty()) {
            cmd.arg("--discovery");
            cmd.arg(dir);
        }


        mTesterProc = cmd.execute();
        if (!mTesterProc) {
            // Failed to start Mission abort!
            LOG(INFO) << "Failed to start (does the emulator binary exist?): " << executable;
            return std::nullopt;
        }

        return mTesterProc->pid();
    }

    bool waitForPred(std::function<bool()> pred) {
        auto maxWait = system_clock::now() + kPauseForLaunch;
        while (system_clock::now() < maxWait && !pred()) {
            std::this_thread::sleep_for(kWaitIntervalMs);
        }
        return pred();
    }

    bool waitForPid(std::string dir, int pid) {
        auto path = pj(dir, "pid_" + std::to_string(pid) + ".ini");
        LOG(INFO) << "Waiting for   : " << path;
        return waitForPred(
                [&path]() { return System::get()->pathIsFile(path); });
    }

    TestTempDir mTempDir{"DiscoveryTest"};
    std::unique_ptr<base::ObservableProcess> mTesterProc;
    const std::string kDiscovery = "studio_discovery_emulator_tester";
    const seconds kPauseForLaunch{10};
    const std::chrono::milliseconds kWaitIntervalMs{5};
};

TEST_F(EmulatorAdvertisementTest, cleans_two_files) {
    // PIDs are positive numbers ;-)
    int pid = PID_MAX_LIMIT + 10;
    std::string hi =
            pj(mTempDir.path(), "pid_" + std::to_string(pid++) + ".ini");
    std::string hi2 =
            pj(mTempDir.path(), "pid_" + std::to_string(pid++) + ".ini");
    write_hello(hi);
    write_hello(hi2);
    EmulatorAdvertisement testCfg({}, mTempDir.path());
    EXPECT_EQ(2, testCfg.garbageCollect());
};

TEST_F(EmulatorAdvertisementTest, cleans_two_directories) {
    // PIDs are positive numbers ;-)
    int pid = PID_MAX_LIMIT + 10;
    std::string dir = pj(mTempDir.path(), std::to_string(pid++));
    std::string dir2 = pj(mTempDir.path(), std::to_string(pid++));
    path_mkdir_if_needed(dir.c_str(), 0700);
    path_mkdir_if_needed(dir2.c_str(), 0700);
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
    EmulatorAdvertisement testCfg({{"hello", "world"}}, mTempDir.path());
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
    }
}

TEST_F(EmulatorAdvertisementTest, no_pid_no_discovery) {
    EmulatorAdvertisement testCfg({}, mTempDir.path());
    EXPECT_TRUE(testCfg.discoverRunningEmulators().empty());
}

TEST_F(EmulatorAdvertisementTest, my_file_is_alive) {
    auto tst1 = pj(mTempDir.path(), "tst1.ini");
    EXPECT_TRUE(OpenPortChecker().isAlive(tst1, tst1));
}

TEST_F(EmulatorAdvertisementTest, bad_pid_file_not_alive) {
    auto tst1 = pj(mTempDir.path(), "tst1.ini");
    EXPECT_FALSE(PidChecker().isAlive(tst1, tst1));
}

TEST_F(EmulatorAdvertisementTest, wrong_pid_file_not_alive) {
    auto ignored = pj(mTempDir.path(), "pid_123.ini");
    // 999999 > maxpid (Usually 32768)
    auto tst1 = pj(mTempDir.path(), "pid_999999.ini");
    EXPECT_FALSE(PidChecker().isAlive(ignored, tst1));
}

TEST_F(EmulatorAdvertisementTest, do_not_delete_emulator_like_process) {
    auto pid = launchInBackground(mTempDir.path(), 1000);
    EXPECT_TRUE(pid);

    // Make sure the process has finished writing the ini file.
    EXPECT_TRUE(waitForPid(mTempDir.path(), *pid));

    auto tst1 = pj(mTempDir.path(),
                   std::string("pid_") + std::to_string(*pid) + ".ini");
    base::IniFile ini(tst1);
    ini.setInt64("grpc.port", 123);
    ini.write();

    auto ignored = pj(mTempDir.path(), "pid_123.ini");
    EXPECT_TRUE(PidChecker().isAlive(ignored, tst1));
}

TEST_F(EmulatorAdvertisementTest, open_port_is_alive) {
    auto tst1 = pj(mTempDir.path(), "tst1.ini");
    base::IniFile ini(tst1);
    auto socket = base::ScopedSocket(base::socketTcp4LoopbackServer(0));
    EXPECT_TRUE(socket.valid());

    ini.setInt64("grpc.port", base::socketGetPort(socket.get()));
    ini.write();
    EXPECT_TRUE(OpenPortChecker().isAlive("foo", tst1));
}

TEST_F(EmulatorAdvertisementTest, open_port_different_file_is_dead) {
    // Validate the case where my discoery file mentions
    // the same ports as another (which is impossible).
    // This can happen if the old emulator crashed with the default ports in
    // pid_file_old and we wrote out pid_file_new talking the same default
    // ports.
    auto tst1 = pj(mTempDir.path(), "tst1.ini");
    auto tst2 = pj(mTempDir.path(), "tst2.ini");
    base::IniFile ini(tst1);
    base::IniFile ini2(tst2);
    auto socket = base::ScopedSocket(base::socketTcp4LoopbackServer(0));
    EXPECT_TRUE(socket.valid());

    ini.setInt64("grpc.port", base::socketGetPort(socket.get()));
    ini2.setInt64("grpc.port", base::socketGetPort(socket.get()));
    ini.write();
    ini2.write();

    EXPECT_FALSE(OpenPortChecker().isAlive(tst1, tst2));
}

TEST_F(EmulatorAdvertisementTest, DISABLED_pid_file_is_cleaned_in_shared_dir) {
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
