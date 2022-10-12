// Copyright 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "aemu/base/Compiler.h"

#include <iosfwd>
#include "android/base/system/System.h"
#include "android/metrics/export.h"
#include "android/session_phase_reporter.h"

// Forward declarations for protobufs
namespace android_studio {

class EmulatorHost;
class EmulatorDetails;
class EmulatorPerformanceStats;

}  // namespace android_studio

namespace android {
namespace CommonReportedInfo {

// Covers host hw info, AVD info
AEMU_METRICS_API void setHostInfo(const android_studio::EmulatorHost* hostinfo);
AEMU_METRICS_API void setDetails(const android_studio::EmulatorDetails* details);
AEMU_METRICS_API void setPerformanceStats(const android_studio::EmulatorPerformanceStats* stats);
void setUptime(base::System::Duration uptime);
void setSessionPhase(AndroidSessionPhase phase);
void appendMemoryUsage();

// For testing and crash reporter integration
AEMU_METRICS_API void writeHostInfo(std::string* res);
AEMU_METRICS_API void writeHostInfo(std::ostream* out);

AEMU_METRICS_API void writeDetails(std::string* res);
AEMU_METRICS_API void writeDetails(std::ostream* out);

void writePerformanceStats(std::string* res);
void writePerformanceStats(std::ostream* out);

AEMU_METRICS_API void readHostInfo(const std::string& res);
AEMU_METRICS_API void readHostInfo(std::istream* in);

AEMU_METRICS_API void readDetails(const std::string& res);
AEMU_METRICS_API void readDetails(std::istream* in);

void readPerformanceStats(const std::string& res);
void readPerformanceStats(std::istream* in);

}  // namespace CommonReportedInfo
}  // namespace android
