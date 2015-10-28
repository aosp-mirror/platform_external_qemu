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

#include "android/base/String.h"
#include "android/base/system/System.h"
#ifdef _WIN32
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#endif
#include "android/utils/debug.h"
#include "android/utils/system.h"

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)


#define PROD 1
#define STAGING 2

#define WAITEXPIRE_MS 500
#define WAITINTERVAL_MS 2

// Taken from breakpad
#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif

namespace android {
namespace crashreport {

CrashReporter::CrashReporter() : mHandler() {}

bool CrashReporter::waitServicePipeReady(const std::string& pipename,
                                         const int timeout_ms) {
    // Start Wait for crash service to come up
    bool serviceReady = false;
#if defined(_WIN32)
    for (int i = 0; i < timeout_ms/WAITINTERVAL_MS; i++) {
        if (::android::base::System::get()->pathIsFile(pipename.c_str())) {
            serviceReady = true;
            D("Crash Server Ready after %d ms\n", i*WAITINTERVAL_MS);
            break;
        }
        sleep_ms(WAITINTERVAL_MS);
    }
#elif defined(__APPLE__)
    mach_port_t task_bootstrap_port = 0;
    mach_port_t port;
    task_get_bootstrap_port(mach_task_self(), &task_bootstrap_port);
    for (int i = 0; i < timeout_ms/WAITINTERVAL_MS; i++) {
        if (bootstrap_look_up(task_bootstrap_port, pipename.c_str(), &port) ==
            KERN_SUCCESS) {
            serviceReady = true;
            D("Crash Server Ready after %d ms\n", i*WAITINTERVAL_MS);
            break;
        }
        sleep_ms(WAITINTERVAL_MS);
    }
#elif defined(__linux__)
    serviceReady = true;
#endif
    return serviceReady;
}

bool CrashReporter::attachCrashHandler(
        const CrashSystem::CrashPipe& crashpipe) {
    if (mHandler) {
        return false;
    }

    // Since we only have OOP, use a temporary directory if we end up
    // switching to in process mode.
    std::string dumpdir(::android::base::System::get()->getTempDir().c_str());

#if defined(__linux__)
    mHandler.reset(new google_breakpad::ExceptionHandler(
            google_breakpad::MinidumpDescriptor(dumpdir), NULL, NULL, nullptr,
            true, std::stoi(crashpipe.mClient)));
#elif defined(_WIN32)
    mHandler.reset(new google_breakpad::ExceptionHandler(
            ::android::base::Win32UnicodeString(dumpdir.c_str(),
                                                dumpdir.length())
                    .c_str(),
            NULL, NULL, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL,
            MiniDumpNormal,
            ::android::base::Win32UnicodeString(crashpipe.mClient.c_str(),
                                                crashpipe.mClient.length())
                    .c_str(),
            nullptr));
#elif defined(__APPLE__)
    mHandler.reset(new google_breakpad::ExceptionHandler(
            dumpdir, NULL, NULL,
            nullptr,  // no callback context
            true,     // install signal handlers
            crashpipe.mClient.c_str()));
#endif
    return mHandler != nullptr;
}

CrashReporter* CrashReporter::get(void) {
    static CrashReporter instance;
    return &instance;
}

extern "C" bool crashhandler_init(void) {
#if CRASHUPLOAD == PROD || CRASHUPLOAD == STAGING

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

    ::android::base::StringVector cmdline;
    cmdline = CrashSystem::get()->getCrashServiceCmdLine(crashpipe.mServer,
                                                         procident);

    int pid = CrashSystem::spawnService(cmdline);
    if (!pid) {
        E("Could not spawn crash service\n");
        return false;
    } else {
#ifdef __linux__
        // Taken from breakpad to allow child to ptrace parent
        sys_prctl(PR_SET_PTRACER, pid, 0, 0, 0);
#endif
    }

    if (!CrashReporter::get()->waitServicePipeReady(crashpipe.mClient,
                                                    WAITEXPIRE_MS)) {
        E("Crash service did not start\n");
        return false;
    }

    return CrashReporter::get()->attachCrashHandler(crashpipe);
#else
    return false;
#endif
};

}  // crashreport
}  // android
