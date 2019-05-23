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

#include "android/base/misc/IpcPipe.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

TEST(IpcPipe, create) {
    int fds[2];
    ASSERT_EQ(0, ipcPipeCreate(&fds[0], &fds[1]));

    ipcPipeClose(fds[0]);
    ipcPipeClose(fds[1]);
}

TEST(IpcPipe, readWrite) {
    char kData[] = "Hello Pipe!";

    int fds[2];
    ASSERT_EQ(0, ipcPipeCreate(&fds[0], &fds[1]));

    ASSERT_EQ(sizeof(kData), ipcPipeWrite(fds[1], kData, sizeof(kData)));

    char buffer[sizeof(kData) + 1] = { 0 };
    ASSERT_EQ(sizeof(kData), ipcPipeRead(fds[0], buffer, sizeof(buffer)));

    ASSERT_STREQ(kData, buffer);

    ipcPipeClose(fds[0]);
    ipcPipeClose(fds[1]);
}

}  // namespace base
}  // namespace android

