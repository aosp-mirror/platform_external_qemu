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
#include "parse-location.h"
#include <libxml/parser.h>

#include <gtest/gtest.h>

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android_skin {

TEST(kml_parser, find_coordinates) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

    KmlParser parser;
    ASSERT_EQ(parser.locations.size(), 0);
    ASSERT_NE(parser.find_coordinates(cur), (void *) NULL);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, find_no_coordinates) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

    KmlParser parser;
    EXPECT_EQ(parser.locations.size(), 0);
    EXPECT_EQ(parser.find_coordinates(cur), (void *) NULL);

    EXPECT_EQ((void *) NULL, (void *) NULL);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_empty_coordinates) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
        "<Placemark>"
        "<Point><coordinates></coordinates>"
        "</Point>"
        "</Placemark>"
        "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    EXPECT_FALSE(parser.parse_coordinates(cur));
    EXPECT_EQ(0, parser.locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_no_coordinates_empty_point) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
        "<Placemark>"
        "<Point>"
        "</Point>"
        "</Placemark>"
        "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    EXPECT_FALSE(parser.parse_coordinates(cur));
    EXPECT_EQ(0, parser.locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_one_coordinate) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    EXPECT_TRUE(parser.parse_coordinates(cur));
    EXPECT_EQ(1, parser.locations.size());

    EXPECT_EQ("-122.0822035425683", parser.locations[0].longitude);
    EXPECT_EQ("37.42228990140251", parser.locations[0].latitude);
    EXPECT_EQ("0", parser.locations[0].elevation);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_mult_coordinates) {
    // also throw some bad whitespace in there for good measure
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
        "<Placemark>"
        "<LineString>"
        "<coordinates>-122.0822035425683,37.42228990140251,0 \
        -122.0822035425683,37.42228990140251,0\t\t-122.0822035425683,37.42228990140251,0"
        "</coordinates>"
        "</LineString>"
        "</Placemark>"
        "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    EXPECT_TRUE(parser.parse_coordinates(cur));
    EXPECT_EQ(3, parser.locations.size());

    for (int i = 0; i < parser.locations.size(); ++i) {
        EXPECT_EQ("-122.0822035425683", parser.locations[i].longitude);
        EXPECT_EQ("37.42228990140251", parser.locations[i].latitude);
        EXPECT_EQ("0", parser.locations[i].elevation);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_bad_coordinates) {
    // also throw some bad whitespace in there for good measure
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
        "<Placemark>"
        "<LineString>"
        "<coordinates>-122.0822035425683,\
        -122.0822035425683,0\t\t-122.0822035425683,37.42228990140251,0"
        "</coordinates>"
        "</LineString>"
        "</Placemark>"
        "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    EXPECT_TRUE(parser.parse_coordinates(cur));

    // checking that previous malformatted coordinates don't affect later ones
    EXPECT_EQ(1, parser.locations.size());

    for (int i = 0; i < parser.locations.size(); ++i) {
        EXPECT_EQ("-122.0822035425683", parser.locations[i].longitude);
        EXPECT_EQ("37.42228990140251", parser.locations[i].latitude);
        EXPECT_EQ("0", parser.locations[i].elevation);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_location_normal) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    parser.traverse_subtree(cur);
    EXPECT_EQ(1, parser.locations.size());

    for (int i = 0; i < parser.locations.size(); ++i) {
        EXPECT_EQ("Simple placemark", parser.locations[i].name);
        EXPECT_EQ("Attached to the ground.", parser.locations[i].description);
        EXPECT_EQ("-122.0822035425683", parser.locations[i].longitude);
        EXPECT_EQ("37.42228990140251", parser.locations[i].latitude);
        EXPECT_EQ("0", parser.locations[i].elevation);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_location_normal_missing_optional_fields) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    parser.traverse_subtree(cur);
    EXPECT_EQ(1, parser.locations.size());

    for (int i = 0; i < parser.locations.size(); ++i) {
        EXPECT_EQ("", parser.locations[i].name);
        EXPECT_EQ("", parser.locations[i].description);
        EXPECT_EQ("-122.0822035425683", parser.locations[i].longitude);
        EXPECT_EQ("37.42228990140251", parser.locations[i].latitude);
        EXPECT_EQ("0", parser.locations[i].elevation);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_location_missing_required_fields) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            "<Placemark>"
                "<name>Simple placemark</name>"
                "<description>Attached to the ground.</description>"
            "</Placemark>"
        "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    parser.traverse_subtree(cur);
    EXPECT_EQ(0, parser.locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_location_name_only_first) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    parser.traverse_subtree(cur);
    EXPECT_EQ(2, parser.locations.size());

    EXPECT_EQ("Simple placemark", parser.locations[0].name);
    EXPECT_EQ("Attached to the ground.", parser.locations[0].description);
    EXPECT_EQ("", parser.locations[1].name);
    EXPECT_EQ("", parser.locations[1].description);

    for (int i = 0; i < parser.locations.size(); ++i) {
        EXPECT_EQ("-122.0822035425683", parser.locations[i].longitude);
        EXPECT_EQ("37.42228990140251", parser.locations[i].latitude);
        EXPECT_EQ("0", parser.locations[i].elevation);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_multiple_locations) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    parser.traverse_subtree(cur);
    EXPECT_EQ(4, parser.locations.size());

    for (int i = 0; i < parser.locations.size(); ++i) {
        if (i != 2) {
            EXPECT_EQ("Simple placemark", parser.locations[i].name);
            EXPECT_EQ("Attached to the ground.", parser.locations[i].description);
        } else {
            EXPECT_EQ("", parser.locations[i].name);
            EXPECT_EQ("", parser.locations[i].description);
        }
        EXPECT_EQ("-122.0822035425683", parser.locations[i].longitude);
        EXPECT_EQ("37.42228990140251", parser.locations[i].latitude);
        EXPECT_EQ("0", parser.locations[i].elevation);
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, traverse_empty_doc) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
        "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    parser.traverse_subtree(cur);
    EXPECT_EQ(0, parser.locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, traverse_bad_doc) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
        "<LineString></LineString>"
        "<name></name>"
        "</kml>";
    xmlDoc * doc = xmlReadDoc((const xmlChar *) str, NULL, NULL, 0);
    xmlNode * cur = xmlDocGetRootElement(doc);
    ASSERT_NE(cur, (void *) NULL);

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    parser.traverse_subtree(cur);
    EXPECT_EQ(0, parser.locations.size());

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, traverse_nested_doc) {
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    parser.traverse_subtree(cur);
    EXPECT_EQ(1, parser.locations.size());

    EXPECT_EQ("Simple placemark", parser.locations[0].name);
    EXPECT_EQ("Attached to the ground.", parser.locations[0].description);
    EXPECT_EQ("-122.0822035425683", parser.locations[0].longitude);
    EXPECT_EQ("37.42228990140251", parser.locations[0].latitude);
    EXPECT_EQ("0", parser.locations[0].elevation);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

}