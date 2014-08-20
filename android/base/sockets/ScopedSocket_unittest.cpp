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

#include "android/base/sockets/ScopedSocket.h"

#include <gtest/gtest.h>

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#endif

namespace android {
namespace base {

namespace {

int OpenNull() {
    return ::socket(AF_INET, SOCK_STREAM, 0);
}

}  // namespace

TEST(ScopedSocket, DefaultConstructor) {
    ScopedSocket f;
    EXPECT_FALSE(f.valid());
    EXPECT_EQ(-1, f.get());
}

TEST(ScopedSocket, Constructor) {
    ScopedSocket f(OpenNull());
    EXPECT_TRUE(f.valid());
}

TEST(ScopedSocket, Release) {
    ScopedSocket f(OpenNull());
    EXPECT_TRUE(f.valid());
    int fd = f.release();
    EXPECT_FALSE(f.valid());
    EXPECT_NE(-1, fd);
    ::close(fd);
}

TEST(ScopedSocket, Close) {
    ScopedSocket f(OpenNull());
    EXPECT_TRUE(f.valid());
    f.close();
    EXPECT_FALSE(f.valid());
}

TEST(ScopedSocket, Swap) {
    ScopedSocket f1;
    ScopedSocket f2(OpenNull());
    EXPECT_FALSE(f1.valid());
    EXPECT_TRUE(f2.valid());
    f1.swap(&f2);
    EXPECT_FALSE(f2.valid());
    EXPECT_TRUE(f1.valid());
}

}  // namespace base
}  // namespace android
