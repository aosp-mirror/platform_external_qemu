// Copyright 2024 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/base/logging/ColorLogSink.h"
#include "absl/strings/str_format.h"
#include "absl/time/time.h"

#include <cstdint>
#include <iostream>

namespace android {
namespace base {

/** Standard ansi Color codes. */
inline static constexpr std::string_view kColorNoFormat = "";
inline static constexpr std::string_view kColorNormal = "\033[0;39m";
inline static constexpr std::string_view kColorNormalRed = "\033[31m";
inline static constexpr std::string_view kColorNormalGreen = "\033[32m";
inline static constexpr std::string_view kColorNormalBlue = "\033[34m";
inline static constexpr std::string_view kColorNormalMagenta = "\033[35m";
inline static constexpr std::string_view kColorNormalYellow = "\033[33m";
inline static constexpr std::string_view kColorNormalCyan = "\033[36m";
inline static constexpr std::string_view kColorBoldNormal = "\033[0;39m\033[1m";
inline static constexpr std::string_view kColorBoldRed = "\033[1;31m";
inline static constexpr std::string_view kColorBoldGreen = "\033[1;32m";
inline static constexpr std::string_view kColorBoldBlue = "\033[1;34m";
inline static constexpr std::string_view kColorBoldMagenta = "\033[1;35m";
inline static constexpr std::string_view kColorBoldYellow = "\033[1;33m";
inline static constexpr std::string_view kColorBoldCyan = "\033[1;36m";
inline static constexpr std::string_view kColorBgRed = "\033[41m";
inline static constexpr std::string_view kColorBgGreen = "\033[42m";
inline static constexpr std::string_view kColorBgBlue = "\033[44m";
inline static constexpr std::string_view kColorBgMagenta = "\033[45m";
inline static constexpr std::string_view kColorBgYellow = "\033[43m";
inline static constexpr std::string_view kColorBgCyan = "\033[46m";

std::string_view ColorLogSink::TranslateSeverity(
        const absl::LogEntry& entry) const {
    if (entry.verbosity() > 0) {
        switch (entry.verbosity()) {
            case 1:
                return "DEBUG  ";
            case 2:
                return "VERBOSE";
            default:
                return "TRACE  ";
        }
    }
    switch (entry.log_severity()) {
        case absl::LogSeverity::kInfo:
            return "INFO        ";
        case absl::LogSeverity::kWarning:
            return "WARNING     ";
        case absl::LogSeverity::kError:
            return "ERROR       ";
        case absl::LogSeverity::kFatal:
            return "FATAL       ";
    }
}

void ColorLogSink::Send(const absl::LogEntry& entry) {
    if (mVerbose) {
        auto now = entry.timestamp();
        auto location = absl::StrFormat("%s:%d", entry.source_basename(),
                                        entry.source_line());
        *mOutputStream << Color(entry.log_severity())
                       << absl::FormatTime("%H:%M:%E6S", now,
                                           absl::LocalTimeZone())
                       << absl::StreamFormat(" %d %s ", entry.tid(),
                                             TranslateSeverity(entry))
                       << absl::StreamFormat("%s:%d | ",
                                             entry.source_basename(),
                                             entry.source_line())
                       << entry.text_message_with_newline();

    } else {
        *mOutputStream << Color(entry.log_severity())
                       << TranslateSeverity(entry) << " | "
                       << entry.text_message_with_newline();
    }

    if (mUseColor) {
        *mOutputStream << kColorNormal;
    }
    if (entry.log_severity() >= absl::LogSeverity::kWarning) {
        mOutputStream->flush();
    }
}

std::string_view ColorLogSink::Color(absl::LogSeverity severity) const {
    if (!mUseColor) {
        return kColorNoFormat;
    }
    switch (severity) {
        case absl::LogSeverity::kInfo:
            return kColorNormal;
        case absl::LogSeverity::kWarning:
            return kColorNormalYellow;
        case absl::LogSeverity::kError:
            return kColorNormalRed;
        case absl::LogSeverity::kFatal:
            return kColorBoldRed;
        default:
            return kColorNoFormat;
    }
}

};  // namespace base
}  // namespace android
