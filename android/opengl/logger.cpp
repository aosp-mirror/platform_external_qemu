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
#include "android/crashreport/CrashReporter.h"
#include "android/opengl/logger.h"

#include <fstream>
#include <stdarg.h>
#include <stdio.h>
#include <string>

using android::base::PathUtils;
using android::crashreport::CrashReporter;

// The purpose of the OpenGL logger is to log
// information about such things as EGL initialization
// and possibly miscellanous OpenGL commands,
// in order to get a better idea of what is going on
// in crash reports.

// The OpenGLLogger implementation's initialization method
// by default uses the crash reporter's data directory.

class OpenGLLogger {
public:
    OpenGLLogger();
    OpenGLLogger(const char* filename);
    void log(const char* str);
private:
    std::string mFileName;
    std::ofstream mFileHandle;
    DISALLOW_COPY_ASSIGN_AND_MOVE(OpenGLLogger);
};

OpenGLLogger::OpenGLLogger() {
    const std::string& data_dir =
        CrashReporter::get()->getDataExchangeDir();
    mFileName = std::string(
                    PathUtils::join(data_dir, "opengl_log.txt").c_str());
    mFileHandle.open(mFileName, std::ios::app);
}

OpenGLLogger::OpenGLLogger(const char* filename) :
    mFileName(filename) {
    mFileHandle.open(mFileName, std::ios::app);
}

void OpenGLLogger::log(const char* str) {
    if (mFileHandle) {
        mFileHandle << str;
        mFileHandle << std::endl;
    }
}

// C interface

static OpenGLLogger* s_opengl_logger;
static const int kBufferLen = 2048;

void android_init_opengl_logger() {
    s_opengl_logger = new OpenGLLogger();
}

void android_opengl_logger_write(const char* fmt, ...) {
    char buf[kBufferLen] = {};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    s_opengl_logger->log(buf);
}

void android_stop_opengl_logger() {
    delete s_opengl_logger;
}

