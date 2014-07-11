/* Copyright (C) 2007 The Android Open Source Project
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

#include "android/android.h"

/* see http://en.wikipedia.org/wiki/List_of_device_bandwidths or a complete list */
const NetworkSpeed  android_netspeeds[] = {
    { "gsm", "GSM/CSD", 14400, 14400 },
    { "hscsd", "HSCSD", 14400, 43200 },
    { "gprs", "GPRS", 40000, 80000 },
    { "edge", "EDGE/EGPRS", 118400, 236800 },
    { "umts", "UMTS/3G", 128000, 1920000 },
    { "hsdpa", "HSDPA", 348000, 14400000 },
    { "full", "no limit", 0, 0 },
    { NULL, NULL, 0, 0 }
};
const size_t android_netspeeds_count =
    sizeof(android_netspeeds) / sizeof(android_netspeeds[0]);

const NetworkLatency  android_netdelays[] = {
    /* FIXME: these numbers are totally imaginary */
    { "gprs", "GPRS", 150, 550 },
    { "edge", "EDGE/EGPRS", 80, 400 },
    { "umts", "UMTS/3G", 35, 200 },
    { "none", "no latency", 0, 0 },
    { NULL, NULL, 0, 0 }
};
const size_t android_netdelays_count =
    sizeof(android_netdelays) / sizeof(android_netdelays[0]);

