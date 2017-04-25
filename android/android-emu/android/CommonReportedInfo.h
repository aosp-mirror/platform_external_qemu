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

#include "android/base/Compiler.h"

#include "android/metrics/proto/studio_stats.pb.h"
#include "android/base/synchronization/Lock.h"

#include <fstream>

namespace android {

// A global class to track the protobuf reported info that is common
// to both metrics and crash reporting.
class CommonReportedInfo {
public:
    CommonReportedInfo() = default;
    static CommonReportedInfo* get();

    // Covers host hw info, AVD info
    void setHostInfo(const android_studio::EmulatorHost& hostinfo);
    void setDetails(const android_studio::EmulatorDetails& details);
    void setPerformanceStats(const android_studio::EmulatorPerformanceStats& stats);
    void setUptime(uint64_t uptime);

    // TODO: UI-related stuff

    // For testing and crash reporter integration
    void writeHostInfo(std::string* res); void writeHostInfo(std::ofstream* out);
    void writeDetails(std::string* res); void writeDetails(std::ofstream* out);
    void writePerformanceStats(std::string* res); void writePerformanceStats(std::ofstream* out);

    void readHostInfo(const std::string& res); void readHostInfo(std::ifstream* in);
    void readDetails(const std::string& res); void readDetails(std::ifstream* in);
    void readPerformanceStats(const std::string& res); void readPerformanceStats(std::ifstream* in);

private:
    android::base::Lock mLock;

    DISALLOW_COPY_AND_ASSIGN(CommonReportedInfo);
};

} // namespace android
