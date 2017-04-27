// Copyright (C) 2017 The Android Open Source Project
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

    send_test_string(opaque, "help");

    // Read back the output of 'help'. Should be <= 1024 bytes.
    int size = socketRecv(sock[0], buff, 2048);
    EXPECT_LE(size, MAX_SIZE);

    test_control_client_close(opaque);
    socketClose(sock[1]);
    socketClose(sock[0]);
}

static void ping_test();

TEST(Console, preauth_ping) {
    ping_test();
}

TEST(Console, postauth_ping) {
    ping_test();
}


static void ping_test() {
    // Verify the 'ping' response.
    // This may be used by automated clients that expect
    // an exact response, so we verify the exact response.
    const char EXPECTED_PING_RESPONSE[] = "I am alive!\r\nOK\r\n";
    const int BUFF_SIZE = 1024;
    char buff[BUFF_SIZE];
    int sock[2];

    ASSERT_EQ(0, socketCreatePair(&sock[0], &sock[1]));
    socketSetBlocking(sock[0]);

    // Create a fake client that will send the output through this socket
    void* opaque = test_control_client_create(sock[1]);
    ASSERT_TRUE(opaque != NULL);

    send_test_string(opaque, "ping");

    // Read back the output
    int size = socketRecv(sock[0], buff, BUFF_SIZE);

    test_control_client_close(opaque);
    socketClose(sock[1]);
    socketClose(sock[0]);

    EXPECT_EQ(size, strlen(EXPECTED_PING_RESPONSE));
    EXPECT_EQ(0, strncmp(buff, EXPECTED_PING_RESPONSE, size));
}

}  // namespace console
}  // namespace android
