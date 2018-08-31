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
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

ANDROID_BEGIN_HEADER

typedef void (*location_qt_settings_writer)(double, double, double, double, double);

// Register callback to sync Qt settings
void location_registerQtSettingsWriter(location_qt_settings_writer);

typedef struct QAndroidLocationAgent {
    // Query whether the device implementation supports GPS.
    bool (*gpsIsSupported)(void);

    // Send a GPS location to the AVD using NMEA sentences
    //   |latitude| and |longitude| are in degrees
    //   |metersElevation| is meters above sea level
    //   |speed| is in knots
    //   |heading| is degrees, -180..+360, 0=North, 90=East
    //   |nSatellites| is the number of satellites used
    //   |time| is UTC, in the format provided by gettimeofday()
    void (*gpsSendLoc)(double latitude, double longitude,
                       double metersElevation,
                       double speed, double heading,
                       int nSatellites,
                       const struct timeval *time);

    // Get the current device location
    //
    // Returns 1 if the device location was successfully
    //   received from the device, 0 if unsuccessful
    // If the return is 1, the output values are set:
    //   |outLatitude| and |outLongitude| are in degrees
    //   |outMetersElevation| is meters above sea level
    //   |outVelocityKnots| is device speed in knots
    //   |outHeading| is device heading in degrees (0=north, 90=east)
    //   |outNSatellites| is the number of satellites used
    // Null 'out' pointers are safely ignored.
    int (*gpsGetLoc)(double* outLatitude, double* outLongitude,
                     double* outMetersElevation,
                     double* outVelocityKnots, double* outHeading,
                     int* outNSatellites);

    // Send an NMEA fix sentence to the device.
    void (*gpsSendNmea)(const char* sentence);

    // Send a gnss sentence to the device.
    void (*gpsSendGnss)(const char* sentence);

    // Should ping the guest GPS update every few seconds or not
    void (*gpsSetPassiveUpdate)(bool enable);
    bool (*gpsGetPassiveUpdate)();
} QAndroidLocationAgent;

ANDROID_END_HEADER
