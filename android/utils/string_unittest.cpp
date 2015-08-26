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

TEST(String, endsWithFailure) {
    EXPECT_FALSE(endswith("", "suffix"));
    EXPECT_FALSE(endswith("fix", "suffix"));
    EXPECT_FALSE(endswith("abc", "xxyyzz"));
}

TEST(String, endsWithSuccess) {
    EXPECT_TRUE(endswith("string", ""));
    EXPECT_TRUE(endswith("", ""));
    EXPECT_TRUE(endswith("abc", "abc"));
    EXPECT_TRUE(endswith("abc", "c"));
    EXPECT_TRUE(endswith("0123", "23"));
}
