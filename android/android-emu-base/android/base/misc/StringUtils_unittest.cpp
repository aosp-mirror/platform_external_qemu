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

#include <list>
#include <string>
#include <string_view>
#include <vector>

namespace android {
namespace base {

#ifdef _WIN32
TEST(StringUtils, memmem) {
    auto src = "string";

    // basic cases - when the substring is there
    EXPECT_EQ(src + 2, memmem(src, strlen(src), "ring", 4));
    EXPECT_EQ(src + 1, memmem(src, strlen(src), "tri", 3));

    // no substring
    EXPECT_EQ(nullptr, memmem(src, strlen(src), "123", 3));
    EXPECT_EQ(nullptr, memmem(src, strlen(src), "strings", 7));

    // make sure it's looking at lengths, and not at the null-terminator
    EXPECT_EQ(src + 2, memmem(src, strlen(src), "ringer", 4));
    EXPECT_EQ(nullptr, memmem(src, 2, "tr", 2));

    // corner cases - empty/single char and the needle same as haystack
    EXPECT_EQ(src, memmem(src, strlen(src), "", 0));
    EXPECT_EQ(src, memmem(src, strlen(src), "s", 1));
    EXPECT_EQ(src, memmem(src, 1, "s", 1));
    EXPECT_EQ(nullptr, memmem("", 0, "s", 1));
    EXPECT_EQ(nullptr, memmem("", 0, "", 0));
    EXPECT_EQ(src, memmem(src, strlen(src), src, strlen(src)));

    // null input parameters
    EXPECT_EQ(nullptr, memmem(src, strlen(src), nullptr, 2));
    EXPECT_EQ(nullptr, memmem(nullptr, 10, "asdf", 4));
    EXPECT_EQ(nullptr, memmem(nullptr, 10, nullptr, 1));
}
#endif // _WIN32

TEST(StringUtils, strDupWithStringView) {
    std::string_view view("Hello World");
    char* s = strDup(view);
    EXPECT_TRUE(s);
    EXPECT_STREQ(view.data(), s);
    EXPECT_NE(view.data(), s);
    free(s);
}

TEST(StringUtils, strDupWithStdString) {
    std::string str("Hello World");
    char* s = strDup(str);
    EXPECT_TRUE(s);
    EXPECT_STREQ(str.data(), s);
    EXPECT_NE(str.data(), s);
    free(s);
}

TEST(StringUtils, strContainsWithStringView) {
    std::string_view haystack("This is a long string to search for stuff");
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

TEST(StringUtils, join) {
    using android::base::join;

    EXPECT_STREQ("", join(std::vector<int>{}).c_str());
    EXPECT_STREQ("aa", join(std::vector<std::string>{"aa"}).c_str());
    EXPECT_STREQ("aa,bb", join(std::list<const char*>{"aa", "bb"}).c_str());
    EXPECT_STREQ("1,2,3", join(std::vector<int>{1, 2, 3}).c_str());
    EXPECT_STREQ("1|2|3", join(std::vector<int>{1, 2, 3}, '|').c_str());
    EXPECT_STREQ("1<|>2<|>sd",
                 join(std::vector<const char*>{"1", "2", "sd"}, "<|>").c_str());
    EXPECT_STREQ("...---...",
                 join(std::vector<const char*>{"...", "..."}, "---").c_str());
    EXPECT_STREQ("...---...",
                 join(std::vector<const char*>{"...", "---", "..."}, "").c_str());

    // check some special cases - lvalue modifiable and const references
    using StringVec = std::vector<std::string>;
    StringVec src = { "1", "a", "foo" };

    EXPECT_STREQ("1,a,foo", join(src).c_str());
    EXPECT_STREQ("1, a, foo", join(src, ", ").c_str());
    EXPECT_STREQ("1,a,foo", join(const_cast<const StringVec&>(src)).c_str());
    EXPECT_STREQ("1, a, foo",
                 join(const_cast<const StringVec&>(src), ", ").c_str());
}

TEST(StringUtils, trim) {
    struct {
        const char* before;
        const char* after;
    } test_data[] = {
            {"no change", "no change"},
            {"", ""},
            {" left", "left"},
            {"right ", "right"},
            {"\nnewline\r", "newline"},
            {" \r\n\tmulti \t\r\n", "multi"},
    };
    size_t TEST_DATA_COUNT = sizeof(test_data) / sizeof(test_data[0]);

    for (size_t i = 0; i < TEST_DATA_COUNT; i++) {
        std::string before(test_data[i].before);
        std::string after = trim(before);
        EXPECT_STREQ(test_data[i].after, after.c_str());
    }
}

TEST(StringUtils, startsWith) {
    struct {
        const char* string;
        const char* prefix;
        bool res;
    } test_data[] = {
            {"", "", true},
            {"123", "", true},
            {"blah", "bl", true},
            {" asD1f asdfsda", " asD1f ", true},
            {"blah", "lah", false},
            {"123", "gasd", false},
            {"123", "1234", false},
            {"", "1", false},
    };
    size_t TEST_DATA_COUNT = sizeof(test_data) / sizeof(test_data[0]);

    for (size_t i = 0; i < TEST_DATA_COUNT; i++) {
        EXPECT_EQ(test_data[i].res, startsWith(test_data[i].string,
                                               test_data[i].prefix));
    }
}

TEST(StringUtils, endsWith) {
    struct {
        const char* string;
        const char* suffix;
        bool res;
    } test_data[] = {
            {"", "", true},
            {"123", "", true},
            {"blah", "ah", true},
            {" asD1f asdf sda ", " sda ", true},
            {"blah", "bla", false},
            {"123", "gasd", false},
            {"123", "0123", false},
            {"", "1", false},
    };
    size_t TEST_DATA_COUNT = sizeof(test_data) / sizeof(test_data[0]);

    for (size_t i = 0; i < TEST_DATA_COUNT; i++) {
        EXPECT_EQ(test_data[i].res, endsWith(test_data[i].string,
                                             test_data[i].suffix));
    }
}

TEST(StringUtils, split) {
    std::vector<std::string_view> results;

    auto testFunc = [&results](std::string_view s) { results.push_back(s); };

    split(std::string_view(""), std::string_view("abc"), testFunc);
    EXPECT_EQ(results.size(), 1);

    split(std::string_view("abc"), std::string_view(""), testFunc);
    EXPECT_EQ(results.size(), 1);

    results.clear();
    split(std::string_view("abc"), std::string_view("a"), testFunc);
    EXPECT_EQ(results.size(), 2);
    EXPECT_EQ(results[0], std::string_view(""));
    EXPECT_EQ(results[1], std::string_view("bc"));

    results.clear();
    split(std::string_view("aaa"), std::string_view("a"), testFunc);
    EXPECT_EQ(4, results.size());
    EXPECT_EQ(std::string_view(""), results[0]);
    EXPECT_EQ(std::string_view(""), results[1]);
    EXPECT_EQ(std::string_view(""), results[2]);
    EXPECT_EQ(std::string_view(""), results[3]);

    results.clear();
    split(std::string_view("1a2a3a4"), std::string_view("a"), testFunc);
    EXPECT_EQ(4, results.size());
    EXPECT_EQ(std::string_view("1"), results[0]);
    EXPECT_EQ(std::string_view("2"), results[1]);
    EXPECT_EQ(std::string_view("3"), results[2]);
    EXPECT_EQ(std::string_view("4"), results[3]);

    results.clear();
    split(std::string_view("1a2aa3a4"), std::string_view("a"), testFunc);
    EXPECT_EQ(5, results.size());
    EXPECT_EQ(std::string_view("1"), results[0]);
    EXPECT_EQ(std::string_view("2"), results[1]);
    EXPECT_EQ(std::string_view(""), results[2]);
    EXPECT_EQ(std::string_view("3"), results[3]);
    EXPECT_EQ(std::string_view("4"), results[4]);

    results.clear();
    split(std::string_view("The quick brown fox jumped over the lazy dog"),
          std::string_view(" "), testFunc);
    EXPECT_EQ(9, results.size());
    EXPECT_EQ(std::string_view("The"), results[0]);
    EXPECT_EQ(std::string_view("quick"), results[1]);
    EXPECT_EQ(std::string_view("brown"), results[2]);
    EXPECT_EQ(std::string_view("fox"), results[3]);
    EXPECT_EQ(std::string_view("jumped"), results[4]);
    EXPECT_EQ(std::string_view("over"), results[5]);
    EXPECT_EQ(std::string_view("the"), results[6]);
    EXPECT_EQ(std::string_view("lazy"), results[7]);
    EXPECT_EQ(std::string_view("dog"), results[8]);

    results.clear();
    split(std::string_view("a; b; c; d"), std::string_view("; "), testFunc);
    EXPECT_EQ(4, results.size());
    EXPECT_EQ(std::string_view("a"), results[0]);
    EXPECT_EQ(std::string_view("b"), results[1]);
    EXPECT_EQ(std::string_view("c"), results[2]);
    EXPECT_EQ(std::string_view("d"), results[3]);
}

}  // namespace base
}  // namespace android
