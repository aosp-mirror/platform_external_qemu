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

#include "android/gps/internal/GpxParserInternal.h"

#include <gtest/gtest.h>

namespace android_gps_internal {

TEST(GpxParserInternal, ParseLocationMissingLatitude) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "<wpt lon=\"9.81\">"
            "<ele>6.02</ele>"
            "<name>Name</name>"
            "<desc>Desc</desc>"
            "</wpt>"
            "</gpx>";

    xmlDocPtr doc = xmlReadDoc((const xmlChar *) text, NULL, NULL, 0);
    ASSERT_NE(doc, (void *) NULL);

    xmlNode *root = xmlDocGetRootElement(doc);
    ASSERT_NE(root, (void *) NULL);

    xmlNode *wptNode = root->children;
    ASSERT_NE(wptNode, (void *) NULL);

    GpsFix wpt;
    std::string error;
    bool isOk = GpxParserInternal::parseLocation(wptNode, doc, &wpt, &error);
    EXPECT_FALSE(isOk);
    GpxParserInternal::cleanupXmlDoc(doc);
}

TEST(GpxParserInternal, ParseLocationMissingLongitude) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "<wpt lat=\"3.1415\">"
            "<ele>6.02</ele>"
            "<name>Name</name>"
            "<desc>Desc</desc>"
            "</wpt>"
            "</gpx>";

    xmlDocPtr doc = xmlReadDoc((const xmlChar *) text, NULL, NULL, 0);
    ASSERT_NE(doc, (void *) NULL);

    xmlNode *root = xmlDocGetRootElement(doc);
    ASSERT_NE(root, (void *) NULL);

    xmlNode *wptNode = root->children;
    ASSERT_NE(wptNode, (void *) NULL);

    GpsFix wpt;
    std::string error;
    bool isOk = GpxParserInternal::parseLocation(wptNode, doc, &wpt, &error);
    EXPECT_FALSE(isOk);
    GpxParserInternal::cleanupXmlDoc(doc);
}

TEST(GpxParserInternal, ParseValidLocation) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "<wpt lon=\"9.81\" lat=\"3.1415\">"
            "<ele>6.02</ele>"
            "<name>Name</name>"
            "<desc>Desc</desc>"
            "</wpt>"
            "</gpx>";

    xmlDocPtr doc = xmlReadDoc((const xmlChar *) text, NULL, NULL, 0);
    ASSERT_NE(doc, (void *) NULL);

    xmlNode *root = xmlDocGetRootElement(doc);
    ASSERT_NE(root, (void *) NULL);

    xmlNode *wptNode = root->children;
    ASSERT_NE(wptNode, (void *) NULL);

    GpsFix wpt;
    std::string error;
    bool isOk = GpxParserInternal::parseLocation(wptNode, doc, &wpt, &error);
    EXPECT_TRUE(isOk);

    EXPECT_EQ("Desc", wpt.description);
    EXPECT_EQ("6.02", wpt.elevation);
    EXPECT_EQ("3.1415", wpt.latitude);
    EXPECT_EQ("9.81", wpt.longitude);
    EXPECT_EQ("Name", wpt.name);

    GpxParserInternal::cleanupXmlDoc(doc);
}

TEST(GpxParserInternal, ParseValidDocument) {
    char text[] =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
                "<wpt lon=\"\" lat=\"\">"
                "<name>Wpt 1</name>"
                "</wpt>"
                "<wpt lon=\"\" lat=\"\">"
                "<name>Wpt 2</name>"
                "</wpt>"
            "<rte>"
                "<rtept lon=\"\" lat=\"\">"
                "<name>Rtept 1</name>"
                "</rtept>"
                "<rtept lon=\"\" lat=\"\">"
                "<name>Rtept 2</name>"
                "</rtept>"
            "</rte>"
            "<trk>"
            "<trkseg>"
                "<trkpt lon=\"\" lat=\"\">"
                "<name>Trkpt 1-1</name>"
                "</trkpt>"
                "<trkpt lon=\"\" lat=\"\">"
                "<name>Trkpt 1-2</name>"
                "</trkpt>"
            "</trkseg>"
            "<trkseg>"
                "<trkpt lon=\"\" lat=\"\">"
                "<name>Trkpt 2-1</name>"
                "</trkpt>"
                "<trkpt lon=\"\" lat=\"\">"
                "<name>Trkpt 2-2</name>"
                "</trkpt>"
            "</trkseg>"
            "</trk>"
            "</gpx>";

    xmlDocPtr doc = xmlReadDoc((const xmlChar *) text, NULL, NULL, 0);
    ASSERT_NE(doc, (void *) NULL);

    GpsFixArray locations;
    std::string error;
    bool isOk = GpxParserInternal::parse(doc, &locations, &error);
    EXPECT_TRUE(isOk);
    EXPECT_EQ(8, locations.size());

    EXPECT_EQ("Wpt 1", locations[0].name);
    EXPECT_EQ("Wpt 2", locations[1].name);
    EXPECT_EQ("Rtept 1", locations[2].name);
    EXPECT_EQ("Rtept 2", locations[3].name);
    EXPECT_EQ("Trkpt 1-1", locations[4].name);
    EXPECT_EQ("Trkpt 1-2", locations[5].name);
    EXPECT_EQ("Trkpt 2-1", locations[6].name);
    EXPECT_EQ("Trkpt 2-2", locations[7].name);
}

}
