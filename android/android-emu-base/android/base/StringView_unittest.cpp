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

#include "android/base/ArraySize.h"
#include "android/base/StringView.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(StringView, InitEmpty) {
    StringView view;
    EXPECT_TRUE(view.empty());
}

TEST(StringView, InitWithCString) {
    static const char* kString = "Hello";
    StringView view(kString);
    EXPECT_STREQ(kString, view.data());
    EXPECT_FALSE(view.empty());
    EXPECT_EQ(strlen(kString), view.size());
}

TEST(StringView, InitWithConstBuffer) {
    static const char kString[] = "Hello";
    StringView view(kString);
    EXPECT_STREQ(kString, view.data());
    EXPECT_FALSE(view.empty());
    EXPECT_EQ(strlen(kString), view.size());
}

TEST(StringView, InitWithMutableBuffer) {
    char string[128] = "Hello";
    ASSERT_NE(stringLiteralLength(string), strlen(string));
    StringView view(string);
    EXPECT_STREQ(string, view.data());
    EXPECT_FALSE(view.empty());
    EXPECT_EQ(strlen(string), view.size());
}

namespace {
    // Just a test class which needs compile-time constant as an agrument
    template <int N>
    struct checkConst {
        checkConst() {
            std::cout << "N = " << N << '/';
        }
    };
}

TEST(StringView, InitConstexpr) {
    static constexpr StringView kStringView = "Hello";

    // these lines compile only if the argument is a compile-time constant
    checkConst<kStringView[0]> checker1;
    checkConst<kStringView.data()[1]> checker2;
    checkConst<kStringView.begin()[2]> checker3;
    checkConst<kStringView.size()> checker4;
    checkConst<kStringView.empty()> checker5;
}

TEST(StringView, InitWithStringView) {
    static const char kString[] = "Hello2";
    StringView view1(kString);
    StringView view2(view1);
    EXPECT_FALSE(view2.empty());
    EXPECT_STREQ(kString, view2.data());
    EXPECT_EQ(strlen(kString), view2.size());
}

TEST(StringView, Clear) {
    StringView view("Hello3");
    EXPECT_FALSE(view.empty());
    view.clear();
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(0U, view.size());
    EXPECT_STREQ("", view.data());
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
    EXPECT_EQ(kString, view.data());
    EXPECT_EQ(strlen(kString), view.size());
}

