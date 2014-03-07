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

#include "android/utils/bufprint.h"

#include <gtest/gtest.h>

TEST(bufprint, SimpleString) {
    char buffer[128], *p = buffer, *end = p + sizeof(buffer);

    p = bufprint(p, end, "%s/%s", "foo", "bar");
    EXPECT_EQ(buffer + 7, p);
    EXPECT_STREQ("foo/bar", buffer);
}

TEST(bufprint, TruncationOnOverflow) {
    char buffer[4], *p, *end = buffer + sizeof(buffer);
    p = bufprint(buffer, end, "foobar");
    EXPECT_EQ(buffer + 4, p);
    EXPECT_STREQ("foo", buffer);
}
