#pragma once
// #include_next "os/log.h"
#include "android/utils/debug.h"
#include <cstdlib>

#define LOGWRAPPER(level, fmt, args...)                                 \
    do {                                                                \
        if (VERBOSE_CHECK(bluetooth)) {                                 \
            __emu_log_print(level, __FILE__, __LINE__, fmt "", ##args); \
        }                                                               \
    } while (false)

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
