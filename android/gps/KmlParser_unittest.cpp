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
#include "android/base/testing/TestTempDir.h"

#include <fstream>
#include <gtest/gtest.h>

using android::base::TestTempDir;
using std::string;

namespace android_gps {

TEST(KmlParser, ParseNonexistentFile) {
    GpsFixArray locations;
    string error;
    ASSERT_FALSE(KmlParser::parseFile("", &locations, &error));
    ASSERT_EQ(0, locations.size());
    EXPECT_EQ(string("KML document not parsed successfully."), error);
}

TEST(KmlParser, ParseEmptyFile) {
    {
        TestTempDir myDir("parse_location_tests");
        ASSERT_TRUE(myDir.path()); // NULL if error during creation.
        android::base::String path = myDir.makeSubPath("test.kml");

        std::ofstream myfile;
        myfile.open(path.c_str());
        myfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
             "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
             "</kml>";
        myfile.close();

        GpsFixArray locations;
        string error;
        ASSERT_TRUE(KmlParser::parseFile(path.c_str(), &locations, &error));
        EXPECT_EQ(0, locations.size());
        EXPECT_EQ("", error);
     }   // destructor removes temp directory and all files under it.
}

TEST(KmlParser, ParseValidFile) {
    {
        TestTempDir myDir("parse_location_tests");
        ASSERT_TRUE(myDir.path()); // NULL if error during creation.
        android::base::String path = myDir.makeSubPath("test.kml");

        std::ofstream myfile;
        myfile.open(path.c_str());
        myfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
         "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
             "<Placemark>"
                 "<name>Simple placemark</name>"
                 "<description>Attached to the ground.</description>"
                 "<Point>"
                 "<coordinates>-122.0822035425683,37.42228990140251,0</coordinates>"
                 "</Point>"
             "</Placemark>"
         "</kml>";
        myfile.close();

        GpsFixArray locations;
        string error;
        ASSERT_TRUE(KmlParser::parseFile(path.c_str(), &locations, &error));
        EXPECT_EQ(1, locations.size());
        EXPECT_EQ("", error);
     }   // destructor removes temp directory and all files under it.
}

TEST(KmlParser, ParseValidComplexFile) {
    {
        TestTempDir myDir("parse_location_tests");
        ASSERT_TRUE(myDir.path()); // NULL if error during creation.
        android::base::String path = myDir.makeSubPath("test.kml");

        std::ofstream myfile;
        myfile.open(path.c_str());
        myfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
         "<kml xmlns=\"http://earth.google.com/kml/2.x\">"
         "<Document>"
             "<name>KML Samples</name>"
             "<Style id=\"globeIcon\">"
             "<IconStyle></IconStyle><LineStyle><width>2</width></LineStyle>"
             "</Style>"
             "<Folder>"
                 "<name>Placemarks</name>"
                 "<description>These are just some</description>"
                 "<LookAt>"
                 "<tilt>40.5575073395506</tilt><range>500.6566641072245</range>"
                 "</LookAt>"
                 "<Placemark>"
                     "<name>Tessellated</name>"
                     "<visibility>0</visibility>"
                     "<description>Black line (10 pixels wide), height tracks terrain</description>"
                     "<LookAt><longitude>-122.0839597145766</longitude></LookAt>"
                     "<styleUrl>#downArrowIcon</styleUrl>"
                     "<Point>"
                         "<altitudeMode>relativeToGround</altitudeMode>"
                         "<coordinates>-122.084075,37.4220033612141,50</coordinates>"
                     "</Point>"
                 "</Placemark>"
                 "<Placemark>"
                     "<name>Transparent</name>"
                     "<visibility>0</visibility>"
                     "<styleUrl>#transRedPoly</styleUrl>"
                     "<Polygon>"
                         "<extrude>1</extrude>"
                         "<altitudeMode>relativeToGround</altitudeMode>"
                         "<outerBoundaryIs>"
                         "<LinearRing>"
                         "<coordinates> -122.084075,37.4220033612141,50</coordinates>"
                         "</LinearRing>"
                         "</outerBoundaryIs>"
                     "</Polygon>"
                 "</Placemark>"
             "</Folder>"
             "<Placemark>"
                 "<name>Fruity</name>"
                 "<visibility>0</visibility>"
                 "<description><![CDATA[If the <tessellate> tag has a value of n]]></description>"
                 "<LookAt><longitude>-112.0822680013139</longitude></LookAt>"
                 "<LineString>"
                     "<tessellate>1</tessellate>"
                     "<coordinates> -122.084075,37.4220033612141,50 </coordinates>"
                 "</LineString>"
             "</Placemark>"
         "</Document>"
         "</kml>";
        myfile.close();

        GpsFixArray locations;
        string error;
        ASSERT_TRUE(KmlParser::parseFile(path.c_str(), &locations, &error));
        EXPECT_EQ("", error);
        EXPECT_EQ(3, locations.size());

        EXPECT_EQ("Tessellated", locations[0].name);
        EXPECT_EQ("Black line (10 pixels wide), height tracks terrain", locations[0].description);
        EXPECT_EQ("Transparent", locations[1].name);
        EXPECT_EQ("", locations[1].description);
        EXPECT_EQ("Fruity", locations[2].name);
        EXPECT_EQ("If the <tessellate> tag has a value of n", locations[2].description);

        for (unsigned i = 0; i < locations.size(); ++i) {
            EXPECT_EQ("-122.084075", locations[i].longitude);
            EXPECT_EQ("37.4220033612141", locations[i].latitude);
            EXPECT_EQ("50", locations[i].elevation);
        }
    }   // destructor removes temp directory and all files under it.
}

}
