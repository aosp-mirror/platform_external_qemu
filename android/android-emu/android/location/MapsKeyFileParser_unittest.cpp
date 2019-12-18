// Copyright (C) 2019 The Android Open Source Project
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
#include "android/location/MapsKeyFileParser.h"

#include <fstream>
#include <gtest/gtest.h>

using android::base::TestTempDir;

namespace android_location {

TEST(MapsKeyFileParser, ReadNonexistantFile) {
    auto key = android::location::parseMapsKeyFromFile("i_dont_exist.key");

    EXPECT_TRUE(key.empty());
}

TEST(MapsKeyFileParser, ReadEmptyFile) {
    char text[] = "";

    TestTempDir myDir("MapsKeyFileParser_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("empty.key");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    auto key = android::location::parseMapsKeyFromFile(path);
    EXPECT_TRUE(key.empty());
}

#ifdef _MSC_VER
TEST(MapsKeyFileParser, DISABLED_ReadGarbageFile) {
#else
TEST(MapsKeyFileParser, ReadGarbageFile) {
#endif
    char text[] = "Some random data\n"
                  "That is not a valid protobuf\n";

    TestTempDir myDir("MapsKeyFileParser_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("bad.key");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    auto key = android::location::parseMapsKeyFromFile(path);
    EXPECT_TRUE(key.empty());
}

TEST(MapsKeyFileParser, ReadValidFile) {
    char text[] = "61 62 63 64 65 66 67 68 69 67 6B 6C 6D 6E 6F "
                  "70 71 72 73 74 75 76 77 78 79 7A 20 41 42 43 "
                  "44 45 46 47 48 49 4A 4B 4C 4D 4E 4F 50 51 52 "
                  "53 54 55 56 57 58 59 5A 60 31 32 33 34 35 36 "
                  "37 38 39 30 2D 3D 7E 21 40 23 24 25 5E 26 2A "
                  "28 29 5F 2B 5B 5D 5C 3B 27 2C 2E 2F 7B 7D 7C "
                  "3A 22 3C 3E 3F 0A ";

    TestTempDir myDir("MapsKeyFileParser_tests");
    ASSERT_TRUE(myDir.path()); // NULL if error during creation.
    std::string path = myDir.makeSubPath("bad.key");

    std::ofstream myfile;
    myfile.open(path.c_str());
    myfile << text;
    myfile.close();

    auto key = android::location::parseMapsKeyFromFile(path);
    EXPECT_STREQ(key.c_str(),
                 "abcdefghigklmnopqrstuvwxyz "
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ`"
                 "1234567890-=~!@#$%^&*()_+[]"
                 "\\;',./{}|:\"<>?\n");
}
}  // namespace android_location
