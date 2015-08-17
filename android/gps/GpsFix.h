// Copyright (C) 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_GPS_GPS_FIX_H
#define ANDROID_GPS_GPS_FIX_H

#include <string>
#include <vector>

// A struct representing a location on a map
struct GpsFix {
    std::string name;
    std::string description;
    std::string latitude;
    std::string longitude;
    std::string elevation;
    std::string time;
    GpsFix(void);
};

typedef std::vector<GpsFix> GpsFixArray;

#endif // ANDROID_GPS_GPS_FIX_H