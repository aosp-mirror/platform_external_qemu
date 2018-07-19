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

#include <errno.h>
#include <cstdio>

#include "android/base/files/FileShareOpen.h"
#include "android/base/files/FileShareOpenImpl.h"

void android::base::createFileForShare(const char* filename) {
    void* handle = internal::openFileForShare(filename);
    if (handle) {
        internal::closeFileForShare(handle);
    }
}

#ifdef _WIN32

#include "android/base/system/Win32UnicodeString.h"

#include <share.h>
#include <windows.h>

FILE* android::base::fsopen(const char* filename,
                            const char* mode,
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
    const Win32UnicodeString filenameW(filename);
    const Win32UnicodeString modeW(mode);
    FILE* file = _wfsopen(filenameW.c_str(), modeW.c_str(), shflag);
    if (!file) {
        fprintf(stderr, "%s open failed errno %d\n", filename, errno);
    }
    return file;
}

void* android::base::internal::openFileForShare(const char* filename) {
    const Win32UnicodeString filenameW(filename);
    void* hndl = CreateFileW(filenameW.c_str(), 0,
            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hndl != INVALID_HANDLE_VALUE) {
        return hndl;
    } else {
        return nullptr;
    }
}

void android::base::internal::closeFileForShare(void* fileHandle) {
    CloseHandle(fileHandle);
}

#else
#include <sys/file.h>
#include <unistd.h>

FILE* android::base::fsopen(const char* filename,
                            const char* mode,
                            android::base::FileShare fileshare) {
    FILE* file = fopen(filename, mode);
    if (!file) {
        return nullptr;
    }
    int operation = LOCK_SH;
    switch (fileshare) {
        case FileShare::Read:
            operation = LOCK_SH;
            break;
        case FileShare::Write:
            operation = LOCK_EX;
            break;
        default:
            operation = LOCK_SH;
            break;
    }
    int fd = fileno(file);
    if (flock(fd, operation | LOCK_NB) == -1) {
        fclose(file);
        fprintf(stderr, "%s lock failed errno %d\n", filename, errno);
        return nullptr;
    }
    return file;
}

void* android::base::internal::openFileForShare(const char* filename) {
    return fopen(filename, "a");
}

void android::base::internal::closeFileForShare(void* fileHandle) {
    fclose((FILE*)fileHandle);
}

#endif
