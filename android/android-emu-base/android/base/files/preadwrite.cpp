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

#include "android/base/files/preadwrite.h"

#ifndef _WIN32
#error Only build this file for Win32, other platforms have native pread() call
#endif

#include <errno.h>
#include <io.h>
#include <windows.h>

namespace android {
namespace base {

int64_t pread(int fd, void* buf, size_t count, int64_t offset) {
    if (fd < 0) {
        errno = EINVAL;
        return -1;
    }
    auto handle = (HANDLE)_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE) {
        errno = EINVAL;
        return -1;
    }

    OVERLAPPED params = {};
    params.Offset = static_cast<DWORD>(offset);
    params.OffsetHigh = static_cast<DWORD>(offset >> 32);
    DWORD countRead = 0;
    if (!::ReadFile(handle, buf, count, &countRead, &params)) {
        switch (::GetLastError()) {
            case ERROR_IO_PENDING:
                errno = EAGAIN;
                return -1;
            default:
                errno = EINVAL;
                return -1;
        }
    }
    return static_cast<int64_t>(countRead);
}

int64_t pwrite(int fd, const void* buf, size_t count, int64_t offset) {
    if (fd < 0) {
        errno = EINVAL;
        return -1;
    }
    auto handle = (HANDLE)_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE) {
        errno = EINVAL;
        return -1;
    }

    OVERLAPPED params = {};
    params.Offset = static_cast<DWORD>(offset);
    params.OffsetHigh = static_cast<DWORD>(offset >> 32);
    DWORD countWritten = 0;
    if (!::WriteFile(handle, buf, count, &countWritten, &params)) {
        switch (::GetLastError()) {
            case ERROR_IO_PENDING:
                errno = EAGAIN;
                return -1;
            default:
                errno = EINVAL;
                return -1;
        }
    }
    return static_cast<int64_t>(countWritten);
}

}  // namespace base
}  // namespace android
