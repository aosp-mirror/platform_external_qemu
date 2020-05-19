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

#include "device_config.h"
#include "android/utils/file_io.h"

extern uint16_t android_modem_simulator_port;
// this file provide emulator hooks
namespace cvd {
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
    // TODO:
  return std::string(file_name);
}

std::string DeviceConfig::DefaultHostArtifactsPath(const std::string& file) {
    // TODO:
  return std::string(file);
}

const char* DeviceConfig::ril_address_and_prefix() {
    // TODO:
    return "192.168.90.6";
}

const char* DeviceConfig::ril_gateway() {
    // TODO:
    return "192.168.90.1";
}
const char* DeviceConfig::ril_dns() {
    // TODO:
    return "8.8.8.8";
}

} // namespace modem
} // namespace cvd
