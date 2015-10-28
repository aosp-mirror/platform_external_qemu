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

#include "android/base/memory/LazyInstance.h"
#include "android/base/String.h"
#include "android/base/system/System.h"
#ifdef _WIN32
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#endif
#include "android/crashreport/CrashPaths.h"
#include "android/utils/debug.h"

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

//Taken from breakpad
#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif

#define PROD 1
#define STAGING 2

google_breakpad::ExceptionHandler* mHandler;

CrashReporter::CrashReporter() {}

bool CrashReporter::spawnService(const ::android::base::StringVector& commandLine) {
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
            command += android::base::Win32Utils::quoteCommandLine(commandLine[n].c_str());
        }

        ::android::base::Win32UnicodeString command_unicode(command);

        // NOTE: CreateProcessW expects a _writable_ pointer as its second
        // parameter, so use .data() instead of .c_str().
        if (!CreateProcessW(comspec,                /* program path */
                            command_unicode.data(), /* command line args */
                            NULL,  /* process handle is not inheritable */
                            NULL,  /* thread handle is not inheritable */
                            FALSE, /* no, don't inherit any handles */
                            CREATE_NO_WINDOW, /* the new process doesn't
                                                 have a console */
                            NULL,     /* use parent's environment block */
                            NULL,     /* use parent's starting directory */
                            &startup, /* startup info, i.e. std handles */
                            &pinfo)) {
            return false;
        }

        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);

        return true;
#else  // !_WIN32
        char** params = new char*[commandLine.size()+1];
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
            //Taken from breakpad
            sys_prctl(PR_SET_PTRACER, pid, 0, 0, 0);
#endif
            // Parent process returns immediately.
            delete [] params;
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

bool filterCallback(void *context)
{
    //printf ("Filtering callback\n");
    return true;
}

#if defined(__linux__)
bool dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
            void *context,
            bool succeeded) {
    printf ("Crash detected. Minidump saved to: %s\n", descriptor.path());
    return true;
}
#elif defined(_WIN32)
bool dumpCallback(const wchar_t *dump_path,
            const wchar_t *minidump_id,
            void *context,
            EXCEPTION_POINTERS *exinfo,
            MDRawAssertionInfo *assertion,
            bool succeeded) {
    return true;
}
#elif defined(__APPLE)
bool dumpCallback(const char *dump_dir,
            const char *minidump_id,
            void *context,
            bool succeeded) {
    return true;
}
#endif

bool CrashReporter::start() {

    //Start the crash_service
    ::android::base::StringVector cmdline;
    ::android::base::String crashServicePath;

#if defined(__linux__)
    int server_fd;
    int client_fd;
    char server_fd_string [8];

    if (!google_breakpad::CrashGenerationServer::CreateReportChannel(&server_fd, &client_fd)) {
        E("CrashReporter CreateReportChannel failed!\n");
        return false;
    }

    sprintf(server_fd_string, "%d", server_fd);
    ::android::base::String server_fd_str(server_fd_string);
#elif defined(_WIN32)
    const ::android::base::String pipename(
          ::android::base::Win32UnicodeString::convertToUtf8(CrashPaths::kWPipeName));
#elif defined(__APPLE__)
    char mach_port_name[128];
    sprintf(mach_port_name, "com.google.AndroidEmulator.CrashServer.%d", getpid());
#endif

    crashServicePath.assign(::android::base::System::get()->getProgramDirectory().c_str());
    crashServicePath+=::android::base::System::kDirSeparator;
    crashServicePath+="emulator";
    if (::android::base::System::get()->getHostBitness() == 64) {
      crashServicePath+="64";
    }
    crashServicePath+="-crash-service";
#ifdef _WIN32
    crashServicePath+= ".exe";
#endif
    cmdline.append(crashServicePath);


#if defined(__linux__)
    cmdline.append(::android::base::String(server_fd_str));
#elif defined(_WIN32)
    cmdline.append(pipename);
#elif defined(__APPLE__)
    cmdline.append(::android::base::String(mach_port_name));
#endif

    if (!spawnService(cmdline)) {
        E("Could not start command %s\n", crashServicePath.c_str());
        return false;
    }
    //Attach the crash handler

#if defined(__linux__)
    mHandler =  \
        new google_breakpad::ExceptionHandler(
        google_breakpad::MinidumpDescriptor(
            CrashPaths::get()->getCrashDirectory()),
        &filterCallback,
        &dumpCallback,
        nullptr,
        true,
        client_fd);
#elif defined(_WIN32)
    //TODO what is a better way to guarantee this?
    //TODO pipe name needs ot have process attached
    for (int i = 0; i < 10; i ++) {
        if (::android::base::System::get()->pathIsFile(CrashPaths::kPipeName)) {
            break;
        }
        Sleep(50);
    }
    mHandler = \
        new google_breakpad::ExceptionHandler(
        CrashPaths::get()->getWCrashDirectory(),
        NULL,
        NULL,
        NULL,
        google_breakpad::ExceptionHandler::HANDLER_ALL,
        MiniDumpNormal,
        CrashPaths::kWPipeName,
        NULL);
#elif defined(__APPLE__)
    //TODO what is a better way to do this?
    //Alternative to this poll loop is to do the following:
    //  Create a temporary receiveport before spawn
    //  Wait for message on receive port after spawn
    //  In the crash server create a receive port ()
    //  In the crash server use bootstrap_look_up to get temporary port as a send right
    //  In the crash server send a message on the temporary send right
    //  Once we receive the message, use the new name to create the exception
    //  handler
    mach_port_t task_bootstrap_port = 0;
    mach_port_t port;
    task_get_bootstrap_port(mach_task_self(), &task_bootstrap_port);
    for (int i = 0; i < 10; i ++) {
        if (bootstrap_look_up(task_bootstrap_port, mach_port_name, &port) == KERN_SUCCESS) {
            break;
        }
        usleep(50*1000);
    }
    mHandler = \
        new google_breakpad::ExceptionHandler(
        CrashPaths::get()->getCrashDirectory(),
        NULL, NULL,
        NULL,    // no callback context
        true,    // install signal handlers
        mach_port_name
        );
#endif
    return mHandler != NULL;
}

void CrashReporter::stop() {

}

::android::base::LazyInstance<CrashReporter> gCrashReporter = LAZY_INSTANCE_INIT;

CrashReporter* CrashReporter::get() {
    return gCrashReporter.ptr();
}
extern "C" bool crashhandler_init(void) {
#if CRASHUPLOAD == PROD || CRASHUPLOAD == STAGING
    return CrashReporter::get()->start();
#else
    return false;
#endif
};
