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

#include <string.h>
#include <chrono>
#include <cinttypes>
#include <cstdarg>
#include <cstring>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>

#ifdef __linux__
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "absl/strings/str_format.h"
#include "aemu/base/logging/LogSeverity.h"
#ifdef _MSC_VER
#include <windows.h>
#include "msvc-posix.h"
#else
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#endif

namespace android {
namespace base {

inline static const std::string_view translate_sev(LogSeverity value) {
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
                                       const std::string& line) {
    return absl::StrFormat("%s | %s", translate_sev(params.severity), line);
};

std::string SimpleLogWithTimeFormatter::format(const LogParams& params,
                                               const std::string& line) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    time_t now = tv.tv_sec;
    struct tm* time = localtime(&now);
    return absl::StrFormat("%02d:%02d:%02d.%06ld %s | %s", time->tm_hour,
                           time->tm_min, time->tm_sec, tv.tv_usec,
                           translate_sev(params.severity), line);
};

#ifndef _WIN32
constexpr char PATH_SEP = '/';
#else
constexpr char PATH_SEP = '\\';
#endif

constexpr int kMaxThreadIdLength =
        7;  // 7 digits for the thread id is what Google uses everywhere.

// Returns the current thread id as a string of at most kMaxThreadIdLength
// characters. We try to avoid using std::this_thread::get_id() because on Linux
// at least it returns a long number (e.g. 139853607339840) which isn't the same
// as the thread id from the OS itself. The current logic is inspired by:
// https://github.com/abseil/abseil-cpp/blob/52d41a9ec23e39db7e2cbce5c9449506cf2d3a5c/absl/base/internal/sysinfo.cc#L367-L381
static std::string getStrThreadID() {
    std::ostringstream ss;
#if defined(_WIN32)
    ss << GetCurrentThreadId();
#elif defined(__linux__)
    ss << syscall(__NR_gettid);
#else
    ss << std::this_thread::get_id();
#endif
    std::string result = ss.str();
    // Truncate on the left if necessary
    return result.length() > kMaxThreadIdLength
                   ? result.substr(result.length() - kMaxThreadIdLength)
                   : result;
}

// Caches the thread id in thread local storage to increase performance
// Inspired by:
// https://github.com/abseil/abseil-cpp/blob/52d41a9ec23e39db7e2cbce5c9449506cf2d3a5c/absl/base/internal/sysinfo.cc#L494-L504
const char* getCachedThreadID() {
    static thread_local std::string thread_id = getStrThreadID();
    return thread_id.c_str();
}

std::string GoogleLogFormatter::format(const LogParams& params,
                                       const std::string& line) {
    auto timestamp_us =
            std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
    std::time_t timestamp_s = timestamp_us / 1000000;

    // Break down the timestamp into the individual time parts
    std::tm ts_parts = {};
#if defined(_WIN32)
    localtime_s(&ts_parts, &timestamp_s);
#else
    localtime_r(&timestamp_s, &ts_parts);
#endif

    // Get the microseconds part of the timestamp since it's not available in
    // the tm struct
    int64_t microseconds = timestamp_us % 1000000;

    // Get the basename of the file.
    std::string_view path = params.file;
    auto loc = path.rfind(PATH_SEP);
    auto filename = path.substr(loc + 1);

    // Output the standard Google logging prefix
    // See also:
    // https://github.com/google/glog/blob/9dc1107f88d3a1613d61b80040d83c1c1acbac3d/src/logging.cc#L1612-L1615
    return absl::StrFormat(
            "%c%02d%02d %02d:%02d:%02d.%06" PRId64 " %7s %s:%d] %s",
            translate_sev(params.severity)[0], ts_parts.tm_mon + 1,
            ts_parts.tm_mday, ts_parts.tm_hour, ts_parts.tm_min,
            ts_parts.tm_sec, microseconds, getCachedThreadID(), filename,
            params.lineno, line);
}

std::string VerboseLogFormatter::format(const LogParams& params,
                                        const std::string& line) {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    time_t now = tv.tv_sec;
    struct tm* time = localtime(&now);

    // Get the basename of the file.
    std::string_view path = params.file;
    auto loc = path.rfind(PATH_SEP);
    auto filename = path.substr(loc + 1);
    auto thread = getCachedThreadID();

    auto readable_location = absl::StrFormat("%s:%d", filename, params.lineno);
    auto logline = absl::StrFormat(
            "%02d:%02d:%02d.%06d %s %s %-34s | %s", time->tm_hour, time->tm_min,
            time->tm_sec, static_cast<uint32_t>(tv.tv_usec), thread,
            translate_sev(params.severity), readable_location, line);
    return logline;
};

NoDuplicateLinesFormatter::NoDuplicateLinesFormatter(
        std::shared_ptr<LogFormatter> logger)
    : mInner(logger) {}

std::string NoDuplicateLinesFormatter::format(const LogParams& params,
                                              const std::string& line) {
    // We really care about order here, so we have to lock..
    // otherwise we can get really bizarre things happen when multiple
    // threads are active.
    const std::lock_guard<std::mutex> lock(mFormatMutex);
    if (mPrevParams == params && mPrevLogLine == line) {
        // Yep! it is, just return an empty string and discard this one.
        mDuplicates++;

        return kEmpty;
    }

    // Nope it is not a duplicate
    auto duplicates = mDuplicates;
    mDuplicates = 0;

    std::string result;

    switch (duplicates) {
        case 0:
            // Not a duplicate, let the inner formatter decorate the message
            // properly.
            result = mInner->format(params, line);
            break;
        case 1:
            // No need to include a counter, just log the double line
            static const auto sgl_format_string =
                    absl::ParsedFormat<'s', 's'>("%s\n%s");
            result = absl::StrFormat(sgl_format_string,
                                     mInner->format(mPrevParams, mPrevLogLine),
                                     mInner->format(params, line));
            break;
        default:
            static const auto dbl_format_string =
                    absl::ParsedFormat<'s', 'd', 's'>("%s (%dx)\n%s");
            // include a counter, we have double logged at least 2 lines.
            result = absl::StrFormat(dbl_format_string,
                                     mInner->format(mPrevParams, mPrevLogLine),
                                     duplicates, mInner->format(params, line));
            break;
    }

    // Handle the case where we have matching logparams, but a mismatch in
    // formatted strings.
    if (line != mPrevLogLine) {
        mPrevLogLine = line;
    }
    mPrevParams = params;

    return result;
};

}  // namespace base
}  // namespace android
