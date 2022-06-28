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
#pragma once

#include <errno.h>      // for errno
#include <stdio.h>      // for size_t, EOF
#include <string.h>     // for strcmp
#include <iostream>     // for ostream, operator<<
#include <vector>       // for vector

#include "android/base/logging/LogSeverity.h"  // for LogSeverity, EMULATOR_...

namespace android {
namespace base {

// Forward declarations.
class StringView;

// Returns the minimal log level.
::LogSeverity getMinLogLevel();
void setMinLogLevel(::LogSeverity level);

class LogFormatter;

void setLogFormatter(LogFormatter* fmt);

// Convert a log level name (e.g. 'INFO') into the equivalent
// ::android::base LOG_<name> constant.
#define LOG_SEVERITY_FROM(x) EMULATOR_LOG_##x

// Helper macro used to test if logging for a given log level is
// currently enabled. |severity| must be a log level without the LOG_
// prefix, as in:
//
//  if (LOG_IS_ON(INFO)) {
//      ... do additionnal logging
//  }
//
#define LOG_IS_ON(severity) \
    (LOG_SEVERITY_FROM(severity) >= ::android::base::getMinLogLevel())

// For performance reasons, it's important to avoid constructing a
// LogMessage instance every time a LOG() or CHECK() statement is
// encountered at runtime, i.e. these objects should only be constructed
// when absolutely necessary, which means:
//  - For LOG() statements, when the corresponding log level is enabled.
//  - For CHECK(), when the tested expression doesn't hold.
//
// At the same time, we really want to use expressions like:
//    LOG(severity) << some_stuff << some_more_stuff;
//
// This means LOG(severity) should expand to something that can take
// << operators on its right hand side. This is achieved with the
// ternary '? :', as implemented by this helper macro.
//
// Unfortunately, a simple thing like:
//
//   !(condition) ? (void)0 : (expr)
//
// will not work, because the compiler complains loudly with:
//
//   error: second operand to the conditional operator is of type 'void',
//   but the third operand is neither a throw-expression nor of type 'void'
#define LOG_LAZY_EVAL(condition, expr) \
    !(condition) ? (void)0 : ::android::base::LogStreamVoidify() & (expr)

// Send a message to the log if |severity| is higher or equal to the current
// logging severity level. This macro expands to an expression that acts as
// an input stream for strings, ints and floating point values, as well as
// LogString instances. Usage example:
//
//    LOG(INFO) << "Starting flux capacitor";
//    fluxCapacitor::start();
//    LOG(INFO) << "Flux capacitor started";
//
// Note that the macro implementation is optimized to avoid doing any work
// if the severity level is disabled.
//
// It's possible to do conditional logging with LOG_IF()
#define LOG(severity) \
    LOG_LAZY_EVAL(LOG_IS_ON(severity), LOG_MESSAGE_STREAM_COMPACT(severity))

// A variant of LOG() that only performs logging if a specific condition
// is encountered. Note that |condition| is only evaluated if |severity|
// is high enough. Usage example:
//
//    LOG(INFO) << "Starting fuel injector";
//    fuelInjector::start();
//    LOG(INFO) << "Fuel injection started";
//    LOG_IF(INFO, fuelInjector::hasOptimalLevel())
//            << "Fuel injection at optimal level";
//
#define LOG_IF(severity, condition)                   \
    LOG_LAZY_EVAL(LOG_IS_ON(severity) && (condition), \
                  LOG_MESSAGE_STREAM_COMPACT(severity))

// A variant of LOG() that avoids printing debug information such as file/line
// information, for user-visible output.
#define QLOG(severity) \
    LOG_LAZY_EVAL(LOG_IS_ON(severity), QLOG_MESSAGE_STREAM_COMPACT(severity))

// A variant of LOG_IF() that avoids printing debug information such as
// file/line information, for user-visible output.
#define QLOG_IF(severity, condition)                  \
    LOG_LAZY_EVAL(LOG_IS_ON(severity) && (condition), \
                  QLOG_MESSAGE_STREAM_COMPACT(severity))

// A variant of LOG() that integrates with the utils/debug.h verbose tags,
// enabling statements to only appear on the console if the "-debug-<tag>"
// command line parameter is provided.  Example:
//
//    VLOG(virtualscene) << "Starting scene.";
//
// Which would only be visible if -debug-virtualscene or -debug-all is passed
// as a command line parameter.
//
// When logging is enabled, VLOG statements are logged at the INFO severity.
#define VLOG(tag) \
    LOG_LAZY_EVAL(VERBOSE_CHECK(tag), LOG_MESSAGE_STREAM_COMPACT(INFO))

// A variant of LOG() that also appends the string message corresponding
// to the current value of 'errno' just before the macro is called. This
// also preserves the value of 'errno' so it can be tested after the
// macro call (i.e. any error during log output does not interfere).
#define PLOG(severity) \
    LOG_LAZY_EVAL(LOG_IS_ON(severity), PLOG_MESSAGE_STREAM_COMPACT(severity))

// A variant of LOG_IF() that also appends the string message corresponding
// to the current value of 'errno' just before the macro is called. This
// also preserves the value of 'errno' so it can be tested after the
// macro call (i.e. any error during log output does not interfere).
#define PLOG_IF(severity, condition)                  \
    LOG_LAZY_EVAL(LOG_IS_ON(severity) && (condition), \
                  PLOG_MESSAGE_STREAM_COMPACT(severity))

// Evaluate |condition|, and if it fails, log a fatal message.
// This is a better version of assert(), in the future, this will
// also break directly into the debugger for debug builds.
//
// Usage is similar to LOG(FATAL), e.g.:
//
//   CHECK(some_condition) << "Something really bad happened!";
//
#define CHECK(condition) \
    LOG_IF(FATAL, !(condition)) << "Check failed: " #condition ". "

// A variant of CHECK() that also appends the errno message string at
// the end of the log message before exiting the process.
#define PCHECK(condition) \
    PLOG_IF(FATAL, !(condition)) << "Check failed: " #condition ". "

// Define ENABLE_DLOG to 1 here if DLOG() statements should be compiled
// as normal LOG() ones in the final binary. If 0, the statements will not
// be compiled.
#ifndef ENABLE_DLOG
#if defined(NDEBUG)
#define ENABLE_DLOG 0
#else
#define ENABLE_DLOG 1
#endif
#endif

// ENABLE_DCHECK controls how DCHECK() statements are compiled:
//    0 - DCHECK() are not compiled in the binary at all.
//    1 - DCHECK() are compiled, but are not performed at runtime, unless
//        the DCHECK level has been increased explicitely.
//    2 - DCHECK() are always compiled as CHECK() in the final binary.
#ifndef ENABLE_DCHECK
#if defined(NDEBUG)
#define ENABLE_DCHECK 1
#else
#define ENABLE_DCHECK 2
#endif
#endif

// DLOG_IS_ON(severity) is used to indicate whether DLOG() should print
// something for the current level.
#if ENABLE_DLOG
#define DLOG_IS_ON(severity) LOG_IS_ON(severity)
#else
// NOTE: The compile-time constant ensures that the DLOG() statements are
//       not compiled in the final binary.
#define DLOG_IS_ON(severity) false
#endif

// DCHECK_IS_ON() is used to indicate whether DCHECK() should do anything.
#if ENABLE_DCHECK == 0
// NOTE: Compile-time constant ensures the DCHECK() statements are
// not compiled in the final binary.
#define DCHECK_IS_ON() false
#elif ENABLE_DCHECK == 1
#define DCHECK_IS_ON() ::android::base::dcheckIsEnabled()
#else
#define DCHECK_IS_ON() true
#endif

// A function that returns true iff DCHECK() should actually do any checking.
bool dcheckIsEnabled();

// Change the DCHECK() level to either false or true. Should only be called
// early, e.g. after parsing command-line arguments. Returns previous value.
bool setDcheckLevel(bool enabled);

// DLOG() is like LOG() for debug builds, and doesn't do anything for
// release one. This is useful to add log messages that you don't want
// to see in the final binaries, but are useful during testing.
#define DLOG(severity) DLOG_IF(severity, true)

// DLOG_IF() is like DLOG() for debug builds, and doesn't do anything for
// release one. See DLOG() comments.
#define DLOG_IF(severity, condition)                   \
    LOG_LAZY_EVAL(DLOG_IS_ON(severity) && (condition), \
                  LOG_MESSAGE_STREAM_COMPACT(severity))

// DCHECK(condition) is used to perform CHECK() in debug builds, or if
// the program called setDcheckLevel(true) previously. Note that it is
// also possible to completely remove them from the final binary by
// using the compiler flag -DENABLE_DCHECK=0
#define DCHECK(condition)                         \
    LOG_IF(FATAL, DCHECK_IS_ON() && !(condition)) \
            << "Check failed: " #condition ". "

// DPLOG() is like DLOG() that also appends the string message corresponding
// to the current value of 'errno' just before the macro is called. This
// also preserves the value of 'errno' so it can be tested after the
// macro call (i.e. any error during log output does not interfere).
#define DPLOG(severity) DPLOG_IF(severity, true)

// DPLOG_IF() tests whether |condition| is true before calling
// DPLOG(severity)
#define DPLOG_IF(severity, condition)                  \
    LOG_LAZY_EVAL(DLOG_IS_ON(severity) && (condition), \
                  PLOG_MESSAGE_STREAM_COMPACT(severity))

// Convenience class used hold a formatted string for logging reasons.
// Usage example:
//
//    LOG(INFO) << LogString("There are %d items in this set", count);
//
class LogString {
public:
    LogString(const char* fmt, ...);
    const char* string() const { return mString.data(); }

private:
    std::vector<char> mString;
};

// Helper structure used to group the parameters of a LOG() or CHECK()
// statement.
struct LogParams {
    LogParams() {}
    LogParams(const char* a_file,
              int a_lineno,
              LogSeverity a_severity,
              bool quiet = false)
        : file(a_file), lineno(a_lineno), severity(a_severity), quiet(quiet) {}

