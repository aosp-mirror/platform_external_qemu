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

#include "absl/base/log_severity.h"
#include "absl/log/internal/log_message.h"
#include "absl/time/time.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/logging/Log.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/crashreport/AnnotationStreambuf.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <string>
#ifndef _MSC_VER
#include <sys/time.h>
#else
#include "msvc-posix.h"
#endif
#include <vector>

// C interface
void android_init_opengl_logger() {}

AndroidOpenglLoggerFlags sLoggingFlags = OPENGL_LOGGER_NONE;

inline bool is_fine_logging_enabled(AndroidOpenglLoggerFlags flags) {
    return (flags & OPENGL_LOGGER_DO_FINE_LOGGING) ==
           OPENGL_LOGGER_DO_FINE_LOGGING;
}

void android_opengl_logger_set_flags(AndroidOpenglLoggerFlags flags) {
    sLoggingFlags = flags;
    if (is_fine_logging_enabled(sLoggingFlags)) {
        LOG(INFO) << "Verbose logging for OpenGL enabled";
    }
}

static std::ostream& crashstream() {
    static android::crashreport::DefaultAnnotationStreambuf
            crashReportStreamBuf{"gl_log"};
    static std::ostream crashReportStream{&crashReportStreamBuf};
    return crashReportStream;
}

void android_opengl_logger_write(char severity,
                                 const char* file,
                                 unsigned int line,
                                 int64_t timestamp_us,
                                 const char* message) {
    absl::LogSeverity sev;
    switch (severity) {
        case 'V':
        case 'D':
            // Not logging details if it is not requested
            if (!is_fine_logging_enabled(sLoggingFlags)) {
                return;
            }
        case 'I':
            sev = absl::LogSeverity::kInfo;
            break;
        case 'W':
            sev = absl::LogSeverity::kWarning;
            break;
        case 'E':
            sev = absl::LogSeverity::kError;
            break;
        case 'F':
            sev = absl::LogSeverity::kFatal;
            break;
    }

    absl::Time converted_time = absl::FromUnixMicros(timestamp_us);
    absl::log_internal::LogMessage(file, line, sev)
                    .WithTimestamp(converted_time)
            << message;
    crashstream() << message << "\n";
}

void android_stop_opengl_logger() {}
