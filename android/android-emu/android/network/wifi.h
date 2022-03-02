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

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// Create a new SSID inside the guest. |ssid| specified the name of the SSID
// and |password| sets the WPA2-PSK password. If password is NULL or an empty
// string the SSID will not have a password set. If an SSID with the same name
// already exists the password will be updated to |password|.
int android_wifi_add_ssid(const char* ssid, const char* password);

// Change the global emulator network latency. |latency| is a string that can
// be recognized by android_network_latency_parse(). Returns true on success,
// or false otherwise (e.g. invalid format).
//
// Block or unblock network traffic on an SSID specified by |ssid|. If |blocked|
// is true no network traffic will reach the host network if the device is
// connected to the SSID, if |blocked| is false network traffic will work as
// usual.
int android_wifi_set_ssid_block_on(const char* ssid, bool blocked);

ANDROID_END_HEADER