    friend bool operator==(const LogParams& s1, const LogParams& s2) {
        return s1.lineno == s2.lineno && s1.severity == s2.severity &&
               s1.quiet == s2.quiet &&
               ((s1.file == nullptr && s2.file == nullptr) ||  // both null..
                (s1.file != nullptr && s2.file != nullptr &&
                 (s1.file == s2.file ||
                  strcmp(s1.file, s2.file) == 0))  // or the same
               );
    }

    const char* file = nullptr;
    int lineno = -1;
    LogSeverity severity = LOG_SEVERITY_FROM(VERBOSE);
    bool quiet = false;
};

class LogstreamBuf : public std::streambuf {
public:
    LogstreamBuf();

    size_t size();
    char* str();

protected:
    int overflow(int c) override;

private:
    std::vector<char> mLongString;
    char mStr[256];
};

// Helper class used to implement an input stream similar to std::istream
// where it's possible to inject strings, integers, floats and LogString
// instances with the << operator.
//
// This also takes a source file, line number and severity to avoid
// storing these in the stack of the functions were LOG() and CHECK()
// statements are called.
class LogStream {
public:
    LogStream(const char* file, int lineno, LogSeverity severity, bool quiet);
    ~LogStream() = default;

    template <typename T>
    std::ostream& operator<<(const T& t) {
        return mStream << t;
    }

