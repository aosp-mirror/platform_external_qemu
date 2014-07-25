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

#include "android/utils/string.h"

#include <algorithm>
#include <string.h>

#if defined(__APPLE__) || defined(__FreeBSD__)

// already defined in C library

#else // __linux__ && _WIN32

// NOTE: linux has this defined in libbsd if we want to switch to
// that some day

size_t strlcpy(char* dst, const char * src, size_t size)
{
    size_t srcLen = strlen(src);
    if (size > 0) {
        size_t copyLen = std::min(srcLen, size-1);
        memcpy(dst, src, copyLen);
        dst[copyLen] = 0;
    }
    return srcLen;
}

#endif
