// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <gtest/gtest.h>

static int said_hello = 0;
const char* msg = "Hello world";

extern "C" {
extern int say_hello();

void hello(int x, char *str) {
    said_hello = 1;
    EXPECT_EQ(x, 127);
    EXPECT_STREQ(str, "Hello world");
}
}

TEST(Yasm, SayHelloFromAsm) {
    EXPECT_EQ(say_hello(), 0xFF);
    EXPECT_EQ(said_hello, 1);
}
