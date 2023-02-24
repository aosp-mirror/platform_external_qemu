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
#include "android/gps.h"
#include "android/emulation/serial_line.h"
#include "android/gps/PassiveGpsUpdater.h"
#include "android/utils/debug.h"
#include "android/utils/stralloc.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <memory>
#include <mutex>

using android::emulation::PassiveGpsUpdater;

CSerialLine* android_gps_serial_line;
// Set to true to ping guest for location updates every few seconds
static bool s_enable_passive_location_update = true;
static bool s_enable_gps_signal = true;
static bool s_enable_gnssgrpcv1 = false;
static std::mutex s_set_location_mutex;
std::unique_ptr<PassiveGpsUpdater> s_updater;

// Last state of the gps, initial coordinates point to Googleplex.
double s_latitude = 39.237256;
double s_longitude = -123.150032;
double s_metersElevation = 0;
double s_speedKnots = 0;
double s_headingDegrees = 0;
int s_nSatellites = 4;

#define D(...) VERBOSE_PRINT(gps, __VA_ARGS__)

typedef void (*android_gnssgrpcv1_send_nmea_func)(const char*, int);

static android_gnssgrpcv1_send_nmea_func s_gnssgrpcv1_send_nmea = NULL;

void android_gps_send_nmea(const char* sentence) {
    if (sentence == NULL)
        return;

    D("sending '%s'", sentence);

    if (android_gps_serial_line == NULL) {
        D("missing GPS channel, ignored");
        return;
    }

    if (s_enable_gnssgrpcv1) {
        if (s_gnssgrpcv1_send_nmea) {
            s_gnssgrpcv1_send_nmea(sentence, strlen(sentence));
        }
        return;
    }
    android_serialline_write(android_gps_serial_line, (const uint8_t*)sentence,
                             strlen(sentence));
    android_serialline_write(android_gps_serial_line, (const uint8_t*)"\n", 1);
}

void android_gps_send_gnss(const char* sentence) {
    if (sentence == NULL)
        return;

    if (android_gps_serial_line == NULL) {
        D("missing GPS channel, ignored");
        return;
    }

    D("sending '%s'", sentence);

    char temp[1024];
    snprintf(temp, sizeof(temp), "$GPGNSSv1,%s", sentence);

    if (s_enable_gnssgrpcv1) {
        // TODO: handle gnss input
        return;
    }

    android_serialline_write(android_gps_serial_line, (const uint8_t*)temp,
                             strlen(temp));
    android_serialline_write(android_gps_serial_line, (const uint8_t*)"\n", 1);
}

// must match
// https://android.googlesource.com/platform/hardware/interfaces/+/refs/heads/master/gnss/common/utils/default/FixLocationParser.cpp
static void send_location_to_device_gnssrpcv1(double latitude,
                                              double longitude,
                                              double metersElevation,
                                              double speedKnots,
                                              double headingDegrees,
                                              const struct timeval* time) {
    constexpr double kAccuracyMeters = 1;
    constexpr double kAccuracySpeed = .5;
    constexpr double kAccuracyHeading = 2;
    constexpr int kUnused = 0;

    const double speedMetersPerSecond = speedKnots / 1.94384;
    char buf[128];
    const long long int tMs = time->tv_usec / 1000 + time->tv_sec * 1000LL;
    const int len = snprintf(buf, sizeof(buf),
                             "$GnssRpcV1,%d,%g,%g,%g,%g,%g,%g,%lld,%g,%g,%d",
                             kUnused,
                             latitude, longitude, metersElevation,
                             speedMetersPerSecond, kAccuracyMeters,
                             headingDegrees, tMs,
                             kAccuracySpeed, kAccuracyHeading,
                             kUnused);

    if (s_gnssgrpcv1_send_nmea) {
        s_gnssgrpcv1_send_nmea(buf, len);
    }
}

