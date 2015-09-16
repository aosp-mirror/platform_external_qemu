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

#include "android/gps/GpxParser.h"
#include "android/gps/internal/GpxParserInternal.h"

#include <string.h>

using std::string;

void GpxParserInternal::cleanupXmlDoc(xmlDoc *doc)
{
    xmlFreeDoc(doc);
    xmlCleanupParser();
}

bool GpxParserInternal::parseLocation(xmlNode *ptNode, xmlDoc *doc, GpsFix *result)
{
    string latitude;
    string longitude;

    xmlAttrPtr attr;
    xmlChar *tmpStr;

    // Check for and get the latitude attribute
    attr = xmlHasProp(ptNode, (const xmlChar *) "lat");
    if (attr == NULL) {
        return false; // Return error since a point *must* have a latitude
    } else {
        tmpStr = xmlGetProp(ptNode, (const xmlChar *) "lat");
        latitude = string((const char *) tmpStr);
        xmlFree(tmpStr); // Caller-freed
    }

    // Check for and get the longitude attribute
    attr = xmlHasProp(ptNode, (const xmlChar *) "lon");
    if (attr == NULL) {
        return false; // Return error since a point *must* have a longitude
    } else {
        tmpStr = xmlGetProp(ptNode, (const xmlChar *) "lon");
        longitude = string((const char *) tmpStr);
        xmlFree(tmpStr); // Caller-freed
    }

    // The result will be valid if this point is reached
    result->latitude = latitude;
    result->longitude = longitude;

    // Check for potential children nodes (including time, elevation, name, and description)
    // Note that none are actually required according to the GPX format.
    int childCount = 0;
    for (xmlNode *field = ptNode->children; field; field = field->next) {
        tmpStr = NULL;

        if ( !strcmp((const char *) field->name, "time") ) {
            tmpStr = xmlNodeListGetString(doc, field->children, 1);
            result->time = string((const char *) tmpStr);
            xmlFree(tmpStr); // Caller-freed
            childCount++;
        }

        else if ( !strcmp((const char *) field->name, "ele") ) {
            tmpStr = xmlNodeListGetString(doc, field->children, 1);
            result->elevation = string((const char *) tmpStr);
            xmlFree(tmpStr); // Caller-freed
            childCount++;
        }

        else if ( !strcmp((const char *) field->name, "name") ) {
            tmpStr = xmlNodeListGetString(doc, field->children, 1);
            result->name = string((const char *) tmpStr);
            xmlFree(tmpStr); // Caller-freed
            childCount++;
        }

        else if ( !strcmp((const char *) field->name, "desc") ) {
            tmpStr = xmlNodeListGetString(doc, field->children, 1);
            result->description = string((const char *) tmpStr);
            xmlFree(tmpStr); // Caller-freed
            childCount++;
        }

        // We only care about 4 potential child fields, so quit after finding those
        if (childCount == 4) {
            break;
        }
    }

    return true;
}

bool GpxParserInternal::parse(xmlDoc *doc, GpsFixArray *fixes, string *error)
{
    xmlNode *root = xmlDocGetRootElement(doc);
    GpsFix location;
    bool isOk;

    for (xmlNode *child = root->children; child; child = child->next) {

        // Individual <wpt> elements are parsed on their own
        if ( !strcmp((const char *) child->name, "wpt") ) {
            isOk = parseLocation(child, doc, &location);
            if (!isOk) {
                cleanupXmlDoc(doc);

                char buf[100];
                sprintf(buf, "Wpt missing a lat or lon on line %d.", child->line);
                *error = string(buf);
                return false;
            }
            fixes->push_back(location);
        }

        // <rte> elements require an additional depth of parsing
        else if ( !strcmp((const char *) child->name, "rte") ) {
            for (xmlNode *rtept = child->children; rtept; rtept = rtept->next) {

                // <rtept> elements are parsed just like <wpt> elements
                if ( !strcmp((const char *) rtept->name, "rtept") ) {
                    isOk = parseLocation(rtept, doc, &location);
                    if (!isOk) {
                        cleanupXmlDoc(doc);

                        char buf[100];
                        sprintf(buf, "Rtept missing a lat or lon on line %d.", rtept->line);
                        *error = string(buf);
                        return false;
                    }
                    fixes->push_back(location);
                }
            }
        }

        // <trk> elements require two additional depths of parsing
        else if ( !strcmp((const char *) child->name, "trk") ) {
            for (xmlNode *trkseg = child->children; trkseg; trkseg = trkseg->next) {

                // Skip non <trkseg> elements
                if ( !strcmp((const char *) trkseg->name, "trkseg") ) {

                    // <trkseg> elements an additional depth of parsing
                    for (xmlNode *trkpt = trkseg->children; trkpt; trkpt = trkpt->next) {

                        // <trkpt> elements are parsed just like <wpt> elements
                        if ( !strcmp((const char *) trkpt->name, "trkpt") ) {
                            isOk = parseLocation(trkpt, doc, &location);
                            if (!isOk) {
                                cleanupXmlDoc(doc);

                                char buf[100];
                                sprintf(buf, "Trkpt missing a lat or lon on line %d.", trkpt->line);
                                *error = string(buf);
                                return false;
                            }
                            fixes->push_back(location);
                        }
                    }
                }
            }
        }
    }

    cleanupXmlDoc(doc);
    return true;
}

bool GpxParser::parseFile(const char *filePath, GpsFixArray *fixes, string *error)
{
    xmlDocPtr doc = xmlReadFile(filePath, NULL, 0);
    if (doc == NULL) {
        GpxParserInternal::cleanupXmlDoc(doc);
        *error = "GPX document not parsed successfully.";
        return false;
    }
    return GpxParserInternal::parse(doc, fixes, error);
}