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

#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/crashreport/CrashReporter.h"
#include "android/opengl/logger.h"

#include <fstream>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <sys/time.h>
#include <vector>

using android::base::PathUtils;
using android::base::String;
using android::crashreport::CrashReporter;

// The purpose of the OpenGL logger is to log
// information about such things as EGL initialization
// and possibly miscellanous OpenGL commands,
// in order to get a better idea of what is going on
// in crash reports.

// The OpenGLLogger implementation's initialization method
// by default uses the crash reporter's data directory.

static const int kBufferLen = 2048;

class OpenGLLogger {
public:
    OpenGLLogger();
    OpenGLLogger(const char* filename);
    // Coarse log: Call this infrequently.
    void writeCoarse(const char* str);
    // Fine log: When we want to log very frequent events.
    // Fine logs can be toggled on/off.
    void writeFine(const char* str);
    void writeFineTimestamped(const char* str);
    void startFineLog();
    void stopFineLog();
    static OpenGLLogger* get();
private:
    bool mFineLogActive;
    std::string mFileName;
    std::ofstream mFileHandle;
    std::string mFineLogFileName;
    std::ofstream mFineLogFileHandle;
    std::vector<std::string> mFineLog;
    DISALLOW_COPY_ASSIGN_AND_MOVE(OpenGLLogger);
};


::android::base::LazyInstance<OpenGLLogger> sOpenGLLogger = LAZY_INSTANCE_INIT;

OpenGLLogger* OpenGLLogger::get() {
    return sOpenGLLogger.ptr();
}

OpenGLLogger::OpenGLLogger() :
    mFineLogActive(false) {
    const std::string& data_dir =
        CrashReporter::get()->getDataExchangeDir();
    mFileName = std::string(PathUtils::join(String(data_dir),
                                "opengl_log.txt").c_str());
    mFileHandle.open(mFileName, std::ios::app);
    mFineLogFileName = std::string(PathUtils::join(String(data_dir),
                                       "opengl_cxt_log.txt").c_str());
    mFineLogFileHandle.open(mFineLogFileName, std::ios::app);
}

OpenGLLogger::OpenGLLogger(const char* filename) :
    mFileName(filename) {
    mFileHandle.open(mFileName, std::ios::app);
}

void OpenGLLogger::writeCoarse(const char* str) {
    if (mFileHandle) {
        mFileHandle << str;
        mFileHandle << std::endl;
    }
}

void OpenGLLogger::writeFine(const char* str) {
    if (mFineLogActive) {
        mFineLog.push_back(std::string(str));
    }
}

void OpenGLLogger::writeFineTimestamped(const char* str) {
    if (mFineLogActive) {
        char buf[kBufferLen] = {};
        struct timeval tv;
        gettimeofday(&tv, NULL);

        uint64_t curr_micros = (tv.tv_usec) % 1000;
        uint64_t curr_millis = (tv.tv_usec / 1000) % 1000;
        uint64_t curr_secs = tv.tv_sec;
        snprintf(buf, sizeof(buf) - 1,
                "time_us="
                "%" PRIu64 " s "
                "%" PRIu64 " ms "
                "%" PRIu64 " us | %s",
                curr_secs,
                curr_millis,
                curr_micros,
                str);
        writeFine(buf);
    }
}

void OpenGLLogger::startFineLog() {
    mFineLogActive = true;
}

void OpenGLLogger::stopFineLog() {
    mFineLogActive = false;
    // Only print message when fine-grained
    // logging is turned on.
    if (!mFineLog.empty()) {
        fprintf(stderr,
                "Writing fine-grained GL log to %s...",
                mFineLogFileName.c_str());
    }
    for (const auto& entry : mFineLog) {
        // The fine log does not print newlines
        // as it is used with the opengl debug
        // printout in emugl, which adds
        // newlines of its own.
        mFineLogFileHandle << entry;
    }
    mFineLogFileHandle.close();
    if (!mFineLog.empty()) {
        fprintf(stderr, "done\n");
    }
    mFineLog.clear();
}

// C interface

void android_init_opengl_logger() {
    OpenGLLogger* gl_log = OpenGLLogger::get();
    gl_log->startFineLog();
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
    char buf[kBufferLen] = {};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    OpenGLLogger::get()->writeFineTimestamped(buf);
}

void android_stop_opengl_logger() {
    OpenGLLogger* gl_log = OpenGLLogger::get();
    gl_log->stopFineLog();
}

