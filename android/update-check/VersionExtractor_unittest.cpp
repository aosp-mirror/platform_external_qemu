// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/update-check/VersionExtractor.h"

#include <gtest/gtest.h>

#include <string>

using android::base::Version;
using android::update_check::VersionExtractor;

TEST(VersionExtractorTest, validVersion) {
    std::string xml =
            R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repository/10"> )"
                    R"(<sdk:tool>)"
                    R"(<sdk:revision>)"
                    R"(<sdk:major>1</sdk:major>)"
                    R"(<sdk:minor>2</sdk:minor>)"
                    R"(<sdk:micro>3</sdk:micro>)"
                    R"(</sdk:revision>)"
                    R"(</sdk:tool>)"
                    R"(</sdk:sdk-repository>)";

    VersionExtractor vx;
    EXPECT_EQ(Version(1, 2, 3), vx.extractVersion(xml));
}

TEST(VersionExtractorTest, pickMaxVersion) {
    std::string xml =
            R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repository/10"> )"
                    R"(<sdk:tool>)"
                    R"(<sdk:revision>)"
                    R"(<sdk:major>1</sdk:major>)"
                    R"(<sdk:minor>2</sdk:minor>)"
                    R"(<sdk:micro>30</sdk:micro>)"
                    R"(</sdk:revision>)"
                    R"(</sdk:tool>)"
                    R"(<sdk:tool>)"
                    R"(<sdk:revision>)"
                    R"(<sdk:major>1</sdk:major>)"
                    R"(<sdk:minor>2</sdk:minor>)"
                    R"(<sdk:micro>0</sdk:micro>)"
                    R"(</sdk:revision>)"
                    R"(</sdk:tool>)"
                    R"(<sdk:tool>)"
                    R"(<sdk:revision>)"
                    R"(<sdk:major>0</sdk:major>)"
                    R"(<sdk:minor>99</sdk:minor>)"
                    R"(<sdk:micro>09</sdk:micro>)"
                    R"(</sdk:revision>)"
                    R"(</sdk:tool>)"
                    R"(</sdk:sdk-repository>)";

    VersionExtractor vx;
    EXPECT_EQ(Version(1, 2, 30), vx.extractVersion(xml));
}

TEST(VersionExtractorTest, badVersion) {
    // bad xml
    std::string xml =
            R"(sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repository/10"> )"
                    R"(<sdk:tool>)"
                    R"(<sdk:revision>)"
                    R"(<sdk:major>1</sdk:major>)"
                    R"(<sdk:minor>2</sdk:minor>)"
                    R"(<sdk:micro>3</sdk:micro>)"
                    R"(</sdk:revision>)"
                    R"(</sdk:tool>)"
                    R"(</sdk:sdk-repository>)";

    VersionExtractor vx;
    EXPECT_EQ(Version::Invalid(), vx.extractVersion(xml));

    // no namespace
    xml = R"(<sdk-repository>)"
            R"(<tool>)"
            R"(<revision>)"
            R"(<major>1</major>)"
            R"(<minor>2</minor>)"
            R"(<micro>3</micro>)"
            R"(</revision>)"
            R"(</tool>)"
            R"(</sdk-repository>)";

    EXPECT_EQ(Version::Invalid(), vx.extractVersion(xml));

    // no 'tool' element
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repository/10"> )"
            R"(<sdk:tool1>)"
            R"(<sdk:revision>)"
            R"(<sdk:major>1</sdk:major>)"
            R"(<sdk:minor>2</sdk:minor>)"
            R"(<sdk:micro>3</sdk:micro>)"
            R"(</sdk:revision>)"
            R"(</sdk:tool1>)"
            R"(</sdk:sdk-repository>)";

    EXPECT_EQ(Version::Invalid(), vx.extractVersion(xml));

    // wrong namespace
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.windows-phone.com/sdk/iOS/repository/10"> )"
            R"(<sdk:tool>)"
            R"(<sdk:revision>)"
            R"(<sdk:major>1</sdk:major>)"
            R"(<sdk:minor>2</sdk:minor>)"
            R"(<sdk:micro>3</sdk:micro>)"
            R"(</sdk:revision>)"
            R"(</sdk:tool>)"
            R"(</sdk:sdk-repository>)";

    EXPECT_EQ(Version::Invalid(), vx.extractVersion(xml));

    // no 'revision'
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repository/10"> )"
            R"(<sdk:tool>)"
            R"(<sdk:evision>)"
            R"(<sdk:major>1</sdk:major>)"
            R"(<sdk:minor>2</sdk:minor>)"
            R"(<sdk:micro>3</sdk:micro>)"
            R"(</sdk:evision>)"
            R"(</sdk:tool>)"
            R"(</sdk:sdk-repository>)";

    EXPECT_EQ(Version::Invalid(), vx.extractVersion(xml));

    // no major
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repository/10"> )"
            R"(<sdk:tool>)"
            R"(<sdk:revision>)"
            R"(<sdk:minor>2</sdk:minor>)"
            R"(<sdk:micro>3</sdk:micro>)"
            R"(</sdk:revision>)"
            R"(</sdk:tool>)"
            R"(</sdk:sdk-repository>)";

    EXPECT_EQ(Version::Invalid(), vx.extractVersion(xml));

    // no minor
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repository/10"> )"
            R"(<sdk:tool>)"
            R"(<sdk:revision>)"
            R"(<sdk:major>2</sdk:major>)"
            R"(<sdk:micro>3</sdk:micro>)"
            R"(</sdk:revision>)"
            R"(</sdk:tool>)"
            R"(</sdk:sdk-repository>)";

    EXPECT_EQ(Version::Invalid(), vx.extractVersion(xml));

    // no micro
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repository/10"> )"
            R"(<sdk:tool>)"
            R"(<sdk:revision>)"
            R"(<sdk:major>1</sdk:major>)"
            R"(<sdk:minor>2</sdk:minor>)"
            R"(</sdk:revision>)"
            R"(</sdk:tool>)"
            R"(</sdk:sdk-repository>)";

    EXPECT_EQ(Version::Invalid(), vx.extractVersion(xml));

    // bad number in version
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repository/10"> )"
            R"(<sdk:tool>)"
            R"(<sdk:revision>)"
            R"(<sdk:major>1asd</sdk:major>)"
            R"(<sdk:minor>2</sdk:minor>)"
            R"(<sdk:micro>3</sdk:micro>)"
            R"(</sdk:revision>)"
            R"(</sdk:tool>)"
            R"(</sdk:sdk-repository>)";

    EXPECT_EQ(Version::Invalid(), vx.extractVersion(xml));

    // bad number in version 2
    xml = R"(<sdk:sdk-repository xmlns:sdk="http://schemas.android.com/sdk/android/repository/10"> )"
            R"(<sdk:tool>)"
            R"(<sdk:revision>)"
            R"(<sdk:major></sdk:major>)"
            R"(<sdk:minor>2</sdk:minor>)"
            R"(<sdk:micro>3</sdk:micro>)"
            R"(</sdk:revision>)"
            R"(</sdk:tool>)"
            R"(</sdk:sdk-repository>)";

    EXPECT_EQ(Version::Invalid(), vx.extractVersion(xml));
}