/**
Sends the current location to the device. Callers should have the
s_set_location_mutex.


@param latitude The current latitude, in decimal degrees.
@param longitude The current longitude, in decimal degrees.
@param metersElevation The current elevation, in meters.
@param speedKnots The current speed, in knots.
@param headingDegrees The current heading, in degrees.
@param nSatellites The number of satellites being used to calculate the location.
@param time The current time.
*/
static void send_location_to_device_internal(double latitude,
                                        double longitude,
                                        double metersElevation,
                                        double speedKnots,
                                        double headingDegrees,
                                        int nSatellites,
                                        const struct timeval* time) {
    STRALLOC_DEFINE(msgStr);
    STRALLOC_DEFINE(elevationStr);
    char* elevStrPtr;

    int latDeg, latMin, latFraction;
    int lngDeg, lngMin, lngFraction;
    char hemiNS, hemiEW;
    int hh = 0, mm = 0, ss = 0;

    // GPGGA format overview:
    //    time of fix      123519     12:35:19 UTC
    //    latitude         4807.038   48 degrees, 07.038 minutes
    //    north/south      N or S
    //    longitude        01131.000  11 degrees, 31. minutes
    //    east/west        E or W
    //    fix quality      1          standard GPS fix
    //    satellites       1 to 12    number of satellites being tracked
    //    HDOP             <dontcare> horizontal dilution
    //    altitude         546.       altitude above sea-level
    //    altitude units   M          to indicate meters
    //    diff             <dontcare> height of sea-level above ellipsoid
    //    diff units       M          to indicate meters (should be <dontcare>)
    //    dgps age         <dontcare> time in seconds since last DGPS fix
    //    dgps sid         <dontcare> DGPS station id

    // time->tv_sec is elapsed seconds since epoch, UTC
    hh = (int)(time->tv_sec / (60 * 60)) % 24;
    mm = (int)(time->tv_sec / 60) % 60;
    ss = (int)(time->tv_sec) % 60;

    stralloc_add_format(msgStr, "$GPGGA,%02d%02d%02d", hh, mm, ss);

    // Latitude
    hemiNS = 'N';
    if (latitude < 0) {
        hemiNS = 'S';
        latitude = -latitude;
    }
    latDeg = (int)latitude;
    latitude = 60 * (latitude - latDeg);
    latMin = (int)latitude;
    latFraction = 10000 * (latitude - latMin);
    stralloc_add_format(msgStr, ",%02d%02d.%04d,%c", latDeg, latMin,
                        latFraction, hemiNS);

    // Longitude
    hemiEW = 'E';
    if (longitude < 0) {
        hemiEW = 'W';
        longitude = -longitude;
    }
    lngDeg = (int)longitude;
    longitude = 60 * (longitude - lngDeg);
    lngMin = (int)longitude;
    lngFraction = 10000 * (longitude - lngMin);
    stralloc_add_format(msgStr, ",%02d%02d.%04d,%c", lngDeg, lngMin,
                        lngFraction, hemiEW);

    // Bogus fix quality (1), satellite count, and bogus dilution
    stralloc_add_format(msgStr, ",1,6,");

    // Altitude (to 0.1 meter precision) + bogus diff
    // Make sure elevation is formatted with a decimal point instead of comma.
    // setlocale isn't used because of thread safety concerns.
    stralloc_add_format(elevationStr, "%.1f", metersElevation);
    for (elevStrPtr = stralloc_cstr(elevationStr); *elevStrPtr; ++elevStrPtr) {
        if (*elevStrPtr == ',') {
            *elevStrPtr = '.';
            break;
        }
    }
    stralloc_add_format(msgStr, ",%s,M,0.,M", stralloc_cstr(elevationStr));
    stralloc_reset(elevationStr);

    // Bogus rest and checksum
    stralloc_add_str(msgStr, ",,,*47");

    // Send it
    android_gps_send_nmea(stralloc_cstr(msgStr));

    // Free it
    stralloc_reset(msgStr);

    // GPRMC format overview:
    // GPRMC,220516,A,5133.82,N,00042.24,W,173.8,231.8,130694,004.2,W*70
    //         1    2    3    4    5     6    7    8      9     10  11 12
    //      1  220516     Time Stamp
    //      2  A          validity - A-ok, V-invalid
    //      3  5133.82    current Latitude
    //      4  N          North/South
    //      5  00042.24   current Longitude
    //      6  W          East/West
    //      7  173.8      Speed in knots
    //      8  231.8      True course
    //      9  130694     Date Stamp (13 June 1994)
    //     10  004.2      Variation
    //     11  W          East/West
    //     12  *70        checksum

    STRALLOC_DEFINE(rmcStr);

    // Time
    stralloc_add_format(rmcStr, "$GPRMC,%02d%02d%02d,A", hh, mm, ss);

    // Latitude
    stralloc_add_format(rmcStr, ",%02d%02d.%04d,%c", latDeg, latMin,
                        latFraction, hemiNS);

    // Longitude
    stralloc_add_format(rmcStr, ",%02d%02d.%04d,%c", lngDeg, lngMin,
                        lngFraction, hemiEW);

    // Speed in knots, course
    if (headingDegrees < 0.0)
        headingDegrees += 360.0;
    stralloc_add_format(rmcStr, ",%.2f,%.2f", speedKnots, headingDegrees);

    // Date
    time_t ttTime = time->tv_sec;
    struct tm* pTm = gmtime(&ttTime);
    stralloc_add_format(rmcStr, ",%02d%02d%02d", pTm->tm_mday, pTm->tm_mon + 1,
                        (pTm->tm_year) % 100);

    if (s_enable_gnssgrpcv1) {
        // TODO: fix checksum
        stralloc_add_format(rmcStr, ",,,W*47");
    } else {
        // Magnetic variation, cksum (both bogus)
        stralloc_add_format(rmcStr, ",0.0,W*47");
    }

    // Send it
    if (s_enable_gps_signal) {
        android_gps_send_nmea(stralloc_cstr(rmcStr));
    }

    // Free it
    stralloc_reset(rmcStr);
}

