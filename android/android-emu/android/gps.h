/* Copyright (C) 2007-2015 The Android Open Source Project
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

#include "android/emulation/serial_line.h"

#include <stdbool.h>
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

/* this is the internal character driver used to communicate with the
 * emulated GPS unit. see qemu_chr_open() in vl.c */
extern CSerialLine* android_gps_serial_line;

extern void  android_gps_send_nmea( const char*  sentence );

extern void  android_gps_send_gnss( const char*  sentence );

// Send a GPS location to the AVD using an NMEA sentence
//
// Inputs: latitude:        Degrees
//         longitude:       Degrees
//         metersElevation: Meters above sea level
//         speedKnots:      Speed in knots
//         headingDegrees:  Heading -180..+360, 0=north, 90=east
//         nSatellites:     Number of satellites used
//         time:            UTC, in the format provided
//                          by gettimeofday()
extern void  android_gps_send_location(double latitude, double longitude,
                                       double metersElevation,
                                       double speedKnots, double headingDegrees,
                                       int nSatellites,
                                       const struct timeval *time);

////////////////////////////////////////////////////////////
//
// android_gps_get_location
//
// Get the devices' current GPS location.
//
// Outputs:
//    Return value:     1 on success, 0 if failed
//
//    Valid only if the return is 1:
//    *outLatitude:        Degrees
//    *outLongitude:       Degrees
//    *outMetersElevation: Meters above sea level
//    *outVelocityKnots:   Knots
//    *outHeading:         Degrees, 0=north, 90=east
//    *outNSatellites:     Number of satellites used
// Null 'out' pointers are ignored.
extern int android_gps_get_location(double* outLatitude, double* outLongitude,
                                    double* outMetersElevation,
                                    double* outVelocityKnots, double* outHeading,
                                    int* outNSatellites);

/* android_gps_set_passive_update sets whether to ping guest for location
 * updates every few seconds.
 * Default to true.
 * Please set it before initializing location page. Do not change it during the
 * run. */
extern void android_gps_set_passive_update(bool enable);

extern bool android_gps_get_passive_update();
