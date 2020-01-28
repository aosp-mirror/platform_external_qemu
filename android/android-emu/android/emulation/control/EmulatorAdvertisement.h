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

#include <string>
#include <unordered_map>

#include "android/base/Compiler.h"

namespace android {
namespace emulation {
namespace control {

// A simple emulator configuration that can be shared with external processes.
// Properties are simple string pairs that are written to disk as ini files with
// "key=value" entries.
using EmulatorProperties = std::unordered_map<std::string, std::string>;

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
    explicit EmulatorAdvertisement(EmulatorProperties&& config);
    EmulatorAdvertisement(EmulatorProperties&& config,
                          std::string sharedDirectory);
    ~EmulatorAdvertisement();

    // The location where the .ini file will be written to.
    std::string location() const;

    // Writes the ini file to the location.
    bool write() const;

    // Removes the file from the file system.
    void remove() const;

    // Deletes all ini files in <user-specific_tmp_directory>/avd/running  for
    // which no corresponding process exists. returns the number of files
    // deleted.
    int garbageCollect() const;

private:
    DISALLOW_COPY_AND_ASSIGN(EmulatorAdvertisement);

    EmulatorProperties mStudioConfig;
    std::string mSharedDirectory;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
