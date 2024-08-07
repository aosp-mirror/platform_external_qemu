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
#include "absl/log/log_sink.h"
#include "android/base/logging/ColorLogSink.h"
#include "android/base/logging/LoggingApi.h"
namespace android {
namespace base {

/**
 * @brief A custom log sink that formats messages intended to be shown to the
 * user.
 *
 * Messages will use a custom level where each level is prefixed with USER_
 *
 * For example:
 *
 * USER_INFO    | Hello this is an info message for the user
 * USER_WARNING | Hello this is a warning message for the user
 * USER_ERROR   | Hello this is an error message for the user
 */
LOGGING_API class StudioLogSink : public ColorLogSink {
public:
    StudioLogSink(std::ostream* stream) : ColorLogSink(stream, false) {}
    virtual ~StudioLogSink() = default;

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
    virtual std::string_view TranslateSeverity(
            const absl::LogEntry& entry) const;
};

/**
 * @brief Gets the singleton instance of the StudioLogSink.
 *
 * @return A pointer to the `StudioLogSink` singleton.
 */
LOGGING_API StudioLogSink* studio_sink();

namespace internal {

/**
 * @brief Sets the output stream for the StudioLogSink (for testing only).
 *
 * @param stream A pointer to the output stream to be used by the
 * `StudioLogSink`.
 *
 * @note This function is intended for testing purposes only and should not be
 * used in production code.
 */
LOGGING_API void setStudioLogStream(std::ostream* stream);

}  // namespace internal

}  // namespace base
}  // namespace android
