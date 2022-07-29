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
#include <string>             // for string
#include <vector>             // for vector

#include "emulator/webrtc/export.h"
#include "nlohmann/json.hpp"  // for json

namespace android {
namespace emulation {
namespace control {

using nlohmann::json;

class TurnConfig {
public:
    ANDROID_WEBRTC_EXPORT explicit TurnConfig(std::string cmd);
    ANDROID_WEBRTC_EXPORT ~TurnConfig() = default;

    ANDROID_WEBRTC_EXPORT json getConfig();
    ANDROID_WEBRTC_EXPORT bool validCommand();

    ANDROID_WEBRTC_EXPORT static bool producesValidTurn(std::string cmd);
private:
    std::vector<std::string> mTurnCmd;

    // We want the turn config delivered in under a second.
    const int kMaxTurnConfigTime = 1000;
};
}
}
}
