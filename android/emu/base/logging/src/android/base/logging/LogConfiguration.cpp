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
#include <stdint.h>  // for uint64_t
#include <memory>    // for make_unique, unique_ptr

#include "aemu/base/logging/CLog.h"          // for LoggingFlags, base_co...
#include "aemu/base/logging/Log.h"           // for setMinLogLevel, setLo...
#include "aemu/base/logging/LogFormatter.h"  // for NoDuplicateLinesForma...
#include "aemu/base/logging/LogSeverity.h"   // for EMULATOR_LOG_INFO

using android::base::LogFormatter;
using android::base::NoDuplicateLinesFormatter;
using android::base::SimpleLogFormatter;
using android::base::SimpleLogWithTimeFormatter;
using android::base::VerboseLogFormatter;

extern "C" uint64_t android_verbose = 0;
extern "C" LogSeverity android_log_severity = EMULATOR_LOG_INFO;
void base_enable_verbose_logs() {
    android::base::setMinLogLevel(EMULATOR_LOG_VERBOSE);
    android_log_severity = EMULATOR_LOG_VERBOSE;
}

void base_disable_verbose_logs() {
    android::base::setMinLogLevel(EMULATOR_LOG_INFO);
    android_log_severity = EMULATOR_LOG_INFO;
}

void base_configure_logs(LoggingFlags flags) {
    // Arguments have been parsed.. Time to fully initialize the log config.
    std::unique_ptr<LogFormatter> formatter =
            std::make_unique<SimpleLogFormatter>();
    if (flags & kLogEnableVerbose) {
        formatter = std::make_unique<VerboseLogFormatter>();
    } else if (flags & kLogEnableTime) {
        formatter = std::make_unique<SimpleLogWithTimeFormatter>();
    }
    if (flags & kLogEnableDuplicateFilter) {
        formatter = std::make_unique<NoDuplicateLinesFormatter>(
                std::move(formatter));
    }
    android::base::setLogFormatter(formatter.release());
}