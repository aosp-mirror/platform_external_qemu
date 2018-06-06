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

#include <cstdio>
#include <errno.h>

#include "android/base/files/FileShareOpen.h"

#ifdef _WIN32

#include <share.h>

FILE* android::base::fsopen(const char* filename, const char* mode,
        android::base::FileShare fileshare) {
    int shflag = _SH_DENYWR;
    switch (fileshare) {
        case FileShare::Read:
            // Others cannot write
            shflag = _SH_DENYWR;
            break;
        case FileShare::Write:
            // Others cannot read nor write
            shflag = _SH_DENYRW;
            break;
    }
    FILE* file = _fsopen(filename, mode, shflag);
    if (!file) {
        fprintf(stderr, "%s open failed errno %d\n", filename, errno);
    }
    return file;
}
#else
 
#include <unistd.h> 
#include <fcntl.h>

FILE* android::base::fsopen(const char* filename, const char* mode,
        android::base::FileShare fileshare) {
    FILE* file = fopen(filename, mode);
    if (!file) {
        return nullptr;
    }
    flock lock;
    switch (fileshare) {
        case FileShare::Read:
            lock.l_type = F_RDLCK;
            break;
        case FileShare::Write:
            lock.l_type = F_WRLCK;
            break;
        default:
            lock.l_type = F_RDLCK;
            break;
    }
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // Lock the whole file.
    int fd = fileno(file);
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        fclose(file);
        fprintf(stderr, "%d lock failed errno %d\n", fd, errno);
        return nullptr;
    }
    return file;
}

#endif
