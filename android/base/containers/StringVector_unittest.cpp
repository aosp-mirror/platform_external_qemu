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

#include "android/base/containers/StringVector.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

// Generate a pseudo-random string.
static String genHashString(size_t n) {
    size_t count = ((n << 3) ^ ((n >> 1) * 11)) % 100;
    String result;
    size_t hash = 3467 * n;
    for (size_t n = 0; n < count; ++n) {
        result += "0123456789abcdefghijklmnopqrstvuwxyz"[hash % 36];
        hash = hash * 3667 + n;
    }
    return result;
}

TEST(StringVector, Empty) {
    StringVector v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(0U, v.size());
}

TEST(StringVector, ResizeGrows) {
    StringVector v;
    const size_t kCount = 100;
    v.resize(kCount);
    EXPECT_EQ(kCount, v.size());
    for (size_t n = 0; n < kCount; ++n) {
        EXPECT_TRUE(v[n].empty());
    }
}

TEST(StringVector, AppendOneString) {
    StringVector v;
    String str("Hello World");
    v.append(str);
    EXPECT_EQ(1U, v.size());
    EXPECT_NE(str.c_str(), v[0].c_str());
    EXPECT_STREQ(str.c_str(), v[0].c_str());
}

TEST(StringVector, AppendLotsOfStrings) {
    StringVector v;
    const size_t kMaxCount = 1000;
    for (size_t n = 0; n < kMaxCount; ++n) {
        v.append(genHashString(n));
    }
    EXPECT_EQ(kMaxCount, v.size());
    for (size_t n = 0; n < kMaxCount; ++n) {
        String expected = genHashString(n);
        EXPECT_EQ(expected.size(), v[n].size());
        EXPECT_STREQ(expected.c_str(), v[n].c_str())
                << "At index " << n;
    }
}

TEST(StringVector, Prepend) {
    StringVector v;
    v.prepend(String("hello"));
    v.prepend(String("world"));
    EXPECT_EQ(2U, v.size());
    EXPECT_STREQ("world", v[0].c_str());
    EXPECT_STREQ("hello", v[1].c_str());
}

TEST(StringVector, PrependLotsOfStrings) {
    StringVector v;
    const size_t kMaxCount = 1000;
    for (size_t n = 0; n < kMaxCount; ++n) {
        v.prepend(genHashString(n));
    }
    EXPECT_EQ(kMaxCount, v.size());
    for (size_t n = 0; n < kMaxCount; ++n) {
        String expected = genHashString(kMaxCount - n - 1U);
        EXPECT_EQ(expected.size(), v[n].size());
        EXPECT_STREQ(expected.c_str(), v[n].c_str())
                << "At index " << n;
    }
}

TEST(StringVector, Swap) {
    static const char* const kList1[] = {
        "Hello", "World!",
    };
    const size_t kList1Len = sizeof(kList1)/sizeof(kList1[0]);

    static const char* const kList2[] = {
        "Menthe", "a", "l'", "eau",
    };
    const size_t kList2Len = sizeof(kList2)/sizeof(kList1[0]);

    StringVector v1;
    for (size_t n = 0; n < kList1Len; ++n) {
        v1.append(StringView(kList1[n]));
    }

    StringVector v2;
    for (size_t n = 0; n < kList2Len; ++n) {
        v2.append(StringView(kList2[n]));
    }

    v1.swap(&v2);

    EXPECT_EQ(kList2Len, v1.size());
    for (size_t n = 0; n < kList2Len; ++n) {
        EXPECT_EQ(String(kList2[n]), v1[n]) << "At index " << n;
    }

    EXPECT_EQ(kList1Len, v2.size());
    for (size_t n = 0; n < kList1Len; ++n) {
        EXPECT_EQ(String(kList1[n]), v2[n]) << "At index " << n;
    }
}

TEST(StringVector, AssignmentOperator) {
    static const char* const kList[] = {
        "Menthe", "a", "l'", "eau",
    };
    const size_t kListLen = sizeof(kList)/sizeof(kList[0]);

    StringVector v1;
    for (size_t n = 0; n < kListLen; ++n) {
        v1.append(StringView(kList[n]));
    }

    StringVector v2;
    v2 = v1;
    v1.reserve(0);
    EXPECT_TRUE(v1.empty());

    for (size_t n = 0; n < kListLen; ++n) {
        EXPECT_EQ(::strlen(kList[n]), v2[n].size()) << "At index " << n;
        EXPECT_STREQ(kList[n], v2[n].c_str()) << "At index " << n;
    }
}

}  // namespace base
}  // namespace android
