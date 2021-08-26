/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/utils/debug.h"

#include <fcntl.h>                 // for open, O_WRONLY
#include <stdint.h>                // for uint64_t
#include <stdio.h>                 // for fileno, fprintf, printf, stdout

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#include <time.h>                  // for localtime, tm, time_t
#include <unistd.h>                // for dup2, close, dup
#endif


// TODO(jansene): Some external libraries (nibmle) still rely on these, so we cannot remove them yet.
#undef dprint
#undef dinfo
#undef derror
#undef dwarning

uint64_t android_verbose = 0;
LogSeverity android_log_severity = EMULATOR_LOG_INFO;

void dprint(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fdprintfnv(stdout, 0, format, args);
    va_end(args);
}

void fdprintf(FILE* fp, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fdprintfnv(fp, 0, format, args);
    va_end(args);
}

void fdprintfnv(FILE* fp, const char* lvl, const char* format, va_list args) {
    if (VERBOSE_CHECK(time)) {
        struct timeval tv;
        gettimeofday(&tv, 0);
        time_t now = tv.tv_sec;
        struct tm* time = localtime(&now);
        fprintf(fp, "%02d:%02d:%02d.%05ld ", time->tm_hour, time->tm_min,
                time->tm_sec, tv.tv_usec);
    }
    fprintf(fp, "emulator: ");
    if (lvl) {
        fprintf(fp, "%s", lvl);
    }
    vfprintf(fp, format, args);
    fprintf(fp, "\n");
}

void dprintn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

void dprintnv(const char* format, va_list args) {
    vfprintf(stdout, format, args);
}
void dinfo(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fdprintfnv(stdout, "INFO: ", format, args);
    va_end(args);
}

void dwarning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fdprintfnv(stdout, "WARNING: ", format, args);
    va_end(args);
}

void derror(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fdprintfnv(stdout, "ERROR: ", format, args);
    va_end(args);
}


/** STDOUT/STDERR REDIRECTION
 **
 ** allows you to shut temporarily shutdown stdout/stderr
 ** this is useful to get rid of debug messages from ALSA and esd
 ** on Linux.
 **/
static int stdio_disable_count;
static int stdio_save_out_fd;
static int stdio_save_err_fd;

#ifdef _WIN32
extern void stdio_disable(void) {
    if (++stdio_disable_count == 1) {
        int null_fd, out_fd, err_fd;
        fflush(stdout);
        out_fd = _fileno(stdout);
        err_fd = _fileno(stderr);
        stdio_save_out_fd = _dup(out_fd);
        stdio_save_err_fd = _dup(err_fd);
        null_fd = _open("NUL", _O_WRONLY);
        _dup2(null_fd, out_fd);
        _dup2(null_fd, err_fd);
        close(null_fd);
    }
}

extern void stdio_enable(void) {
    if (--stdio_disable_count == 0) {
        int out_fd, err_fd;
        fflush(stdout);
        out_fd = _fileno(stdout);
        err_fd = _fileno(stderr);
        _dup2(stdio_save_out_fd, out_fd);
        _dup2(stdio_save_err_fd, err_fd);
        _close(stdio_save_out_fd);
        _close(stdio_save_err_fd);
    }
}
#else
extern void stdio_disable(void) {
    if (++stdio_disable_count == 1) {
        int null_fd, out_fd, err_fd;
        fflush(stdout);
        out_fd = fileno(stdout);
        err_fd = fileno(stderr);
        stdio_save_out_fd = dup(out_fd);
        stdio_save_err_fd = dup(err_fd);
        null_fd = open("/dev/null", O_WRONLY);
        dup2(null_fd, out_fd);
        dup2(null_fd, err_fd);
        close(null_fd);
    }
}

extern void stdio_enable(void) {
    if (--stdio_disable_count == 0) {
        int out_fd, err_fd;
        fflush(stdout);
        out_fd = fileno(stdout);
        err_fd = fileno(stderr);
        dup2(stdio_save_out_fd, out_fd);
        dup2(stdio_save_err_fd, err_fd);
        close(stdio_save_out_fd);
        close(stdio_save_err_fd);
    }
}
#endif
