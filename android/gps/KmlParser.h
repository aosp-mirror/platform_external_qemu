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

#ifndef ANDROID_GPS_KML_PARSER_H
#define ANDROID_GPS_KML_PARSER_H

#include <string>
#include <vector>

using std::string;

// A struct representing a location on a map
struct GpsFix {
    string name;
    string description;
    string latitude;
    string longitude;
    string elevation;
    string time;
    GpsFix(void);
};

typedef std::vector<GpsFix> GpsFixArray;

// Parses a given .kml file at |filePath| and extracts all contained GPS
// fixes into |*fixes|.
// Returns true on success, false otherwise. if false is returned, |error|
// is set to point at string describing the error
bool parseFile(const char * filePath, GpsFixArray * fixes, string * error);

#endif // ANDROID_GPS_KML_PARSER_H