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
#include "android/base/sockets/SocketDrainer.h"
#include "android/looper.h"
#include "android/utils/tempfile.h"

// The following unit tests only work on Linux and OS X, because they rely on "fork()"
// system call, which is not supported by Windows yet.

#ifndef _WIN32

#include <gtest/gtest.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

namespace android {
namespace base {

namespace {

const int kBigNumber = (1<<10);

int writeToSocket(const char* filename, int socket_fd) {
    FILE* ff = fopen(filename, "w");
    int i = 0;
    for (i = 0; i < kBigNumber; ++i) {
        int size = ::send(socket_fd, "a", 1, 0);
        if (size == 0) {
            break;
        } else if (size < 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            } else {
                break;
            }
        }
    }
    fprintf(ff, "%d", i);
    fclose(ff);
    shutdown(socket_fd, SHUT_WR);
    return 0;
}

void socket_set_nonblock(int fd) {
    int flags = ::fcntl(fd, F_GETFL);
    ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int socket_pair(int* fd1, int* fd2) {
    int   fds[2];
    int   ret = ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    if (!ret) {
        socket_set_nonblock(fds[0]);
        socket_set_nonblock(fds[1]);
        *fd1 = fds[0];
        *fd2 = fds[1];
    }
    return ret;
}

}  // namespace

TEST(SocketDrainer, GracefulShutDownFromC) {
    int socket_fd1 = -1;
    int socket_fd2 = -1;
    int error = socket_pair(&socket_fd1, &socket_fd2);
    EXPECT_EQ(error, 0);
    ASSERT_GE(socket_fd1, 0);
    ASSERT_GE(socket_fd2, 0);

    TempFile* childFile = tempfile_create();
    const char* kChildFileName = tempfile_path(childFile);

    // create a process to send bytes from the writing socket
    int childpid = fork();
    int childExitStatus;
    if (childpid == 0) {
        close(socket_fd2);
        _exit(writeToSocket(kChildFileName, socket_fd1));
    } else {
        close(socket_fd1);
        ASSERT_GT(childpid, 0);
    }

    // drain the reading socket: C style
    Looper* mainLooper = looper_newGeneric();
    socket_drainer_start(mainLooper);
    socket_drainer_drain_and_close(socket_fd2);
    looper_run(mainLooper);
    socket_drainer_stop();
    looper_free(mainLooper);
    wait(&childExitStatus);

    // find out how many bytes have been written
    FILE* fchild = fopen(kChildFileName, "r");
    int numWritten = 0;
    fscanf(fchild, "%d", &numWritten);
    tempfile_close(childFile);

    // should not miss any byte
    EXPECT_EQ(numWritten, kBigNumber);
}

TEST(SocketDrainer, GracefulShutDownFromCpp) {
    int socket_fd1 = -1;
    int socket_fd2 = -1;
    int error = socket_pair(&socket_fd1, &socket_fd2);
    EXPECT_EQ(error, 0);
    ASSERT_GE(socket_fd1, 0);
    ASSERT_GE(socket_fd2, 0);

    TempFile* childFile = tempfile_create();
    const char* kChildFileName = tempfile_path(childFile);

    int childpid = fork();
    int childExitStatus;
    if (childpid == 0) {
        close(socket_fd2);
        _exit(writeToSocket(kChildFileName, socket_fd1));
    } else {
        close(socket_fd1);
        ASSERT_GT(childpid, 0);
    }

    // drain the reading socket: C++ style
    Looper* mainLooper = looper_newGeneric();
    {
        SocketDrainer socketDrainer(mainLooper);
        socketDrainer.drainAndClose(socket_fd2);
        looper_run(mainLooper);
    }
    looper_free(mainLooper);
    wait(&childExitStatus);

    FILE* fchild = fopen(kChildFileName, "r");
    int numWritten = 0;
    fscanf(fchild, "%d", &numWritten);
    tempfile_close(childFile);

    EXPECT_EQ(numWritten, kBigNumber);
}

} // namespace base
} // namespace android

#endif
