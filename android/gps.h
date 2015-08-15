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
#ifndef ANDROID_GPS_H
#define ANDROID_GPS_H

#include "qemu-common.h"

/* this is the internal character driver used to communicate with the
 * emulated GPS unit. see qemu_chr_open() in vl.c */
extern CharDriverState*  android_gps_cs;

extern void  android_gps_send_nmea( const char*  sentence );

// Send a GPS location to the AVD using an NMEA sentence
//
// Inputs: latitude:        Degrees
//         longitude:       Degrees
//         metersElevation: Meters above sea level
//         nSatellites:     Number of satellites used
//         time:            UTC, in the format provided
//                          by gettimeofday()
extern void  android_gps_send_location(double latitude, double longitude,
                                       double metersElevation, int nSatellites,
                                       const struct timeval *time);

#endif // ANDROID_GPS_H
