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

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <sys/time.h>

ANDROID_BEGIN_HEADER

typedef struct QAndroidLocationAgent {
    // Query whether the device implementation supports GPS.
    bool (*gpsIsSupported)(void);

    // Send a GPS location to the AVD using an NMEA sentence
    //   |latitude| and |longitude| are in degrees
    //   |metersElevation| is meters above sea level
    //   |nSatellites| is the number of satellites used
    //   |time| is UTC, in the format provided by gettimeofday()
    void (*gpsCmd)(double latitude, double longitude,
                   double metersElevation, int nSatellites,
                   const struct timeval *time);

    // Send an NMEA fix sentence to the device.
    void (*gpsSendNmea)(const char* sentence);
} QAndroidLocationAgent;

ANDROID_END_HEADER
