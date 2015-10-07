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

#include <gtest/gtest.h>

using std::string;

TEST(KmlParserInternal, FindCoordinates) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<name>Simple placemark</name>"
                    "<description>Attached to the ground. Intelligently places itself"
                    "at the height of the underlying terrain.</description>"
                    "<Point>"
                    "<coordinates>-122.0822035425683,37.42228990140251,0</coordinates>"
                    "</Point>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    ASSERT_NE(KmlParserInternal::findCoordinates(cur), (void *) NULL);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, FindNoCoordinates) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<name>Simple placemark</name>"
                    "<description>Attached to the ground. Intelligently places itself"
                    "at the height of the underlying terrain.</description>"
                    "<Point>"
                    "</Point>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    EXPECT_EQ(KmlParserInternal::findCoordinates(cur), (void *) NULL);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, ParseEmptyCoordinates) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<Point><coordinates></coordinates>"
                    "</Point>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    EXPECT_EQ(0, locations.size());
    EXPECT_FALSE(KmlParserInternal::parseCoordinates(cur, &locations));
    EXPECT_EQ(0, locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, ParseNoCoordinatesEmptyPoint) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<Point>"
                    "</Point>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    EXPECT_EQ(0, locations.size());
    EXPECT_FALSE(KmlParserInternal::parseCoordinates(cur, &locations));
    EXPECT_EQ(0, locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, ParseOneCoordinate) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<Point>"
                    "<coordinates>-122.0822035425683,37.42228990140251,0</coordinates>"
                    "</Point>"
                "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    EXPECT_EQ(0, locations.size());
    EXPECT_TRUE(KmlParserInternal::parseCoordinates(cur, &locations));
    EXPECT_EQ(1, locations.size());

    EXPECT_EQ("-122.0822035425683", locations[0].longitude);
    EXPECT_EQ("37.42228990140251", locations[0].latitude);
    EXPECT_EQ("0", locations[0].elevation);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, ParseMultCoordinates) {
    // also throw some extra whitespace between triples for good measure
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<LineString>"
                    "<coordinates>-122.0822035425683,37.42228990140251,0 \
                    10.4,39.,20\t\t0,21.4,1"
                    "</coordinates>"
                    "</LineString>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    EXPECT_EQ(0, locations.size());
    EXPECT_TRUE(KmlParserInternal::parseCoordinates(cur, &locations));
    EXPECT_EQ(3, locations.size());

    EXPECT_EQ("-122.0822035425683", locations[0].longitude);
    EXPECT_EQ("37.42228990140251", locations[0].latitude);
    EXPECT_EQ("0", locations[0].elevation);
    EXPECT_EQ("10.4", locations[1].longitude);
    EXPECT_EQ("39.", locations[1].latitude);
    EXPECT_EQ("20", locations[1].elevation);
    EXPECT_EQ("0", locations[2].longitude);
    EXPECT_EQ("21.4", locations[2].latitude);
    EXPECT_EQ("1", locations[2].elevation);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, ParseBadCoordinates) {
    // also throw some bad whitespace in there for good measure
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<LineString>"
                    "<coordinates>-122.0822035425683, 37.42228990140251, 0 \
                    10.4,39.20\t021.41"
                    "</coordinates>"
                    "</LineString>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    EXPECT_EQ(0, locations.size());
    EXPECT_FALSE(KmlParserInternal::parseCoordinates(cur, &locations));

    // checking that previous malformatted coordinates don't affect later ones
    EXPECT_EQ(0, locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, ParseLocationNormal) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<name>Simple placemark</name>"
                    "<description>Attached to the ground.</description>"
                    "<Point>"
                    "<coordinates>-122.0822035425683,37.42228990140251,0</coordinates>"
                    "</Point>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    string error;
    EXPECT_EQ(0, locations.size());
    EXPECT_TRUE(KmlParserInternal::traverseSubtree(cur, &locations, &error));
    EXPECT_EQ("", error);
    EXPECT_EQ(1, locations.size());

    for (unsigned i = 0; i < locations.size(); ++i) {
        EXPECT_EQ("Simple placemark", locations[i].name);
        EXPECT_EQ("Attached to the ground.", locations[i].description);
        EXPECT_EQ("-122.0822035425683", locations[i].longitude);
        EXPECT_EQ("37.42228990140251", locations[i].latitude);
        EXPECT_EQ("0", locations[i].elevation);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, ParseLocationNormalMissingOptionalFields) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<Point>"
                    "<coordinates>-122.0822035425683,37.42228990140251,0</coordinates>"
                    "</Point>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    string error;
    EXPECT_EQ(0, locations.size());
    EXPECT_TRUE(KmlParserInternal::traverseSubtree(cur, &locations, &error));
    EXPECT_EQ("", error);
    EXPECT_EQ(1, locations.size());

    for (unsigned i = 0; i < locations.size(); ++i) {
        EXPECT_EQ("", locations[i].name);
        EXPECT_EQ("", locations[i].description);
        EXPECT_EQ("-122.0822035425683", locations[i].longitude);
        EXPECT_EQ("37.42228990140251", locations[i].latitude);
        EXPECT_EQ("0", locations[i].elevation);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, ParseLocationMissingRequiredFields) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<name>Simple placemark</name>"
                    "<description>Attached to the ground.</description>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    string error;
    EXPECT_EQ(0, locations.size());
    EXPECT_FALSE(KmlParserInternal::traverseSubtree(cur, &locations, &error));
    EXPECT_EQ("Location found with missing or malformed coordinates", error);
    EXPECT_EQ(0, locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, ParseLocationNameOnlyFirst) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<name>Simple placemark</name>"
                    "<description>Attached to the ground.</description>"
                    "<LineString>"
                    "<coordinates>-122.0822035425683,37.42228990140251,0\
                    -122.0822035425683,37.42228990140251,0</coordinates>"
                    "</LineString>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    string error;
    EXPECT_EQ(0, locations.size());
    EXPECT_TRUE(KmlParserInternal::traverseSubtree(cur, &locations, &error));
    EXPECT_EQ("", error);
    EXPECT_EQ(2, locations.size());

    EXPECT_EQ("Simple placemark", locations[0].name);
    EXPECT_EQ("Attached to the ground.", locations[0].description);
    EXPECT_EQ("", locations[1].name);
    EXPECT_EQ("", locations[1].description);

    for (unsigned i = 0; i < locations.size(); ++i) {
        EXPECT_EQ("-122.0822035425683", locations[i].longitude);
        EXPECT_EQ("37.42228990140251", locations[i].latitude);
        EXPECT_EQ("0", locations[i].elevation);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, ParseMultipleLocations) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                    "<name>Simple placemark</name>"
                    "<description>Attached to the ground.</description>"
                    "<Point>"
                    "<coordinates>-122.0822035425683,37.42228990140251,0</coordinates>"
                    "</Point>"
            "</Placemark>"
            "<Placemark>"
                    "<name>Simple placemark</name>"
                    "<description>Attached to the ground.</description>"
                    "<Point>"
                    "<coordinates>-122.0822035425683,37.42228990140251,0\
                    -122.0822035425683,37.42228990140251,0</coordinates>"
                    "</Point>"
            "</Placemark>"
            "<Placemark>"
                    "<name>Simple placemark</name>"
                    "<description>Attached to the ground.</description>"
                    "<Point>"
                    "<coordinates>-122.0822035425683,37.42228990140251,0</coordinates>"
                    "</Point>"
            "</Placemark>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    string error;
    EXPECT_EQ(0, locations.size());
    EXPECT_TRUE(KmlParserInternal::traverseSubtree(cur, &locations, &error));
    EXPECT_EQ("", error);
    EXPECT_EQ(4, locations.size());

    for (unsigned i = 0; i < locations.size(); ++i) {
        if (i != 2) {
            EXPECT_EQ("Simple placemark", locations[i].name);
            EXPECT_EQ("Attached to the ground.", locations[i].description);
        } else {
            EXPECT_EQ("", locations[i].name);
            EXPECT_EQ("", locations[i].description);
        }
        EXPECT_EQ("-122.0822035425683", locations[i].longitude);
        EXPECT_EQ("37.42228990140251", locations[i].latitude);
        EXPECT_EQ("0", locations[i].elevation);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, TraverseEmptyDoc) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<kml xmlns=\"http://earth.google.com/kml/2.x\"></kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    string error;
    EXPECT_EQ(0, locations.size());
    EXPECT_TRUE(KmlParserInternal::traverseSubtree(cur, &locations, &error));
    EXPECT_EQ("", error);
    EXPECT_EQ(0, locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, TraverseDocNoPlacemarks) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
        "<LineString></LineString>"
        "<name></name>"
        "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    string error;
    EXPECT_EQ(0, locations.size());
    EXPECT_TRUE(KmlParserInternal::traverseSubtree(cur, &locations, &error));
    EXPECT_EQ("", error);
    EXPECT_EQ(0, locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(KmlParserInternal, TraverseNestedDoc) {
    const char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Document>"
            "<Folder>"
                    "<name>Placemarks</name>"
                    "<description>These are just some of the different kinds of placemarks with"
                    "which you can mark your favorite places</description>"
                    "<LookAt>"
                            "<longitude>-122.0839597145766</longitude>"
                            "<latitude>37.42222904525232</latitude>"
                            "<altitude>0</altitude>"
                            "<heading>-148.4122922628044</heading>"
                            "<tilt>40.5575073395506</tilt>"
                            "<range>500.6566641072245</range>"
                    "</LookAt>"
                    "<Placemark>"
                            "<name>Simple placemark</name>"
                            "<description>Attached to the ground.</description>"
                            "<Point>"
                            "<coordinates>-122.0822035425683,37.42228990140251,0</coordinates>"
                            "</Point>"
                    "</Placemark>"
            "</Folder>"
            "</Document>"
            "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    GpsFixArray locations;
    string error;
    EXPECT_EQ(0, locations.size());
    EXPECT_TRUE(KmlParserInternal::traverseSubtree(cur, &locations, &error));
    EXPECT_EQ("", error);
    EXPECT_EQ(1, locations.size());

    EXPECT_EQ("Simple placemark", locations[0].name);
    EXPECT_EQ("Attached to the ground.", locations[0].description);
    EXPECT_EQ("-122.0822035425683", locations[0].longitude);
    EXPECT_EQ("37.42228990140251", locations[0].latitude);
    EXPECT_EQ("0", locations[0].elevation);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}
