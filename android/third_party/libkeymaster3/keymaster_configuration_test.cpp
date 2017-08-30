/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <keymaster/keymaster_configuration.h>

#ifdef HOST_BUILD
extern "C" {
int __android_log_print(int prio, const char* tag, const char* fmt);
int __android_log_print(int prio, const char* tag, const char* fmt) {
    (void)prio, (void)tag, (void)fmt;
    std::cout << fmt << std::endl;
    return 0;
}
}  // extern "C"
#endif  // HOST_BUILD

namespace keymaster {
namespace test {

TEST(VersionParsingTest, Full) {
    EXPECT_EQ(612334U, GetOsVersion("61.23.34"));
    EXPECT_EQ(680000U, GetOsVersion("681.23.24"));
    EXPECT_EQ(682300U, GetOsVersion("68.231.24"));
    EXPECT_EQ(682324U, GetOsVersion("68.23.241"));
    EXPECT_EQ(60102U, GetOsVersion("6.1.2-extrajunk"));
    EXPECT_EQ(0U, GetOsVersion("extra6.1.2"));
}

TEST(VersionParsingTest, FullWithExtraChars) {}

TEST(VersionParsingTest, MajorOnly) {
    EXPECT_EQ(60000U, GetOsVersion("6"));
    EXPECT_EQ(680000U, GetOsVersion("68"));
    EXPECT_EQ(680000U, GetOsVersion("681"));
    EXPECT_EQ(60000U, GetOsVersion("6.junk"));
}

TEST(VersionParsingTest, MajorMinorOnly) {
    EXPECT_EQ(60100U, GetOsVersion("6.1"));
    EXPECT_EQ(60100U, GetOsVersion("6.1junk"));
}

TEST(PatchLevelParsingTest, Full) {
    EXPECT_EQ(201603U, GetOsPatchlevel("2016-03-23"));
    EXPECT_EQ(0U, GetOsPatchlevel("2016-13-23"));
    EXPECT_EQ(0U, GetOsPatchlevel("2016-03"));
    EXPECT_EQ(0U, GetOsPatchlevel("2016-3-23"));
    EXPECT_EQ(0U, GetOsPatchlevel("2016-03-23r"));
    EXPECT_EQ(0U, GetOsPatchlevel("r2016-03-23"));
}

}  // namespace test
}  // namespace keymaster
