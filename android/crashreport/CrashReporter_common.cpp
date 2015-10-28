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

#include "android/crashreport/CrashReporter.h"

#include "android/base/containers/StringVector.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

namespace android {
namespace crashreport {

CrashReporter::CrashReporter()
    : mDumpDir(::android::base::System::get()->getTempDir().c_str()) {}

CrashReporter::~CrashReporter() {}

const std::string& CrashReporter::getDumpDir() const {
    return mDumpDir;
}

extern "C" bool crashhandler_init() {
    if (CrashSystem::CrashType::CRASHUPLOAD == CrashSystem::CrashType::NONE) {
        return false;
    }

    if (!CrashSystem::get()->validatePaths()) {
        return false;
    }

    CrashSystem::CrashPipe crashpipe(CrashSystem::get()->getCrashPipe());

    const std::string procident = CrashSystem::get()->getProcessId();

    if (procident.empty()) {
        return false;
    }

    if (!crashpipe.isValid()) {
        return false;
    }

    ::android::base::StringVector cmdline =
            CrashSystem::get()->getCrashServiceCmdLine(crashpipe.mServer,
                                                       procident);

    int pid = CrashSystem::spawnService(cmdline);
    if (!pid) {
        E("Could not spawn crash service\n");
        return false;
    } else {
        CrashReporter::get()->setupChildCrashProcess(pid);
    }

    if (!CrashReporter::get()->waitServicePipeReady(crashpipe.mClient)) {
        E("Crash service did not start\n");
        return false;
    }

    return CrashReporter::get()->attachCrashHandler(crashpipe);
};

}  // namespace crashreport
}  // namespace android
