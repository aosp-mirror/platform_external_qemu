// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/CrossSessionSocket.h"

#include "android/base/files/InplaceStream.h"
#include "android/base/sockets/SocketUtils.h"

#include <cstring>
#include <gtest/gtest.h>

namespace android {
namespace emulation {

TEST(CrossSessionSocket, ReclaimWithoutRegister) {
    CrossSessionSocket::clearRecycleSockets();
    const int kUnknownFd = 1;
    EXPECT_FALSE(CrossSessionSocket::reclaimSocket(kUnknownFd).valid());
}

TEST(CrossSessionSocket, Recycle) {
    CrossSessionSocket::clearRecycleSockets();
    int fd = base::socketCreateTcp4();
    CrossSessionSocket socket(fd);
    EXPECT_TRUE(socket.valid());
    CrossSessionSocket::registerForRecycle(socket.fd());
    CrossSessionSocket::recycleSocket(std::move(socket));
    EXPECT_FALSE(socket.valid());
    CrossSessionSocket recycled = CrossSessionSocket::reclaimSocket(fd);
    EXPECT_TRUE(recycled.valid());
    EXPECT_EQ(recycled.fd(), fd);
    CrossSessionSocket::clearRecycleSockets();
}

TEST(CrossSessionSocket, ReclaimTwice) {
    CrossSessionSocket::clearRecycleSockets();
    int fd = base::socketCreateTcp4();
    CrossSessionSocket socket(fd);
    EXPECT_TRUE(socket.valid());
    CrossSessionSocket::registerForRecycle(socket.fd());
    CrossSessionSocket::recycleSocket(std::move(socket));
    CrossSessionSocket::reclaimSocket(fd);
    CrossSessionSocket recycled = CrossSessionSocket::reclaimSocket(fd);
    EXPECT_FALSE(recycled.valid());
    CrossSessionSocket::clearRecycleSockets();
}

TEST(CrossSessionSocket, RecycleWithoutRegister) {
    CrossSessionSocket::clearRecycleSockets();
    int fd = base::socketCreateTcp4();
    CrossSessionSocket socket(fd);
    EXPECT_TRUE(socket.valid());
    CrossSessionSocket::recycleSocket(std::move(socket));
    EXPECT_TRUE(socket.valid());
    CrossSessionSocket recycled = CrossSessionSocket::reclaimSocket(fd);
    EXPECT_FALSE(recycled.valid());
    CrossSessionSocket::clearRecycleSockets();
}

TEST(CrossSessionSocket, DrainStale) {
    const char* kContents = "Stale text";
    const int kContentSize = strlen(kContents) + 1;
    // Make buffer slightly larger so that it can catch if there are
    // unexpected extra bits written to the buffer.
    const int kBufferSize = kContentSize + 10;
    // Number of bytes going to be written into the snapshot
    const int kSnapshotSize = kBufferSize + 4;
    CrossSessionSocket::clearRecycleSockets();

    int fd1, fd2;
    int ret = base::socketCreatePair(&fd1, &fd2);
    EXPECT_EQ(0, ret);
    CrossSessionSocket socket(fd1);
    CrossSessionSocket::registerForRecycle(socket.fd());

    ssize_t sendSize = base::socketSend(fd2, kContents, kContentSize);
    EXPECT_EQ(kContentSize, sendSize);
    socket.drainSocket(CrossSessionSocket::DrainBehavior::AppendToBuffer);
    char snapshotBuffer[kSnapshotSize];
    base::InplaceStream snapshotStream(snapshotBuffer, kSnapshotSize);
    socket.onSave(&snapshotStream);

    CrossSessionSocket::recycleSocket(std::move(socket));
    CrossSessionSocket socket2 = CrossSessionSocket::reclaimSocket(fd1);
    socket2.onLoad(&snapshotStream);

    char readbackBuffer[kBufferSize];
    ssize_t readSize = socket2.readStaleData(readbackBuffer, kBufferSize);
    EXPECT_EQ(kContentSize, readSize);
    EXPECT_EQ(0, memcmp(kContents, readbackBuffer, kContentSize));

    // fd1 will be closed automatically
    base::socketClose(fd2);
    CrossSessionSocket::clearRecycleSockets();
}

} // namespace emulation
} // namespace android
