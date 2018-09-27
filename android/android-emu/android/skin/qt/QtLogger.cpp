// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/skin/qt/QtLogger.h"

#include "android/base/Log.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/crashreport/CrashReporter.h"

using android::base::LazyInstance;
using android::crashreport::CrashReporter;
using android::base::pj;

static LazyInstance<QtLogger> sLogger = LAZY_INSTANCE_INIT;

static const int kBufferLen = 2048;

// static
QtLogger* QtLogger::get() {
    return sLogger.ptr();
}

// static
void QtLogger::stop() {
    if (sLogger.hasInstance()) {
        sLogger->closeLog();
    }
}

QtLogger::QtLogger() :
    mLogFilePath(CrashReporter::get()->getDataExchangeDir()) {

    mFileHandle.open(pj(mLogFilePath, "qt_log.txt"),
                     std::ios::app);
}

QtLogger::~QtLogger() {
    stop();
}

void QtLogger::closeLog() {
    if (mFileHandle) {
        mFileHandle.close();
    }
}

void QtLogger::write(const char* fmt, ...) {
    char buf[kBufferLen] = {};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    LOG(INFO) << buf << "\n";

    if (!mFileHandle) return;

    mFileHandle << buf << std::endl;
}
