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

#include "android/base/String.h"

#include "android/base/StringView.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(String, DefaultConstructor) {
    String s;
    EXPECT_EQ(0U, s.size());
    EXPECT_TRUE(s.empty());
}

TEST(String, SimpleConstructor) {
    static const char kString[] = "hello you";
    const size_t kStringLen = sizeof(kString) - 1U;

    String s(kString);
    EXPECT_NE(kString, s.c_str());
    EXPECT_EQ(kStringLen, s.size());
    EXPECT_FALSE(s.empty());
    EXPECT_STREQ(kString, s.c_str());
}

TEST(String, SimpleConstructorWithLongString) {
    static const char kString[] =
            "this is a long string that should exceed the "
            "small-string-optimization capacity";
    const size_t kStringLen = sizeof(kString) - 1U;

    String s(kString);
    EXPECT_NE(kString, s.c_str());
    EXPECT_EQ(kStringLen, s.size());
    EXPECT_FALSE(s.empty());
    EXPECT_STREQ(kString, s.c_str());
}

TEST(String, CopyConstructor) {
    String s1("Hello World!");
    String s2(s1);
    EXPECT_EQ(s1.size(), s2.size());
    EXPECT_STREQ(s1.c_str(), s2.c_str());
}

TEST(String, ConstructorFromStringView) {
    StringView view("Hello Cowboys");
    String s(view);
    EXPECT_NE(view.str(), s.c_str());
    EXPECT_EQ(view.size(), s.size());
    EXPECT_STREQ(view.str(), s.c_str());
}

TEST(String, ConstructorWithCountAndFill) {
    const size_t kCount = 1024;
    const char kFill = 0x42;

    String s(kCount, kFill);
    EXPECT_EQ(kCount, s.size());
    for (size_t n = 0; n < kCount; ++n)
        EXPECT_EQ(kFill, s[n]);
}

TEST(String, empty) {
    String s1("");
    EXPECT_TRUE(s1.empty());

    String s2("foo");
    EXPECT_FALSE(s2.empty());
}

TEST(String, size) {
    const size_t kCount = 4096;
    String s;
    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_EQ(n, s.size());
        s += 'x';
    }
    EXPECT_EQ(kCount, s.size());
}

TEST(String, clear) {
    String s("foo bar");
    EXPECT_FALSE(s.empty());
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(0U, s.size());
}

TEST(String, IndexedAccess) {
    String s("foobar");
    EXPECT_EQ('f', s[0]);
    EXPECT_EQ('o', s[1]);
    EXPECT_EQ('o', s[2]);
    EXPECT_EQ('b', s[3]);
    EXPECT_EQ('a', s[4]);
    EXPECT_EQ('r', s[5]);
    EXPECT_EQ('\0', s[6]);
}

TEST(String, AssignOperatorWithCString) {
    static const char kString[] = "Hello 10234";
    const size_t kStringLen = sizeof(kString) - 1U;

    String s;
    s = kString;
    EXPECT_NE(kString, s.c_str());
    EXPECT_EQ(kStringLen, s.size());
    EXPECT_STREQ(kString, s.c_str());
}

TEST(String, AssignOperatorWithOtherString) {
    String s0("A fish named Wanda");
    String s;
    s = s0;
    EXPECT_EQ(s0.size(), s.size());
    EXPECT_STREQ(s0.c_str(), s.c_str());
}

TEST(String, AssignOperatorWithStringView) {
    StringView v0("What a beautiful world");
    String s;
    s = v0;
    EXPECT_EQ(v0.size(), s.size());
    EXPECT_STREQ(v0.str(), s.c_str());
}

TEST(String, AssignOperatorWithChar) {
    String s;
    s = 'f';
    EXPECT_EQ(1U, s.size());
    EXPECT_EQ('f', s[0]);
}

TEST(String, assignWithCString) {
    static const char kString[] = "Hello";
    const size_t kStringLen = sizeof(kString) - 1U;

    String  s;
    s.assign(kString);
    EXPECT_EQ(kStringLen, s.size());
    EXPECT_STREQ(kString, s.c_str());
}

TEST(String, assignWithString) {
    String s0("Input string");
    String s;
    s.assign(s0);
    EXPECT_EQ(s0.size(), s.size());
    EXPECT_STREQ(s0.c_str(), s.c_str());
}

TEST(String, assignWithStringAndLen) {
    static const char kString[] = "Who you're gonna call?";
    const size_t kStringLen = sizeof(kString) - 1U;

    String s;
    s.assign(kString, kStringLen);
    EXPECT_NE(kString, s.c_str());
    EXPECT_EQ(kStringLen, s.size());
    EXPECT_STREQ(kString, s.c_str());
}

