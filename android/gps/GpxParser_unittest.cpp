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

#include "android/base/testing/TestTempDir.h"
#include "android/gps/GpxParser.h"

#include <fstream>
#include <gtest/gtest.h>

using android::base::TestTempDir;
using std::string;

namespace android_gps {

TEST(GpxParser, ParseFileNotFound) {
    GpsFixArray locations;
    string error;
    bool isOk = GpxParser::parseFile("i_dont_exist.gpx", &locations, &error);
    EXPECT_FALSE(isOk);
}

TEST(GpxParser, ParseFileEmpty) {
    char *text =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "</gpx>";

    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    android::base::String path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
    EXPECT_TRUE(isOk);
    EXPECT_EQ(0, locations.size());
}

TEST(GpxParser, ParseFileEmptyRteTrk) {
    char *text =
            "<?xml version=\"1.0\"?>"
            "<gpx>"
            "<rte>"
            "</rte>"
            "<trk>"
            "<trkseg>"
            "</trkseg>"
            "</trk>"
            "</gpx>";

    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    android::base::String path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
    EXPECT_TRUE(isOk);
    EXPECT_EQ(0, locations.size());
}

TEST(GpxParser, ParseFileValid) {
    char *text =
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

    TestTempDir myDir("parse_location_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    android::base::String path = myDir.makeSubPath("test.gpx");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    GpsFixArray locations;
    string error;
    bool isOk = GpxParser::parseFile(path.c_str(), &locations, &error);
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