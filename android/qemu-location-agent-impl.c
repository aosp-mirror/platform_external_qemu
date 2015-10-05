/* Copyright (C) 2015 The Android Open Source Project
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

#include "android/qemu-control-impl.h"

#include "android/emulation/control/location_agent.h"
#include "android/gps.h"

#include <stdbool.h>

static bool location_gpsIsSupported() {
    return android_gps_cs != NULL;
}

static const QAndroidLocationAgent sQAndroidLocationAgent = {
        .gpsIsSupported = location_gpsIsSupported,
        .gpsCmd = android_gps_send_location,
        .gpsSendNmea = android_gps_send_nmea};
const QAndroidLocationAgent* const gQAndroidLocationAgent =
        &sQAndroidLocationAgent;