int android_gps_get_location(double* outLatitude,
                             double* outLongitude,
                             double* outMetersElevation,
                             double* outVelocityKnots,
                             double* outHeading,
                             int* outNSatellites) {
    // Note: This does not retrieve the gps state from the actual device
    // which means that it is possible to observe a gps state that is
    // different from what is actually reported in the device.
    // 2 cases:
    //
    // - Someone managed to fake the gps coordinates from inside the device
    // - Android itself has not yet sampled this value. (Usually 1hz)
    if (outLatitude)
        *outLatitude = s_latitude;
    if (outLongitude)
        *outLongitude = s_longitude;
    if (outMetersElevation)
        *outMetersElevation = s_metersElevation;
    if (outVelocityKnots)
        *outVelocityKnots = s_speedKnots;
    if (outHeading)
        *outHeading = s_headingDegrees;
    if (outNSatellites)
        *outNSatellites = s_nSatellites;

    return 0;
}

////////////////////////////////////////////////////////////
//
// android_gps_send_location
//
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
void android_gps_send_location(double latitude,
                               double longitude,
                               double metersElevation,
                               double speedKnots,
                               double headingDegrees,
                               int nSatellites,
                               const struct timeval* time) {
    const std::lock_guard<std::mutex> lock(s_set_location_mutex);

    s_latitude = latitude;
    s_longitude = longitude;
    s_metersElevation = metersElevation;
    s_speedKnots = speedKnots;
    s_headingDegrees = headingDegrees;
    s_nSatellites = nSatellites;

    if (s_enable_gnssgrpcv1) {
        send_location_to_device_gnssrpcv1(latitude, longitude, metersElevation,
                                          speedKnots, headingDegrees, time);
    } else {
        send_location_to_device_internal(latitude, longitude, metersElevation,
                                         speedKnots, headingDegrees, nSatellites,
                                         time);
    }
}

void android_gps_refresh() {
    const std::lock_guard<std::mutex> lock(s_set_location_mutex);
    timeval timeVal = {};
    gettimeofday(&timeVal, nullptr);

    if (s_enable_gnssgrpcv1) {
        send_location_to_device_gnssrpcv1(
            s_latitude, s_longitude, s_metersElevation, s_speedKnots,
            s_headingDegrees, &timeVal);
    } else {
        send_location_to_device_internal(
            s_latitude, s_longitude, s_metersElevation, s_speedKnots,
            s_headingDegrees, s_nSatellites, &timeVal);
    }
}

void android_gps_set_passive_update(bool enable) {
    s_enable_passive_location_update = enable;
    if (enable) {
        // Start updater.
        s_updater = std::make_unique<PassiveGpsUpdater>(android_gps_refresh);
    } else {
        s_updater.reset();
    }
}

void android_gps_enable_gnssgrpcv1() {
    s_enable_gnssgrpcv1 = true;
}

bool android_gps_get_passive_update() {
    return s_enable_passive_location_update;
}

void android_gps_set_send_nmea_func(void* fptr) {
    android_gnssgrpcv1_send_nmea_func myf =
            (android_gnssgrpcv1_send_nmea_func)(fptr);
    s_gnssgrpcv1_send_nmea = myf;
}

bool android_gps_get_gps_signal() {
    return s_enable_gps_signal;
}

void android_gps_set_gps_signal(bool enable) {
    s_enable_gps_signal = enable;
}
