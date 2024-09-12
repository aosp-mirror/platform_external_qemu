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
#pragma once
#include <ostream>
#include <string_view>

#include "absl/log/log_sink.h"

#include "android/base/logging/LoggingApi.h"

namespace android {
namespace base {

/**
 * @brief A custom log sink that can format messages using ANSI colors.
 *
 * The sink will output to std::cout.
 */
LOGGING_API class ColorLogSink : public absl::LogSink {
public:
    /**
     * @brief Construct a new Color Log Sink object.
     *
     * You usually only want to use color when isatty(fileno(stdout)) is true.
     *
     * @param useColor Whether to use ANSI color codes in the output.
     */
    ColorLogSink(std::ostream* stream, bool useColor) : mOutputStream(stream), mUseColor(useColor) {}
    virtual ~ColorLogSink() = default;

    /**
     * @brief Sends a formatted log entry to the output stream.
     *
     * @param entry The log entry to be formatted and sent.
     */
    void Send(const absl::LogEntry& entry) override;

    /**
     * @brief Ansi color used for a given severity.
     *
     * Note this can be empty if useColor is false.
     *
     * @param severity The log severity
     * @return std::string_view The ANSI color code for the given severity.
     */
    std::string_view Color(absl::LogSeverity severity) const;

    /**
     * Translates the severity of a log entry into a human-readable string
     * representation.
     *
     * This function extracts the severity level from the provided log entry and
     * converts it into a corresponding text label, such as "INFO", "WARNING",
     * or "ERROR". The translation is independent of any color formatting that
     * might be applied by the log sink.
     *
     * @param entry The log entry whose severity level needs to be translated.
     * @return A string_view containing the textual representation of the
     * severity.
     */
    virtual std::string_view TranslateSeverity(const absl::LogEntry& entry) const;


    void SetVerbosity(bool verbose) { mVerbose = verbose; };

    void SetOutputStream(std::ostream* stream) { mOutputStream = stream; };

private:
    std::ostream* mOutputStream;
    bool mUseColor;
    bool mVerbose{false};
};
}  // namespace base
}  // namespace android