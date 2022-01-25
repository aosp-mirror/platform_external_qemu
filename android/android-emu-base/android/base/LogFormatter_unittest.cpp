// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/base/LogFormatter.h"

#include <gtest/gtest.h>  // for Message, TestPartResult, Sui...
#include <regex>          // for regex_match, match_results
#include <string>         // for string
#include <memory>

#include "android/utils/log_severity.h"  // for EMULATOR_LOG_INFO, EMULATOR_...

namespace android {
namespace base {

inline static std::string translate_sev(LogSeverity value) {
#define SEV(p, str)        \
    case (LogSeverity::p): \
        return str;        \
        break;

    switch (value) {
        SEV(EMULATOR_LOG_VERBOSE, "VERBOSE")
        SEV(EMULATOR_LOG_DEBUG, "DEBUG")
        SEV(EMULATOR_LOG_INFO, "INFO")
        SEV(EMULATOR_LOG_WARNING, "WARNING")
        SEV(EMULATOR_LOG_ERROR, "ERROR")
        SEV(EMULATOR_LOG_FATAL, "FATAL")
        default:
            return "UNKWOWN";
    }
#undef SEV
}

TEST(LogFormatter, SimpleFormatterMatchesRegex) {
    // See b/198468198
    SimpleLogFormatter formatter;
    const std::regex simle_log(
            "^(VERBOSE|DEBUG|INFO|WARNING|ERROR|FATAL|UNKWOWN)\\s+\\| (.*)");
    std::smatch m;
    for (int sev = EMULATOR_LOG_DEBUG; sev < EMULATOR_LOG_NUM_SEVERITIES;
         sev++) {
        auto severity = static_cast<LogSeverity>(sev);
        LogParams lg = {"filename.c", 123, severity};
        std::string logline = formatter.LogFormatter::format(lg, "Hello World");
        ASSERT_TRUE(std::regex_match(logline, m, simle_log));
        ASSERT_EQ(translate_sev(severity), m[1]) << "Unexpected severity!";
        ASSERT_EQ("Hello World", m[2]);
    }
}

TEST(LogFormatter, SimpleFormatterWithTimeMatchesRegex) {
    // See b/198468198
    SimpleLogWithTimeFormatter formatter;
    const std::regex simle_log(
            "^(\\d+:\\d+:\\d+\\.\\d+)\\s+(VERBOSE|DEBUG|INFO|WARNING|ERROR|"
            "FATAL|UNKWOWN)\\s+\\| (.*)");
    std::smatch m;
    for (int sev = EMULATOR_LOG_DEBUG; sev < EMULATOR_LOG_NUM_SEVERITIES;
         sev++) {
        auto severity = static_cast<LogSeverity>(sev);
        LogParams lg = {"filename.c", 123, severity};
        std::string logline = formatter.LogFormatter::format(lg, "Hello World");
        ASSERT_TRUE(std::regex_match(logline, m, simle_log));
        ASSERT_EQ(translate_sev(severity), m[2]) << "Unexpected severity!";
        ASSERT_EQ("Hello World", m[3]);
    }
}

TEST(LogFormatter, VerboseFormatterMatchesRegex) {
    // See b/198468198
    VerboseLogFormatter formatter;
    const std::regex verbose_log(
            R"REG(^(\d+:\d+:\d+\.\d+) (\d+)\s+(VERBOSE|DEBUG|INFO|WARNING|ERROR|FATAL|UNKWOWN)\s+([\w-]+\.[A-Za-z]+:\d+)\s+\| (.*))REG");

    std::smatch m;
    for (int sev = EMULATOR_LOG_DEBUG; sev < EMULATOR_LOG_NUM_SEVERITIES;
         sev++) {
        auto severity = static_cast<LogSeverity>(sev);
        LogParams lg = {"filename.c", 123, severity};
        std::string logline = formatter.LogFormatter::format(lg, "Hello World");
        ASSERT_TRUE(std::regex_match(logline, m, verbose_log))
                << "Logine =  " << logline;
        ASSERT_EQ(translate_sev(severity), m[3]) << "Unexpected severity!";
        ASSERT_EQ("filename.c:123", m[4]);
        ASSERT_EQ("Hello World", m[5]);
    }
}

TEST(LogFormatter, DuplicateVerboseFormatterMatchesRegex) {
    // See b/198468198
    // This validates that the duplicate logger does not mess around with the
    // format.
    NoDuplicateLinesFormatter ndlf(std::make_shared<VerboseLogFormatter>());
    const std::regex verbose_log(
            R"REG(^(\d+:\d+:\d+\.\d+) (\d+)\s+(VERBOSE|DEBUG|INFO|WARNING|ERROR|FATAL|UNKWOWN)\s+([\w-]+\.[A-Za-z]+:\d+)\s+\| (.*))REG");

    std::smatch m;
    for (int sev = EMULATOR_LOG_DEBUG; sev < EMULATOR_LOG_NUM_SEVERITIES;
         sev++) {
        auto severity = static_cast<LogSeverity>(sev);
        LogParams lg = {"filename.c", 123, severity};

        std::string logline = ndlf.LogFormatter::format(lg, "Hello World");
        ASSERT_TRUE(std::regex_match(logline, m, verbose_log))
                << "Logine =  " << logline;
        ASSERT_EQ(translate_sev(severity), m[3]) << "Unexpected severity!";
        ASSERT_EQ("filename.c:123", m[4]);
        ASSERT_EQ("Hello World", m[5]);
    }
}

TEST(LogFormatter, DuplicateDoesNotLogALine) {
    NoDuplicateLinesFormatter ndlf(std::make_shared<SimpleLogFormatter>());
    LogParams lg = {"filename.c", 123, EMULATOR_LOG_INFO};
    ndlf.LogFormatter::format(lg, "Hello World");
    ASSERT_EQ("", ndlf.LogFormatter::format(lg, "Hello World"));
}

TEST(LogFormatter, DuplicateDoubleLogsALine) {
    NoDuplicateLinesFormatter ndlf(std::make_shared<SimpleLogFormatter>());
    LogParams lg = {"filename.c", 123, EMULATOR_LOG_INFO};
    ndlf.LogFormatter::format(lg, "Hello World");
    ndlf.LogFormatter::format(lg, "Hello World");
    ASSERT_EQ(
            "INFO    | Hello World\n"
            "INFO    | x",
            ndlf.LogFormatter::format(lg, "x"));
}

TEST(LogFormatter, DuplicatTripleLogsALine) {
    NoDuplicateLinesFormatter ndlf(std::make_shared<SimpleLogFormatter>());
    LogParams lg = {"filename.c", 123, EMULATOR_LOG_INFO};
    ASSERT_EQ("INFO    | Hello World",
              ndlf.LogFormatter::format(lg, "Hello World"));

    ndlf.LogFormatter::format(lg, "Hello World");
    ndlf.LogFormatter::format(lg, "Hello World");
    ndlf.LogFormatter::format(lg, "Hello World");
    ASSERT_EQ(
            "INFO    | Hello World (3x)\n"
            "INFO    | x",
            ndlf.LogFormatter::format(lg, "x"));
}

}  // namespace base
}  // namespace android
