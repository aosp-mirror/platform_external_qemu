// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "aemu/base/logging/Log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <string_view>

#include "absl/strings/str_format.h"
#include "aemu/base/logging/Log.h"
#include "aemu/base/logging/LogFormatter.h"
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#endif

#define ENABLE_THREAD_ID 0

using android::base::LogFormatter;
using android::base::LogParams;
using android::base::SimpleLogFormatter;
using android::base::testing::LogOutput;

// The current log output.
LogOutput* gLogOutput = nullptr;

bool gDcheckLevel = false;
LogSeverity gMinLogLevel = EMULATOR_LOG_INFO;

SimpleLogFormatter defaultFormatter;
LogFormatter* gFormatter = &defaultFormatter;

void write_log_line(LogSeverity prio, const std::string& msg) {
    FILE* fp = prio >= LOG_SEVERITY_FROM(WARNING) ? stderr : stdout;
    if (!msg.empty()) {
        fwrite(msg.c_str(), 1, msg.size(), fp);
        if (msg.back() != '\n') {
            constexpr char newline = '\n';
            fwrite(&newline, sizeof(newline), 1, fp);
        }
    }

    if (prio >= LOG_SEVERITY_FROM(FATAL)) {
        fflush(stderr);
        std::abort();
    }
}

void __emu_log_print_str(LogSeverity prio,
                         const char* file,
                         int line,
                         const std::string& msg) {
    write_log_line(prio, gFormatter->format({file, line, prio},
                                                      msg));
}

LOGGING_API extern "C" void __emu_log_print(LogSeverity prio,
                                const char* file,
                                int line,
                                const char* fmt,
                                ...) {
    const int bufferSize = 2048;  // 2KB buffer size
    char buffer[bufferSize];
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(buffer, bufferSize, fmt, args);
    va_end(args);
    auto logline = std::string(buffer, size);

    write_log_line(prio, gFormatter->format({file, line, prio}, logline));
}

void logMessage(const LogParams& params,
                const char* message,
                size_t messageLen) {
    if (gLogOutput) {
        gLogOutput->logMessage(params, message, messageLen);
    } else {
        __emu_log_print_str(params.severity, params.file, params.lineno,
                            std::string(message, messageLen));
    }
}

namespace android {
namespace base {

namespace {}  // namespace

// DCHECK level.

bool dcheckIsEnabled() {
    return gDcheckLevel;
}

bool setDcheckLevel(bool enabled) {
    bool ret = gDcheckLevel;
    gDcheckLevel = enabled;
    return ret;
}

// LogSeverity

extern "C" LogSeverity getMinLogLevel() {
    return gMinLogLevel;
}

extern "C" void setMinLogLevel(LogSeverity level) {
    gMinLogLevel = level;
}

void setLogFormatter(LogFormatter* fmt) {
    gFormatter = fmt;
}

// LogString

LogString::LogString(const char* fmt, ...) {
    size_t capacity = 100;
    for (;;) {
        mString.resize(capacity);
        va_list args;
        va_start(args, fmt);
        int ret = vsnprintf(mString.data(), capacity, fmt, args);
        va_end(args);
        if (ret >= 0 && static_cast<size_t>(ret) < capacity) {
            break;
        }
        capacity *= 2;
    }
}

// LogStream

LogStream::LogStream(const char* file,
                     int lineno,
                     LogSeverity severity,
                     bool quiet)
    : mParams(file, lineno, severity, quiet), mStream(&mStreamBuf) {}

std::ostream& operator<<(std::ostream& stream,
                         const android::base::LogString& str) {
    stream << str.string();
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const std::string_view& str) {
    if (!str.empty()) {
        stream.write(str.data(), str.size());
    }
    return stream;
}

LogstreamBuf::LogstreamBuf() {
    setp(mStr, mStr + sizeof(mStr));
}

size_t LogstreamBuf::size() {
    return this->pptr() - this->pbase();
}

int LogstreamBuf::overflow(int c) {
    if (mLongString.empty()) {
        // Case 1, we have been using our fast small buffer, but decided to log
        // a really long line. We now transfer ownership of the buffer to a
        // std::vector that will manage allocation. We resize to 2x size of our
        // current size.
        mLongString.resize(sizeof(mStr) * 2);
        memcpy(&mLongString[0], &mStr[0], sizeof(mStr));
    } else {
        // Case 2: The std::vector is already managing the buffer, but the
        // current log line no longer fits. We are going to resize the vector.
        // The resize will reallocate the existing elements properly.
        mLongString.resize(mLongString.size() * 2);
    }

    // We have just resized the std::vector so we know we have enough space
    // available and can add our overflow character at the proper offset.
    int offset = pptr() - pbase();
    mLongString[offset] = c;

    // We let std::streambuf know that it can use the memory area covered by
    // std::vector. this call resets pptr, so we will move it back to the right
    // offset later on.
    setp(mLongString.data(), mLongString.data() + mLongString.size());

    // We bump pptr to our offset + 1, which can be used for future writes.
    pbump(offset + 1);
    return c;
}

char* LogstreamBuf::str() {
    return this->pbase();
}

// LogMessage

LogMessage::LogMessage(const char* file,
                       int line,
                       LogSeverity severity,
                       bool quiet)
    : mStream(new LogStream(file, line, severity, quiet)) {}

LogMessage::~LogMessage() {
    logMessage(mStream->params(), mStream->str(), mStream->size());
    delete mStream;
}

// ErrnoLogMessage

ErrnoLogMessage::ErrnoLogMessage(const char* file,
                                 int line,
                                 LogSeverity severity,
                                 int errnoCode)
    : mStream(nullptr), mErrno(errnoCode) {
    mStream = new LogStream(file, line, severity, false);
}

ErrnoLogMessage::~ErrnoLogMessage() {
    (*mStream) << "Error message: " << strerror(mErrno);
    logMessage(mStream->params(), mStream->str(), mStream->size());
    delete mStream;
    // Restore the errno.
    errno = mErrno;
}

// LogOutput

namespace testing {

// static
LogOutput* LogOutput::setNewOutput(LogOutput* newOutput) {
    LogOutput* ret = gLogOutput;
    gLogOutput = newOutput;
    return ret;
}

}  // namespace testing

}  // namespace base
}  // namespace android
