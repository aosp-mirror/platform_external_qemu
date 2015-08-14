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

#ifndef PARSE_LOCATION_H
#define PARSE_LOCATION_H

#include <QString>
#include <vector>

struct LocationSet {
    QString name;
    QString description;
    QString latitude;
    QString longitude;
    QString elevation;
    QString time;
    LocationSet(void);
};

void parse_Kml (QString& fileName, std::vector<LocationSet>& result);

#endif // PARSE_LOCATION_H