
// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <stdio.h>
#include "compat_compiler.h"
#include <Windows.h>

ANDROID_BEGIN_HEADER

int _ftruncate(int fd, int64_t length)
{
    LARGE_INTEGER li;
    DWORD dw;
    LONG high;
    HANDLE h;
    BOOL res;

    if ((GetVersion() & 0x80000000UL) && (length >> 32) != 0)
        return -1;

    h = (HANDLE)_get_osfhandle(fd);

    /* get current position, ftruncate do not change position */
    li.HighPart = 0;
    li.LowPart = SetFilePointer (h, 0, &li.HighPart, FILE_CURRENT);
    if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        return -1;
    }

    high = length >> 32;
    dw = SetFilePointer(h, (DWORD) length, &high, FILE_BEGIN);
    if (dw == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        return -1;
    }
    res = SetEndOfFile(h);

    /* back to old position */
    SetFilePointer(h, li.LowPart, &li.HighPart, FILE_BEGIN);
    return res ? 0 : -1;
}

int mkstemp(char *tmplate) {
    // Check if the tmplate string is null or doesn't end with "XXXXXX"
    if (tmplate == nullptr || std::strlen(tmplate) < 6 || std::strcmp(tmplate + std::strlen(tmplate) - 6, "XXXXXX") != 0) {
        errno = EINVAL;  // Invalid argument
        return -1;
    }

    // Generate a unique filename
    if (_mktemp_s(tmplate, std::strlen(tmplate) + 1) != 0) {
        errno = EIO;  // I/O error
        return -1;
    }

    // Open the file with read and write access
    int fd;
    if (_sopen_s(&fd, tmplate, _O_RDWR | _O_CREAT | _O_EXCL, _SH_DENYNO, _S_IREAD | _S_IWRITE) != 0) {
        return -1;
    }

    return fd;
}

int mkostemp(char *tmplate, int flags) {
    // Use mkstemp for Windows as it doesn't have all the flags like O_APPEND or O_SYNC
    return mkstemp(tmplate);
}

int mkstemps(char *tmplate, int suffixlen) {
    // Check if the tmplate string is null or doesn't end with "XXXXXX"
    if (tmplate == nullptr || std::strlen(tmplate) < 6 || std::strcmp(tmplate + std::strlen(tmplate) - 6, "XXXXXX") != 0) {
        errno = EINVAL;  // Invalid argument
        return -1;
    }

    // Generate a unique filename
    if (_mktemp_s(tmplate, std::strlen(tmplate) + 1) != 0) {
        errno = EIO;  // I/O error
        return -1;
    }

    // Add the suffix to the tmplate
    std::strncat(tmplate, "suffix", suffixlen);

    // Open the file with read and write access
    int fd;
    if (_sopen_s(&fd, tmplate, _O_RDWR | _O_CREAT | _O_EXCL, _SH_DENYNO, _S_IREAD | _S_IWRITE) != 0) {
        return -1;
    }

    return fd;
}

int mkostemps(char *tmplate, int suffixlen, int flags) {
    // Use mkstemps for Windows as it doesn't have all the flags like O_APPEND or O_SYNC
    return mkstemps(tmplate, suffixlen);
}

ANDROID_END_HEADER