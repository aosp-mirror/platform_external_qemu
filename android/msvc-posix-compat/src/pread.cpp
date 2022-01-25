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
#include "msvc-posix.h"

#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>

ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
    if (fd < 0) {
        errno = EINVAL;
        return -1;
    }
    auto handle = (HANDLE)_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return -1;
    }

    DWORD cRead;
    OVERLAPPED overlapped = {
            .OffsetHigh = (DWORD)((offset & 0xFFFFFFFF00000000LL) >> 32),
            .Offset = (DWORD)(offset & 0xFFFFFFFFLL)};
    bool rd = ReadFile(handle, buf, count, &cRead, &overlapped);
    if (!rd) {
        auto err = GetLastError();
        switch(err) {
            case ERROR_IO_PENDING: 
                errno = EAGAIN;
                break;
            case ERROR_HANDLE_EOF: 
                 cRead = 0;
                 errno = 0;
                 return 0;
            default:
                // Oh oh
                 errno = EINVAL;
        }
        return -1;
    }

    return cRead;
}