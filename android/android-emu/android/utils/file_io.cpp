// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/utils/file_io.h"
#include "android/base/files/Fd.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/system/Win32UnicodeString.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <share.h>
#include "android/base/files/PathUtils.h"
using android::base::PathUtils;
using android::base::Win32UnicodeString;
using android::base::ScopedCPtr;
#endif

// Provide different macros for different number of string arguments where each
// string argument requires conversion
#ifdef _WIN32
#define WIDEN_CALL_1(func, str1, ...) \
      _w ## func(Win32UnicodeString(str1).c_str() , ##__VA_ARGS__)
#define WIDEN_CALL_2(func, str1, str2, ...) \
      _w ## func(Win32UnicodeString(str1).c_str(), \
                 Win32UnicodeString(str2).c_str() , ##__VA_ARGS__)
#else
#define WIDEN_CALL_1(func, str1, ...) func(str1 , ##__VA_ARGS__)
#define WIDEN_CALL_2(func, str1, str2, ...) func(str1, str2 , ##__VA_ARGS__)
#endif

extern "C" {

FILE* android_fopen(const char* path, const char* mode) {
#if _MSC_VER
    Win32UnicodeString wmode(mode);
    auto normalized = PathUtils::recompose(PathUtils::decompose(path));
    Win32UnicodeString wpath(normalized);

    const wchar_t* wide_path = wpath.c_str();
    const wchar_t* wide_mode = wmode.c_str();

    FILE* res = NULL;
    int err = _wfopen_s(&res, wide_path, wide_mode);
    if (err != 0) {
        printf("Failed to open %s, err: %d\n", path, err);
    }
    return res;
#else
    return WIDEN_CALL_2(fopen, path, mode);
#endif
}

FILE* android_popen(const char* path, const char* mode) {
    return WIDEN_CALL_2(popen, path, mode);
}

int android_open_without_mode(const char* path, int flags) {
    int res = WIDEN_CALL_1(open, path, flags | O_CLOEXEC);
    android::base::fdSetCloexec(res);
    return res;
}

int android_open_with_mode(const char* path, int flags, mode_t mode) {
    int res = WIDEN_CALL_1(open, path, flags | O_CLOEXEC, mode);
    android::base::fdSetCloexec(res);
    return res;
}

#ifdef _WIN32
int android_stat(const char* path, struct _stati64* buf) {
  return _wstati64(Win32UnicodeString(path).c_str(),buf);
}
#else
int android_stat(const char* path, struct stat* buf) {
    return stat(path, buf);
}
#endif

#ifndef _WIN32
int android_lstat(const char* path, struct stat* buf) {
    return lstat(path, buf);
}
#endif

int android_access(const char* path, int mode) {
    return WIDEN_CALL_1(access, path, mode);
}

#ifdef _WIN32
// The Windows version does not have a mode parameter so just drop that name to
// avoid compiler warnings about unused parameters
int android_mkdir(const char* path, mode_t) {
    return _wmkdir(Win32UnicodeString(path).c_str());
}
#else
int android_mkdir(const char* path, mode_t mode) {
    return mkdir(path, mode);
}
#endif

int android_creat(const char* path, mode_t mode) {
#ifndef _WIN32
    return android_open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
#else
    int fd = -1;
    Win32UnicodeString unipath(path);
    // Be careful here! The security model in windows is very different.
    _wsopen_s(&fd, unipath.c_str(), _O_CREAT | _O_BINARY | _O_TRUNC | _O_WRONLY, _SH_DENYNO, _S_IWRITE);
    return fd;
#endif
}

int android_unlink(const char* path) {
    return WIDEN_CALL_1(unlink, path);
}

int android_chmod(const char* path, mode_t mode) {
    return WIDEN_CALL_1(chmod, path, mode);
}

int android_rmdir(const char* path) {
#ifdef _MSC_VER
   // Callers expect 0 on success, win api returns true on success.
   return !RemoveDirectoryW(Win32UnicodeString(path).c_str());
#else
    return WIDEN_CALL_1(rmdir, path);
#endif
}
// The code below uses the fact that GCC supports something called weak linking.
// Several functions in glibc are weakly linked which means that if the same
// function name is found in the application binary that function will be used
// instead. On Windows we use this to change these calls to call the wchar_t
// equivalent function with the parameter converted from UTF-8 to UTF-16.
//
// Unfortunately stat is not weakly linked which is why it is not listed here
// and any code that calls stat should use android_stat instead.
// TODO(joshuaduong): Looks like we can't use weak linking with MSVC. Either
// need to find another way to do this or rename all of these calls to
// android_*.
#if defined(_WIN32) && !defined(_MSC_VER)

// getcwd cannot use the same macro as the other calls because it places data
// in one of its parameters and it returns a char pointer, not a result code
char* __cdecl getcwd(char* buffer, int maxlen) {
    ScopedCPtr<wchar_t> wideCwd(_wgetcwd(nullptr, 0));
    if (wideCwd.get() == nullptr) {
        return nullptr;
    }
    if (buffer == nullptr) {
        // This is a valid use case and we need to allocate memory and return it
        auto narrowCwd = Win32UnicodeString::convertToUtf8(wideCwd.get());
        return strdup(narrowCwd.c_str());
    }

    int written = Win32UnicodeString::convertToUtf8(buffer,
                                                    maxlen,
                                                    wideCwd.get());
    if (written < 0 || written >= maxlen) {
        return nullptr;
    }
    return buffer;
}

int __cdecl remove(const char* path) {
    return WIDEN_CALL_1(remove, path);
}

int __cdecl rmdir(const char* dirname) {
    return WIDEN_CALL_1(rmdir, dirname);
}

int __cdecl chmod(const char* filename, int pmode) {
    return WIDEN_CALL_1(chmod, filename, pmode);
}

int __cdecl unlink(const char* filename) {
    return WIDEN_CALL_1(unlink, filename);
}

int __cdecl mkdir(const char* dirname) {
    return WIDEN_CALL_1(mkdir, dirname);
}

int __cdecl creat(const char* path, int mode) {
    return WIDEN_CALL_1(creat, path, mode);
}

int __cdecl access(const char* path, int mode) {
    return WIDEN_CALL_1(access, path, mode);
}

int __cdecl open(const char* pathname, int flags, ...) {
    va_list ap;

    // Since open comes in two versions and C does not support function
    // overloading we use varargs for the mode parameters instead.
    va_start(ap, flags);
    int result = WIDEN_CALL_1(open, pathname, flags, va_arg(ap, int));
    va_end(ap);
    return result;
}

FILE* __cdecl fopen(const char* path, const char* mode) {
    return WIDEN_CALL_2(fopen, path, mode);
}

#endif  // _WIN32 && !_MSC_VER

}  // extern "C"
