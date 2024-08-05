// Copyright (C) 2024 The Android Open Source Project
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

#include "android-qemu2-glue/utils/register_crash_info.h"
#include "android/avd/info.h"
#include "android/base/files/IniFile.h"
#include "android/base/system/System.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/metrics/MetricsReporter.h"
#include "host-common/crash-handler.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using android ::base::System;
using crashinfo = std::unordered_map<std::string, std::string>;
using android::base::IniFile;

crashinfo collectCrashInfo(AvdInfo* avd, const std::string& sessionId) {
    crashinfo info;
    if (avd == nullptr) {
        return info;
    }

    auto avdCfgIni = avdInfo_getConfigIni(avd);
    // Only collect the information if it is available.
    if (avdCfgIni == nullptr) {
        return info;
    }

    const auto* cfgIni = reinterpret_cast<const IniFile*>(avdCfgIni);
    std::vector<const char*> literals{
            "disk.dataPartition.size",
            "hw.cpu.ncore",
            "hw.gpu.enabled",
            "hw.gpu.mode",
            "hw.lcd.height",
            "hw.lcd.width",
            "hw.ramSize",
            "hw.sensor.hinge",
            "PlayStore.enabled",
    };

    for (auto param : literals) {
        info[param] = cfgIni->getString(param, "?");
    }

    info["cpu_cores"] = std::to_string(System::get()->getCpuCoreCount());
    info["free_ram"] = std::to_string(System::get()->freeRamMb());

    // Path to core hw ini path, this is usually the location where the avd,
    // snapshots etc, are stored.
    System::FileSize availableSpace;
    System::get()->pathFreeSpace(avdInfo_getCoreHwIniPath(avd),
                                 &availableSpace);
    info["avd_space"] = std::to_string(availableSpace);

    // Now retrieve the build.prop if they are available.
    auto data = avdInfo_getBuildProperties(avd);
    if (data == nullptr) {
        return info;
    }

    IniFile buildprop(reinterpret_cast<char*>(data->data), data->size);
    info["ro.build.fingerprint"] =
            buildprop.getString("ro.build.fingerprint", "??");

    // The emulator sessionId that crashed.
    info["sessionId"] = sessionId;
    return info;
}

void registerCrashInfo() {
    auto info = collectCrashInfo(
            getConsoleAgents()->settings->avdInfo(),
            android::metrics::MetricsReporter::get().sessionId());

    for (const auto& [key, value] : info) {
        LOG(DEBUG) << "Crash info: " << key << "=" << value;
        crashhandler_add_string(key.c_str(), value.c_str());
    }
}
