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

#include <stdio.h>

// gettimeofday taken from oslib-win32.c
// Offset between 1/1/1601 and 1/1/1970 in 100 nanosec units
#define _W32_FT_OFFSET (116444736000000000ULL)
int gettimeofday(struct timeval* tp) {
    union {
        unsigned long long ns100; /* time since 1 Jan 1601 in 100ns units */
        FILETIME ft;
    } _now;

    if (tp) {
        GetSystemTimeAsFileTime(&_now.ft);
        tp->tv_usec = (long)((_now.ns100 / 10ULL) % 1000000ULL);
        tp->tv_sec = (long)((_now.ns100 - _W32_FT_OFFSET) % 10000000ULL);
    }
    /* Always return 0 as per Open Group Base Specifications Issue 6.
     * Do not set errno on error. */
    return 0;
}

// From https://msdn.microsoft.com/en-us/library/28d5ce15.aspx
int asprintf(char** buf, char* format, ...) {
    va_list args;
    int     len;

    if (buf == NULL) {
        return -1;
    }

    // retrieve the variable arguments
    va_start( args, format );

    len = _vscprintf( format, args ) // _vscprintf doesn't count
                                + 1; // terminating '\0'

    if (len <= 0) {
        return len;
    }

    *buf = (char*)malloc( len * sizeof(char) );

    vsprintf( *buf, format, args ); // C4996
    // Note: vsprintf is deprecated; consider using vsprintf_s instead
    return len;
}

#ifdef _WIN32
// From https://msdn.microsoft.com/en-us/library/28d5ce15.aspx
static int vasprintf( char** buf, const char* format, va_list args )
{
    int     len;

    if (buf == NULL) {
        return -1;
    }

    len = _vscprintf( format, args ) // _vscprintf doesn't count
                                + 1; // terminating '\0'

    if (len <= 0) {
        return len;
    }

    *buf = (char*)malloc( len * sizeof(char) );

    vsprintf( *buf, format, args ); // C4996
    // Note: vsprintf is deprecated; consider using vsprintf_s instead
    return len;
}
#endif
