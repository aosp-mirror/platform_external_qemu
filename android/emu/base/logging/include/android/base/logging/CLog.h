/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "android/base/logging/LogSeverity.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    kLogDefaultOptions = 0,
    kLogEnableDuplicateFilter = 1,
    kLogEnableTime = 1 << 2,
    kLogEnableVerbose = 1 << 3,
} LoggingFlags;


// Enable/disable verbose logs from the base/* family.
extern void base_enable_verbose_logs();
extern void base_disable_verbose_logs();

// Configure the logging framework.
extern void base_configure_logs(LoggingFlags flags);
extern void __emu_log_print(LogSeverity prio,
                            const char* file,
                            int line,
                            const char* fmt,
                            ...);

#ifndef EMULOG
#define EMULOG(priority, fmt, ...) \
    __emu_log_print(priority, __FILE__, __LINE__, fmt, ##__VA_ARGS__);
#endif

// Logging support.
#define dprint(fmt, ...)                                 \
    if (EMULATOR_LOG_VERBOSE >= android_log_severity) {  \
        EMULOG(EMULATOR_LOG_VERBOSE, fmt, ##__VA_ARGS__) \
    }

#define dinfo(fmt, ...)                               \
    if (EMULATOR_LOG_INFO >= android_log_severity) {  \
        EMULOG(EMULATOR_LOG_INFO, fmt, ##__VA_ARGS__) \
    }
#define dwarning(fmt, ...)                               \
    if (EMULATOR_LOG_WARNING >= android_log_severity) {  \
        EMULOG(EMULATOR_LOG_WARNING, fmt, ##__VA_ARGS__) \
    }
#define derror(fmt, ...)                               \
    if (EMULATOR_LOG_ERROR >= android_log_severity) {  \
        EMULOG(EMULATOR_LOG_ERROR, fmt, ##__VA_ARGS__) \
    }
#define dfatal(fmt, ...) EMULOG(EMULATOR_LOG_FATAL, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

