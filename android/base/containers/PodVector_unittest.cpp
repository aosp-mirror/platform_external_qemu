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

#include "android/base/containers/PodVector.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

static int  hashIndex(size_t n) {
    return static_cast<int>(((n >> 14) * 13773) + (n * 51));
}

TEST(PodVector, Empty) {
    PodVector<int> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(0U, v.size());
}

TEST(PodVector, AppendOneItem) {
    PodVector<int> v;
    v.append(10234);
    EXPECT_FALSE(v.empty());
    EXPECT_EQ(1U, v.size());
    EXPECT_EQ(10234, v[0]);
}

TEST(PodVector, AppendLotsOfItems) {
    PodVector<int> v;
    const size_t kMaxCount = 10000;
    for (size_t n = 0; n < kMaxCount; ++n) {
        v.append(hashIndex(n));
    }
    EXPECT_EQ(kMaxCount, v.size());
    for (size_t n = 0; n < kMaxCount; ++n) {
        EXPECT_EQ(hashIndex(n), v[n]) << "At index " << n;
    }
}

TEST(PodVector, RemoveFrontItems) {
    PodVector<int> v;
    const size_t kMaxCount = 100;
    for (size_t n = 0; n < kMaxCount; ++n) {
        v.append(hashIndex(n));
    }
    EXPECT_EQ(kMaxCount, v.size());
    for (size_t n = 0; n < kMaxCount; ++n) {
        EXPECT_EQ(hashIndex(n), v[0]) << "At index " << n;
        v.remove(0U);
        EXPECT_EQ(kMaxCount - n - 1U, v.size()) << "At index " << n;
    }
}

TEST(PodVector, PrependItems) {
    PodVector<int> v;
    const size_t kMaxCount = 100;
    for (size_t n = 0; n < kMaxCount; ++n) {
        v.prepend(hashIndex(n));
    }
    EXPECT_EQ(kMaxCount, v.size());
    for (size_t n = 0; n < kMaxCount; ++n) {
        EXPECT_EQ(hashIndex(kMaxCount - n - 1), v[n]) << "At index " << n;
    }
}

TEST(PodVector, ResizeExpands) {
    PodVector<int> v;
    const size_t kMaxCount = 100;
    const size_t kMaxCount2 = 10000;
    for (size_t n = 0; n < kMaxCount; ++n) {
        v.append(hashIndex(n));
    }
    EXPECT_EQ(kMaxCount, v.size());
    v.resize(kMaxCount2);
    EXPECT_EQ(kMaxCount2, v.size());
    for (size_t n = 0; n < kMaxCount; ++n) {
        EXPECT_EQ(hashIndex(n), v[n]) << "At index " << n;
    }
}

TEST(PodVector, ResizeTruncates) {
    PodVector<int> v;
    const size_t kMaxCount = 10000;
    const size_t kMaxCount2 = 10;
    for (size_t n = 0; n < kMaxCount; ++n) {
        v.append(hashIndex(n));
    }
    EXPECT_EQ(kMaxCount, v.size());
    v.resize(kMaxCount2);
    EXPECT_EQ(kMaxCount2, v.size());
    for (size_t n = 0; n < kMaxCount2; ++n) {
        EXPECT_EQ(hashIndex(n), v[n]) << "At index " << n;
    }
}


TEST(PodVector, AssignmentOperator) {
    PodVector<int> v1;
    const size_t kMaxCount = 10000;
    for (size_t n = 0; n < kMaxCount; ++n) {
        v1.append(hashIndex(n));
    }
    EXPECT_EQ(kMaxCount, v1.size());

    PodVector<int> v2;
    v2 = v1;

    v1.reserve(0);

    EXPECT_EQ(kMaxCount, v2.size());
    for (size_t n = 0; n < kMaxCount; ++n) {
        EXPECT_EQ(hashIndex(n), v2[n]) << "At index " << n;
    }
}

}  // namespace base
}  // namespace android
