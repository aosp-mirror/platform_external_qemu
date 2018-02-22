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

#include "android/emulation/control/AdbInterface.h"
#include <map>

namespace android {
namespace emulation {

using base::Optional;

// Adb locator that knows all the adbs passed into the constructor.
class StaticAdbLocator : public AdbLocator {
public:
    StaticAdbLocator(std::map<std::string, int> map) : mAdb(map) {
        for (auto const& path : map) {
            mAvailable.push_back(path.first);
        }
    }
    std::vector<std::string> availableAdb() override { return mAvailable; }

    base::Optional<int> getAdbProtocolVersion(StringView adbPath) override {
        if (mAdb.find(adbPath) == mAdb.end())
            return {};
        return base::makeOptional(mAdb[adbPath]);
    }

    std::map<std::string, int> mAdb;
    std::vector<std::string> mAvailable;
};

class FakeAdbDaemon : public AdbDaemon {
 public:
    Optional<int> getProtocolVersion() override { return mVersion; }

    void setProtocolVersion(Optional<int> version) { mVersion = version; }

 private:
    Optional<int> mVersion;
};
}  // namespace emulation
}  // namespace android
