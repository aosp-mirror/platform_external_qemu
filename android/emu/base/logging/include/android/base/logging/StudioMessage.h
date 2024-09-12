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

#include "absl/log/absl_log.h"
#include "android/base/logging/StudioLogSink.h"

/**
 * @def USER_MESSAGE
 * @brief Logs a message specifically formatted for Android Studio recognition.
 *
 * This macro simplifies the process of logging messages that Android Studio can
 * identify and display with appropriate UI elements based on the log level.
 * It utilizes the `StudioLogSink` to ensure the messages are correctly
 * formatted.
 *
 * @param verbose_level The desired verbosity level for the log message
 * (e.g., `INFO`, `WARNING`).
 *
 * Messages will appear on the console log as follows:
 *
 * USER_INFO    | Hello this is an info message for the user
 * USER_WARNING | Hello this is a warning message for the user
 * USER_ERROR   | Hello this is an error message for the user
 */
#define USER_MESSAGE(verbose_level)              \
    ABSL_LOG_INTERNAL_LOG_IMPL(_##verbose_level) \
            .ToSinkOnly(android::base::studio_sink())
