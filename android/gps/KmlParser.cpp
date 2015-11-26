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


#include "android/gps/KmlParser.h"
#include "android/gps/internal/KmlParserInternal.h"

#include <string.h>
#include <unistd.h>

using std::string;

// Coordinates can be nested arbitrarily deep within a Placemark, depending
// on the type of object (Point, LineString, Polygon) the Placemark contains
xmlNode * KmlParserInternal::findCoordinates(xmlNode * current) {
    for (; current != NULL; current = current->next) {
        if (!strcmp((const char *) current->name, "coordinates")) {
            return current;
        }
        xmlNode * children = findCoordinates(current->xmlChildrenNode);
        if (children != NULL) {
            return children;
        }
    }
    return NULL;
}

// Coordinates have the following format:
//        <coordinates> -112.265654928602,36.09447672602546,2357
//                ...
//                -112.2657374587321,36.08646312301303,2357
//        </coordinates>
// often entirely contained in a single string, necessitating regex
bool KmlParserInternal::parseCoordinates(xmlNode * current, GpsFixArray * fixes) {
    xmlNode * coordinates_node = findCoordinates(current);
    if (coordinates_node == NULL
            || coordinates_node->xmlChildrenNode == NULL
            || coordinates_node->xmlChildrenNode->content == NULL)
        return false;

    // strtok modifies the string it's given, so we make a copy to operate on
    char* coordinates = strdup((const char *) coordinates_node->xmlChildrenNode->content);

    #ifdef _WIN32
    // in windows, each thread saves its own copy of the static required variables
    char* split = strtok(coordinates, " \t\n\v\r\f");
    #else
    char* saveptr;
    char* split = strtok_r(coordinates, " \t\n\v\r\f", &saveptr);
    #endif
    while (split != NULL) {

        string triple = string(split);
        size_t first = triple.find(",");
        size_t second = triple.find(",", first + 1);
        if (first != string::npos && second != string::npos) {
            int ind = fixes->size();
            fixes->push_back(GpsFix());
            (*fixes)[ind].longitude = triple.substr(0, first);
            (*fixes)[ind].latitude  = triple.substr(first + 1, second - first - 1);
            (*fixes)[ind].elevation = triple.substr(second + 1);
        } else {
            return false;
        }

        #ifdef _WIN32
        split = strtok(NULL, " \t\n\v\r\f");
        #else
        split = strtok_r(NULL, " \t\n\v\r\f", &saveptr);
        #endif
    }

    free(coordinates);
    return true;
}

bool KmlParserInternal::parsePlacemark(xmlNode * current, GpsFixArray * fixes) {
    string description = "";
    string name = "";
    size_t ind = -1;

    // not worried about case-sensitivity since .kml files
    // are expected to be machine-generated
    for (; current != NULL; current = current->next) {
        if (!strcmp((const char *) current->name, "description")) {
            description = reinterpret_cast<const char*>(current->xmlChildrenNode->content);
        } else if (!strcmp((const char *) current->name, "name")) {
            name = (const char *) current->xmlChildrenNode->content;
        } else if (!strcmp((const char *) current->name, "Point") ||
                !strcmp((const char *) current->name, "LineString") ||
                !strcmp((const char *) current->name, "Polygon")) {
            ind = fixes->size();
            if (!parseCoordinates(current->xmlChildrenNode, fixes)) {
                return false;
            }
        }
    }

    // only assign name and description to the first of the
    // points to avoid needless repitition
    if (ind != string::npos && ind < fixes->size()) {
        (*fixes)[ind].description = description;
        (*fixes)[ind].name = name;
    }

    return ind != string::npos && ind < fixes->size();
}

// Placemarks (aka locations) can be nested arbitrarily deep
bool KmlParserInternal::traverseSubtree(xmlNode * current, GpsFixArray * fixes, string * error) {
    for (; current; current = current->next) {
        if (current->name != NULL &&
                !strcmp((const char *) current->name, "Placemark")) {

            if (!parsePlacemark(current->xmlChildrenNode, fixes)) {
                *error = "Location found with missing or malformed coordinates";
                return false;
            }
        } else if (current->name != NULL &&
                strcmp((const char *) current->name, "text")) {

            // if it's not a Placemark we must go deeper
            if (!traverseSubtree(current->xmlChildrenNode, fixes, error)) {
                return false;
            }
        }
    }
    *error = "";
    return true;
}

bool KmlParser::parseFile(const char * filePath, GpsFixArray * fixes, string * error) {
    // This initializes the library and checks potential ABI mismatches between
    // the version it was compiled for and the actual shared library used.
    LIBXML_TEST_VERSION

    xmlDocPtr doc = xmlReadFile(filePath, NULL, 0);
    if (doc == NULL) {
        *error = "KML document not parsed successfully.";
        xmlFreeDoc(doc);
        return false;
    }

    xmlNodePtr cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        *error = "Could not get root element of parsed KML file.";
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return false;
    }
    bool wellFormed = KmlParserInternal::traverseSubtree(cur, fixes, error);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return wellFormed;
}
