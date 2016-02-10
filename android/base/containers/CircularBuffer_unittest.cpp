// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/containers/CircularBuffer.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(CircularBuffer, Empty) {
    CircularBuffer<int> cb(1);
    EXPECT_TRUE(cb.empty());
    EXPECT_EQ(0, cb.size());
}

TEST(CircularBuffer, AppendOnce) {
    CircularBuffer<int> cb(1);
    cb.push_back(10098);
    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(1, cb.size());
    auto it = cb.begin();
    EXPECT_EQ(10098, *it);
}

TEST(CircularBuffer, AppendTwice) {
    CircularBuffer<int> cb(1);
    cb.push_back(10098);
    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(1, cb.size());
    auto it = cb.begin();
    EXPECT_EQ(10098, *it);
    cb.push_back(10099);
    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(1, cb.size());
    it = cb.begin();
    EXPECT_EQ(10099, *it);
}

TEST(CircularBuffer, ManyItems) {
    CircularBuffer<int> cb(5);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    cb.push_back(4);
    cb.push_back(5);
    cb.push_back(6);
    cb.push_back(7);

    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(5, cb.size());
    int expected_values[] = {3, 4, 5, 6, 7};
    for (int i = 0; i < cb.size(); ++i) {
        EXPECT_EQ(expected_values[i], *(cb.begin() + i));
    }
}

TEST(CircularBuffer, PopFront) {
    CircularBuffer<int> cb(1);
    cb.push_back(10098);
    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(1, cb.size());
    auto it = cb.begin();
    EXPECT_EQ(10098, *it);
    cb.pop_front();
    EXPECT_TRUE(cb.empty());
    EXPECT_EQ(0, cb.size());
}

TEST(CircularBuffer, PopBack) {
    CircularBuffer<int> cb(1);
    cb.push_back(10098);
    EXPECT_FALSE(cb.empty());
    EXPECT_EQ(1, cb.size());
    auto it = cb.begin();
    EXPECT_EQ(10098, *it);
    cb.pop_back();
    EXPECT_TRUE(cb.empty());
    EXPECT_EQ(0, cb.size());
}

}
}

