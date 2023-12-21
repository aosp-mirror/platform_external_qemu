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

#include "android/opengl/logger.h"

#include "aemu/base/logging/Log.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/crashreport/AnnotationStreambuf.h"

#include <algorithm>
#include <fstream>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#ifndef _MSC_VER
#include <sys/time.h>
#else
#include "msvc-posix.h"
#endif
#include <vector>

using android::base::AutoLock;
using android::base::Lock;
using android::base::PathUtils;

// The purpose of the OpenGL logger is to log
// information about such things as EGL initialization
// and possibly miscellanous OpenGL commands,
// in order to get a better idea of what is going on
// in crash reports.

// The OpenGLLogger implementation's initialization method
// by default uses the crash reporter's data directory.

static const int kBufferLen = 2048;

typedef std::pair<uint64_t, std::string> TimestampedLogEntry;

class OpenGLLogger {
public:
    OpenGLLogger() = default;
    void stop();

    // Coarse log: Call this infrequently.
    void writeCoarse(const char* str);

    // Fine log: When we want to log very frequent events.
    // Fine logs can be toggled on/off.
    void writeFineTimestamped(const char* str);

    void setLoggerFlags(AndroidOpenglLoggerFlags flags);
    bool isFineLogging() const;

    static OpenGLLogger* get();

private:

    void writeFineLocked(uint64_t time, const char* str);

    Lock mLock;
    AndroidOpenglLoggerFlags mLoggerFlags = OPENGL_LOGGER_NONE;
    uint64_t mPrevTimeUs = 0;
    // A preallocated crash report buffer, 8k size.
    // The report will be truncated when exceeding maximum size.
    android::crashreport::DefaultAnnotationStreambuf mCrashReportStreamBuf { "gl_log" };
    std::ostream mCrashReportStream { &mCrashReportStreamBuf };
    DISALLOW_COPY_ASSIGN_AND_MOVE(OpenGLLogger);
};

::android::base::LazyInstance<OpenGLLogger> sOpenGLLogger = LAZY_INSTANCE_INIT;

OpenGLLogger* OpenGLLogger::get() {
    return sOpenGLLogger.ptr();
}

void OpenGLLogger::writeCoarse(const char* str) {
    AutoLock lock(mLock);
    if (mLoggerFlags & OPENGL_LOGGER_PRINT_TO_STDOUT) {
        printf("%s", str);
    }
    mCrashReportStream << str;
}

void OpenGLLogger::writeFineLocked(uint64_t time, const char* str) {
    if (mLoggerFlags & OPENGL_LOGGER_PRINT_TO_STDOUT) {
        printf("%s", str);
    }
}

void OpenGLLogger::writeFineTimestamped(const char* str) {
    if (mLoggerFlags & OPENGL_LOGGER_DO_FINE_LOGGING) {
        char buf[kBufferLen] = {};
        struct timeval tv;
        gettimeofday(&tv, NULL);

        uint64_t curr_micros = (tv.tv_usec) % 1000;
        uint64_t curr_millis = (tv.tv_usec / 1000) % 1000;
        uint64_t curr_secs = tv.tv_sec;
        uint64_t curr_us = tv.tv_sec * 1000000ULL + tv.tv_usec;
        snprintf(buf, sizeof(buf) - 1,
                "time_us="
                "%" PRIu64 " s "
                "%" PRIu64 " ms "
                "%" PRIu64 " us deltaUs "
                "%" PRIu64 " | %s",
                curr_secs,
                curr_millis,
                curr_micros,
                curr_us - mPrevTimeUs,
                str);
        AutoLock lock(mLock);
        writeFineLocked(curr_micros + 1000ULL * curr_millis +
                  1000ULL * 1000ULL * curr_secs, buf);
        mPrevTimeUs = curr_us;
    }
}

void OpenGLLogger::setLoggerFlags(AndroidOpenglLoggerFlags flags) {
    AutoLock lock(mLock);
    mLoggerFlags = flags;
}

bool OpenGLLogger::isFineLogging() const {
    // For speed, we'll just let this read of mLoggerFlags race.
    return (mLoggerFlags & OPENGL_LOGGER_DO_FINE_LOGGING);
}

// C interface

void android_init_opengl_logger() {
    OpenGLLogger* gl_log = OpenGLLogger::get();
}

void android_opengl_logger_set_flags(AndroidOpenglLoggerFlags flags) {
    OpenGLLogger::get()->setLoggerFlags(flags);
}

void android_opengl_logger_write(const char* fmt, ...) {
    char buf[kBufferLen] = {};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    OpenGLLogger::get()->writeCoarse(buf);
}

void android_opengl_cxt_logger_write(const char* fmt, ...) {
    auto gl_log = OpenGLLogger::get();

    if (!gl_log->isFineLogging()) return;

    char buf[kBufferLen] = {};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    gl_log->writeFineTimestamped(buf);
}

void android_stop_opengl_logger() { }
