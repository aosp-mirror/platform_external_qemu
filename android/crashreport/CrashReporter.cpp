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

#include "android/base/memory/LazyInstance.h"
#include "android/base/String.h"
#include "android/base/system/System.h"
#ifdef _WIN32
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#endif
#include "android/crashreport/CrashSystem.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

#if defined(__linux__)
#include "client/linux/crash_generation/crash_generation_server.h"
#include "client/linux/handler/exception_handler.h"
#elif defined(__APPLE__)
#include "client/mac/crash_generation/crash_generation_server.h"
#include "client/mac/handler/exception_handler.h"
#elif defined(_WIN32)
#include "client/windows/crash_generation/crash_generation_server.h"
#include "client/windows/handler/exception_handler.h"
#else
#error Breakpad not supported on this platform
#endif

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

// Taken from breakpad
#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif

#define PROD 1
#define STAGING 2

#define WAITINTERVAL_MS 2

namespace android {
namespace crashreport {

namespace {

google_breakpad::ExceptionHandler* mHandler = NULL;

// spawnService was extracted from android::base::System::runSilentCommand
// Spawning crash service required the following modifications
// Win32 - Inheritable handles so we could get access to the parent process
//        handle
//        Added some extra flags for hiding console
// Linux - Need to allow child process to ptrace the parent process
bool spawnService(const ::android::base::StringVector& commandLine) {
    // Sanity check.
    if (commandLine.empty()) {
        return false;
    }

#ifdef _WIN32
    STARTUPINFOW startup;
    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWMINIMIZED;

    PROCESS_INFORMATION pinfo;
    ZeroMemory(&pinfo, sizeof(pinfo));

    const wchar_t* comspec = _wgetenv(L"COMSPEC");
    if (!comspec || !comspec[0]) {
        comspec = L"cmd.exe";
    }

    // Run the command.
    ::android::base::String command = "/C";
    for (size_t n = 0; n < commandLine.size(); ++n) {
        command += " ";
        command += android::base::Win32Utils::quoteCommandLine(
                commandLine[n].c_str());
    }

    ::android::base::Win32UnicodeString command_unicode(command);

    // NOTE: CreateProcessW expects a _writable_ pointer as its second
    // parameter, so use .data() instead of .c_str().
    if (!CreateProcessW(comspec,                /* program path */
                        command_unicode.data(), /* command line args */
                        NULL, /* process handle is not inheritable */
                        NULL, /* thread handle is not inheritable */
                        TRUE, /* Yes Inherit handles */
                        CREATE_NO_WINDOW, /* the new process doesn't
                                             have a console */
                        NULL,             /* use parent's environment block */
                        NULL,             /* use parent's starting directory */
                        &startup,         /* startup info, i.e. std handles */
                        &pinfo)) {
        return false;
    }

    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);

    return true;
#else  // !_WIN32
    char** params = new char*[commandLine.size() + 1];
    for (size_t n = 0; n < commandLine.size(); ++n) {
        params[n] = (char*)commandLine[n].c_str();
    }
    params[commandLine.size()] = NULL;

    int pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid != 0) {
#ifdef __linux__
        // Taken from breakpad to allow child to ptrace parent
        sys_prctl(PR_SET_PTRACER, pid, 0, 0, 0);
#endif
        // Parent process returns immediately.
        delete[] params;
        return true;
    }

    // In the child process.
    int fd = open("/dev/null", O_WRONLY);
    dup2(fd, 1);
    dup2(fd, 2);

    execvp(commandLine[0].c_str(), params);
    // Should not happen.
    exit(1);
#endif  // !_WIN32
}

bool filterCallback(void* context) {
    return true;
}

#if defined(__linux__)
bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                  void* context,
                  bool succeeded) {
    D("Crash detected. Minidump saved to: %s\n", descriptor.path());
    return true;
}
#elif defined(_WIN32)
bool dumpCallback(const wchar_t* dump_path,
                  const wchar_t* minidump_id,
                  void* context,
                  EXCEPTION_POINTERS* exinfo,
                  MDRawAssertionInfo* assertion,
                  bool succeeded) {
    return true;
}
#elif defined(__APPLE)
bool dumpCallback(const char* dump_dir,
                  const char* minidump_id,
                  void* context,
                  bool succeeded) {
    return true;
}
#endif

bool start() {
    // Check if already started
    if (mHandler != NULL) {
        return false;
    }

    if (!CrashSystem::get()->validatePaths()) {
        return false;
    }

    // Start the crash_service
    ::android::base::StringVector cmdline;

#if defined(__linux__)
    int server_fd;
    int client_fd;
    if (!google_breakpad::CrashGenerationServer::CreateReportChannel(
                &server_fd, &client_fd)) {
        E("CrashReporter CreateReportChannel failed!\n");
        return false;
    }
    const std::string pipename = CrashSystem::get()->getPipeName(server_fd);
#elif defined(__APPLE__)
    const std::string pipename = CrashSystem::get()->getPipeName(getpid());
#elif defined(_WIN32)
    const std::string pipename =
            CrashSystem::get()->getPipeName(GetCurrentProcessId());
#endif

#if defined(__linux__) || defined(__APPLE__)
    const std::string procident = std::to_string(getpid());
#elif defined(_WIN32)
    // Did not find a PPID equivalent for Windows
    // Instead using process to self, inherited to child
    HANDLE pseudoProcHandle = GetCurrentProcess();
    HANDLE procHandle;
    bool rSuccess = DuplicateHandle(pseudoProcHandle, pseudoProcHandle,
                                    pseudoProcHandle, &procHandle, 0, true,
                                    DUPLICATE_SAME_ACCESS);
    if (!rSuccess) {
        E("Coudln't duplicate proc handle\n");
        return false;
    }
    const std::string procident =
            std::to_string(reinterpret_cast<std::size_t>(procHandle));
#endif

    cmdline = CrashSystem::get()->getCrashServiceCmdLine(pipename, procident);
    if (!spawnService(cmdline)) {
        E("Could not spawn crash service\n");
        return false;
    }

    // Wait for crash service to come up
    bool serviceReady = false;
#if defined(_WIN32)
    for (int i = 0; i < 10; i++) {
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
    for (int i = 0; i < 10; i++) {
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

    if (!serviceReady) {
        E("Crash service did not start\n");
        return false;
    }

// Attach the crash handler
#if defined(__linux__)
    mHandler = new google_breakpad::ExceptionHandler(
            google_breakpad::MinidumpDescriptor(
                    CrashSystem::get()->getCrashDirectory()),
            &filterCallback, &dumpCallback, nullptr, true, client_fd);
#elif defined(_WIN32)
    mHandler = new google_breakpad::ExceptionHandler(
            CrashSystem::get()->getWCrashDirectory(), NULL, NULL, NULL,
            google_breakpad::ExceptionHandler::HANDLER_ALL, MiniDumpNormal,
            ::android::base::Win32UnicodeString(pipename.c_str(),
                                                pipename.length())
                    .c_str(),
            NULL);
#elif defined(__APPLE__)
    mHandler = new google_breakpad::ExceptionHandler(
            CrashSystem::get()->getCrashDirectory(), NULL, NULL,
            NULL,  // no callback context
            true,  // install signal handlers
            pipename.c_str());
#endif
    return mHandler != NULL;
}

}  // anonymous

}  // crashreport
}  // android

extern "C" bool crashhandler_init(void) {
#if CRASHUPLOAD == PROD || CRASHUPLOAD == STAGING
    return ::android::crashreport::start();
#else
    return false;
#endif
};
