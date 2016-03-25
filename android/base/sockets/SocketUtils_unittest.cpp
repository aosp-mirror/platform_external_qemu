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

}  // namespace base
}  // namespace android
