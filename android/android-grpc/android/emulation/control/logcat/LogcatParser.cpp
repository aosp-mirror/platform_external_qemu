// Copyright (C) 2019 The Android Open Source Project
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
#include "android/emulation/control/logcat/LogcatParser.h"
#include <cinttypes>
#include <ctime>
#include <iomanip>
#include <locale>
#include <regex>
#include <sstream>
#include <utility>
#include <vector>
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

static auto parseLevel(std::string level) {
    if (level.size() != 1)
        return LogcatEntry::UNKNOWN;
    switch (level[0]) {
        case '?':
            return LogcatEntry::UNKNOWN;
        case 'V':
            return LogcatEntry::VERBOSE;
        case 'D':
            return LogcatEntry::DEBUG;
        case 'I':
            return LogcatEntry::INFO;
        case 'W':
            return LogcatEntry::WARN;
        case 'E':
            return LogcatEntry::ERR;
        case 'F':
            return LogcatEntry::FATAL;
        case 'S':
            return LogcatEntry::SILENT;
        default:
            return LogcatEntry::UNKNOWN;
    }
}

static uint64_t parseTimeToEpochMs(std::string timestr) {
    // Initialize tm to include current year, as logcat will not give use this.
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    tm->tm_isdst = -1;
    uint64_t msec;

    // 10-11 11:23:29.463
    if (sscanf(timestr.c_str(), "%d-%d %d:%d:%d.%" SCNd64, &tm->tm_mon,
               &tm->tm_mday, &tm->tm_hour, &tm->tm_min, &tm->tm_sec,
               &msec) != 6) {
        return 0;
    };
    uint64_t time = static_cast<uint64_t>(std::mktime(tm)) * 1000;
    time += msec;
    return time;
}

std::pair<int, std::vector<LogcatEntry>> LogcatParser::parseLines(
        const std::string lines) {
    static const std::regex logline(
            // timestamp[1]
            "^(\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}.\\d{3})"
            // pid/tid and log level [2-4]
            "(?:\\s+[0-9A-Za-z]+)?\\s+(\\d+)\\s+(\\d+)\\s+([A-Z])\\s+"
            // tag and message [5-6]
            "(.+?)\\s*: (.*)$");

    int skip = 0;
    auto start = lines.begin();
    auto end = lines.begin();
    std::vector<LogcatEntry> result;
    while (end != lines.end()) {
        if (*end == '\n') {
            std::smatch m;
            if (std::regex_match(start, end, m, logline)) {
                LogcatEntry entry;
                entry.set_timestamp(parseTimeToEpochMs(m[1]));
                entry.set_pid(std::stoi(m[2]));
                entry.set_tid(std::stoi(m[3]));
                entry.set_level(parseLevel(m[4]));
                entry.set_tag(m[5]);
                entry.set_msg(m[6]);
                result.push_back(entry);
            }
            start = ++end;
        } else {
            end++;
        }
    }

    skip += (end - lines.begin()) - (end - start);
    return std::make_pair(skip, result);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
