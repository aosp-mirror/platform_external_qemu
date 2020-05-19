/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/avd/util.h"
#include "android/base/files/PathUtils.h"
#include "android/cmdline-option.h"
#include "android/avd/info.h"
#include "android/avd/hw-config.h"

#include "android/globals.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"
#include "android/utils/system.h"
#include "device_config.h"

//extern AndroidHwConfig   android_hw[1];

extern "C" {
uint16_t android_modem_simulator_port = 0;
}

// this file provide emulator hooks
namespace cuttlefish {
namespace modem {

int DeviceConfig::host_port() {
    // TODO:
    // this is the modem port, right now
    // emulator has 5554 as telnet port
    // 5555 as adb port
    // the host port here is the modem port that can
    // enable two emulators to send sms to each other
    // this has to be given by option: -modem-port <7788>
    // also the modem simulator insists 4 digits only
    return android_modem_simulator_port;
}

std::string DeviceConfig::PerInstancePath(const char* file_name) {
    if (!android_hw->avd_name) {
        return std::string{file_name};
    }
    char* avdPath(path_dirname(android_hw->disk_dataPartition_path));
    if (!avdPath) {
        return std::string{file_name};
    }
    std::string modem_dir =
            android::base::PathUtils::join(avdPath, "modem_simulator");
    std::string file = android::base::PathUtils::join(modem_dir, file_name);
    AFREE(avdPath);
    return file;
}

std::string DeviceConfig::DefaultHostArtifactsPath(const std::string& file) {
    return PerInstancePath(file.c_str());
}

const char* DeviceConfig::ril_address_and_prefix() {
    // hardcoded one in goldfish
    return "10.0.2.15/24";
}

const char* DeviceConfig::ril_gateway() {
    // hardcoded one in goldfish
    return "10.0.2.2";
}

const char* DeviceConfig::ril_dns() {
    // Note: the android_cmdLineOptions->dns_server sometimes is just
    // 127.0.0.1 and it does not work
    // TODO: improve it
    return "8.8.8.8";
}

}  // namespace modem
}  // namespace cuttlefish
