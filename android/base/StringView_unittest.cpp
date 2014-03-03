// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/StringView.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(StringView, InitEmpty) {
    StringView view;
    EXPECT_TRUE(view.empty());
}

TEST(StringView, InitWithCString) {
    static const char kString[] = "Hello";
    StringView view(kString);
    EXPECT_STREQ(kString, view.str());
    EXPECT_FALSE(view.empty());
    EXPECT_EQ(strlen(kString), view.size());
}

TEST(StringView, InitWithStringView) {
    static const char kString[] = "Hello2";
    StringView view1(kString);
    StringView view2(view1);
    EXPECT_FALSE(view2.empty());
    EXPECT_STREQ(kString, view2.str());
    EXPECT_EQ(strlen(kString), view2.size());
}

TEST(StringView, Clear) {
    StringView view("Hello3");
    EXPECT_FALSE(view.empty());
    view.clear();
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(0U, view.size());
    EXPECT_FALSE(view.str());
}

TEST(StringView, SetEmpty) {
    StringView view("Hello4");
    view.set("");
    EXPECT_TRUE(view.empty());
}

TEST(StringView, SetEmptyWithLength) {
    StringView view("Hello5");
    view.set("Oops", 0U);
    EXPECT_TRUE(view.empty());
}

TEST(StringView, SetWithCString) {
    static const char kString[] = "Wow";
    StringView view("Hello6");
    view.set(kString);
    EXPECT_EQ(kString, view.str());
    EXPECT_EQ(strlen(kString), view.size());
}

TEST(StringView, SetWithStringView) {
    static const char kString[] = "Wazza";
    StringView view1(kString);
    StringView view("Nope");
    view.set(view1);
    EXPECT_EQ(kString, view.str());
    EXPECT_EQ(strlen(kString), view.size());
}

TEST(StringView, OperatorAt) {
    static const char kString[] = "Whatever";
    static const size_t kStringLen = sizeof(kString) - 1;
    StringView view(kString);
    for (size_t n = 0; n < kStringLen; ++n) {
        EXPECT_EQ(kString[n], view[n]) << "at index " << n;
    }
}

TEST(StringView, Iterators) {
    static const char kString[] = "What else?";
    static const size_t kStringLen = sizeof(kString) - 1;
    StringView view(kString);
    EXPECT_EQ(kString, view.begin());
    EXPECT_EQ(kString + kStringLen, view.end());

    size_t n = 0;
    for (StringView::const_iterator it = view.begin();
         it != view.end(); ++it, ++n) {
        EXPECT_EQ(kString[n], *it);
    }
}

TEST(StringView, ComparisonOperators) {
    char kHello1[] = "Hello";
    char kHello2[] = "Hello";
    StringView view1(kHello1);
    StringView view2(kHello2);
    EXPECT_TRUE(view1 == view2);
    EXPECT_FALSE(view1 != view2);
    EXPECT_TRUE(view1 <= view2);
    EXPECT_TRUE(view1 >= view2);
    EXPECT_FALSE(view1 < view2);
    EXPECT_FALSE(view1 > view2);

    StringView view3("hell");  // Shorter, but first char is larger.
    EXPECT_FALSE(view1 == view3);
    EXPECT_TRUE(view1 != view3);
    EXPECT_TRUE(view1 < view3);
    EXPECT_TRUE(view1 <= view3);
    EXPECT_FALSE(view1 > view3);
    EXPECT_FALSE(view1 >= view3);

    StringView view4("Hell");  // Shorter, but first char is smaller.
    EXPECT_FALSE(view1 == view4);
    EXPECT_TRUE(view1 != view4);
    EXPECT_FALSE(view1 < view4);
    EXPECT_FALSE(view1 <= view4);
    EXPECT_TRUE(view1 > view4);
    EXPECT_TRUE(view1 >= view4);
}

// TODO(digit): String

}  // namespace base
}  // namespace android
