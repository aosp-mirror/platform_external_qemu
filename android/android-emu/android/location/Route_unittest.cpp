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
#include "android/location/Route.h"

#include "android/location/proto/point.pb.h"

#include <fstream>
#include <gtest/gtest.h>

using android::base::TestTempDir;

namespace android_location {

TEST(Route, ReadNonexistantFile) {
    android::location::Route route("i_dont_exist.pb");

    EXPECT_EQ(nullptr, route.getProtoInfo());
}

TEST(Route, ReadEmptyFile) {
    char text[] = "";

    TestTempDir myDir("location_route_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("route.pb");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    android::location::Route route(path.c_str());

    EXPECT_EQ(nullptr, route.getProtoInfo());
}

TEST(Route, ReadGarbageFile) {
    char text[] = "Some random data\n"
                  "That is not a valid protobuf\n";

    TestTempDir myDir("location_route_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("route.pb");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    android::location::Route route(path.c_str());

    EXPECT_EQ(nullptr, route.getProtoInfo());
}

TEST(Route, WriteReadDefaultFile) {
    // Create a Route protobuf in memory, using default values
    emulator_location::RouteMetadata rtMetadata;
    const uint64_t nowMsec = android::base::System::get()->getHighResTimeUs() / 1000LL;

    rtMetadata.set_logical_name("Country roads");
    rtMetadata.set_creation_time(nowMsec);

    // Write to protobuf to disk
    TestTempDir myDir("location_route_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("route.pb");
    std::ofstream outStream(path.c_str(), std::ofstream::binary);
    rtMetadata.SerializeToOstream(&outStream);
    outStream.close();

    EXPECT_TRUE(android::base::System::get()->pathExists(path));
    android::base::System::FileSize fileSize;
    android::base::System::get()->pathFileSize(path, &fileSize);
    EXPECT_TRUE(fileSize > 0);

    // Read the protobuf
    android::location::Route inRoute(path.c_str());

    const emulator_location::RouteMetadata* inputMetadata = inRoute.getProtoInfo();

    EXPECT_NE(nullptr, inputMetadata);

    EXPECT_STREQ("Country roads",
                       inputMetadata->logical_name().c_str());
    EXPECT_EQ(nowMsec, inputMetadata->creation_time());
    // Default values:
    EXPECT_STREQ("",   inputMetadata->description().c_str());
    EXPECT_EQ(emulator_location::RouteMetadata_Mode_DRIVING,
                       inputMetadata->mode_of_travel());
    EXPECT_EQ(0,       inputMetadata->duration());
}

TEST(Route, WriteReadExplicitFile) {
    // Create a Route protobuf in memory, explicitly setting all the fields
    emulator_location::RouteMetadata rtMetadata;
    const uint64_t nowMsec = android::base::System::get()->getHighResTimeUs() / 1000LL;

    rtMetadata.set_logical_name("Grandmother's house");
    rtMetadata.set_creation_time(nowMsec);
    rtMetadata.set_description("Over the river and through the wood");
    rtMetadata.set_mode_of_travel(emulator_location::RouteMetadata_Mode_WALKING);
    rtMetadata.set_duration(13579LL);

    // Write to protobuf to disk
    TestTempDir myDir("location_route_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("route.pb");
    std::ofstream outStream(path.c_str(), std::ofstream::binary);
    rtMetadata.SerializeToOstream(&outStream);
    outStream.close();

    EXPECT_TRUE(android::base::System::get()->pathExists(path));
    android::base::System::FileSize fileSize;
    android::base::System::get()->pathFileSize(path, &fileSize);
    EXPECT_TRUE(fileSize > 0);

    // Read the protobuf
    android::location::Route inRoute(path.c_str());

    const emulator_location::RouteMetadata* inputMetadata = inRoute.getProtoInfo();

    EXPECT_NE(nullptr, inputMetadata);

    EXPECT_STREQ("Grandmother's house",
                       inputMetadata->logical_name().c_str());
    EXPECT_EQ(nowMsec, inputMetadata->creation_time());
    EXPECT_STREQ("Over the river and through the wood",
                       inputMetadata->description().c_str());
    EXPECT_EQ(emulator_location::RouteMetadata_Mode_WALKING,
                       inputMetadata->mode_of_travel());
    EXPECT_EQ(13579LL, inputMetadata->duration());
}
}
