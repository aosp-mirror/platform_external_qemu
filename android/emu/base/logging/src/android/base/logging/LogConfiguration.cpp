// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include <stdint.h>

#include <iostream>
#include <memory>

#include "absl/base/call_once.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/internal/globals.h"
#include "absl/log/log.h"
#include "absl/log/log_sink_registry.h"

#include "aemu/base/logging/LogSeverity.h"

#include "android/base/logging/ColorLogSink.h"
#include "android/base/logging/StudioLogSink.h"
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

uint64_t android_verbose = 0;
LogSeverity android_log_severity = EMULATOR_LOG_INFO;

LogSeverity severity() {
    return android_log_severity;
}
void setSeverity(LogSeverity severity) {
    android_log_severity = severity;
}

extern "C" void verbose_disable(uint64_t tag) {
    android_verbose &= (1ULL << tag);
}

extern "C" bool verbose_check(uint64_t tag) {
    return (android_verbose & (1ULL << tag)) != 0;
}

extern "C" bool verbose_check_any() {
    return android_verbose != 0;
}

extern "C" void verbose_enable(uint64_t tag) {
    android_verbose |= (1ULL << tag);
}

extern "C" void set_verbosity_mask(uint64_t mask) {
    android_verbose = mask;
}

extern "C" uint64_t get_verbosity_mask() {
    return android_verbose;
}

void base_enable_verbose_logs() {
    setMinLogLevel(EMULATOR_LOG_DEBUG);
    android_log_severity = EMULATOR_LOG_DEBUG;
    absl::SetMinLogLevel(static_cast<absl::LogSeverityAtLeast>(-2));
}

void base_disable_verbose_logs() {
    setMinLogLevel(EMULATOR_LOG_INFO);
    android_log_severity = EMULATOR_LOG_INFO;
    absl::SetMinLogLevel(absl::LogSeverityAtLeast::kInfo);
}

static void initlogs_once(absl::LogSink* sink) {
    absl::InitializeLog();
    absl::log_internal::EnableSymbolizeLogStackTrace(true);
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfinity);
    absl::AddLogSink(sink);
}

static bool useColor() {
#ifndef _WIN32
    return isatty(fileno(stdout));
#else
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD dwMode;
    if (!GetConsoleMode(hStdout, &dwMode)) {
        return false;
    }

    // Check if ENABLE_VIRTUAL_TERMINAL_PROCESSING is already enabled
    if (dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) {
        return true;
    }

    // Try to enable ENABLE_VIRTUAL_TERMINAL_PROCESSING
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hStdout, dwMode)) {
        return false;
    }

    return true;
#endif
}

void base_configure_logs(LoggingFlags flags) {
    static absl::once_flag initlogs;
    static android::base::ColorLogSink logSink(&std::cout, useColor());

    absl::call_once(initlogs, initlogs_once, &logSink);
    if (flags & kLogEnableVerbose) {
        logSink.SetVerbosity(true);
        android::base::studio_sink()->SetVerbosity(true);
    }
    VLOG(1) << "Logging " << (useColor() ? "in color" : "without color");
}
