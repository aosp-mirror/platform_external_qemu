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

#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <chrono>
#include <fstream>
#include <initializer_list>
#include <utility>
#include <vector>

#include "aemu/base/Log.h"
#include "aemu/base/StringFormat.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/logging/LogSeverity.h"
#include "aemu/base/process/Process.h"
#include "aemu/base/sockets/ScopedSocket.h"
#include "aemu/base/sockets/SocketUtils.h"
#include "android/base/files/IniFile.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/path.h"

namespace android {
namespace emulation {
namespace control {

#define DEBUG 0
/* set  for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)0
#else
#include "android/utils/debug.h"
#define DD(...) dinfo(__VA_ARGS__);
#endif

using android::base::PathUtils;
using android::base::Process;
using android::base::System;
static const char* location_format = "pid_%d.ini";

static bool isMe(std::string discoveryFile) {
    std::string entry(PathUtils::decompose(discoveryFile).back());
    int pid = 0;
    if (sscanf(entry.c_str(), location_format, &pid) != 1) {
        // Not a discovery file..
        return false;
    }
    return pid == Process::me()->pid();
}

static bool canConnectToPort(int64_t port) {
    if (port == 0) {
        return false;
    }
    base::ScopedSocket fd(base::socketTcp6LoopbackClient(port));
    if (fd.valid()) {
        return true;
    }
    fd = android::base::socketTcp4LoopbackClient(port);
    if (fd.valid()) {
        return true;
    }

    return false;
}

// Liveness checker that tries to load the discovery file
// and tries to see if any of the declared ports are accessible
// and validates that the ports do not point to me.
bool OpenPortChecker::isAlive(std::string myFile,
                              std::string discoveryFile) const {
    if (myFile == discoveryFile) {
        return true;
    }

    DD("Checking liveness of entry %s", discoveryFile.c_str());
    base::IniFile ini(discoveryFile);
    base::IniFile me(myFile);
    if (!ini.read()) {
        DD("Invalid ini file: %s", discoveryFile.c_str());
        return false;
    }

    if (!System::get()->pathExists(myFile) || !me.read()) {
        DD("Invalid ini file: %s (that's ok)", myFile.c_str());
    }

    // Check if we can connect to any of the ports that are defined in the
    // discovery file.. If we can, then we are alive..
    for (const auto& port : {"grpc.port", "port.serial", "port.adb"}) {
        auto checkPort = ini.getInt64(port, 0);
        auto myPort = me.getInt64(port, 0);

        if (myPort != 0 && myPort == checkPort) {
            DD("Imposter! Your ini file contains ports that are owned by me!");
            return false;
        }

        if (checkPort != 0 && canConnectToPort(checkPort)) {
            DD("Able to connect to port %d", checkPort);
            return true;
        }
    }

    return false;
}

bool PidChecker::isAlive(std::string myFile, std::string discoveryFile) const {
    // Check to see if the process is alive
    std::string entry(PathUtils::decompose(discoveryFile).back());
    int pid = 0;
    if (System::get()->pathIsFile(discoveryFile)) {
        if (sscanf(entry.c_str(), location_format, &pid) != 1) {
            // Not a discovery file..
            return false;
        }
    }

    if (System::get()->pathIsDir(discoveryFile)) {
        if (sscanf(entry.c_str(), "%d", &pid) != 1) {
            // Not a discovery dir..
            return false;
        }
    }

    DD("Checking liveness of %d", pid);
    DD("I'm an emulator proc, my name is: %s (%d)",
       Process::me()->exe().c_str(), Process::me()->pid());

    // Maybe it is me, i'm alive!
    if (Process::me()->pid() == pid) {
        return true;
    }

    auto proc = Process::fromPid(pid);
    if (!proc) {
        // tsk tsk process is not alive.
        DD("pid: %d is not alive", pid);
        return false;
    }

    auto name = proc->exe();
    if (name.find("emulator") != std::string::npos ||
        name.find("qemu-system-") != std::string::npos) {
        // It's a qemu or emulator process.. Let's keep it alive.
        DD("%s is an emulator process, so the pid file is alive.",
           name.c_str());
        return true;
    }

    DD("No idea what %s is.. you are dead.", name.c_str());
    return false;
}

EmulatorAdvertisement::EmulatorAdvertisement(
        EmulatorProperties&& config,
        std::unique_ptr<EmulatorLivenessStrategy> livenessChecker)
    : mStudioConfig(config), mLivenessChecker(std::move(livenessChecker)) {
    mSharedDirectory = android::ConfigDirs::getDiscoveryDirectory();
    if (!System::get()->pathExists(mSharedDirectory)) {
        LOG(WARNING) << "Discovery directory: " << mSharedDirectory
                     << ", does not exist. creating";
        path_mkdir_if_needed(mSharedDirectory.data(), 0700);
    }
    assert(System::get()->pathExists(mSharedDirectory));
}

EmulatorAdvertisement::EmulatorAdvertisement(
        EmulatorProperties&& config,
        std::string sharedDirectory,
        std::unique_ptr<EmulatorLivenessStrategy> livenessChecker)
    : mStudioConfig(config),
      mSharedDirectory(sharedDirectory),
      mLivenessChecker(std::move(livenessChecker)) {
    if (!System::get()->pathExists(mSharedDirectory)) {
        LOG(WARNING) << "Discovery directory: " << mSharedDirectory
                     << ", does not exist. creating";
        path_mkdir_if_needed(mSharedDirectory.data(), 0700);
    }
    assert(System::get()->pathExists(mSharedDirectory));
}

EmulatorAdvertisement::~EmulatorAdvertisement() {
    garbageCollect();
}


int EmulatorAdvertisement::garbageCollect() const {
    auto start = std::chrono::high_resolution_clock::now();
    DD("Starting garbage collection of advertisement.");
    int collected = 0;
    for (const auto& entry :
         System::get()->scanDirEntries(mSharedDirectory, true)) {
        DD("Checking: %s", entry.c_str());
        if (!mLivenessChecker->isAlive(location(), entry)) {
            DD("Deleting %s", entry.c_str());
            collected++;
            // Emulator is not running, or unreachable.
            if (System::get()->pathIsFile(entry)) {
                System::get()->deleteFile(entry);
            }
            if (System::get()->pathIsDir(entry)) {
                path_delete_dir(entry.c_str());
            }
        }
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start);
    DD("It took %lld ms to collect %d leftovers", duration.count(), collected);
    return collected;
}

std::vector<std::string> EmulatorAdvertisement::discoverRunningEmulators() {
    DD("Scanning %s", mSharedDirectory.c_str());
    std::vector<std::string> discovered;
    for (const auto& entry :
         System::get()->scanDirEntries(mSharedDirectory, true)) {
        if (entry != location() &&
            mLivenessChecker->isAlive(location(), entry)) {
            discovered.push_back(entry);
        }
    }

    return discovered;
}
std::string EmulatorAdvertisement::discoverEmulatorWithProperties(
        EmulatorProperties props) {
    for (const auto& discoveryFile : discoverRunningEmulators()) {
        base::IniFile ini(discoveryFile);
        if (!ini.read())
            continue;

        bool match = true;
        for (const auto [key, val] : props) {
            match = match && ini.hasKey(key) && ini.getString(key, "") == val;
        }
        if (match)
            return discoveryFile;
    }

    return "";
}

void EmulatorAdvertisement::remove() const {
    System::get()->deleteFile(location());
    auto pid_dir = android::base::pj(mSharedDirectory,
                                     std::to_string(Process::me()->pid()));
    if (System::get()->pathIsDir(pid_dir)) {
        DD("Deleting my pid dir %s", pid_dir.c_str());
        path_delete_dir(pid_dir.c_str());
    }
}

std::string EmulatorAdvertisement::location() const {
    auto pid = System::get()->getCurrentProcessId();
    std::string pidfile = android::base::StringFormat(location_format, pid);
    std::string result = android::base::pj(mSharedDirectory, pidfile);
    return result;
}

// True if a advertisement exists for the given pid.
bool EmulatorAdvertisement::exists(System::Pid pid) {
    std::string pidfile = android::base::StringFormat(location_format, pid);
    std::string pidPath = android::base::pj(
            android::ConfigDirs::getDiscoveryDirectory(), pidfile);
    return System::get()->pathIsFile(pidPath);
}

bool EmulatorAdvertisement::write() const {
    auto pidFile = location();
    if (System::get()->pathExists(pidFile)) {
        LOG(WARNING) << "Overwriting existing discovery file: " << pidFile;
    }
    LOG(INFO) << "Advertising in: " << pidFile;
    auto shareFile =
            std::ofstream(PathUtils::asUnicodePath(pidFile.data()).c_str());
    for (const auto& elem : mStudioConfig) {
        shareFile << elem.first << "=" << elem.second << "\n";
    }
    shareFile.flush();
    shareFile.close();

#ifndef _WIN32
    // To ensure that your files are not removed, they should have their access
    // time timestamp modified at least once every 6 hours of monotonic time or
    // the 'sticky' bit should be set on the file.
    chmod(pidFile.c_str(), S_IRUSR | S_ISVTX | S_IWUSR);
#endif
    return !shareFile.bad();
}

}  // namespace control
}  // namespace emulation
}  // namespace android
