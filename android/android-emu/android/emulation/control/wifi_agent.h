/* Copyright (C) 2020 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER
typedef struct QAndroidWifiAgent {
    bool (*add_ssid)(const char* ssid, const char* password);
    bool (*set_ssid_block_on)(const char* ssid, bool blocked);
} QAndroidWifiAgent;

ANDROID_END_HEADER
