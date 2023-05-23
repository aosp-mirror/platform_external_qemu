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
#include "android/emulation/control/TurnConfig.h"

#include "aemu/base/Log.h"
#include "aemu/base/Optional.h"          // for Optional
#include "aemu/base/ProcessControl.h"    // for parseEscapedLaunchString
#include "android/base/system/System.h"  // for System, System::ProcessExit...

namespace android {
namespace emulation {
namespace control {

using android::base::System;

bool TurnConfig::producesValidTurn(std::string cmd) {
    return TurnConfig(cmd).validCommand();
}

static bool hasIceServers(json config) {
    return !config.is_discarded() &&
           (config.count("iceServers") || config.count("ice_servers"));
}

TurnConfig::TurnConfig(std::string cmd)
    : mTurnCmd(android::base::parseEscapedLaunchString(cmd)) {}

int TurnConfig::getMaxTurnCfgTime() {
    int res = kMaxTurnConfigTime;
    const auto str = System::get()->envGet("ANDROID_EMU_MAX_TURNCFG_TIME");
    if (!str.empty()) {
        if (sscanf(str.c_str(), "%d", &res) == 1) {
            LOG(DEBUG) << "TurnCFG: Use max turn config time from "
                          "ANDROID_EMU_MAX_TURNCFG_TIME: "
                       << str << " ms";
        } else {
            LOG(WARNING) << "TurnCFG: Failed to parse max turn config time: "
                         << str;
            res = kMaxTurnConfigTime;
        }
    }
    return res;
}

json TurnConfig::getConfig() {
    json turnConfig = "{}"_json;
    if (mTurnCmd.size() > 0) {
        System::ProcessExitCode exitCode;
        auto turn = System::get()->runCommandWithResult(
                mTurnCmd, getMaxTurnCfgTime(), &exitCode);
        if (exitCode == 0 && turn) {
            json config = json::parse(*turn, nullptr, false);
            if (hasIceServers(config)) {
                turnConfig = config;
            } else {
                LOG(ERROR) << "Unusable turn config: " << turn;
            }
        }
    }
    return turnConfig;
}

bool TurnConfig::validCommand() {
    if (mTurnCmd.empty()) {
        return true;
    }

    LOG(INFO) << "TurnCFG: --- Command --- ";
    for (auto param : mTurnCmd) {
        LOG(INFO) << "TurnCFG: param:" << param;
    }
    LOG(INFO) << "TurnCFG: --- Running --- ";

    android::base::System::ProcessExitCode exitCode = -1;
    auto turn = android::base::System::get()->runCommandWithResult(
            mTurnCmd, getMaxTurnCfgTime(), &exitCode);

    if (turn) {
        json config = json::parse(*turn, nullptr, false);
        if (exitCode == 0 && !config.is_discarded()) {
            if (hasIceServers(config)) {
                LOG(INFO) << "TurnCFG: Turn command produces valid json.";
                return true;
            } else {
                LOG(ERROR) << "TurnCFG: JSON is missing iceServers/ice_servers "
                              "tag, received:"
                           << config;
            }
        } else {
            if (exitCode != 0) {
                LOG(ERROR) << "TurnCFG: bad exit: " << exitCode;
            } else {
                LOG(ERROR) << "TurnCFG: Bad json result: " << *turn;
            }
        }
    } else {
        LOG(ERROR) << "TurnCFG: Produces no result, exit: " << exitCode;
    }

    return false;
}
}  // namespace control
}  // namespace emulation
}  // namespace android