TEST(StringView, SetWithStringView) {
    static const char kString[] = "Wazza";
    StringView view1(kString);
    StringView view("Nope");
    view.set(view1);
    EXPECT_EQ(kString, view.data());
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

TEST(StringView, IsNullTerminated) {
    EXPECT_TRUE(StringView().isNullTerminated());
    EXPECT_TRUE(StringView("blah").isNullTerminated());
    EXPECT_FALSE(StringView("blah", 1).isNullTerminated());
    EXPECT_TRUE(StringView("blah", 4).isNullTerminated());
    auto sv = StringView{};
    sv.set("blah");
    EXPECT_TRUE(sv.isNullTerminated());
    sv.set("blah", 2);
    EXPECT_FALSE(sv.isNullTerminated());
    sv.set("blah", 4);
    EXPECT_TRUE(sv.isNullTerminated());
    sv.set(nullptr);
    EXPECT_TRUE(sv.isNullTerminated());
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

    EXPECT_EQ(StringView(""), StringView(""));
    EXPECT_GE(StringView(""), StringView(""));
    EXPECT_GT(StringView("1"), StringView(""));
    EXPECT_GE(StringView("1"), StringView(""));
    EXPECT_LT(StringView(""), StringView("2"));
    EXPECT_LE(StringView(""), StringView("2"));
}

TEST(StringView, Compare) {
    // empty string
    EXPECT_EQ(StringView("").compare(StringView("")), 0);
    EXPECT_GT(StringView("1").compare(StringView("")), 0);
    EXPECT_LT(StringView("").compare(StringView("2")), 0);

    // non-empty strings, same length
    EXPECT_EQ(StringView("1").compare(StringView("1")), 0);
    EXPECT_GT(StringView("2").compare(StringView("1")), 0);
    EXPECT_LT(StringView("1").compare(StringView("2")), 0);

    // non-empty strings, first shorter
    EXPECT_GT(StringView("2").compare(StringView("12")), 0);
    EXPECT_LT(StringView("1").compare(StringView("11")), 0);

    // non-empty strings, first longer
    EXPECT_GT(StringView("11").compare(StringView("1")), 0);
    EXPECT_LT(StringView("12").compare(StringView("2")), 0);
}

TEST(StringView, Find) {
    size_t no = std::string::npos;
    // empty string
    EXPECT_EQ(0, StringView("").find(StringView("")));
    EXPECT_EQ(0, StringView("1").find(StringView("")));
    EXPECT_EQ(0, StringView("1").find(StringView(""), 96));
    EXPECT_EQ(no, StringView("").find(StringView("2")));

    // non-empty strings, same length
    EXPECT_EQ(0, StringView("1").find(StringView("1")));
    EXPECT_EQ(no, StringView("2").find(StringView("1")));
    EXPECT_EQ(no, StringView("1").find(StringView("2")));

    // non-empty strings, first shorter
    EXPECT_EQ(no, StringView("2").find(StringView("12")));
    EXPECT_EQ(no, StringView("1").find(StringView("11")));

    // non-empty strings, first longer
    EXPECT_EQ(0, StringView("11").find(StringView("1")));
    EXPECT_EQ(1, StringView("12").find(StringView("2")));
    EXPECT_EQ(0, StringView("12").find(StringView("1")));
    EXPECT_EQ(no, StringView("12").find(StringView("1"), 1));

    // allocate a string with a stringview that is shorter.
    std::string longString("a b c d e f g");
    StringView longView(longString);
    std::string longStringSub = longString.substr(0, 1);
    StringView shortView(longStringSub);

    EXPECT_EQ(6, longView.find(StringView("d")));
    EXPECT_EQ(no, shortView.find(StringView("d")));

    // two occurrences, which wins?
    StringView twice("qwerty abcdabcd");
    EXPECT_EQ(7, twice.find(StringView("abcd")));

    // With offset?
    EXPECT_EQ(11, twice.find(StringView("abcd"), 11));
}

TEST(StringView, GetSubstr) {
    StringView empty = StringView("");

    EXPECT_EQ(empty, StringView("").getSubstr(StringView("")));
    EXPECT_EQ(empty, StringView("").getSubstr(StringView("1")));
    EXPECT_EQ(empty, StringView("").getSubstr(StringView("12")));
    EXPECT_EQ(StringView("1"), StringView("1").getSubstr(StringView("")));
    EXPECT_EQ(StringView("12"), StringView("12").getSubstr(StringView("")));
    EXPECT_EQ(empty, StringView("12").getSubstr(StringView("123")));

    StringView s1234("12341234");
    EXPECT_EQ(s1234, s1234.getSubstr(StringView("12341234")));
    EXPECT_EQ(s1234, s1234.getSubstr(StringView("12")).data());
    EXPECT_EQ(s1234.substr(1), s1234.getSubstr(StringView("23")).data());
    EXPECT_EQ(s1234.substr(2), s1234.getSubstr(StringView("34")).data());

    StringView s1234prefix = s1234.substr(0, 4);
    EXPECT_EQ(empty, s1234prefix.getSubstr("3412"));
}

TEST(StringView, Substr) {
    StringView empty = StringView("");

    EXPECT_EQ(empty, StringView("").substr(0, 0));
    EXPECT_EQ(empty, StringView("").substr(1, 0));
    EXPECT_EQ(empty, StringView("").substr(2, 0));
    EXPECT_EQ(empty, StringView("1").substr(0, 0));

    EXPECT_EQ(StringView("1"), StringView("1").substr(0, 1));
    EXPECT_EQ(StringView("2"), StringView("2").substr(0, 1));
    EXPECT_TRUE(StringView("1").substr(0, 1) != StringView("2"));

    EXPECT_EQ(StringView("1"), StringView("12").substr(0, 1));
    EXPECT_EQ(StringView("2"), StringView("12").substr(1, 1));
    EXPECT_EQ(StringView("12"), StringView("12").substr(0, 2));

    EXPECT_EQ(StringView("23"), StringView("1234").substr(1, 2));

    EXPECT_EQ(StringView(""), StringView("").substr(0));
    EXPECT_EQ(StringView(""), StringView("").substr(1));
    EXPECT_EQ(StringView(""), StringView("").substr(2));
    EXPECT_EQ(StringView(""), StringView("1").substr(1));
    EXPECT_EQ(StringView("1"), StringView("1").substr(0));

    EXPECT_EQ(StringView("1234"), StringView("1234").substr(0));
    EXPECT_EQ(StringView(""), StringView("1234").substr(4));
    EXPECT_EQ(StringView("4"), StringView("1234").substr(3));
    EXPECT_EQ(StringView("34"), StringView("1234").substr(2));
    EXPECT_EQ(StringView("234"), StringView("1234").substr(1));
}

TEST(StringView, SubstrAbs) {
    StringView empty = StringView("");

    EXPECT_EQ(empty, StringView("").substrAbs(0, 0));
    EXPECT_EQ(empty, StringView("").substrAbs(1, 1));
    EXPECT_EQ(empty, StringView("").substrAbs(2, 2));
    EXPECT_EQ(empty, StringView("1").substrAbs(0, 0));

    EXPECT_EQ(StringView("1"), StringView("1").substrAbs(0, 1));
    EXPECT_EQ(StringView("2"), StringView("2").substrAbs(0, 1));
    EXPECT_TRUE(StringView("1").substrAbs(0, 1) != StringView("2"));

    EXPECT_EQ(StringView("1"), StringView("12").substrAbs(0, 1));
    EXPECT_EQ(StringView("2"), StringView("12").substrAbs(1, 2));
    EXPECT_EQ(StringView("12"), StringView("12").substrAbs(0, 2));

    EXPECT_EQ(StringView("23"), StringView("1234").substr(1, 2));

    EXPECT_EQ(StringView(""), StringView("").substrAbs(0));
    EXPECT_EQ(StringView(""), StringView("").substrAbs(1));
    EXPECT_EQ(StringView(""), StringView("").substrAbs(2));
    EXPECT_EQ(StringView(""), StringView("1").substrAbs(1));
    EXPECT_EQ(StringView("1"), StringView("1").substrAbs(0));

    EXPECT_EQ(StringView("1234"), StringView("1234").substrAbs(0));
    EXPECT_EQ(StringView(""), StringView("1234").substrAbs(4));
    EXPECT_EQ(StringView("4"), StringView("1234").substrAbs(3));
    EXPECT_EQ(StringView("34"), StringView("1234").substrAbs(2));
    EXPECT_EQ(StringView("234"), StringView("1234").substrAbs(1));
}

TEST(StringView, NullTerminated) {
    const char* kString = "0123456789";
    StringView str(kString);
    StringView firstByte(kString, 1);

    // Normally, a slice doesn't modify the string so STREQ could not stop at
    // the end.
    EXPECT_STREQ(firstByte.data(), kString);

    // But when wrapped in CStrWrapper, it is copied.
    EXPECT_STREQ(c_str(firstByte), "0");

    // If the string is already null terminated, it doesn't copy.
    EXPECT_EQ(c_str(str).get(), kString);

    EXPECT_STREQ(c_str(StringView()), "");
    EXPECT_STREQ(c_str(StringView("")), "");
    EXPECT_STREQ(c_str(StringView(kString, size_t(0))), "");
}

// TODO(digit): String

}  // namespace base
}  // namespace android
