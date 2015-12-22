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

#include "android/crashreport/CrashSystem.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#ifdef _WIN32
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#endif
#include "android/emulation/ConfigDirs.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"

#if defined(__linux__)
#include "client/linux/crash_generation/crash_generation_server.h"
#endif

#ifdef _WIN32
#include <process.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#import <Carbon/Carbon.h>
#endif  // __APPLE__

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

namespace android {
namespace crashreport {

namespace {

using namespace android::base;

const char kCrashURL_prod[] = "https://clients2.google.com/cr/report";
const char kCrashURL_staging[] =
        "https://clients2.google.com/cr/staging_report";

#if defined(_WIN32)
const char kPipeName[] = "\\\\.\\pipe\\AndroidEmulatorCrashService";
#elif defined(__APPLE__)
const char kPipeName[] = "com.google.AndroidEmulator.CrashService";
#endif

const char kCrashSubDir[] = "breakpad";

class HostCrashSystem : public CrashSystem {
public:
    HostCrashSystem() : mCrashDir(), mCrashURL() {}

    virtual ~HostCrashSystem() {}

    virtual const std::string& getCrashDirectory() override {
        if (mCrashDir.empty()) {
            mCrashDir.assign(::android::ConfigDirs::getUserDirectory().c_str());
            mCrashDir += System::kDirSeparator;
            mCrashDir += kCrashSubDir;
        }
        return mCrashDir;
    }

    virtual const std::string& getCrashURL() override {
        if (mCrashURL.empty()) {
            if (CrashType::CRASHUPLOAD == CrashType::PROD) {
                mCrashURL.assign(kCrashURL_prod);
            } else if (CrashType::CRASHUPLOAD == CrashType::STAGING) {
                mCrashURL.assign(kCrashURL_staging);
            }
        }
        return mCrashURL;
    }

    virtual bool validatePaths() override {
        bool valid = CrashSystem::validatePaths();
        std::string crashDir = getCrashDirectory();
        if (!System::get()->pathExists(crashDir)) {
            if (path_mkdir_if_needed(crashDir.c_str(), 0744) == -1) {
                W("Couldn't create crash dir %s\n", crashDir.c_str());
                valid = false;
            }
        } else if (System::get()->pathIsFile(crashDir)) {
            W("Crash dir is a file! %s\n", crashDir.c_str());
            valid = false;
        }
        return valid;
    }

private:
    std::string mCrashDir;
    std::string mCrashURL;
};

LazyInstance<HostCrashSystem> sCrashSystem = LAZY_INSTANCE_INIT;
CrashSystem* sCrashSystemForTesting = nullptr;

}  // namespace

// spawnService was extracted from android::base::System::runSilentCommand
// Spawning crash service required the following modification:
//    Returns pid of spawned process on success, 0 otherwise
//
int CrashSystem::spawnService(
        const ::android::base::StringVector& commandLine) {
    // Sanity check.
    if (commandLine.empty()) {
        return 0;
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
                        nullptr, /* process handle is not inheritable */
                        nullptr, /* thread handle is not inheritable */
                        FALSE,   /* Do not Inherit handles */
                        CREATE_NO_WINDOW, /* the new process doesn't
                                             have a console */
                        nullptr,          /* use parent's environment block */
                        nullptr,          /* use parent's starting directory */
                        &startup,         /* startup info, i.e. std handles */
                        &pinfo)) {
        return 0;
    }

    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);

    return pinfo.dwProcessId;
#else   // !_WIN32
    char** params = new char*[commandLine.size() + 1];
    for (size_t n = 0; n < commandLine.size(); ++n) {
        params[n] = (char*)commandLine[n].c_str();
    }
    params[commandLine.size()] = nullptr;

