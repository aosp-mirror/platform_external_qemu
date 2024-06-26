// Copyright 2022 The Android Open Source Project
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

#include "android/base/system/System.h"

namespace android {
namespace base {

std::string getEnvironmentVariable(const std::string& key) {
    return System::getEnvironmentVariable(key);
}

void setEnvironmentVariable(const std::string& key, const std::string& value) {
    System::setEnvironmentVariable(key, value);
}

uint64_t getUnixTimeUs() {
    return System::get()->getUnixTimeUs();
}

uint64_t getHighResTimeUs() {
    return System::get()->getHighResTimeUs();
}

uint64_t getUptimeMs() {
    const System::Times times = System::get()->getProcessTimes();
    return (unsigned int)(times.wallClockMs);
}

std::string getProgramDirectory() {
    return System::get()->getProgramDirectory();
}

std::string getLauncherDirectory() {
    return System::get()->getLauncherDirectory();
}

bool getFileSize(int fd, uint64_t* size) {
    return System::get()->fileSize(fd, size);
}

void sleepMs(uint64_t ms) {
    return System::get()->sleepMs(ms);
}

void sleepUs(uint64_t us) {
    return System::get()->sleepUs(us);
}

CpuTime cpuTime() {
    return System::cpuTime();
}

bool queryFileVersionInfo(const char* filename, int* major, int* minor, int* build1, int* build2) {
    return System::queryFileVersionInfo(filename, major, minor, build1, build2);
}

int getCpuCoreCount() {
    return System::get()->getCpuCoreCount();
}

bool pathExists(const char* path) {
    return System::get()->pathExists(path);
}

} // namespace base
} // namespace android
