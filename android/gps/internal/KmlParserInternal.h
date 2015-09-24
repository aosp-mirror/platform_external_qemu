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

#pragma once

#include <libxml/parser.h>

class KmlParserInternal {
public:
    // Given any xml node, this function will return a pointer to the
    // first encountered child <coordinates> node, or NULL if no such
    // node exists.
    static xmlNode * findCoordinates(xmlNode * current);

    // Given a KML node |current| that points to a <coordinates> node,
    // this function parses all contained coordinates and adds the points
    // onto the end of the |*fixes|.
    // Returns false if the coordinates were malformed, true otherwise.
    static bool parseCoordinates(xmlNode * current, GpsFixArray * fixes);

    // Given a KML node |current| that points to a Placemark node,
    // appends all the enclosed points onto the end of |*fixes|.
    // Returns false if any coordinates contained within the Placemark
    // node were malformatted or missing, true otherwise.
    static bool parsePlacemark(xmlNode * current, GpsFixArray * fixes);

    // Recursively traverses all the siblings and children of |current|
    // parsing any found Placemarks onto the end of |*fixes|. Returns false
    // with a description of what went in |*error| if any contained
    // Placemarks are found to be malformed, true otherwise.
    static bool traverseSubtree(xmlNode * current, GpsFixArray * fixes, std::string * error);
};
