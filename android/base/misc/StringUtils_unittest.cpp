// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/misc/StringUtils.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(StringUtils, strDupWithStringView) {
    StringView view("Hello World");
    char* s = strDup(view);
    EXPECT_TRUE(s);
    EXPECT_STREQ(view.c_str(), s);
    EXPECT_NE(view.c_str(), s);
    free(s);
}

TEST(StringUtils, strDupWithStdString) {
    std::string str("Hello World");
    char* s = strDup(str);
    EXPECT_TRUE(s);
    EXPECT_STREQ(str.c_str(), s);
    EXPECT_NE(str.c_str(), s);
    free(s);
}

TEST(StringUtils, strContainsWithStringView) {
    StringView haystack("This is a long string to search for stuff");
    EXPECT_FALSE(strContains(haystack, "needle"));
    EXPECT_FALSE(strContains(haystack, "stuffy"));
    EXPECT_TRUE(strContains(haystack, "stuf"));
    EXPECT_TRUE(strContains(haystack, "stuff"));
    EXPECT_FALSE(strContains(haystack, "This is a short phrase"));
    EXPECT_TRUE(strContains(haystack, "a long string"));
}

TEST(StringUtils, strContainsWithStdString) {
    std::string haystack("This is a long string to search for stuff");
    EXPECT_FALSE(strContains(haystack, "needle"));
    EXPECT_FALSE(strContains(haystack, "stuffy"));
    EXPECT_TRUE(strContains(haystack, "stuff"));
    EXPECT_FALSE(strContains(haystack, "This is a short phrase"));
    EXPECT_TRUE(strContains(haystack, "a long string"));
}

}  // namespace base
}  // namespace android
