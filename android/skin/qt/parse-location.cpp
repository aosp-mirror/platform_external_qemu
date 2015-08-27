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
#include <QRegExp>
#include <QStringList>
#include <libxml/parser.h>
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

/* Coordinates can be nested arbitrarily deep within a Placemark, depending
 on the type of object (Point, LineString, Polygon) the Placemark contains*/
xmlNode * KmlParser::find_coordinates(xmlNode * current) {
    for (; current != NULL; current = current->next) {
        if (!strcasecmp(reinterpret_cast<const char*>(current->name), "coordinates")) {
            return current;
        }
        xmlNode * children = find_coordinates(current->xmlChildrenNode);
        if (children != NULL) {
            return children;
        }
    }
    return NULL;
}

/* Coordinates have the following format:
	<coordinates> -112.265654928602,36.09447672602546,2357
				...
				-112.2657374587321,36.08646312301303,2357
    </coordinates>
	often entirely contained in a single string, necessitating regex
*/
bool KmlParser::parse_coordinates(xmlNode * current) {
    xmlNode * coordinates_node = find_coordinates(current);
    if (coordinates_node == NULL)
		return false;

    QString coordinates = (const char *) coordinates_node->xmlChildrenNode->content;
    QStringList triple = coordinates.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    for (int i = 0; i < triple.size(); i++) {
		int ind = locations.size();
		this->locations.push_back(LocationSet());
        this->locations[ind].longitude = triple[i].section(',', 0, 0);
        this->locations[ind].latitude  = triple[i].section(',', 1, 1);
        this->locations[ind].elevation = triple[i].section(',', 2, 2);
    }

    return true;
}

bool KmlParser::parse_location(xmlNode * current) {
	QString description = "";
	QString name = "";
	int ind = -1;

    for (; current != NULL; current = current->next) {
        if (!strcasecmp(reinterpret_cast<const char*>(current->name), "description")) {
            description = reinterpret_cast<const char*>(current->xmlChildrenNode->content);
        } else if (!strcasecmp(reinterpret_cast<const char*>(current->name), "name")) {
            name = (const char *) current->xmlChildrenNode->content;
        } else if (!strcasecmp(reinterpret_cast<const char*>(current->name), "point") ||
            !strcasecmp(reinterpret_cast<const char*>(current->name), "lineString") ||
            !strcasecmp(reinterpret_cast<const char*>(current->name), "polygon")) {
			ind = locations.size();
            if (!parse_coordinates(current->xmlChildrenNode))
				return false;
        }
    }

    // only assign name and description to the first of the
    // points to avoid needless repitition
    if (ind > -1 && ind < locations.size()) {
		locations[ind].description = description;
		locations[ind].name = name;
    }
    return true;
}

// Placemarks (aka locations) can be nested arbitrarily deep
void KmlParser::traverse_subtree(xmlNode * current) {
    for (; current; current = current->next) {
        if (current->name != NULL &&
			!strcasecmp(reinterpret_cast<const char*>(current->name), "placemark")) {
			if (!parse_location(current->xmlChildrenNode)) {
				fprintf(stderr, "Location found missing coordinates\n");
			}
        } else if (current->name != NULL &&
			strcasecmp(reinterpret_cast<const char*>(current->name), "text")) {

		// if it's not a <placemark> - we must go deeper
            traverse_subtree(current->xmlChildrenNode);
        }
    }
}

void KmlParser::parse(const QString& fileName) {
    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    xmlDocPtr doc = xmlReadFile(fileName.toStdString().c_str(), NULL, 0);
    if (doc == NULL) {
        fprintf(stderr,"KML document not parsed successfully.\n");
        xmlFreeDoc(doc);
        return;
    }

    xmlNodePtr cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr,"Could not get root element of parsed KML file\n");
        xmlFreeDoc(doc);
		xmlCleanupParser();
        return;
    }

    traverse_subtree(cur);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}