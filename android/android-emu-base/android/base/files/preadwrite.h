// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#ifndef _WIN32
#include <unistd.h>
#else
#include <stdint.h>
#endif

namespace android {
namespace base {

// This is a cross-platform version of pread(2)/pwrite(2) syscalls, with the
// following from their POSIX versions:
//
// - On Windows it utilizes ReadFile()/WriteFile() + OVERLAPPED structure to
//   pass the position in. As per MSDN, this way APIs still update the current
//   file position (as opposed to the POSIX ones)
//
// - Windows version doesn't return most of the |errno| error codes, using
//   EINVAL for most of the issues and EAGAIN if it's a nonblocking FD.
//
// That's why the best way of using android::base::pread() and pwrite() is to
// never use it together with regular read()/write() calls, especially in
// multi-threaded code. Nobody can predict the active read position when using
// them on different platforms.
//

#ifndef _WIN32
using ::pread;
using ::pwrite;
#else
int64_t pread(int fd, void* buf, size_t count, int64_t offset);
int64_t pwrite(int fd, const void* buf, size_t count, int64_t offset);
#endif

}  // namespace base
}  // namespace android