    int pid = fork();
    if (pid != 0) {
        // Parent process returns immediately.
        delete[] params;
        return pid;
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

const std::string& CrashSystem::getCaBundlePath() {
    if (mCaBundlePath.empty()) {
        mCaBundlePath.assign(System::get()->getLauncherDirectory().c_str());
        mCaBundlePath += System::kDirSeparator;
        mCaBundlePath += "lib";
        mCaBundlePath += System::kDirSeparator;
        mCaBundlePath += "ca-bundle.pem";
    }
    return mCaBundlePath;
}

const std::string& CrashSystem::getCrashServicePath() {
    if (mCrashServicePath.empty()) {
        mCrashServicePath.assign(
                ::android::base::System::get()->getLauncherDirectory().c_str());
        mCrashServicePath += System::kDirSeparator;
        mCrashServicePath += "emulator";
        if (::android::base::System::kProgramBitness == 64) {
            mCrashServicePath += "64";
        }
        mCrashServicePath += "-crash-service";
#ifdef _WIN32
        mCrashServicePath += ".exe";
#endif
    }
    return mCrashServicePath;
}

const std::string& CrashSystem::getProcessId() {
    if (mProcessId.empty()) {
#if defined(__linux__) || defined(__APPLE__)
        mProcessId = std::to_string(getpid());
#elif defined(_WIN32)
        mProcessId = std::to_string(GetCurrentProcessId());
#endif
    }
    return mProcessId;
}

const CrashSystem::CrashPipe& CrashSystem::getCrashPipe() {
    if (!mCrashPipe.isValid()) {
#if defined(__linux__)
        int server_fd;
        int client_fd;
        if (!google_breakpad::CrashGenerationServer::CreateReportChannel(
                    &server_fd, &client_fd)) {
            W("CrashReporter CreateReportChannel failed!\n");
        } else {
            mCrashPipe = CrashPipe(server_fd, client_fd);
        }
#elif defined(__APPLE__) || defined(_WIN32)
        std::string pipename;
        pipename.append(kPipeName);
        pipename.append(".");
        pipename.append(getProcessId());
        mCrashPipe = CrashPipe(pipename);
#endif
    }
    return mCrashPipe;
}

const StringVector CrashSystem::getCrashServiceCmdLine(
        const std::string& pipe,
        const std::string& proc) {
    StringVector cmdline;
    cmdline.append(::android::base::StringView(getCrashServicePath().c_str()));
    cmdline.append(::android::base::StringView("-pipe"));
    cmdline.append(::android::base::StringView(pipe.c_str()));
    cmdline.append(::android::base::StringView("-ppid"));
    cmdline.append(::android::base::StringView(proc.c_str()));
    return cmdline;
}

bool CrashSystem::validatePaths() {
    bool valid = true;
    std::string caBundlePath = getCaBundlePath();
    std::string crashServicePath = getCrashServicePath();
    if (!System::get()->pathIsFile(caBundlePath)) {
        W("Couldn't find file %s\n", caBundlePath.c_str());
        valid = false;
    }
    if (!System::get()->pathIsFile(crashServicePath)) {
        W("Couldn't find crash service executable %s\n",
          crashServicePath.c_str());
        valid = false;
    }
    return valid;
}

CrashSystem* CrashSystem::get() {
    CrashSystem* result = sCrashSystemForTesting;
    if (!result) {
        result = sCrashSystem.ptr();
    }
    return result;
}

// static
bool CrashSystem::isDump(const std::string& path) {
    static const char kDumpSuffix[] = ".dmp";
    static const int kDumpSuffixSize = sizeof(kDumpSuffix) - 1;
    return System::get()->pathIsFile(path) && path.size() > kDumpSuffixSize &&
           (path.compare(path.size() - kDumpSuffixSize, kDumpSuffixSize,
                         kDumpSuffix) == 0);
}

// static
CrashSystem* CrashSystem::setForTesting(CrashSystem* crashsystem) {
    CrashSystem* result = sCrashSystemForTesting;
    sCrashSystemForTesting = crashsystem;
    return result;
}

}  // namespace crashreport
}  // namespace android
