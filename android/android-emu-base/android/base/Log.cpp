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

#include "android/base/Log.h"

#include <stdarg.h>                        // for va_end, va_list, va_start
#include <stdio.h>                         // for fprintf, fflush, vsnprintf
#include <stdlib.h>                        // for free, malloc
#include <string.h>                        // for memcpy, strerror

#include "android/base/ArraySize.h"        // for arraySize
#include "android/base/Debug.h"            // for DebugBreak, IsDebuggerAtta...
#include "android/base/StringView.h"       // for StringView, c_str, CStrWra...
#include "android/base/files/PathUtils.h"  // for PathUtils

#define ENABLE_THREAD_ID 0

namespace android {
namespace base {

namespace {

// The current log output.
testing::LogOutput* gLogOutput = NULL;

bool gDcheckLevel = false;
LogSeverity gMinLogLevel = LOG_INFO;

// Convert a severity level into a string.
const char* severityLevelToString(LogSeverity severity) {
    const char* kSeverityStrings[] = {
            "INFO",
            "WARNING",
            "ERROR",
            "FATAL",
    };
    if (severity >= 0 && severity < LOG_NUM_SEVERITIES)
        return kSeverityStrings[severity];
    if (severity == -1) {
        return "VERBOSE";
    }
    return "UNKNOWN";
}

// Default log output function
void defaultLogMessage(const LogParams& params,
                       const char* message,
                       size_t messageLen) {
    const char* tidStr = "";
#if ENABLE_THREAD_ID
    // Obtaining current thread's id is expensive. Only do this for
    // verbose logs.
    std::string tidStrData;
    if (params.severity <= LOG_VERBOSE) {
        tidStr = android::base::StringFormat("[%lu]:", getCurrentThreadId());
    }
#endif

    FILE* output = params.severity >= LOG_WARNING ? stderr : stdout;
    if (params.quiet) {
        fprintf(output, "emulator: %s: %.*s\n",
                severityLevelToString(params.severity), int(messageLen),
                message);
    } else {
        StringView path = params.file;
        StringView filename;
        if (!PathUtils::split(path, nullptr, &filename)) {
            filename = path;
        }

        fprintf(output, "emulator: %s%s: %s:%d: %.*s\n", tidStr,
                severityLevelToString(params.severity), c_str(filename).get(),
                params.lineno, int(messageLen), message);

        if (params.severity >= LOG_WARNING) {
            // Note: by default, stderr is non buffered, but the program might
            // have altered this setting, so always flush explicitly to ensure
            // that the log is displayed as soon as possible. This avoids log
            // messages being lost when a crash happens, and makes debugging
            // easier. On the other hand, it means lots of logging will impact
            // performance.
            fflush(stderr);
        }
    }

    if (params.severity >= LOG_FATAL) {
        if (IsDebuggerAttached()) {
            android::base::DebugBreak();
        }
        _exit(1);
    }
}

void logMessage(const LogParams& params,
                const char* message,
                size_t messageLen) {
    if (gLogOutput) {
        gLogOutput->logMessage(params, message, messageLen);
    } else {
        defaultLogMessage(params, message, messageLen);
    }
}

}  // namespace

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

LogSeverity getMinLogLevel() {
    return gMinLogLevel;
}

void setMinLogLevel(LogSeverity level) {
    gMinLogLevel = level;
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
        if (ret >= 0 && size_t(ret) < capacity)
            break;
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

std::ostream& operator<<(std::ostream& stream,
                         const android::base::StringView& str) {
    if (!str.empty()) {
        stream.write(str.data(), str.size());
    }
    return stream;
}

LogstreamBuf::LogstreamBuf() {
    setp(mStr, mStr + arraySize(mStr));
}

size_t LogstreamBuf::size() {
    return this->pptr() - this->pbase();
}

int LogstreamBuf::overflow(int c) {
    if (mLongString.empty()) {
        // Case 1, we have been using our fast small buffer, but decided to log a really long line.
        // We now transfer ownership of the buffer to a std::vector that will manage allocation.
        // We resize to 2x size of our current size.
        mLongString.resize(arraySize(mStr) * 2);
        memcpy(&mLongString[0], &mStr[0], arraySize(mStr));
    } else {
        // Case 2: The std::vector is already managing the buffer, but the current log line no longer fits.
        // We are going to resize the vector. The resize will reallocate the existing elements properly.
        mLongString.resize(mLongString.size() * 2);
    }

    // We have just resized the std::vector so we know we have enough space available and can
    // add our overflow character at the proper offset.
    int offset = pptr() - pbase();
    mLongString[offset] = c;

    // We let std::streambuf know that it can use the memory area covered by std::vector.
    // this call resets pptr, so we will move it back to the right offset later on.
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
