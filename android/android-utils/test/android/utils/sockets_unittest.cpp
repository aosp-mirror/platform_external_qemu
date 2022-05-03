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

#include "android/utils/sockets.h"

#include <gtest/gtest.h>


TEST(SockAddress, ResolveIp4AndToString) {
    SockAddress s[1];
    char tmp[256];
    android_socket_init();

    EXPECT_EQ(0 ,sock_address_init_resolve(s, "216.58.194.164", 53, 0));

    // An ip4 "hostname" can resolve to an ip6 socket address, depending on OS and
    // network configuration (see b/178218218). However if this resolved to
    // an ip4 address we expect this to be an ip4 string.
    if (s->family == SOCKET_INET) {
        EXPECT_STREQ("216.58.194.164:53", sock_address_to_string(s, tmp, sizeof(tmp)));
    }
}

TEST(SockAddress, ResolveIp6AndToString) {
    SockAddress s[1];
    char tmp[256];
    android_socket_init();

    // An ip6 address *ALWAYS* resolves to an ip6 socket address and hence an ip6 string.
    EXPECT_EQ(0 ,sock_address_init_resolve(s, "2001:4860:f802::9b", 53, 0));
    EXPECT_EQ(SOCKET_IN6, s->family);
    EXPECT_STREQ("[2001:4860:f802::9b]:53", sock_address_to_string(s, tmp, sizeof(tmp)));
}
