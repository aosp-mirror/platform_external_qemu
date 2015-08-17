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

// Eventually this struct can be generalized to parse both KML + GPX
struct KmlParser {
	std::vector<LocationSet> locations;
	/* Given any KML node, this function will return a pointer to the
	* first encountered child <coordinates> node, or NULL if no such
	* node exists.
	*/
	xmlNode * find_coordinates(xmlNode * current);

	/* Given a KML node |current| that points to a <coordinates> node,
	* this function parses all internal coordinates and adds the points
	* onto the end of the |locations| vector.
	*/
	bool parse_coordinates(xmlNode * current);

	/* Given a KML node |current| that points to a <Placemark> node,
	* appends all the enclosed points onto the end of |locations|, returning
	* false if any coordinates contained within the <Placemark> node were
	* malformatted.
	*/
	bool parse_location(xmlNode * current);

	/* This function reads |filename| into an xmllib2 tree and then
	* proceeds to parse the tree according to the KML format, returning
	* a list of formatted coordinates into |locations|.
	*/
	void parse(const QString& fileName);

	/* Recursively traverses all the siblings and children of |current|
	* calling other functions along the way to parse the KML placemarks into
	* |locations|.
	*/
	void traverse_subtree(xmlNode * current);
};
#endif // PARSE_LOCATION_H