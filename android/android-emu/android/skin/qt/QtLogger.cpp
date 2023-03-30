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

#include <stdarg.h>                            // for va_end, va_list, va_start
#include <stdio.h>                             // for vsnprintf
#include <wchar.h>

#include "android/base/memory/LazyInstance.h"  // for LazyInstance, LAZY_INS...
#include "android/utils/debug.h"               // for dinfo


using android::base::LazyInstance;
static LazyInstance<QtLogger> sLogger = LAZY_INSTANCE_INIT;

static const int kBufferLen = 2048;

// static
QtLogger* QtLogger::get() {
    return sLogger.ptr();
}


void QtLogger::write(const char* fmt, ...) {
    wchar_t buf[kBufferLen] = {};
    std::vector<wchar_t> wfmt(strlen(fmt) + 1);
    mbstowcs(wfmt.data(), fmt, wfmt.size());
    va_list ap;
    va_start(ap, fmt);
    vswprintf(buf, sizeof(buf) - 1, wfmt.data(), ap);
    va_end(ap);

    dinfo("%S", buf);
    mFileHandle << buf << std::endl;
}
