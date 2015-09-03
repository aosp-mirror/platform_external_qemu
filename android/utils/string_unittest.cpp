// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/string.h"

#include <gtest/gtest.h>

TEST(String, str_ends_withFailure) {
    EXPECT_FALSE(str_ends_with("", "suffix"));
    EXPECT_FALSE(str_ends_with("fix", "suffix"));
    EXPECT_FALSE(str_ends_with("abc", "xxyyzz"));
}

TEST(String, str_ends_withSuccess) {
    EXPECT_TRUE(str_ends_with("string", ""));
    EXPECT_TRUE(str_ends_with("", ""));
    EXPECT_TRUE(str_ends_with("abc", "abc"));
    EXPECT_TRUE(str_ends_with("abc", "c"));
    EXPECT_TRUE(str_ends_with("0123", "23"));
}
