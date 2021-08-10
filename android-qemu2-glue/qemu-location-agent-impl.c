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

#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/emulation/control/location_agent.h"
#include "android/gps.h"

#include <stdbool.h>

static bool location_gpsIsSupported() {
    return android_gps_serial_line != NULL;
}

static const QAndroidLocationAgent sQAndroidLocationAgent = {
        .gpsIsSupported = location_gpsIsSupported,
        .gpsSendLoc = android_gps_send_location,
        .gpsGetLoc = android_gps_get_location,
        .gpsSendNmea = android_gps_send_nmea,
        .gpsSendGnss = android_gps_send_gnss,
        .gpsSetPassiveUpdate = android_gps_set_passive_update,
        .gpsEnableGnssGrpcV1 = android_gps_enable_gnssgrpcv1,
        .gpsGetPassiveUpdate = android_gps_get_passive_update,
        .gpsGetGpsSignal = android_gps_get_gps_signal,
        .gpsSetGpsSignal = android_gps_set_gps_signal};
const QAndroidLocationAgent* const gQAndroidLocationAgent =
        &sQAndroidLocationAgent;
