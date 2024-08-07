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
#include "android/base/logging/StudioLogSink.h"
#include <iostream>

namespace android {
namespace base {

std::string_view StudioLogSink::TranslateSeverity(
        const absl::LogEntry& entry) const {
    switch (entry.log_severity()) {
        case absl::LogSeverity::kInfo:
            return "USER_INFO   ";
        case absl::LogSeverity::kWarning:
            return "USER_WARNING";
        case absl::LogSeverity::kError:
            return "USER_ERROR  ";
        case absl::LogSeverity::kFatal:
            return "FATAL       ";
    }
}


StudioLogSink* studio_sink() {
    static StudioLogSink studioLog(&std::cout);
    return &studioLog;
}


}  // namespace base
}  // namespace android