// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/verified-boot/load_config.h"
#include "android/utils/file_io.h"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <string>
#include <vector>

using ::android::verifiedboot::getParameters;
using ::android::verifiedboot::getParametersFromFile;
using ::android::verifiedboot::Status;

// Simple baseline test.
TEST(VerifiedBootLoadConfigTest, Baseline) {
    const std::string textproto =
            "major_version: 1\n"
            "dm_param: \"foo\"\n"
            "param: \"bar\"";
    std::vector<std::string> params;
    EXPECT_EQ(Status::OK, getParameters(textproto, &params));
    EXPECT_EQ(2, params.size());
    EXPECT_EQ("dm=\"foo\"", params[0]);
    EXPECT_EQ("bar", params[1]);
}

// Create a file and read the proto from it
TEST(VerifiedBootLoadConfigTest, ReadFromFile) {
    const char* path = tmpnam(NULL);
    const std::string textproto =
            "major_version: 1\n"
            "dm_param: \"foo\"\n"
            "param: \"bar\"";
    int fd = open(path, O_CREAT | O_WRONLY | O_EXCL, 0666);
    ASSERT_LT(-1, fd);
    write(fd, textproto.c_str(), textproto.length());
    close(fd);

    std::vector<std::string> params;
    EXPECT_EQ(Status::OK, getParametersFromFile(path, &params));
    EXPECT_EQ(2, params.size());
    EXPECT_EQ("dm=\"foo\"", params[0]);
    EXPECT_EQ("bar", params[1]);

    android_unlink(path);
}

// Pass a NULL to the file API
TEST(VerifiedBootLoadConfigTest, NullPathname) {
    std::vector<std::string> params;
    EXPECT_EQ(Status::CouldNotOpenFile, getParametersFromFile(NULL, &params));
}

// Try to read from invalid pathname
TEST(VerifiedBootLoadConfigTest, ReadFromInvalidPathname) {
    const char* path = tmpnam(NULL);
    std::vector<std::string> params;
    EXPECT_EQ(Status::CouldNotOpenFile, getParametersFromFile(path, &params));
}

// Use real-world parameters to flush out problems that might come up with
// read world data (such as unexpected illegal characters or formatting).
TEST(VerifiedBootLoadConfigTest, RealWorldParams) {
    const std::string textproto =
            "major_version: 1\n"
            "dm_param: \"1\"\n"
            "dm_param: \"vroot\"\n"
            "dm_param: \"none\"\n"
            "dm_param: \"ro\"\n"
            "dm_param: \"1,0\"\n"
            "dm_param: \"5159992\"\n"
            "dm_param: \"verity\"\n"
            "dm_param: \"1\"\n"
            "dm_param: \"PARTUUID=1ff336db-96f8-4906-9c0b-149da994cf78\"\n"
            "dm_param: \"PARTUUID=1ff336db-96f8-4906-9c0b-149da994cf78\"\n"
            "dm_param: \"4096\"\n"
            "dm_param: \"4096\"\n"
            "dm_param: \"644999\"\n"
            "dm_param: \"644999\"\n"
            "dm_param: \"sha1\"\n"
            "dm_param: \"07e8da3c6ffe2ea52d14ad342beb6bb15060cd45\"\n"
            "dm_param: \"27e6b2075f9a47160d21f824b923f4ebdb755c3f\"\n"
            "dm_param: \"1\"\n"
            "dm_param: \"ignore_zero_blocks\"\n"
            "param: \"androidboot.veritymode=enforcing\"\n"
            "param: \"root=/dev/dm-0\"";
    std::vector<std::string> params;
    EXPECT_EQ(Status::OK, getParameters(textproto, &params));
    EXPECT_EQ(3, params.size());
    EXPECT_EQ(
            "dm=\"1 vroot none ro 1,0 5159992 verity 1 "
            "PARTUUID=1ff336db-96f8-4906-9c0b-149da994cf78 "
            "PARTUUID=1ff336db-96f8-4906-9c0b-149da994cf78 "
            "4096 4096 644999 644999 sha1 "
            "07e8da3c6ffe2ea52d14ad342beb6bb15060cd45 "
            "27e6b2075f9a47160d21f824b923f4ebdb755c3f "
            "1 ignore_zero_blocks\"",
            params[0]);
    EXPECT_EQ("androidboot.veritymode=enforcing", params[1]);
    EXPECT_EQ("root=/dev/dm-0", params[2]);
}

// Pass data that can not be parsed as a proto.
TEST(VerifiedBootLoadConfigTest, ParseError) {
    const std::string textproto = "::";
    std::vector<std::string> params;
    EXPECT_EQ(Status::ParseError, getParameters(textproto, &params));
}