    const char* str() { return mStreamBuf.str(); }

    const size_t size() { return mStreamBuf.size(); }
    const LogParams& params() const { return mParams; }

private:
    LogParams mParams;
    LogstreamBuf mStreamBuf;
    std::ostream mStream;
};

// Add your own types when needed:
std::ostream& operator<<(std::ostream& stream,
                         const android::base::LogString& str);
std::ostream& operator<<(std::ostream& stream,
                         const android::base::StringView& str);

// Helper class used to avoid compiler errors, see LOG_LAZY_EVAL for
// more information.
class LogStreamVoidify {
public:
    LogStreamVoidify() {}
    // This has to be an operator with a precedence lower than << but
    // higher than ?:
    void operator&(std::ostream&) {}
};

// This represents an log message. At creation time, provide the name of
// the current file, the source line number and a severity.
// You can them stream stuff into it with <<. For example:
//
//   LogMessage(__FILE__, __LINE__, LOG_INFO) << "Hello World!\n";
//
// When destroyed, the message sends the final output to the appropriate
// log (e.g. stderr by default).
class LogMessage {
public:
    // To suppress printing file/line, set quiet = true.
    LogMessage(const char* file,
               int line,
               LogSeverity severity,
               bool quiet = false);
    ~LogMessage();

    LogStream& stream() const { return *mStream; }

protected:
    // Avoid that each LOG() statement
    LogStream* mStream;
};

// Helper macros to avoid too much typing. This creates a new LogMessage
// instance with the appropriate file source path, file source line and
// severity.
#define LOG_MESSAGE_COMPACT(severity) \
    ::android::base::LogMessage(__FILE__, __LINE__, LOG_SEVERITY_FROM(severity))

#define LOG_MESSAGE_STREAM_COMPACT(severity) \
    LOG_MESSAGE_COMPACT(severity).stream()

// A variant of LogMessage for outputting user-visible messages to the console,
// without debug information.
#define QLOG_MESSAGE_COMPACT(severity)              \
    ::android::base::LogMessage(__FILE__, __LINE__, \
                                LOG_SEVERITY_FROM(severity), true)

#define QLOG_MESSAGE_STREAM_COMPACT(severity) \
    QLOG_MESSAGE_COMPACT(severity).stream()

// A variant of LogMessage that saves the errno value on creation,
// then restores it on destruction, as well as append a strerror()
// error message to the log before sending it for output. Used by
// the PLOG() implementation(s).
//
// This cannot be a sub-class of LogMessage because the destructor needs
// to restore the saved errno message after sending the message to the
// LogOutput and deleting the stream.
class ErrnoLogMessage {
public:
    ErrnoLogMessage(const char* file,
                    int line,
                    LogSeverity severity,
                    int errnoCode);
    ~ErrnoLogMessage();

