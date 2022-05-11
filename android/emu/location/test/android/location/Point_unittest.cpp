// Copyright (C) 2018 The Android Open Source Project
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
#include "android/base/testing/Utils.h"
#include "android/location/Point.h"

#include <fstream>
#include <gtest/gtest.h>

using android::base::TestTempDir;

namespace android_location {

TEST(Point, ReadNonexistantFile) {
    android::location::Point point("i_dont_exist.pb");

    EXPECT_EQ(nullptr, point.getProtoInfo());
}

TEST(Point, ReadEmptyFile) {
    char text[] = "";

    TestTempDir myDir("location_point_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("point.pb");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    android::location::Point point(path.c_str());

    EXPECT_EQ(nullptr, point.getProtoInfo());
}

TEST(Point, ReadGarbageFile) {
    char text[] = "Some random data\n"
                  "That is not a valid protobuf\n";

    TestTempDir myDir("location_point_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("point.pb");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    android::location::Point point(path.c_str());

    EXPECT_EQ(nullptr, point.getProtoInfo());
}

TEST(Point, WriteReadFile) {
    // Create a Point protobuf in memory
    emulator_location::PointMetadata ptMetadata;
    const uint64_t nowMsec = android::base::System::get()->getHighResTimeUs() / 1000LL;

    ptMetadata.set_logical_name("Point name");
    ptMetadata.set_creation_time(nowMsec);
    ptMetadata.set_latitude(22.2222);
    ptMetadata.set_longitude(-123.4567);
    ptMetadata.set_altitude(11.1111);
    ptMetadata.set_description("A single point");
    ptMetadata.set_address("123 Main St., Anytown, USA");

    // Write to protobuf to disk
    TestTempDir myDir("location_point_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("point.pb");
    std::ofstream outStream(path.c_str(), std::ofstream::binary);
    ptMetadata.SerializeToOstream(&outStream);
    outStream.close();

    EXPECT_TRUE(android::base::System::get()->pathExists(path));
    android::base::System::FileSize fileSize;
    android::base::System::get()->pathFileSize(path, &fileSize);
    EXPECT_TRUE(fileSize > 0);

    // Read the protobuf
    android::location::Point inPoint(path.c_str());

    const emulator_location::PointMetadata* inputMetadata = inPoint.getProtoInfo();

    EXPECT_NE(nullptr, inputMetadata);

    EXPECT_STREQ("Point name",     inputMetadata->logical_name().c_str());
    EXPECT_EQ(nowMsec,             inputMetadata->creation_time());
    EXPECT_FLOAT_EQ(  22.2222,     inputMetadata->latitude());
    EXPECT_FLOAT_EQ(-123.4567,     inputMetadata->longitude());
    EXPECT_FLOAT_EQ(  11.1111,     inputMetadata->altitude());
    EXPECT_STREQ("A single point", inputMetadata->description().c_str());
    EXPECT_STREQ("123 Main St., Anytown, USA",
                                   inputMetadata->address().c_str());
}
}
