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

#include "android/base/sockets/SocketUtils.h"

#include "android/base/sockets/ScopedSocket.h"
#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(SocketUtils, socketSendAll) {
    char kData[] = "Hello World!";
    const size_t kDataLen = sizeof(kData) - 1U;

    int sock[2];
    ASSERT_EQ(0, socketCreatePair(&sock[0], &sock[1]));

    EXPECT_TRUE(socketSendAll(sock[0], kData, kDataLen));

    char data[kDataLen] = {};
    for (size_t n = 0; n < kDataLen; ++n) {
        EXPECT_EQ(1, socketRecv(sock[1], &data[n], 1)) << "#" << n;
        EXPECT_EQ(kData[n], data[n]) << "#" << n;
    }

    socketClose(sock[1]);
    socketClose(sock[0]);
}

TEST(SocketUtils, socketRecvAll) {
    char kData[] = "Hello World!";
    const size_t kDataLen = sizeof(kData) - 1U;

    int sock[2];
    ASSERT_EQ(0, socketCreatePair(&sock[0], &sock[1]));

    for (size_t n = 0; n < kDataLen; ++n) {
        EXPECT_EQ(1, socketSend(sock[0], &kData[n], 1)) << "#" << n;
    }

    char data[kDataLen] = {};
    EXPECT_TRUE(socketRecvAll(sock[1], data, kDataLen));

    for (size_t n = 0; n < kDataLen; ++n) {
        EXPECT_EQ(kData[n], data[n]) << "#" << n;
    }

    socketClose(sock[1]);
    socketClose(sock[0]);
}

TEST(SocketUtils, socketGetPort) {
    ScopedSocket s0;
    // Find a free TCP IPv4 port and bind to it.
    int port;
    for (port = 1024; port < 65536; ++port) {
        s0.reset(socketTcp4LoopbackServer(port));
        if (s0.valid()) {
            break;
        }
    }
    ASSERT_TRUE(s0.valid()) << "Could not find free TCP port";
    EXPECT_EQ(port, socketGetPort(s0.get()));

    // Connect to it.
    ScopedSocket s1(socketTcp4LoopbackClient(port));
    ASSERT_TRUE(s1.valid());

    // Accept it to create a pair of connected socket.
    // The port number of |s2| should be |port|.
    ScopedSocket s2(socketAcceptAny(s0.get()));
    ASSERT_TRUE(s2.valid());

    EXPECT_EQ(port, socketGetPort(s2.get()));
}

TEST(SocketUtils, socketGetPeerPort) {
    // Create a server socket bound to a random port.
    ScopedSocket s0(socketTcp4LoopbackServer(0));
    ASSERT_TRUE(s0.valid());
    int serverPort = socketGetPort(s0.get());
    ASSERT_GT(serverPort, 0);

    // Connect to it.
    ScopedSocket s1(socketTcp4LoopbackClient(serverPort));
    ASSERT_TRUE(s1.valid()) << "Could not connect to server socket";

    // Accept it to create a pair of connected sockets.
    ScopedSocket s2(socketAcceptAny(s0.get()));
    ASSERT_TRUE(s2.valid());

    EXPECT_EQ(socketGetPort(s1.get()), socketGetPeerPort(s2.get()));
    EXPECT_EQ(socketGetPort(s2.get()), socketGetPeerPort(s1.get()));
}

}  // namespace base
}  // namespace android