    LogStream& stream() const { return *mStream; }

private:
    LogStream* mStream;
    int mErrno;
};

// Helper macros to avoid too much typing.
#define PLOG_MESSAGE_COMPACT(severity)                   \
    ::android::base::ErrnoLogMessage(__FILE__, __LINE__, \
                                     LOG_SEVERITY_FROM(severity), errno)

#define PLOG_MESSAGE_STREAM_COMPACT(severity) \
    PLOG_MESSAGE_COMPACT(severity).stream()

namespace testing {

// Abstract interface to the output where the log messages are sent.
// IMPORTANT: Only use this for unit testing the log facility.
class LogOutput {
public:
    LogOutput() {}
    virtual ~LogOutput() {}

    // Send a full log message to the output. Not zero terminated, and
    // Does not have a trailing \n which can be added by the implementation
    // when writing the message to a file.
    // Note: if |severity| is LOG_FATAL, this should also terminate the
    // process.
    virtual void logMessage(const LogParams& params,
                            const char* message,
                            size_t message_len) = 0;

    // Set a new log output, and return pointer to the previous
    // implementation, which will be NULL for the default one.
    // |newOutput| is either NULL (which means the default), or a
    // custom instance of LogOutput.
    static LogOutput* setNewOutput(LogOutput* newOutput);
};

}  // namespace testing

}  // namespace base
}  // namespace android
