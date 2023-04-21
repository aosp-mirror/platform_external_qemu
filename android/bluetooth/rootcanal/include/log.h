#pragma once
#include <cstdlib>      // for abort
#include <memory>       // for shared_ptr
#include <ostream>      // for ostream
#include <string_view>  // for string_view

#include "aemu/base/logging/LogSeverity.h"  // for EMULATOR_LOG_INFO, EMULATOR_...

#ifdef ANDROID_EMULATOR_BUILD
#include "android/utils/debug.h" // for __emu_log_print, VERBOSE_CHECK
#else
#include "aemu/base/logging/CLog.h"
#endif

extern "C" void __blue_write_to_file(LogSeverity prio,
                                     const char* file,
                                     int line,
                                     const char* fmt,
                                     ...);

namespace android::bluetooth {
// Gets a log stream that can be used to write logging information.
// The id can be used to uniquely identify the stream. If the id has
// already been used it will be prefixed by %d_ and a number.
std::shared_ptr<std::ostream> getLogstream(std::string_view id);
}  // namespace android::bluetooth

// Note that we log both to a file as well as the emulator log system.
#ifdef ANDROID_EMULATOR_BUILD
#define LOGWRAPPER(level, fmt, args...)                                      \
    do {                                                                     \
        if (VERBOSE_CHECK(bluetooth)) {                                      \
            __blue_write_to_file(level, __FILE__, __LINE__, fmt "", ##args); \
            __emu_log_print(level, __FILE__, __LINE__, fmt "", ##args);      \
        }                                                                    \
    } while (false)
#else
#define LOGWRAPPER(level, fmt, args...)                                  \
    do {                                                                 \
        __blue_write_to_file(level, __FILE__, __LINE__, fmt "", ##args); \
        __emu_log_print(level, __FILE__, __LINE__, fmt "", ##args);      \
    } while (false)
#endif

#define LOG_VERBOSE(fmt, args...) LOGWRAPPER(EMULATOR_LOG_VERBOSE, fmt, ##args);
#define LOG_DEBUG(fmt, args...) LOGWRAPPER(EMULATOR_LOG_DEBUG, fmt, ##args);
#define LOG_INFO(...) LOGWRAPPER(EMULATOR_LOG_INFO, __VA_ARGS__)
#define LOG_WARN(...) LOGWRAPPER(EMULATOR_LOG_WARNING, __VA_ARGS__)
#define LOG_ERROR(...) LOGWRAPPER(EMULATOR_LOG_ERROR, __VA_ARGS__)

#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL(...)                                                 \
    do {                                                                      \
        __emu_log_print(EMULATOR_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__); \
        std::abort();                                                         \
    } while (false)
#endif

#define ASSERT(condition)                                          \
    do {                                                           \
        if (!(condition)) {                                        \
            LOG_ALWAYS_FATAL("assertion '" #condition "' failed"); \
        }                                                          \
    } while (false)

#define ASSERT_LOG(condition, fmt, args...)                              \
    do {                                                                 \
        if (!(condition)) {                                              \
            LOG_ALWAYS_FATAL("assertion '" #condition "' failed - " fmt, \
                             ##args);                                    \
        }                                                                \
    } while (false)

#ifndef CASE_RETURN_TEXT
#define CASE_RETURN_TEXT(code) \
    case code:                 \
        return #code
#endif
