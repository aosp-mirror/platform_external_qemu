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

#include "android/base/containers/Lookup.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(Lookup, FindsHashMap) {
    std::unordered_map<int, char> m = {{1, '1'}, {2,'2'}, {3, '3'}};

    EXPECT_TRUE(find(m, 1));
    EXPECT_EQ('1', *find(m, 1));
    EXPECT_EQ('1', findOrDefault(m, 1, 'x'));

    EXPECT_TRUE(find(m, 2));
    EXPECT_EQ('2', *find(m, 2));
    EXPECT_EQ('2', findOrDefault(m, 2, 'x'));

    EXPECT_FALSE(find(m, 4));
    EXPECT_EQ('x', findOrDefault(m, 4, 'x'));

    EXPECT_TRUE(findFirstOf(m, {5, 4, 3, 2}));
    EXPECT_EQ('3', *findFirstOf(m, {5, 4, 3, 2}));
    EXPECT_EQ('3', findFirstOfOrDefault(m, {5, 4, 3, 2}, 'x'));
    EXPECT_EQ('x', findFirstOfOrDefault(m, {5, 4}, 'x'));
}

TEST(Lookup, FindsMap) {
    std::map<int, char> m = {{1, '1'}, {2,'2'}, {3, '3'}};

    EXPECT_TRUE(find(m, 1));
    EXPECT_EQ('1', *find(m, 1));
    EXPECT_EQ('1', findOrDefault(m, 1, 'x'));

    EXPECT_TRUE(find(m, 2));
    EXPECT_EQ('2', *find(m, 2));
    EXPECT_EQ('2', findOrDefault(m, 2, 'x'));

    EXPECT_FALSE(find(m, 4));
    EXPECT_EQ('x', findOrDefault(m, 4, 'x'));

    EXPECT_TRUE(findFirstOf(m, {5, 4, 3, 2}));
    EXPECT_EQ('3', *findFirstOf(m, {5, 4, 3, 2}));
    EXPECT_EQ('3', findFirstOfOrDefault(m, {5, 4, 3, 2}, 'x'));
    EXPECT_EQ('x', findFirstOfOrDefault(m, {5, 4}, 'x'));
}

TEST(Lookup, Exists) {
    EXPECT_TRUE(exists(std::set<int>{1,2}, 1));
    EXPECT_TRUE(existsAnyOf(std::set<int>{1,2}, {4,3,2}));

    EXPECT_FALSE(exists(std::unordered_set<int>{1,2}, 5));
    EXPECT_FALSE(existsAnyOf(std::unordered_set<int>{1,2}, {4,3}));

    EXPECT_TRUE(exists(std::map<int, int>{{1,1},{2,2}}, 1));
    EXPECT_TRUE(existsAnyOf(std::map<int, int>{{1,1},{2,2}}, {4,3,2}));

    EXPECT_FALSE(exists(std::unordered_map<int, int>{{1,1},{2,2}}, 5));
    EXPECT_FALSE(existsAnyOf(std::unordered_map<int, int>{{1,1},{2,2}}, {4,3}));
}

}  // namespace base
}  // namespace android
