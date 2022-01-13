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

#include "android/utils/debug.h"

#include <memory>
#include "android/base/Log.h"
#include "android/base/LogFormatter.h"

using android::base::LogFormatter;
using android::base::NoDuplicateLinesFormatter;
using android::base::SimpleLogFormatter;
using android::base::SimpleLogWithTimeFormatter;
using android::base::VerboseLogFormatter;

void base_enable_verbose_logs() {
    VERBOSE_ENABLE(init);
    android::base::setMinLogLevel(EMULATOR_LOG_VERBOSE);
    android_log_severity = EMULATOR_LOG_VERBOSE;
}

void base_disable_verbose_logs() {
    VERBOSE_DISABLE(init);
    android::base::setMinLogLevel(EMULATOR_LOG_INFO);
    android_log_severity = EMULATOR_LOG_INFO;
}

void base_configure_logs(LoggingFlags flags) {
    // Arguments have been parsed.. Time to fully initialize the log config.
    std::unique_ptr<LogFormatter> formatter =
            std::make_unique<SimpleLogFormatter>();
    if (VERBOSE_CHECK(log)) {
        formatter = std::make_unique<VerboseLogFormatter>();
    } else if (VERBOSE_CHECK(time)) {
        formatter = std::make_unique<SimpleLogWithTimeFormatter>();
    }
    if (flags & kLogEnableDuplicateFilter) {
        formatter = std::make_unique<NoDuplicateLinesFormatter>(
                std::move(formatter));
    }
    android::base::setLogFormatter(formatter.release());
}