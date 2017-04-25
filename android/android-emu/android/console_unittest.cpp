// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/console_internal.h"

#include "android/base/sockets/SocketUtils.h"

#include <gtest/gtest.h>

namespace android {

using namespace base;

namespace console {

TEST(Console, postauth_help_size_max) {
    // Android studio has a read buffer size of 1024 bytes.
    // Older versions of studio use the 'help' command to ping the
    // emulator, while newer versions will just use the ping command.
    // So for now, we'll be careful to not pass the 1024-byte limit on
    // the help printout.
    const int MAX_SIZE = 1024;
    const int BUFF_SIZE = MAX_SIZE * 2;
    char buff[BUFF_SIZE];
    int sock[2];

    ASSERT_EQ(0, socketCreatePair(&sock[0], &sock[1]));
    socketSetBlocking(sock[0]);

    // Create a fake client that will send the output through this socket
    void* opaque = test_control_client_create(sock[1]);
    ASSERT_TRUE(opaque != NULL);

    do_help_test(opaque, NULL);

    // Read back the output of 'help'. Should be <= 1024 bytes.
    int size = socketRecv(sock[0], buff, 2048);
    EXPECT_LE(size, MAX_SIZE);
    printf("size=%d\n", size);
    printf("buff:\n\n%s\n", buff);

    test_control_client_close(opaque);
}

}  // namespace console
}  // namespace android
