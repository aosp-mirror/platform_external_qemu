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

#ifndef ANDROID_LOCATION_AGENT_H
#define ANDROID_LOCATION_AGENT_H

#include <sys/time.h>

typedef struct LocationAgent {
    void (*gpsCmd)(double latitude, double longitude, 
                   double metersElevation, int nSatellites,
                   struct timeval *time);
} LocationAgent;


#endif // ANDROID_LOCATION_AGENT_H
