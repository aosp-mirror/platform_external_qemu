// Copyright 2015 The Android Open Source Project
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

#include "android/crashreport/CrashService_darwin.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"

#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <memory>
#include <sstream>

#include <stdint.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

#define HWINFO_CMD "system_profiler -detailLevel mini"

namespace android {

using ::android::base::PathUtils;
using ::android::base::System;

namespace crashreport {

HostCrashService::~HostCrashService() {
}

void HostCrashService::OnClientDumpRequest(
        void* context,
        const int& client_info,
        const std::string& file_path) {
}

void HostCrashService::OnClientExit(
        void* context,
        const int& client_info) {
}

bool HostCrashService::startCrashServer(const std::string& pipe) {
    return false;
}

bool HostCrashService::stopCrashServer() {
    return false;
}

bool HostCrashService::isClientAlive() {
    return false;
}

bool HostCrashService::getHWInfo() {
    return false;
}

// Convenience function to convert a value to a value in kilobytes
template<typename T>
static T toKB(T value) {
    return value / 1024;
}

bool HostCrashService::getMemInfo() {
    return false;
}


}  // namespace crashreport
}  // namespace android
