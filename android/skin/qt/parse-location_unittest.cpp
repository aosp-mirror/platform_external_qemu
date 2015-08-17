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

#include "android/base/testing/TestTempDir.h"

#include <fstream>
#include <gtest/gtest.h>
#include <libxml/parser.h>

using android::base::TestTempDir;

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
    // also throw some extra whitespace between triples for good measure
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    EXPECT_TRUE(parser.parse_coordinates(cur));
    EXPECT_EQ(3, parser.locations.size());

    EXPECT_EQ("-122.0822035425683", parser.locations[0].longitude);
    EXPECT_EQ("37.42228990140251", parser.locations[0].latitude);
    EXPECT_EQ("0", parser.locations[0].elevation);
    EXPECT_EQ("10.4", parser.locations[1].longitude);
    EXPECT_EQ("39.", parser.locations[1].latitude);
    EXPECT_EQ("20", parser.locations[1].elevation);
    EXPECT_EQ("0", parser.locations[2].longitude);
    EXPECT_EQ("21.4", parser.locations[2].latitude);
    EXPECT_EQ("1", parser.locations[2].elevation);

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

TEST(kml_parser, parse_bad_coordinates) {
    // also throw some bad whitespace in there for good measure
    char * str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
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

    KmlParser parser;
    EXPECT_EQ(0, parser.locations.size());
    EXPECT_TRUE(parser.parse_coordinates(cur));

    // checking that previous malformatted coordinates don't affect later ones
    EXPECT_EQ(0, parser.locations.size());

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
        "<kml xmlns=\"http://earth.google.com/kml/2.x\"></kml>";
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

TEST(kml_parser, parse_noexistent_file) {
    KmlParser parser;
    parser.parse("");
    ASSERT_EQ(0, parser.locations.size());
}

TEST(kml_parser, parse_empty_file) {
    {
        TestTempDir myDir("parse_location_tests");   // creates new temp directory.
        ASSERT_TRUE(myDir.path());      // NULL if error during creation.
        android::base::String path = myDir.makeSubPath("test.kml");

        std::ofstream myfile;
        myfile.open(path.c_str());
        myfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            << "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            << "</kml>";
        myfile.close();

        KmlParser parser;
        parser.parse(string(path.c_str()));
        ASSERT_EQ(0, parser.locations.size());
     }   // destructor removes temp directory and all files under it.
}

TEST(kml_parser, parse_valid_file) {
    {
        TestTempDir myDir("parse_location_tests");
        ASSERT_TRUE(myDir.path()); // NULL if error during creation.
        android::base::String path = myDir.makeSubPath("test.kml");

        std::ofstream myfile;
        myfile.open(path.c_str());
        myfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        << "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
            << "<Placemark>"
                << "<name>Simple placemark</name>"
                << "<description>Attached to the ground.</description>"
                << "<Point>"
                << "<coordinates>-122.0822035425683,37.42228990140251,0</coordinates>"
                << "</Point>"
            << "</Placemark>"
        << "</kml>";
        myfile.close();

        KmlParser parser;
        parser.parse(string(path.c_str()));
        ASSERT_EQ(1, parser.locations.size());
     }   // destructor removes temp directory and all files under it.
}

TEST(kml_parser, parse_valid_complex_file) {
    {
        TestTempDir myDir("parse_location_tests");
        ASSERT_TRUE(myDir.path()); // NULL if error during creation.
        android::base::String path = myDir.makeSubPath("test.kml");

        std::ofstream myfile;
        myfile.open(path.c_str());
        myfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        << "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
        << "<Document>"
            << "<name>KML Samples</name>"
            << "<Style id=\"globeIcon\">"
            << "<IconStyle></IconStyle><LineStyle><width>2</width></LineStyle>"
            << "</Style>"
            << "<Folder>"
                << "<name>Placemarks</name>"
                << "<description>These are just some</description>"
                << "<LookAt>"
                << "<tilt>40.5575073395506</tilt><range>500.6566641072245</range>"
                << "</LookAt>"
                << "<Placemark>"
                    << "<name>Tessellated</name>"
                    << "<visibility>0</visibility>"
                    << "<description>Black line (10 pixels wide), height tracks terrain</description>"
                    << "<LookAt><longitude>-122.0839597145766</longitude></LookAt>"
                    << "<styleUrl>#downArrowIcon</styleUrl>"
                    << "<Point>"
                        << "<altitudeMode>relativeToGround</altitudeMode>"
                        << "<coordinates>-122.084075,37.4220033612141,50</coordinates>"
                    << "</Point>"
                << "</Placemark>"
                << "<Placemark>"
                    << "<name>Transparent</name>"
                    << "<visibility>0</visibility>"
                    << "<styleUrl>#transRedPoly</styleUrl>"
                    << "<Polygon>"
                        << "<extrude>1</extrude>"
                        << "<altitudeMode>relativeToGround</altitudeMode>"
                        << "<outerBoundaryIs>"
                        << "<LinearRing>"
                        << "<coordinates> -122.084075,37.4220033612141,50</coordinates>"
                        << "</LinearRing>"
                        << "</outerBoundaryIs>"
                    << "</Polygon>"
                << "</Placemark>"
            << "</Folder>"
            << "<Placemark>"
                << "<name>Fruity</name>"
                << "<visibility>0</visibility>"
                << "<description><![CDATA[If the <tessellate> tag has a value of n]]></description>"
                << "<LookAt><longitude>-112.0822680013139</longitude></LookAt>"
                << "<LineString>"
                    << "<tessellate>1</tessellate>"
                    << "<coordinates> -122.084075,37.4220033612141,50 </coordinates>"
                << "</LineString>"
            << "</Placemark>"
        << "</Document>"
        << "</kml>";
        myfile.close();

        KmlParser parser;
        parser.parse(string(path.c_str()));
        ASSERT_EQ(3, parser.locations.size());

        EXPECT_EQ("Tessellated", parser.locations[0].name);
        EXPECT_EQ("Black line (10 pixels wide), height tracks terrain", parser.locations[0].description);
        EXPECT_EQ("Transparent", parser.locations[1].name);
        EXPECT_EQ("", parser.locations[1].description);
        EXPECT_EQ("Fruity", parser.locations[2].name);
        EXPECT_EQ("If the <tessellate> tag has a value of n", parser.locations[2].description);

        for (int i = 0; i < parser.locations.size(); ++i) {
            EXPECT_EQ("-122.084075", parser.locations[i].longitude);
            EXPECT_EQ("37.4220033612141", parser.locations[i].latitude);
            EXPECT_EQ("50", parser.locations[i].elevation);
        }
    }   // destructor removes temp directory and all files under it.
}

}