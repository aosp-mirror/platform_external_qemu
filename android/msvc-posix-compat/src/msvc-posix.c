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
#include "android/utils/win32_unicode.h"


#include <io.h>

#include <stdio.h>
#include <stdlib.h>

int mkstemp(char* t) {
    int len = strlen(t) + 1;
    errno_t err = _mktemp_s(t, len);

    if (err != 0) {
        return -1;
    }

    wchar_t* wt = win32_utf8_to_utf16_str(t);
    int fd = _wsopen(wt, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY, _SH_DENYRW,
                  _S_IREAD | _S_IWRITE);
    free(wt);
    return fd;
}

// From https://msdn.microsoft.com/en-us/library/28d5ce15.aspx
int asprintf(char** buf, const char* format, ...) {
    va_list args;
    int len;

    if (buf == NULL) {
        return -1;
    }

    // retrieve the variable arguments
    va_start(args, format);

    len = _vscprintf(format, args)  // _vscprintf doesn't count
          + 1;                      // terminating '\0'

    if (len <= 0) {
        return len;
    }

    *buf = (char*)malloc(len * sizeof(char));

    vsprintf(*buf, format, args);  // C4996
    // Note: vsprintf is deprecated; consider using vsprintf_s instead
    return len;
}

// From https://msdn.microsoft.com/en-us/library/28d5ce15.aspx
static int vasprintf(char** buf, const char* format, va_list args) {
    int len;

    if (buf == NULL) {
        return -1;
    }

    len = _vscprintf(format, args)  // _vscprintf doesn't count
          + 1;                      // terminating '\0'

    if (len <= 0) {
        return len;
    }

    *buf = (char*)malloc(len * sizeof(char));

    vsprintf(*buf, format, args);  // C4996
    // Note: vsprintf is deprecated; consider using vsprintf_s instead
    return len;
}

