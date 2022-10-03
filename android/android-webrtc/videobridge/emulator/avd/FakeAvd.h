// Copyright (C) 2021 The Android Open Source Project
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
#include <unordered_map>  // for unordered_map
#include "android/emulation/control/utils/EmulatorGrcpClient.h"

using android::emulation::control::EmulatorGrpcClient;
using Properties = std::unordered_map<std::string, std::string>;

namespace android {
namespace emulation {
namespace control {

// A FakeAvd creates a set of ini files that android studio recognizes as valid
// avds. the fake avd obtains the configuration from the remote emulator and
// writes out a proper configuration for it.
//
// A Fake avd will have a "fake_avd_pid" file in the AVD directory that is used
// to identify fake avds.
//
// A Fake avd will be named:
// remote_[grpc_address]_[remote_id], for example:
// remote_35.23.22.12:9554_P
// would indicate fake avd connected to 35.23.22.12 on port 9554, where the id P
// is running.
class FakeAvd {
public:
    FakeAvd(EmulatorGrpcClient* fromRemoteAvd);
    ~FakeAvd();

    // Removes this fake avd, cleans up the .ini & .avd dir.
    void remove();

    // Create a fake avd based upon the configuration of the remote emulator.
    bool create();

    // Removes all fake avds for which we do not have a corresponding discovery
    // file. This cleans up fake avds that were not properly deleted.
    void garbageCollect();

    std::string name() { return mRemoteProperties["avd.name"]; };
    std::string dir() { return mRemoteProperties["avd.dir"]; };
    std::string id() { return mRemoteProperties["avd.id"]; };

private:
    bool retrieveRemoteProperties();
    void deleteRecursive(const std::string& path);

    std::string mAvdId;
    Properties mRemoteProperties;
    EmulatorGrpcClient* mClient;
    static std::string sPIDFILE;
    static std::string sFAKE_PREFIX;
};
}  // namespace control
}  // namespace emulation
}  // namespace android
