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

#include "emulator/base/StringView.h"
#include "rtc_base/logging.h"
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>

#include <string>

namespace emulator {
namespace base {

enum LogSeverity {
    LOG_VERBOSE = 1,
    LOG_INFO = 2,
    LOG_WARNING = 3,
    LOG_ERROR = 4,
    LOG_FATAL = 4,
    LOG_NUM_SEVERITIES,

// DFATAL will be ERROR in release builds, and FATAL in debug ones.
#ifdef NDEBUG
    LOG_DFATAL = LOG_ERROR,
#else
    LOG_DFATAL = LOG_FATAL,
#endif
};

// Returns the minimal log level.
LogSeverity getMinLogLevel();
void setMinLogLevel(LogSeverity level);

// Convert a log level name (e.g. 'INFO') into the equivalent
// ::emulator::base LOG_<name> constant.
#define LOG_SEVERITY_FROM(x)  rtc::LS_ ## x

// Helper macro used to test if logging for a given log level is
// currently enabled. |severity| must be a log level without the LOG_
// prefix, as in:
//
//  if (LOG_IS_ON(INFO)) {
//      ... do additionnal logging
//  }
//
#define LOG_IS_ON(severity) \
        (LOG_SEVERITY_FROM(severity) >=  rtc::LogMessage::GetMinLogSeverity())

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
  !(condition) ? (void)0 : ::rtc::LogMessageVoidify() & (expr)

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
//#define LOG(severity)  \
//        LOG_LAZY_EVAL(LOG_IS_ON(severity), \
//        LOG_MESSAGE_STREAM_COMPACT(severity))

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
#define LOG_IF(severity, condition) \
        LOG_LAZY_EVAL(LOG_IS_ON(severity) && (condition), \
                      LOG_MESSAGE_STREAM_COMPACT(severity))


// A variant of LOG() that also appends the string message corresponding
// to the current value of 'errno' just before the macro is called. This
// also preserves the value of 'errno' so it can be tested after the
// macro call (i.e. any error during log output does not interfere).
//#define PLOG(severity)  \
//        LOG_LAZY_EVAL(LOG_IS_ON(severity), \
//        PLOG_MESSAGE_STREAM_COMPACT(severity))

// A variant of LOG_IF() that also appends the string message corresponding
// to the current value of 'errno' just before the macro is called. This
// also preserves the value of 'errno' so it can be tested after the
// macro call (i.e. any error during log output does not interfere).
#define PLOG_IF(severity, condition) \
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
    LOG_IF(ERROR, !(condition)) << "Check failed: " #condition ". "


// A variant of CHECK() that also appends the errno message string at
// the end of the log message before exiting the process.
#define PCHECK(condition) \
    PLOG_IF(ERROR, !(condition)) << "Check failed: " #condition ". "


// Define ENABLE_DLOG to 1 here if DLOG() statements should be compiled
// as normal LOG() ones in the final binary. If 0, the statements will not
// be compiled.
#ifndef ENABLE_DLOG
#  if defined(NDEBUG)
#    define ENABLE_DLOG 0
#  else
#    define ENABLE_DLOG 1
#  endif
#endif

// ENABLE_DCHECK controls how DCHECK() statements are compiled:
//    0 - DCHECK() are not compiled in the binary at all.
//    1 - DCHECK() are compiled, but are not performed at runtime, unless
//        the DCHECK level has been increased explicitely.
//    2 - DCHECK() are always compiled as CHECK() in the final binary.
#ifndef ENABLE_DCHECK
#  if defined(NDEBUG)
#    define ENABLE_DCHECK 1
#  else
#    define ENABLE_DCHECK 2
#  endif
#endif

// DLOG_IS_ON(severity) is used to indicate whether DLOG() should print
// something for the current level.
#if ENABLE_DLOG
#  define DLOG_IS_ON(severity) LOG_IS_ON(severity)
#else
// NOTE: The compile-time constant ensures that the DLOG() statements are
//       not compiled in the final binary.
#  define DLOG_IS_ON(severity) false
#endif

// DCHECK_IS_ON() is used to indicate whether DCHECK() should do anything.
#if ENABLE_DCHECK == 0
    // NOTE: Compile-time constant ensures the DCHECK() statements are
    // not compiled in the final binary.
#  define DCHECK_IS_ON()  false
#elif ENABLE_DCHECK == 1
#  define DCHECK_IS_ON()  rtc::dcheckIsEnabled()
#else
#  define DCHECK_IS_ON()  true
#endif

// A function that returns true iff DCHECK() should actually do any checking.
bool dcheckIsEnabled();

// Change the DCHECK() level to either false or true. Should only be called
// early, e.g. after parsing command-line arguments. Returns previous value.
bool setDcheckLevel(bool enabled);

// DLOG() is like LOG() for debug builds, and doesn't do anything for
// release one. This is useful to add log messages that you don't want
// to see in the final binaries, but are useful during testing.
#define DLOG(severity)  DLOG_IF(severity, true)

// DLOG_IF() is like DLOG() for debug builds, and doesn't do anything for
// release one. See DLOG() comments.
#define DLOG_IF(severity, condition)  \
        LOG_LAZY_EVAL(DLOG_IS_ON(severity) && (condition), \
                      LOG_MESSAGE_STREAM_COMPACT(severity))

// DCHECK(condition) is used to perform CHECK() in debug builds, or if
// the program called setDcheckLevel(true) previously. Note that it is
// also possible to completely remove them from the final binary by
// using the compiler flag -DENABLE_DCHECK=0
#define DCHECK(condition) \
        LOG_IF(ERROR, DCHECK_IS_ON() && !(condition)) \
            << "Check failed: " #condition ". "

// DPLOG() is like DLOG() that also appends the string message corresponding
// to the current value of 'errno' just before the macro is called. This
// also preserves the value of 'errno' so it can be tested after the
// macro call (i.e. any error during log output does not interfere).
#define DPLOG(severity)  DPLOG_IF(severity, true)

// DPLOG_IF() tests whether |condition| is true before calling
// DPLOG(severity)
#define DPLOG_IF(severity, condition)  \
        LOG_LAZY_EVAL(DLOG_IS_ON(severity) && (condition), \
                      PLOG_MESSAGE_STREAM_COMPACT(severity))

// Convenience class used hold a formatted string for logging reasons.
// Usage example:
//
//    LOG(INFO) << LogString("There are %d items in this set", count);
//


// Helper macros to avoid too much typing. This creates a new LogMessage
// instance with the appropriate file source path, file source line and
// severity.
#define LOG_MESSAGE_COMPACT(severity) \
    rtc::LogMessage( \
            __FILE__, \
            __LINE__, \
            LOG_SEVERITY_FROM(severity))


#define LOG_MESSAGE_STREAM_COMPACT(severity) \
    LOG_MESSAGE_COMPACT(severity).stream()


// A variant of LogMessage that saves the errno value on creation,
// then restores it on destruction, as well as append a strerror()
// error message to the log before sending it for output. Used by
// the PLOG() implementation(s).
//
// This cannot be a sub-class of LogMessage because the destructor needs
// to restore the saved errno message after sending the message to the
// LogOutput and deleting the stream.


// Helper macros to avoid too much typing.
#define PLOG_MESSAGE_COMPACT(severity) \
    rtc::ErrnoLogMessage( \
            __FILE__, \
            __LINE__, \
            LOG_SEVERITY_FROM(severity), \
            errno)

#define PLOG_MESSAGE_STREAM_COMPACT(severity) \
    PLOG_MESSAGE_COMPACT(severity).stream()


}  // namespace base
}  // namespace emulator
