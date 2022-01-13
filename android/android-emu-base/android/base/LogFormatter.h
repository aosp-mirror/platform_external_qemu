
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
#include <stdarg.h>  // for va_list
#include <stddef.h>  // for size_t
#include <mutex>     // for mutex
#include <string>    // for string
#include <memory>

#include "android/base/Log.h"  // for LogParams

namespace android {
namespace base {

// A LogFormatter formats a log line.
class LogFormatter {
public:
    virtual ~LogFormatter() = default;

    // Mainly to facilitate unit testing..
    std::string format(const LogParams& params, const char* fmt, ...);

    // Formats the given line, returning the string that should be printed, or
    // empty in case nothing should be printed.
    //
    // The last line should not be terminated by a newline.
    virtual std::string format(const LogParams& params,
                               const char* fmt,
                               va_list args) = 0;
};

// This simply logs the level, and message according to the following regex:
// ^(VERBOSE|DEBUG|INFO|WARNING|ERROR|FATAL|UNKWOWN)\s+\| (.*)
class SimpleLogFormatter : public LogFormatter {
public:
    std::string format(const LogParams& params,
                       const char* fmt,
                       va_list args) override;
};

// This simply logs the time, level and message according to the following
// regex:
// ^(\d+:\d+:\d+\.\d+)\s+(VERBOSE|DEBUG|INFO|WARNING|ERROR|FATAL|UNKWOWN)\s+\|
// (.*)
class SimpleLogWithTimeFormatter : public LogFormatter {
public:
    std::string format(const LogParams& params,
                       const char* fmt,
                       va_list args) override;
};

// This is a more verbose log line, which includes all we know:
//
// According to the regex below:
// ^(\d+:\d+:\d+\.\d+)
// (\d+)\s+(VERBOSE|DEBUG|INFO|WARNING|ERROR|FATAL|UNKWOWN)\s+([\w-]+\.[A-Za-z]+:\d+)\s+\|
// (.*)
//
// Where:
// group 1: Time stamp
// group 2: Thread Id
// group 3: Log level
// group 4: File:Line
// group 5: Log message
class VerboseLogFormatter : public LogFormatter {
public:
    std::string format(const LogParams& params,
                       const char* fmt,
                       va_list args) override;
};

// This formatter removes all duplicate lines, replacing them with an occurrence
// count.
//
// WARNING: This logger does not neccessarily produces output, and buffers the
// last line if it was a duplicate.
class NoDuplicateLinesFormatter : public LogFormatter {
public:
    NoDuplicateLinesFormatter(std::shared_ptr<LogFormatter> logger);

    // Will return "" when the last line was a duplicate.
    std::string format(const LogParams& params,
                       const char* fmt,
                       va_list args) override;

private:
    std::shared_ptr<LogFormatter> mInner;
    std::string kEmpty{};
    char mPrevLog[4096] = {0};
    LogParams mPrevParams;
    int mDuplicates{0};
    std::mutex mFormatMutex;
};

}  // namespace base
}  // namespace android
