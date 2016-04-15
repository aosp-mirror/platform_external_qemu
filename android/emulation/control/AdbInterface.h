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

#pragma once

#include <string>
#include <vector>

// A convenience class used to check the presence of the host 'adb'
// command-line client executable, and whether its version is fresh
// enough to be used by the emulator. Usage is the following:
//
//  1/ Create AdbInterface() instance.
//
//  2/ Call detectedAdbPath() to retrieve the path of the 'adb'
//     executable, which will be empty if it could not be found.
//
//  3/ Call isAdbVersionCurrent(), which returns true if said executable
//     is of a sufficiently recent version.
//
// TODO: Rename this to something less ambiguous, e.g. HostAdbLocator
namespace android {
namespace emulation {

class AdbInterface {
public:
    AdbInterface();

    // Returns true is the ADB version is fresh enough.
    bool isAdbVersionCurrent() const { return mAdbVersionCurrent; }

    // Returns the automatically detected path to adb
    const std::string& detectedAdbPath() const { return mAdbPath; }

private:
    std::string mAdbPath;
    bool mAdbVersionCurrent;
};

}
}
