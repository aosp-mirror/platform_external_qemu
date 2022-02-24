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
#include "android/featurecontrol/FeatureControl.h"
#include "android/network/NetworkPipe.h"
#include "android/utils/debug.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

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


extern "C"
const char* android_wifi_default_dns_address() {
    // Reference: https://developers.google.com/speed/public-dns/docs/using
    return "2001:4860:4860::8844,2001:4860:4860::8888,8.8.8.8,8.8.4.4";
}