TEST(String, assignWithCountAndFill) {
    const size_t kCount = 1024;
    const char kFill = '\x7f';
    String s;
    s.assign(kCount, kFill);
    EXPECT_EQ(kCount, s.size());
    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_EQ(kFill, s[n]) << "At index " << n;
    }
}

TEST(String, assignWithStringView) {
    StringView v0("Input string view");
    String s;
    s.assign(v0);
    EXPECT_EQ(v0.size(), s.size());
    EXPECT_STREQ(v0.str(), s.c_str());
}

TEST(String, assignWithChar) {
    String s;
    s.assign('f');
    EXPECT_EQ(1U, s.size());
    EXPECT_STREQ("f", s.c_str());
}

typedef struct {
    const char* s1;
    const char* s2;
    int expected;
} Comparison;

static const Comparison kComparisons[] = {
    { "", "", 0 },
    { "", "foo", -1 },
    { "foo", "", +1 },
    { "foo", "zoo", -1 },
    { "foo", "foo", 0 },
    { "foo", "fo", +1 },
    { "foo", "fooo", -1 },
    { "foo", "fop", -1 },
    { "foo", "fon", +1 },
};

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

TEST(String, compareWithCString) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s(comp.s1);
        EXPECT_EQ(comp.expected, s.compare(comp.s2))
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}

TEST(String, compareWithCStringWithLen) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s(comp.s1);
        size_t len2 = ::strlen(comp.s2);
        EXPECT_EQ(comp.expected, s.compare(comp.s2, len2))
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}


TEST(String, compareWithString) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s1(comp.s1);
        String s2(comp.s2);
        EXPECT_EQ(comp.expected, s1.compare(s2))
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}

TEST(String, compareWithStringView) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s1(comp.s1);
        StringView s2(comp.s2);
        EXPECT_EQ(comp.expected, s1.compare(s2))
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}

TEST(String, equalsWithCString) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s(comp.s1);
        EXPECT_EQ(!comp.expected, s.equals(comp.s2))
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}

TEST(String, equalsWithCStringWithLen) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s(comp.s1);
        size_t len2 = ::strlen(comp.s2);
        EXPECT_EQ(!comp.expected, s.equals(comp.s2, len2))
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}


TEST(String, equalsWithString) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s1(comp.s1);
        String s2(comp.s2);
        EXPECT_EQ(!comp.expected, s1.equals(s2))
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}

TEST(String, equalsWithStringView) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s1(comp.s1);
        StringView s2(comp.s2);
        EXPECT_EQ(!comp.expected, s1.equals(s2))
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}

TEST(String, EqualsOperatorWithCString) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s(comp.s1);
        EXPECT_EQ(!comp.expected, s == comp.s2)
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}

TEST(String, EqualsOperatorWithString) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s1(comp.s1);
        String s2(comp.s2);
        EXPECT_EQ(!comp.expected, s1 == s2)
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}

TEST(String, EqualsOperatorWithStringView) {
    for (size_t n = 0; n < ARRAY_SIZE(kComparisons); ++n) {
        const Comparison& comp = kComparisons[n];
        String s1(comp.s1);
        StringView s2(comp.s2);
        EXPECT_EQ(!comp.expected, s1 == s2)
                << "Comparing '" << comp.s1 << "' with '"
                << comp.s2 << "'";
    }
}

TEST(String, Reserve0OnEmptyString) {
    String s;
    // This is really to check that this doesn't crash.
    ::memset(&s, 0, sizeof(s));
    s.reserve(0U);
}

TEST(String, Swap) {
    const String kShortString1("Hello World!");
    const String kLongString1("A very long string that will likely "
            "exceed the small string optimization storage buffer");
    const String kShortString2("Menthe a l'eau");
    const String kLongString2("Harold Ramis, Chicago actor, writer "
            "and director, dead at 69");

    // Swap two short strings.
    {
        String sa = kShortString1;
        String sb = kShortString2;
        sa.swap(&sb);
        EXPECT_EQ(kShortString2, sa);
        EXPECT_EQ(kShortString1, sb);
    }

    // Swap one short + one long string.
    {
        String sa = kShortString1;
        String sb = kLongString1;
        sa.swap(&sb);
        EXPECT_EQ(kLongString1, sa);
        EXPECT_EQ(kShortString1, sb);
    }

    // Swap one long + one short string.
    {
        String sa = kLongString1;
        String sb = kLongString2;
        sa.swap(&sb);
        EXPECT_EQ(kLongString2, sa);
        EXPECT_EQ(kLongString1, sb);
    }
}

}  // namespace base
}  // namespace android
