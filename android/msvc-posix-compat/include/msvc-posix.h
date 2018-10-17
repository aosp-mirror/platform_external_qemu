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

#pragma once

#include <direct.h>
#include <ehstorioctl.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <windows.h>
#include <winsock2.h>
//#include <basetsd.h>

// From <unistd.h>
typedef SSIZE_T ssize_t;
typedef int mode_t;
#ifdef _WIN64
typedef int64_t pid_t;
#else
typedef int pid_t;
#endif
#define STDIN_FILENO _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define STDERR_FILENO _fileno(stderr)
#define lseek(a, b, c) _lseek(a, c, b)
#define lseek64 _lseeki64

// Need <dirent.h>

// Define for convenience only in mingw. This is
// convenient for the _access function in Windows.
#define F_OK 0 /* Check for file existence */
#define X_OK 1 /* Check for execute permission (not supported in Windows) */
#define W_OK 2 /* Check for write permission */
#define R_OK 4 /* Check for read permission */

// From <sys/stat.h>
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
// These functions were deprecated and replaced with ISO C++ conformant ones
// in MSVC 2017.
/*
#define strdup _strdup
#define mkdir _mkdir
#define rmdir _rmdir
#define getcwd _getcwd
#define getpid _getpid
#define close _close
#define open _open
#define read _read
#define write _write
#define creat _creat
*/
#define fstat64 _fstat64

// From <fcntl.h>
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

// From <sys/types.h>
typedef int64_t off64_t;

// From <sys/cdefs.h>
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS /* empty */
#define __END_DECLS   /* empty */
#endif

// From <sys/time.h>
struct timezone {
    int tz_minuteswest; /* of Greenwich */
    int tz_dsttime;     /* type of dst correction to apply */
};

// From <strings.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

// From <stdio.h>
#define fseeko64 _fseeki64
#define ftello64 _ftelli64

// From <linux/limits.h>
#define PATH_MAX MAX_PATH

__BEGIN_DECLS

// <getopt.h>
extern int optind;   /* index of first non-option in argv      */
extern int optopt;   /* single option character, as parsed     */
extern int opterr;   /* flag to enable built-in diagnostics... */
                     /* (user may set to zero, to suppress)    */
extern char* optarg; /* pointer to argument of current option  */

extern int getopt(int nargc, char* const* nargv, const char* options);

struct option /* specification for a long form option...	*/
{
    const char* name; /* option name, without leading hyphens */
    int has_arg;      /* does it take an argument?		*/
    int* flag;        /* where to save its status, or NULL	*/
    int val;          /* its associated status value		*/
};

enum                 /* permitted values for its `has_arg' field...	*/
{ no_argument = 0,   /* option never takes an argument	*/
  required_argument, /* option always requires an argument	*/
  optional_argument  /* option may take an argument		*/
};

extern int getopt_long(int nargc,
                       char* const* nargv,
                       const char* options,
                       const struct option* long_options,
                       int* idx);
extern int getopt_long_only(int nargc,
                            char* const* nargv,
                            const char* options,
                            const struct option* long_options,
                            int* idx);
extern int gettimeofday(struct timeval* tp, struct timezone* tz);
extern int asprintf(char** buf, const char* format, ...);
extern int vasprintf(char** buf, const char* format, va_list args);
extern int mkstemp(char* t);

__END_DECLS
