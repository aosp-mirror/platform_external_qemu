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

#ifndef ANDROID_GPS_INTERNAL_GPX_PARSER_INTERNAL_H
#define ANDROID_GPS_INTERNAL_GPX_PARSER_INTERNAL_H

#include "android/gps/GpsFix.h"

#include <libxml/parser.h>

class GpxParserInternal {
public:

    /* Performs standard libxml2 cleanup of |doc| in case of parsing failure or completion. */
    static void cleanupXmlDoc(xmlDoc *doc);

    /* Given a GPX node |ptNode| and the associated xmlDoc |doc|, parses a GPX
     * <wpt>, <rtept>, or <trkpt> into the GpsFix |result| and stors it in the member
     * vector.
     *
     * Returns true on success, false otherwise.
     */
    static bool parseLocation(xmlNode *ptNode, xmlDoc *doc, GpsFix *result);

    /* Parses |doc| according to the GPX format, storing the GpsFixes in the input array |fixes|.
     *
     * Returns true on success, false otherwise. If false is returned, |error|
     * is set to a string describing the error.
     */
    static bool parse(xmlDoc *doc, GpsFixArray *fixes, std::string *error);
};

#endif // ANDROID_GPS_INTERNAL_GPX_PARSER_INTERNAL_H
