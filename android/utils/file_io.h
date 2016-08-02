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

#pragma once

#include "android/utils/compiler.h"

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

ANDROID_BEGIN_HEADER

FILE* android_fopen(const char* path, const char* mode);
FILE* android_popen(const char* path, const char* mode);

// This thing uses macro varargs to select which open function to use.
// Because __VA_ARGS__ consumes a different amount of arguments the call_name
// will have a different name depending on how many arguments are passed
#define DETERMINE_OPEN_CALL(first, second, third, call_name, ...) call_name
#define android_open(...) DETERMINE_OPEN_CALL(__VA_ARGS__, \
                                              android_open_with_mode, \
                                              android_open_without_mode) \
                                                  (__VA_ARGS__)

// WARNING: Do not use these, just use the android_open macro above the way
// you would use open. The version to use gets picked automatically
int android_open_without_mode(const char* path, int flags);
int android_open_with_mode(const char* path, int flags, mode_t mode);

int android_stat(const char* path, struct stat* buf);
int android_lstat(const char* path, struct stat* buf);

#if defined(_WIN32)
// We cannot override stat() for proper Win32 Unicode path support because
// it is not a weak symbol, so use a macro trick instead. This is very subtle
// though, so here's a little technical explanation:
//
// The Mingw headers do the following declaration (in order):
//
// - First of all, off_t is normally defined as 'long', which is 32-bit
//   on Win32, and 64-bit on Win64, but this can be overriden by defining
//   _FILE_OFFSET_BITS to 64 on the command-line, or before inclusion of
//   Mingw system headers to force it to 64-bit on Win32.
//
// - time_t is normally 64-bit on Win32, except if _USE_32BIT_TIME_T is
//   defined at compile time.
//
// - <sys/stat.h> declares various stat-related structures:
//
//     - 'struct _stat32': contains off_t file size and 32-bit times.
//     - 'struct _stat64': contains 64-bit file size and 64-bit times.
//     - 'struct stat': contains off_t file size and time_t times.
//     - 'struct _stat32i64': contains 64-bit file size and 32-bit times.
//     - 'struct _stat64i32': contains off_t file size and 64-bit times.
//
// - It declares a stat() function that takes a 'struct stat' as its second
//   parameter. Also for _stat64()/struct _stat64, and
//   _stat64i32()/struct _stat64i32, etc.
//
// - If _FILE_OFFSET_BITS is set to 64, if defines 'stat' as a macro to
//   _stat32i64 (if _USE_32BIT_TIME_T is defined) or _stat64.
//   Similarly, 'fstat' is defined as a macro to _fstat32i64 or _fstat64
//   under the same condition.
//
#if !defined(_FILE_OFFSET_BITS) || _FILE_OFFSET_BITS != 64
#error "_FILE_OFFSET_BITS=64 is required when building for Android!"
#endif
#ifndef stat
#error "stat should be defined as a macro here!"
#endif
#ifdef _USE_32BIT_TIME_T
// 'stat' defined as '_stati64', and 'fstat' defined as '_fstati64'
// The following ensures that 'struct stat' will expand to 'struct _stati64'
// and stat(...) will expand to android_stat(...).
#define _stati64(path, buf)  android_stat(path, buf)
#else  // !_USE_32BIT_TIME_T
// 'stat' defined as '_stat64' and 'fstat' as '_fstat64'
// The following ensures that 'struct stat' will expand to 'struct _stat64'
// and stat(...) will expand to android_stat(...).
#define _stat64(path, buf) android_stat(path, buf)
#endif  // !_USE_32BIT_TIME_T

// popen() and opendir() are a lot simpler, fortunately :)
#undef popen
#define popen(command, type) android_popen(command, type)

// opendir() is a special case, because the Unicode version of the function
// on Windows returns a _WDIR*, not a DIR*, so provide our own wrapper
// type for it.
typedef struct {
    _WDIR* wdir;           // as returned by _wopendir().
    struct dirent dirent;  // converted from the result of _wreaddir().
} ANDROID_DIR;

ANDROID_DIR* android_opendir(const char* name);
struct dirent* android_readdir(ANDROID_DIR* dir);
int android_closedir(ANDROID_DIR* dir);
#define opendir(path) android_opendir(path)
#define readdir(dir) android_readdir(dir)
#define closedir(dir) android_closedir(dir)
#define DIR ANDROID_DIR

#endif  // _WIN32

int android_access(const char* path, int mode);
int android_mkdir(const char* path, mode_t mode);

int android_creat(const char* path, mode_t mode);

int android_unlink(const char* path);
int android_chmod(const char* path, mode_t mode);

ANDROID_END_HEADER

