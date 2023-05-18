// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/network/wifi.h"

#include "android/emulation/HostapdController.h"
#include "android/emulation/control/adb/AdbInterface.h"
#include "android/network/NetworkPipe.h"
#include "android/utils/debug.h"
#include "host-common/FeatureControl.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <regex>

namespace fc = android::featurecontrol;
using android::emulation::HostapdController;

static int send_command(const char* fmt, ...) {
    char buffer[1024];
    va_list args;

    va_start(args, fmt);
    int status = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (status < 0) {
        return -status;
    }
    size_t size = static_cast<size_t>(status);
    if (size >= sizeof(buffer)) {
        return -ENOSPC;
    }
    int sent = android::network::sendNetworkCommand(buffer, size);
    if (sent > 0 && static_cast<size_t>(sent) == size) {
        return 0;
    }
    return -1;
}

extern "C"
int android_wifi_add_ssid(const char* ssid, const char* password) {
    if (fc::isEnabled(fc::Wifi)) {
        return send_command("wifi add %s %s\n", ssid,
                            (password && *password) ? password : "");
    } else if (fc::isEnabled(fc::VirtioWifi)) {
        auto* hostapd = HostapdController::getInstance();
        return hostapd->setSsid(ssid, (password && *password) ? password : "")
                       ? 0
                       : -1;
    } else {
        return -1;
    }
}

extern "C"
int android_wifi_set_ssid_block_on(const char* ssid, bool blocked) {
    return send_command("wifi %s %s\n", blocked ? "block" : "unblock", ssid);
}

extern "C" int android_wifi_set_mac_address(const char* mac_addr) {
    if (!mac_addr)
        return -1;
    std::regex regex("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$");
    if (!std::regex_match(mac_addr, regex)) {
        dwarning("%s: Mac address is invalid: %s.\n", __func__, mac_addr);
        return -1;
    }
    auto adbInterface = android::emulation::AdbInterface::getGlobal();
    if (!adbInterface) {
        dwarning("%s: No adb binary found, cannot set the wifi mac address.\n",
                 __func__);
        return -1;
    }
    adbInterface->enqueueCommand(
            {"shell", "su", "0", "ip", "link", "set", "dev", "wlan0", "down"});
    adbInterface->enqueueCommand({"shell", "su", "0", "ip", "link", "set",
                                  "dev", "wlan0", "address", mac_addr});
    adbInterface->enqueueCommand(
            {"shell", "su", "0", "ip", "link", "set", "wlan0", "up"});
    adbInterface->enqueueCommand({"shell", "su", "0", "svc", "wifi", "enable"});
    adbInterface->enqueueCommand({"shell", "su", "0", "pkill", "dhcpclient"});
    return 0;
}