// Pass the wrong type for version.
TEST(VerifiedBootLoadConfigTest, IntTypeExpected) {
    const std::string textproto =
            "major_version: \"1\"\n"
            "dm_param: \"foo\"\n"
            "param: \"bar\"";
    std::vector<std::string> params;
    EXPECT_EQ(Status::ParseError, getParameters(textproto, &params));
}

// Pass the wrong type for dm_param.
TEST(VerifiedBootLoadConfigTest, DMParamNeedsString) {
    const std::string textproto =
            "major_version: 1\n"
            "dm_param: 5\n"
            "param: \"bar\"";
    std::vector<std::string> params;
    EXPECT_EQ(Status::ParseError, getParameters(textproto, &params));
}

// Pass the wrong type for param.
TEST(VerifiedBootLoadConfigTest, ParamNeedsString) {
    const std::string textproto =
            "major_version: 1\n"
            "dm_param: \"foo\"\n"
            "param: 5";
    std::vector<std::string> params;
    EXPECT_EQ(Status::ParseError, getParameters(textproto, &params));
}

// Pass in an empty param.
TEST(VerifiedBootLoadConfigTest, EmptyParameter) {
    const std::string textproto =
            "major_version: 1\n"
            "dm_param: \"foo\"\n"
            "param: \"\"";
    std::vector<std::string> params;
    EXPECT_EQ(Status::EmptyParameter, getParameters(textproto, &params));
}

// Pass in an empty dm_param.
TEST(VerifiedBootLoadConfigTest, DMEmptyParameter) {
    const std::string textproto =
            "major_version: 1\n"
            "dm_param: \"\"\n"
            "param: \"foo\"";
    std::vector<std::string> params;
    EXPECT_EQ(Status::EmptyParameter, getParameters(textproto, &params));
}

// Test for illegal characters in param and dm_param.
TEST(VerifiedBootLoadConfigTest, IllegalChars) {
    const std::string baseproto =
            "major_version: 1\n"
            "dm_param: \"foo\"\n"
            "param: \"foo\"\n";

    const std::string illegal_chars = " !@#$%^&*()+[]{}|';:?<>";
    std::vector<std::string> params;

    for (const char c : illegal_chars) {
        const std::string textproto = baseproto + "param: \"aaa" + c + "bbb\"";
        EXPECT_EQ(Status::IllegalCharacter, getParameters(textproto, &params));
    }

    for (const char c : illegal_chars) {
        const std::string textproto =
                baseproto + "dm_param: \"ccc" + c + "ddd\"";
        EXPECT_EQ(Status::IllegalCharacter, getParameters(textproto, &params));
    }
}

// Test for illegal quoted characters.
TEST(VerifiedBootLoadConfigTest, IllegalQuotedChars) {
    const std::string baseproto =
            "major_version: 1\n"
            "dm_param: \"foo\"\n"
            "param: \"foo\"\n";

    const std::string illegal_chars = "\"\\";
    std::vector<std::string> params;

    for (const char c : illegal_chars) {
        const std::string textproto =
                baseproto + "param: \"aaa\\" + c + "bbb\"";
        EXPECT_EQ(Status::IllegalCharacter, getParameters(textproto, &params));
    }

    for (const char c : illegal_chars) {
        const std::string textproto =
                baseproto + "dm_param: \"ccc\\" + c + "ddd\"";
        EXPECT_EQ(Status::IllegalCharacter, getParameters(textproto, &params));
    }
}

// Does not pass in any dm_params.
TEST(VerifiedBootLoadConfigTest, MissingDMParam) {
    const std::string textproto =
            "major_version: 1\n"
            "param: \"foo\"";
    std::vector<std::string> params;
    EXPECT_EQ(Status::MissingDMParam, getParameters(textproto, &params));
}

// Omit version information.
TEST(VerifiedBootLoadConfigTest, MissingVersion) {
    const std::string textproto =
            "dm_param: \"foo\"\n"
            "param: \"foo\"";
    std::vector<std::string> params;
    EXPECT_EQ(Status::MissingVersion, getParameters(textproto, &params));
}

// Pass in an unsupported version.
TEST(VerifiedBootLoadConfigTest, UnsupportedVersion) {
    const std::string textproto =
            "major_version: 3\n"
            "dm_param: \"foo\"\n"
            "param: \"foo\"";
    std::vector<std::string> params;
    EXPECT_EQ(Status::UnsupportedVersion, getParameters(textproto, &params));
}
