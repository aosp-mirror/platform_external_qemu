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

#include "android/base/Limits.h"
#include "android/wear-agent/SocketDrainer.h"
extern "C" {
#include "android/looper.h"
#include "android/sockets.h"
#include "android/utils/debug.h"
#include <sys/socket.h>
extern void start_socket_drainer(Looper* looper);
extern void socket_drainer_add(int fd);
extern void stop_socket_drainer();
}
#include <gtest/gtest.h>
#define  DPRINT(...)  do {  if (VERBOSE_CHECK(adb)) dprintn(__VA_ARGS__); } while (0)

namespace android {
namespace wear {

#ifndef _WIN32
static const int kBigNumber = (1<<10);
static int writeToSocket(int socket_fd) {
    // Just keep writing to socket
    for (int i = 0; i < kBigNumber; ++i) {
        int size = socket_send(socket_fd, "a", 1);
        if (size == 0) {
            DPRINT("Error in sending 1\n");
            return 1;
        } else if (size < 0) {
            if (errno == EWOULDBLOCK) {
                continue;
            } else {
                DPRINT("Error in sending 2: %d already sent\n", i);
                return 1;
            }
        }
    }
    shutdown(socket_fd, SHUT_WR);
    return 0;
}

TEST(SocketDrainer, GracefulShutDownFromC) {
    //VERBOSE_ENABLE(adb);
    int socket_fd1 = -1;
    int socket_fd2 = -1;
    int error = socket_pair(&socket_fd1, &socket_fd2);
    EXPECT_EQ(error, 0);
    ASSERT_GE(socket_fd1, 0);
    ASSERT_GE(socket_fd2, 0);
    int childpid = fork();
    int childExitStatus;
    if (childpid == 0) {
        _exit(writeToSocket(socket_fd1));
    } else {
        ASSERT_GT(childpid, 0);
    }
    Looper* mainLooper = looper_newGeneric();
    start_socket_drainer(mainLooper);
    socket_drainer_add(socket_fd2);
    looper_run(mainLooper);
    stop_socket_drainer();
    looper_free(mainLooper);
    wait(&childExitStatus);
    int childExitCode = WEXITSTATUS (childExitStatus);
    DPRINT("child exit code: %d\n", childExitCode);
    VERBOSE_DISABLE(adb);
    EXPECT_EQ(childExitCode, 0);
}

TEST(SocketDrainer, GracefulShutDownFromCpp) {
    //VERBOSE_ENABLE(adb);
    int socket_fd1 = -1;
    int socket_fd2 = -1;
    int error = socket_pair(&socket_fd1, &socket_fd2);
    EXPECT_EQ(error, 0);
    ASSERT_GE(socket_fd1, 0);
    ASSERT_GE(socket_fd2, 0);
    int childpid = fork();
    int childExitStatus;
    if (childpid == 0) {
        _exit(writeToSocket(socket_fd1));
    } else {
        ASSERT_GT(childpid, 0);
    }
    Looper* mainLooper = looper_newGeneric();
    {
        SocketDrainer socketDrainer(mainLooper);
        socketDrainer.add(socket_fd2);
        looper_run(mainLooper);
    }
    looper_free(mainLooper);
    wait(&childExitStatus);
    int childExitCode = WEXITSTATUS (childExitStatus);
    DPRINT("child exit code: %d\n", childExitCode);
    VERBOSE_DISABLE(adb);
    EXPECT_EQ(childExitCode, 0);
}
#endif

} // namespace wear
} // namespace android
