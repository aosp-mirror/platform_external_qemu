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

#include <libxml/parser.h>
#include <vector>
#include <string>
#include <sstream>

using std::string;

struct LocationSet {
    string name;
    string description;
    string latitude;
    string longitude;
    string elevation;
    string time;
    LocationSet(void);
};

struct GpxParser {
    std::vector<LocationSet> locations;

    /* Performs standard libxml2 cleanup of |doc| in case of parsing failure or completion. */
    void cleanup_xmlDoc(xmlDoc *doc);

    /* Given a GPX node |ptNode| and the associated document, parses a GPX
     * <wpt>, <rtept>, or <trkpt> into a LocationSet and stors it in the member
     * vector.
     *
     * Returns false if an error occurred in parsing, true otherwise.
     */
    bool parse_location(xmlNode *ptNode, xmlDoc *doc);

    /* Reads |filename| in a libxml2 tree and then parses the tree according to
     * the GPX format, storing the resulting locations in the member vector.
     *
     * Returns false if an error occurred in parsing, and if so,
     * an error message in the input string stream.
     */
    bool parse_file(const string &fileName, std::ostringstream *errMsg);

    /* Parses |doc| according to the GPX format, storing the resulting locations in
     * the member vector.
     *
     * Returns false if an error occurred in parsing, and if so,
     * an error message in the input string stream.
     */
    bool parse(xmlDoc *doc, std::ostringstream *errMsg);

};

#endif // PARSE_LOCATION_H