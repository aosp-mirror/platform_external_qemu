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

#define __STDC_LIMIT_MACROS

#include <unistd.h>
#include <string.h>
#include <libxml/tree.h>

#include "parse-location.h"

// Name and description are not required fields
LocationSet::LocationSet(void) {
	this->name = "";
	this->description = "";
	this->time = "10";
	this->elevation = "0";
	this->latitude = "0";
	this->longitude = "0";
}

// Gpx parsing implementations

void GpxParser::cleanup_xmlDoc(xmlDoc *doc)
{
    xmlFreeDoc(doc);
    xmlCleanupParser(); // TODO: Should this be done somewhere at a more global scope?
}

bool GpxParser::parse_location(xmlNode *ptNode, xmlDoc *doc)
{
    LocationSet result;
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
        return false; // Return error since a point *must* have a latitude
    } else {
        tmpStr = xmlGetProp(ptNode, (const xmlChar *) "lon");
        longitude = string((const char *) tmpStr);
        xmlFree(tmpStr); // Caller-freed
    }

    // The result will be valid if this point is reached
    result.latitude = latitude;
    result.longitude = longitude;

    // Check for potential children nodes (including time, elevation, name, and description)
    // Note that none are actually required according to the GPX format.
    int childCount = 0;
    for (xmlNode *field = ptNode->children; field; field = field->next) {
        tmpStr = NULL;

        if ( !strcmp((const char *) field->name, "time") ) {
            tmpStr = xmlNodeListGetString(doc, field->children, 1);
            result.time = string((const char *) tmpStr);
            xmlFree(tmpStr); // Caller-freed
            childCount++;
            continue;
        }

        if ( !strcmp((const char *) field->name, "ele") ) {
            tmpStr = xmlNodeListGetString(doc, field->children, 1);
            result.elevation = string((const char *) tmpStr);
            xmlFree(tmpStr); // Caller-freed
            childCount++;
            continue;
        }

        if ( !strcmp((const char *) field->name, "name") ) {
            tmpStr = xmlNodeListGetString(doc, field->children, 1);
            result.name = string((const char *) tmpStr);
            xmlFree(tmpStr); // Caller-freed
            childCount++;
            continue;
        }

        if ( !strcmp((const char *) field->name, "desc") ) {
            tmpStr = xmlNodeListGetString(doc, field->children, 1);
            result.description = string((const char *) tmpStr);
            xmlFree(tmpStr); // Caller-freed
            childCount++;
            continue;
        }

        // We only care about 4 potential child fields, so quit after finding those
        if (childCount == 4) {
            break;
        }
    }

    locations.push_back(result);
    return true;
}

bool GpxParser::parse_file(const string &fileName, std::ostringstream *errMsg)
{
    xmlDocPtr doc;
    doc = xmlReadFile(fileName.c_str(), NULL, 0);
    if (doc == NULL) {
        cleanup_xmlDoc(doc);
        *errMsg << "File could not be found";
        return false;
    }
    return parse(doc, errMsg);
}

bool GpxParser::parse(xmlDoc *doc, std::ostringstream *errMsg)
{
    xmlNode *root = xmlDocGetRootElement(doc);
    bool res;

    for (xmlNode *child = root->children; child; child = child->next) {

        // Individual <wpt> elements are parsed on their own
        if ( !strcmp((const char *) child->name, "wpt") ) {
            res = parse_location(child, doc);
            if (!res) {
                cleanup_xmlDoc(doc);
                *errMsg << "Point missing a lat or lon on line "
                        << child->line;
                return false;
            }
        }

        // <rte> elements require an additional depth of parsing
        else if ( !strcmp((const char *) child->name, "rte") ) {
            for (xmlNode *rtept = child->children; rtept; rtept = rtept->next) {

                // <rtept> elements are parsed just like <wpt> elements
                if ( !strcmp((const char *) rtept->name, "rtept") ) {
                    res = parse_location(rtept, doc);
                    if (!res) {
                        cleanup_xmlDoc(doc);
                        *errMsg << "Point missing a lat or lon on line "
                                << rtept->line;
                        return false;
                    }
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
                            res = parse_location(trkpt, doc);
                            if (!res) {
                                cleanup_xmlDoc(doc);
                                *errMsg << "Point missing a lat or lon on line "
                                        << trkpt->line;
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }

    cleanup_xmlDoc(doc);
    return true;
}