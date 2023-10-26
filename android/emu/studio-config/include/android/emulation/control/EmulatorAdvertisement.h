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
#pragma once

#include <memory>         // for make_unique, unique_ptr
#include <string>         // for string, hash, operator==
#include <unordered_map>  // for unordered_map
#include <vector>         // for vector

#include "aemu/base/Compiler.h"          // for DISALLOW_COPY_AND_ASSIGN
#include "android/base/system/System.h"  // for System, System::Pid

namespace android {
namespace emulation {
namespace control {

// A simple emulator configuration that can be shared with external processes.
// Properties are simple string pairs that are written to disk as ini files with
// "key=value" entries.
using EmulatorProperties = std::unordered_map<std::string, std::string>;

// Strategy pattern for checking if the emulator corresponding
// the discovery file is actually alive.
//
// Mainly here so you can write proper unit tests.
class EmulatorLivenessStrategy {
public:
    virtual ~EmulatorLivenessStrategy(){};
    virtual bool isAlive(std::string myFile,
                         std::string discoveryFile) const = 0;
};

// Liveness checker that tries to load the discovery file
// and tries to see if any of the declared ports are accessible
//
/// NOTE: this can be very slow, so best not to use it.
class OpenPortChecker : public EmulatorLivenessStrategy {
public:
    bool isAlive(std::string myFile, std::string discoveryFile) const override;
};

// Liveness checker that tries to load the discovery file
// and tries to see that the pid exists and has the proper name
// (i.e. contains: "emulator", or "qemu-system-")
class PidChecker : public EmulatorLivenessStrategy {
public:
    bool isAlive(std::string myFile, std::string discoveryFile) const override;
};

// External services might need to know where to find information about
// running emulators. An EmulatorAdvertisement can write an
// EmulatorConfiguration dictionary to a
// predefined location.
//
// The location is defined as follows:
//
// <user-specific_tmp_directory>/avd/running  where the
// user-specific_tmp_directory is:
//  - $XDG_RUNTIME_DIR on Linux,
//  - $HOME/Library/Caches/TemporaryItems on Mac
//  - %LOCALAPPDATA%/Temp on Windows.
//
// The file will be named "pid_%d_info.ini" where %d is
// the process id of the emulator.
class EmulatorAdvertisement {
public:
    explicit EmulatorAdvertisement(
            EmulatorProperties&& config,
            std::unique_ptr<EmulatorLivenessStrategy> livenessChecker =
                    std::make_unique<PidChecker>());
    EmulatorAdvertisement(
            EmulatorProperties&& config,
            std::string sharedDirectory,
            std::unique_ptr<EmulatorLivenessStrategy> livenessChecker =
                    std::make_unique<PidChecker>());
    ~EmulatorAdvertisement();

    // The location where the .ini file will be written to.
    std::string location() const;

    // Writes the ini file to the location.
    bool write() const;

    // Removes the file from the file system.
    void remove() const;

    // True if a advertisement exists for the given pid.
    static bool exists(base::System::Pid pid);

    // Deletes all ini files in <user-specific_tmp_directory>/avd/running and
    // directories <user-specific_tmp_directory>/avd/running/<pid> for
    // which no corresponding process exists. returns the number of files
    // deleted.
    int garbageCollect() const;

    // Discovers all the advertisement files of active emulators, excluding us.
    std::vector<std::string> discoverRunningEmulators();

    // Discovers the first advertisment file of active emulators that
    // has the set of props available.
    std::string discoverEmulatorWithProperties(EmulatorProperties props);

private:
    DISALLOW_COPY_AND_ASSIGN(EmulatorAdvertisement);

    EmulatorProperties mStudioConfig;
    std::string mSharedDirectory;
    std::unique_ptr<EmulatorLivenessStrategy> mLivenessChecker;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
