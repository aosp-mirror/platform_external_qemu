// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "aemu/base/logging/LogFormatter.h"

#include <stdarg.h>     // for va_list, va_end, va_start
#include <stdint.h>     // for uint32_t
#include <stdio.h>      // for vsnprintf, snprintf
#include <string.h>     // for strncmp, strncpy
#include <string>       // for basic_string
#include <string_view>  // for string_view
#include <thread>       // for thread, thread::id
#include <type_traits>  // for remove_extent_t

#include "absl/strings/str_format.h"           // for StrFormat, ParsedFormat
#include "aemu/base/logging/LogSeverity.h"  // for LogSeverity
#ifdef _MSC_VER
#include "msvc-posix.h"
#include <windows.h>
#else
#include <sys/time.h>  // for gettimeofday, timeval
#include <time.h>      // for localtime, tm, time_t
#include <pthread.h>
#endif


namespace android {
namespace base {

// Loglines over 2kb are not supported, they will be cut off.
constexpr int MAX_LOG_LINE_LENGTH = 2048;

std::string LogFormatter::format(const LogParams& params,
                                 const char* fmt,
                                 ...) {
    va_list args;
    va_start(args, fmt);
    auto res = format(params, fmt, args);
    va_end(args);
    return res;
}

inline static const char* translate_sev(LogSeverity value) {
#define SEV(p, str)        \
    case (LogSeverity::p): \
        return str;        \
        break;

    switch (value) {
        SEV(EMULATOR_LOG_VERBOSE, "VERBOSE")
        SEV(EMULATOR_LOG_DEBUG, "DEBUG  ")
        SEV(EMULATOR_LOG_INFO, "INFO   ")
        SEV(EMULATOR_LOG_WARNING, "WARNING")
        SEV(EMULATOR_LOG_ERROR, "ERROR  ")
        SEV(EMULATOR_LOG_FATAL, "FATAL  ")
        default:
            return "UNKWOWN";
    }
#undef SEV
}

inline static const char* translate_color(LogSeverity value) {
#define SEV(p, col)           \
    case (LogSeverity::p):    \
        return col "\x1b[0m"; \
        break;

    switch (value) {
        SEV(EMULATOR_LOG_VERBOSE, "\x1b[94mVERBOSE")
        SEV(EMULATOR_LOG_DEBUG, "\x1b[36mDEBUG  ")
        SEV(EMULATOR_LOG_INFO, "\x1b[32mINFO   ")
        SEV(EMULATOR_LOG_WARNING, "\x1b[33mWARNING")
        SEV(EMULATOR_LOG_ERROR, "\x1b[31mERROR  ")
        SEV(EMULATOR_LOG_FATAL, "\x1b[35mFATAL  ")
        default:
            return "\x1b[94mUNKNOWN";
    }
#undef SEV
}

std::string SimpleLogFormatter::format(const LogParams& params,
                                       const char* fmt,
                                       va_list args) {
    char logline[MAX_LOG_LINE_LENGTH] = {0};
    char* log = logline;
    int cLog = sizeof(logline);
    int w = 0;

#ifdef PRINT_WITH_COLOR
    // Warning! This looks cool and all, but likely has all sorts
    // of unexpected side effects. Handy during debugging of prints though.
    w = snprintf(log, cLog, "%s | ", translate_color(params.severity));
#else
    w = snprintf(log, cLog, "%s | ", translate_sev(params.severity));
#endif
    if (w > 0 && w < cLog) {
        log += w;
        cLog -= w;
    }

    vsnprintf(log, cLog, fmt, args);
    return logline;
};

std::string SimpleLogWithTimeFormatter::format(const LogParams& params,
                                               const char* fmt,
                                               va_list args) {
    char logline[MAX_LOG_LINE_LENGTH] = {0};
    char* log = logline;
    int cLog = sizeof(logline);
    int w = 0;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    time_t now = tv.tv_sec;
    struct tm* time = localtime(&now);
    w = snprintf(log, cLog, "%02d:%02d:%02d.%06ld %s | ", time->tm_hour,
                 time->tm_min, time->tm_sec, tv.tv_usec,
                 translate_sev(params.severity));

    if (w > 0 && w < cLog) {
        log += w;
        cLog -= w;
    }

    vsnprintf(log, cLog, fmt, args);
    return logline;
};

#ifndef _WIN32
constexpr char PATH_SEP = '/';
uint64_t getThreadId() {
    pthread_t tid = pthread_self();
    // POSIX doesn't require pthread_t to be a numeric type.
    // Instead, just pick up the first sizeof(long) bytes as the "id".
    static_assert(sizeof(tid) >= sizeof(uint64_t),
                  "Expected pthread_t to be at least sizeof(long) wide");
    return *reinterpret_cast<uint64_t*>(&tid);
}
#else
constexpr char PATH_SEP = '\\';
uint64_t getThreadId() {
        return static_cast<uint64_t>(GetCurrentThreadId());
}
#endif

std::string VerboseLogFormatter::format(const LogParams& params,
                                        const char* fmt,
                                        va_list args) {
    char logline[MAX_LOG_LINE_LENGTH] = {0};
    char* log = logline;
    int cLog = sizeof(logline);
    int w = 0;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    time_t now = tv.tv_sec;
    struct tm* time = localtime(&now);

    // Get the basename of the file.
    std::string_view path = params.file;
    auto loc = path.rfind('/');
    auto filename = path.substr(loc + 1);

    char readable_location[35];
    snprintf(readable_location, sizeof(readable_location), "%s:%d",
             filename.data(), params.lineno);

    auto thread = getThreadId();
    w = snprintf(log, cLog, "%02d:%02d:%02d.%06d %-15lu %s %-34s | ",
                 time->tm_hour, time->tm_min, time->tm_sec,
                 static_cast<uint32_t>(tv.tv_usec), thread,
                 translate_sev(params.severity), readable_location);
    if (w > 0 && w < cLog) {
        log += w;
        cLog -= w;
    }

    vsnprintf(log, cLog, fmt, args);
    return logline;
};

NoDuplicateLinesFormatter::NoDuplicateLinesFormatter(
        std::shared_ptr<LogFormatter> logger)
    : mInner(logger) {}

std::string NoDuplicateLinesFormatter::format(const LogParams& params,
                                              const char* fmt,
                                              va_list args) {
    // We really care about order here, so we have to lock..
    // otherwise we can get really bizarre things happen when multiple
    // threads are active.
    const std::lock_guard<std::mutex> lock(mFormatMutex);
    char buffer[MAX_LOG_LINE_LENGTH] = {0};
    char* logline = mPrevLog;

    // We might have a duplicate, let's format the line and make sure.
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (mPrevParams == params &&
        strncmp(buffer, mPrevLog, sizeof(buffer)) == 0) {
        // Yep! it is, just return an empty string and discard this one.
        mDuplicates++;

        return kEmpty;
    }

    // Nope it is not a duplicate.
    logline = buffer;
    auto duplicates = mDuplicates;
    mDuplicates = 0;

    std::string result;

    switch (duplicates) {
        case 0:
            // Not a duplicate, let the inner formatter decorate the message
            // properly.
            result = mInner->format(params, logline);
            break;
        case 1:
            // No need to include a counter, just log the double line
            static const auto sgl_format_string =
                    absl::ParsedFormat<'s', 's'>("%s\n%s");
            result = absl::StrFormat(sgl_format_string,
                                     mInner->format(mPrevParams, mPrevLog),
                                     mInner->format(params, logline));
            break;
        default:
            static const auto dbl_format_string =
                    absl::ParsedFormat<'s', 'd', 's'>("%s (%dx)\n%s");
            // include a counter, we have double logged at least 2 lines.
            result = absl::StrFormat(
                    dbl_format_string, mInner->format(mPrevParams, mPrevLog),
                    duplicates, mInner->format(params, logline));
            break;
    }

    // Handle the case where we have matching logparams, but a mismatch in
    // formatted strings.
    if (logline != mPrevLog) {
        strncpy(mPrevLog, buffer, sizeof(mPrevLog));
    }
    mPrevParams = params;

    return result;
};

}  // namespace base
}  // namespace android
