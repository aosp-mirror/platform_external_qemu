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

#include "android/network/NetworkPipe.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

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
    return send_command("wifi add %s %s\n",
                        ssid,
                        (password && *password) ? password : "");
}

extern "C"
int android_wifi_set_ssid_block_on(const char* ssid, bool blocked) {
    return send_command("wifi %s %s\n", blocked ? "block" : "unblock", ssid);
}

