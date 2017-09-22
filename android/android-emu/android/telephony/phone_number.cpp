/* Copyright (C) 2017 The Android Open Source Project
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

#include <string.h>

#include "android/telephony/phone_number.h"
#include "android/utils/debug.h"

#include "android/cmdline-option.h"

static const char kDefaultPhonePrefix[] = "1555521";

extern "C" const char* get_phone_number_prefix() {
    if (android_cmdLineOptions &&
        android_cmdLineOptions->phone_number_prefix != nullptr) {
        return android_cmdLineOptions->phone_number_prefix;
    }

    return kDefaultPhonePrefix;
}
