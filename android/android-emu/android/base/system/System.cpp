// Copyright (C) 2015 The Android Open Source Project
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

#include <inttypes.h>
#include "android/base/EintrWrapper.h"
#include "android/base/Log.h"
#include "android/base/StringFormat.h"
#include "android/base/StringParse.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/threads/Thread.h"
#include "android/crashreport/crash-handler.h"
#include "android/utils/file_io.h"
#include "android/utils/path.h"
#include "android/utils/tempfile.h"

#ifdef _WIN32
#include "android/base/files/ScopedRegKey.h"
#include "android/base/files/ScopedFileHandle.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#endif

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#include <psapi.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include <tlhelp32.h>
#endif

#ifdef __APPLE__
#import <Carbon/Carbon.h>
#include <libproc.h>
#include <mach/clock.h>
#include <mach/mach.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/sysctl.h>

// Instead of including this private header let's copy its important
// definitions in.
// #include <CoreFoundation/CFPriv.h>
extern "C" {
/* System Version file access */
CF_EXPORT CFDictionaryRef _CFCopySystemVersionDictionary(void);
CF_EXPORT CFDictionaryRef _CFCopyServerVersionDictionary(void);
CF_EXPORT const CFStringRef _kCFSystemVersionProductNameKey;
CF_EXPORT const CFStringRef _kCFSystemVersionProductVersionKey;
}  // extern "C"
#endif  // __APPLE__

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <vector>
#include <unordered_set>

#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <sys/statvfs.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _MSC_VER
#include "msvc-posix.h"
#include "dirent.h"
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#if defined (__linux__)
#include <fstream>
#include <string>
#include <sys/sysmacros.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#endif

// This variable is a pointer to a zero-terminated array of all environment
// variables in the current process.
// Posix requires this to be declared as extern at the point of use
// NOTE: Apple developer manual states that this variable isn't available for
// the shared libraries, and one has to use the _NSGetEnviron() function instead
#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#include <sys/utsname.h>
#else
extern "C" char** environ;
#endif

namespace android {
namespace base {

using std::string;
using std::unique_ptr;
using std::vector;

#ifdef __APPLE__
// Defined in system-native-mac.mm
Optional<System::DiskKind> nativeDiskKind(int st_dev);
#endif

namespace {

struct TickCountImpl {
private:
    System::WallDuration mStartTimeUs;
#ifdef _WIN32
    long long mFreqPerSec = 0;    // 0 means 'high perf counter isn't available'
#elif defined(__APPLE__)
    clock_serv_t mClockServ;
#endif

public:
    TickCountImpl() {
#ifdef _WIN32
        LARGE_INTEGER freq;
        if (::QueryPerformanceFrequency(&freq)) {
            mFreqPerSec = freq.QuadPart;
        }
#elif defined(__APPLE__)
        host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &mClockServ);
#endif
        mStartTimeUs = getUs();
    }

#ifdef __APPLE__
    ~TickCountImpl() {
        mach_port_deallocate(mach_task_self(), mClockServ);
    }
#endif

    System::WallDuration getStartTimeUs() const {
        return mStartTimeUs;
    }

    System::WallDuration getUs() const {
#ifdef _WIN32
    if (!mFreqPerSec) {
        return ::GetTickCount() * 1000;
    }
    LARGE_INTEGER now;
    ::QueryPerformanceCounter(&now);
    return (now.QuadPart * 1000000ull) / mFreqPerSec;
#elif defined __linux__
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000ll + ts.tv_nsec / 1000;
#else // APPLE
    mach_timespec_t mts;
    clock_get_time(mClockServ, &mts);
    return mts.tv_sec * 1000000ll + mts.tv_nsec / 1000;
#endif
    }
};

// This is, maybe, the only static variable that may not be a LazyInstance:
// it holds the actual timestamp at startup, and has to be initialized as
// soon as possible after the application launch.
static const TickCountImpl kTickCount;

}  // namespace

#ifdef _WIN32
// Check if we're currently running under Wine
static bool isRunningUnderWine() {
    // this is the only good way of detecting Wine: it exports a function
    // 'wine_get_version()' from its ntdll.dll
    // Note: the typedef and casting here are for documentation purposes:
    //  if you need to get the actual Wine version, you just already know the
    //  type, calling convention and arguments.
    using wineGetVersionFunc = const char* __attribute__((cdecl)) ();

    // Make sure we don't call FreeLibrary() for this handle as
    // GetModuleHandle() doesn't increment the reference count
    const HMODULE ntDll = ::GetModuleHandleW(L"ntdll.dll");
    if (!ntDll) {
        // some strange version of Windows, definitely not Wine
        return false;
    }

    if (const auto wineGetVersion = reinterpret_cast<wineGetVersionFunc*>(
                ::GetProcAddress(ntDll, "wine_get_version")) != nullptr) {
        return true;
    }
    return false;
}

static bool extractFullPath(std::string* cmd) {
    if (PathUtils::isAbsolute(*cmd)) {
        return true;
    } else {
        // try searching %PATH% and current directory for the binary
        const Win32UnicodeString name(*cmd);
        const Win32UnicodeString extension(PathUtils::kExeNameSuffix);
        Win32UnicodeString buffer(MAX_PATH);

        DWORD size = ::SearchPathW(nullptr, name.c_str(), extension.c_str(),
                          buffer.size() + 1, buffer.data(), nullptr);
        if (size > buffer.size()) {
            // function may ask for more space
            buffer.resize(size);
            size = ::SearchPathW(nullptr, name.c_str(), extension.c_str(),
                                 buffer.size() + 1, buffer.data(), nullptr);
        }
        if (size == 0) {
            // Couldn't find anything matching the passed name
            return false;
        }
        if (buffer.size() != size) {
            buffer.resize(size);
        }
        *cmd = buffer.toString();
    }
    return true;
}

#endif

namespace {

bool parseBooleanValue(const char *value, bool def) {
    if (0 == strcmp(value, "1")) return true;
    if (0 == strcmp(value, "y")) return true;
    if (0 == strcmp(value, "yes")) return true;
    if (0 == strcmp(value, "Y")) return true;
    if (0 == strcmp(value, "YES")) return true;

    if (0 == strcmp(value, "0")) return false;
    if (0 == strcmp(value, "n")) return false;
    if (0 == strcmp(value, "no")) return false;
    if (0 == strcmp(value, "N")) return false;
    if (0 == strcmp(value, "NO")) return false;

    return def;
}

class HostSystem : public System {
public:
    HostSystem() : mProgramDir(), mHomeDir(), mAppDataDir() {
        ::atexit(HostSystem::atexit_HostSystem);
        configureHost();
    }

    virtual ~HostSystem() {}

    const std::string& getProgramDirectory() const override {
        if (mProgramDir.empty()) {
            mProgramDir.assign(getProgramDirectoryFromPlatform());
        }
        return mProgramDir;
    }

    std::string getCurrentDirectory() const override {
#if defined(_WIN32)
        int currentLen = GetCurrentDirectoryW(0, nullptr);
        if (currentLen < 0) {
            // Could not get size of working directory. Something is really
            // fishy here, return an empty string.
            return std::string();
        }
        wchar_t* currentDir =
                static_cast<wchar_t*>(calloc(currentLen + 1, sizeof(wchar_t)));
        if (!GetCurrentDirectoryW(currentLen + 1, currentDir)) {
            // Again, some unexpected problem. Can't do much here.
            // Make the string empty.
            currentDir[0] = L'0';
        }

        std::string result = Win32UnicodeString::convertToUtf8(currentDir);
        ::free(currentDir);
        return result;
#else   // !_WIN32
        char currentDir[PATH_MAX];
        if (!getcwd(currentDir, sizeof(currentDir))) {
            return std::string();
        }
        return std::string(currentDir);
#endif  // !_WIN32
    }

    bool setCurrentDirectory(StringView directory) override {
#if defined(_WIN32)
        Win32UnicodeString directory_unicode(directory);
        return SetCurrentDirectoryW(directory_unicode.c_str());
#else   // !_WIN32
        char currentDir[PATH_MAX];
        return chdir(c_str(directory)) == 0;
#endif  // !_WIN32
    }

    const std::string& getLauncherDirectory() const override {
        if (mLauncherDir.empty()) {
            std::string launcherDirEnv = envGet("ANDROID_EMULATOR_LAUNCHER_DIR");
            if (!launcherDirEnv.empty()) {
                mLauncherDir = std::move(launcherDirEnv);
                return mLauncherDir;
            }

            const std::string& programDir = getProgramDirectory();
            std::string launcherName = PathUtils::toExecutableName("emulator");

            // Let's first check if this is qemu2 binary, which lives in
            // <launcher-dir>/qemu/<os>-<arch>/
            // look for the launcher in grandparent directory
            auto programDirVector = PathUtils::decompose(programDir);
            if (programDirVector.size() >= 2) {
                programDirVector.resize(programDirVector.size() - 2);
                std::string grandparentDir = PathUtils::recompose(programDirVector);
                programDirVector.push_back(launcherName);
                std::string launcherPath = PathUtils::recompose(programDirVector);
                if (pathIsFile(launcherPath)) {
                    mLauncherDir = std::move(grandparentDir);
                    return mLauncherDir;
                }
            }

            std::vector<StringView> pathList = {programDir, launcherName};
            std::string launcherPath = PathUtils::recompose(pathList);
            if (pathIsFile(launcherPath)) {
                mLauncherDir = programDir;
                return mLauncherDir;
            }

            mLauncherDir.assign("<unknown-launcher-dir>");
        }
        return mLauncherDir;
    }

    const std::string& getHomeDirectory() const override {
        if (mHomeDir.empty()) {
#if defined(_WIN32)
            // NOTE: SHGetFolderPathW always takes a buffer of MAX_PATH size,
            // so don't use a Win32UnicodeString here to avoid un-necessary
            // dynamic allocation.
            wchar_t path[MAX_PATH] = {0};
            // Query Windows shell for known folder paths.
            // SHGetFolderPath acts as a wrapper to KnownFolders;
            // this is preferred for simplicity and XP compatibility.
            if (SUCCEEDED(
                        SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path))) {
                mHomeDir = Win32UnicodeString::convertToUtf8(path);
            } else {
                // Fallback to windows-equivalent of HOME env var
                std::string homedrive = envGet("HOMEDRIVE");
                std::string homepath = envGet("HOMEPATH");
                if (!homedrive.empty() && !homepath.empty()) {
                    mHomeDir.assign(homedrive);
                    mHomeDir.append(homepath);
                }
            }
#elif defined(__linux__) || (__APPLE__)
            // Try getting HOME from env first
            const char* home = getenv("HOME");
            if (home != NULL) {
                mHomeDir.assign(home);
            } else {
                // If env HOME appears empty for some reason,
                // try getting HOME by querying system password database
                const struct passwd *pw = getpwuid(getuid());
                if (pw != NULL && pw->pw_dir != NULL) {
                    mHomeDir.assign(pw->pw_dir);
                }
            }
#else
#error "Unsupported platform!"
#endif
        }
        return mHomeDir;
    }

    const std::string& getAppDataDirectory() const override {
#if defined(_WIN32)
        if (mAppDataDir.empty()) {
            // NOTE: See comment in getHomeDirectory().
            wchar_t path[MAX_PATH] = {0};
            if (SUCCEEDED(
                        SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
                mAppDataDir = Win32UnicodeString::convertToUtf8(path);
            } else {
                const wchar_t* appdata = _wgetenv(L"APPDATA");
                if(appdata != NULL) {
                    mAppDataDir = Win32UnicodeString::convertToUtf8(appdata);
                }
            }
        }
#elif defined (__APPLE__)
        if (mAppDataDir.empty()) {
            // The equivalent of AppData directory in MacOS X is
            // under ~/Library/Preferences. Apple does not offer
            // a C/C++ API to query this location (in ObjC cocoa
            // applications NSSearchPathForDirectoriesInDomains
            // can be used), so we apply the common practice of
            // hard coding it
            mAppDataDir.assign(getHomeDirectory());
            mAppDataDir.append("/Library/Preferences");
        }
#elif defined(__linux__)
        ; // not applicable
#else
#error "Unsupported platform!"
#endif
        return mAppDataDir;
    }


    int getHostBitness() const override {
#ifdef __x86_64__
        return 64;
#elif defined(_WIN32)
        // Retrieves the path of the WOW64 system directory, which doesn't
        // exist on 32-bit systems.
        // NB: we don't really need the directory, we just want to see if
        //     Windows has it - so let's not even try to pass a buffer that
        //     is long enough; return value is the required buffer length.
        std::array<wchar_t, 1> directory;
        const unsigned len = GetSystemWow64DirectoryW(
                           directory.data(),
                           static_cast<unsigned>(directory.size()));
        if (len == 0) {
            return 32;
        } else {
            return 64;
        }

#else  // !__x86_64__ && !_WIN32
        // This function returns 64 if host is running 64-bit OS, or 32 otherwise.
        //
        // It uses the same technique in ndk/build/core/ndk-common.sh.
        // Here are comments from there:
        //
        // ## On Linux or Darwin, a 64-bit kernel (*) doesn't mean that the
        // ## user-land is always 32-bit, so use "file" to determine the bitness
        // ## of the shell that invoked us. The -L option is used to de-reference
        // ## symlinks.
        // ##
        // ## Note that on Darwin, a single executable can contain both x86 and
        // ## x86_64 machine code, so just look for x86_64 (darwin) or x86-64
        // ## (Linux) in the output.
        //
        // (*) ie. The following code doesn't always work:
        //     struct utsname u;
        //     int host_runs_64bit_OS = (uname(&u) == 0 &&
        //                              strcmp(u.machine, "x86_64") == 0);
        //
        // Note: system() call on MacOS disables SIGINT signal and fails
        //  to restore it back. As of now we don't have 32-bit Darwin binaries
        //  so this code path won't ever happen, but you've been warned.
        //
        if (system("file -L \"$SHELL\" | grep -q \"x86[_-]64\"") == 0) {
                return 64;
        } else if (system("file -L \"$SHELL\" > /dev/null")) {
            fprintf(stderr, "WARNING: Cannot decide host bitness because "
                    "$SHELL is not properly defined; 32 bits assumed.\n");
        }
        return 32;
#endif // !_WIN32
    }

    OsType getOsType() const override {
#ifdef _WIN32
        return OsType::Windows;
#elif defined(__APPLE__)
        return OsType::Mac;
#elif defined(__linux__)
        return OsType::Linux;
#else
        #error getOsType(): unsupported OS;
#endif
    }

    string getOsName() override {
      static string lastSuccessfulValue;
      if (!lastSuccessfulValue.empty()) {
        return lastSuccessfulValue;
      }
#ifdef _WIN32
        using android::base::ScopedRegKey;
        HKEY hkey = 0;
        LONG result =
                RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                             0, KEY_READ, &hkey);
        if (result != ERROR_SUCCESS) {
          string errorStr =
              StringFormat("Error: RegGetValueW failed %ld %s", result,
                           Win32Utils::getErrorString(result));
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }
        ScopedRegKey hOsVersionKey(hkey);

        DWORD osNameSize = 0;
        const WCHAR productNameKey[] = L"ProductName";
        result = RegGetValueW(hOsVersionKey.get(), nullptr, productNameKey,
                              RRF_RT_REG_SZ, nullptr, nullptr, &osNameSize);
        if (result != ERROR_SUCCESS && ERROR_MORE_DATA != result) {
          string errorStr =
              StringFormat("Error: RegGetValueW failed %ld %s", result,
                           Win32Utils::getErrorString(result));
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }

        Win32UnicodeString osName;
        osName.resize((osNameSize - 1) / sizeof(wchar_t));
        result = RegGetValueW(hOsVersionKey.get(), nullptr, productNameKey,
                              RRF_RT_REG_SZ, nullptr, osName.data(),
                              &osNameSize);
        if (result != ERROR_SUCCESS) {
          string errorStr =
              StringFormat("Error: RegGetValueW failed %ld %s", result,
                           Win32Utils::getErrorString(result));
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }
        lastSuccessfulValue = osName.toString();
        return lastSuccessfulValue;
#elif defined(__APPLE__)
        // Taken from https://opensource.apple.com/source/DarwinTools/DarwinTools-1/sw_vers.c
        /*
         * Copyright (c) 2005 Finlay Dobbie
         * All rights reserved.
         *
         * Redistribution and use in source and binary forms, with or without
         * modification, are permitted provided that the following conditions
         * are met:
         * 1. Redistributions of source code must retain the above copyright
         *    notice, this list of conditions and the following disclaimer.
         * 2. Redistributions in binary form must reproduce the above copyright
         *    notice, this list of conditions and the following disclaimer in the
         *    documentation and/or other materials provided with the distribution.
         * 3. Neither the name of Finlay Dobbie nor the names of his contributors
         *    may be used to endorse or promote products derived from this software
         *    without specific prior written permission.
         *
         * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
         * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
         * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
         * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
         * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
         * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
         * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
         * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
         * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
         * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
         * POSSIBILITY OF SUCH DAMAGE.
         */

        CFDictionaryRef dict = _CFCopyServerVersionDictionary();
        if (!dict) {
            dict = _CFCopySystemVersionDictionary();
        }
        if (!dict) {
             LOG(VERBOSE) << "Failed to get a version dictionary";
             return "<Unknown>";
        }

        CFStringRef str =
                CFStringCreateWithFormat(
                    nullptr, nullptr,
                    CFSTR("%@ %@"),
                    CFDictionaryGetValue(dict, _kCFSystemVersionProductNameKey),
                    CFDictionaryGetValue(dict, _kCFSystemVersionProductVersionKey));
        if (!str) {
            CFRelease(dict);
            LOG(VERBOSE) << "Failed to get a version string from a dictionary";
            return "<Unknown>";
        }
        int length = CFStringGetLength(str);
        if (!length) {
            CFRelease(str);
            CFRelease(dict);
            LOG(VERBOSE) << "Failed to get a version string length";
            return "<Unknown>";
        }
        std::string version(length, '\0');
        if (!CFStringGetCString(str, &version[0], version.size() + 1, CFStringGetSystemEncoding())) {
            CFRelease(str);
            CFRelease(dict);
            LOG(VERBOSE) << "Failed to get a version string as C string";
            return "<Unknown>";
        }
        CFRelease(str);
        CFRelease(dict);
        lastSuccessfulValue = std::move(version);
        return lastSuccessfulValue;

#elif defined(__linux__)
        using android::base::ScopedFd;
        using android::base::trim;
        const auto versionNumFile = android::base::makeCustomScopedPtr(
              tempfile_create(), tempfile_close);

        if (!versionNumFile) {
          string errorStr =
            "Error: Internal error: could not create a temporary file";
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }

        string tempPath = tempfile_path(versionNumFile.get());

        int exitCode = -1;
        vector<string> command{"lsb_release", "-d"};
        runCommand(command,
                   RunOptions::WaitForCompletion |
                           RunOptions::TerminateOnTimeout |
                           RunOptions::DumpOutputToFile,
                   1000,  // timeout ms
                   &exitCode, nullptr, tempPath);

        if (exitCode) {
          string errorStr = "Could not get host OS product version.";
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }

        ScopedFd tempfileFd(open(tempPath.c_str(), O_RDONLY));
        if (!tempfileFd.valid()) {
            LOG(VERBOSE) << "Could not open" << tempPath << " : "
                         << strerror(errno);
            return "";
        }

        string contents;
        android::readFileIntoString(tempfileFd.get(), &contents);
        if (contents.empty()) {
          string errorStr = StringFormat(
              "Error: Internal error: could not read temporary file '%s'",
              tempPath);
          LOG(VERBOSE) << errorStr;
          return errorStr;
        }
        //"lsb_release -d" output is "Description:      [os-product-version]"
        lastSuccessfulValue = trim(contents.substr(12, contents.size() - 12));
        return lastSuccessfulValue;
#else
#error getOsName(): unsupported OS;
#endif
    }

    bool isRunningUnderWine() const override {
#ifndef _WIN32
        return false;
#else
        static const bool isUnderWine = android::base::isRunningUnderWine();
        return isUnderWine;
#endif
    }

    System::Pid getCurrentProcessId() const override
    {
#ifdef _WIN32
        return ::GetCurrentProcessId();
#else
        return getpid();
#endif
    }

    std::string getMajorOsVersion() const override {
        int majorVersion = 0, minorVersion = 0;
#ifdef _WIN32
        OSVERSIONINFOEXW ver;
        ver.dwOSVersionInfoSize = sizeof(ver);
        GetVersionExW((OSVERSIONINFOW*)&ver);
        majorVersion = ver.dwMajorVersion;
        minorVersion = ver.dwMinorVersion;
#else
        struct utsname name;
        if (uname(&name) == 0) {
            // Now parse out version numbers, as this will look something like:
            // 4.19.67-xxx or 18.6.0 or so.
            sscanf(name.release, "%d.%d", &majorVersion, &minorVersion);
        }
#endif
        return std::to_string(majorVersion) + "." +
               std::to_string(minorVersion);
    }

    WaitExitResult waitForProcessExit(int pid, Duration timeoutMs) const override {
#ifdef __APPLE__
        struct kevent monitor;
        struct kevent result;
        int kq, kevent_ret;
        struct timespec timeout;

        timeout.tv_sec = timeoutMs / 1000;
        timeout.tv_nsec = (timeoutMs % 1000) * 1000000;

        kq = kqueue();

        if (kq == -1) {
            return WaitExitResult::Error;
        }

        EV_SET(&monitor, pid, EVFILT_PROC, EV_ADD,
               NOTE_EXIT , 0, nullptr);
        memset(&result, 0, sizeof(struct kevent));

        while (true) {
            kevent_ret = kevent(kq, &monitor, 1 /* events to monitor */,
                                    &result, 1 /* resulting events */,
                                    &timeout);

            if (!kevent_ret) { // timed out
                close(kq);
                return WaitExitResult::Timeout;
            }

            if (result.fflags & NOTE_EXIT) {
                close(kq);
                return WaitExitResult::Exited;
            }
        }
#elif defined(_WIN32)
        HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if (!process) {
            DWORD lastErr = GetLastError();
            if (lastErr == ERROR_ACCESS_DENIED) {
                return WaitExitResult::Error;
            }
            // For everything else, assume the process has
            // exited.
            return WaitExitResult::Exited;
        }
        DWORD ret = WaitForSingleObject(process, timeoutMs);
        CloseHandle(process);
        if (ret == WAIT_OBJECT_0) {
            return WaitExitResult::Exited;
        } else {
            return WaitExitResult::Timeout;
        }
#else // linux
        uint64_t remainingMs = timeoutMs;
        const uint64_t pollMs = 100;
        errno = 0;

        int ret = HANDLE_EINTR(kill(pid, 0));
        if (ret < 0 && errno == ESRCH) {
            return WaitExitResult::Exited; // successfully waited out the pid
        }

        while (true) {
            sleepMs(pollMs);
            ret = HANDLE_EINTR(kill(pid, 0));
            if (ret < 0 && errno == ESRCH) {
                return WaitExitResult::Exited; // successfully waited out the pid
            }
            if (remainingMs < pollMs) {
                return WaitExitResult::Timeout; // timed out
            }
            remainingMs -= pollMs;
        }
#endif
    }


    int getCpuCoreCount() const override {
#ifdef _WIN32
        SYSTEM_INFO si = {};
        ::GetSystemInfo(&si);
        return si.dwNumberOfProcessors < 1 ? 1 : si.dwNumberOfProcessors;
#else
        auto res = (int)::sysconf(_SC_NPROCESSORS_ONLN);
        return res < 1 ? 1 : res;
#endif
    }

    MemUsage getMemUsage() const override {
        MemUsage res = {};
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX memCounters = {sizeof(memCounters)};

        if (::GetProcessMemoryInfo(::GetCurrentProcess(),
                    reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memCounters),
                    sizeof(memCounters))) {

            uint64_t pageFileUsageCommit =
                memCounters.PagefileUsage
                ? memCounters.PagefileUsage
                : memCounters.PrivateUsage;

            res.resident = memCounters.WorkingSetSize;
            res.resident_max = memCounters.PeakWorkingSetSize;
            res.virt = pageFileUsageCommit;
            res.virt_max = memCounters.PeakPagefileUsage;
        }

        MEMORYSTATUSEX mem = {sizeof(mem)};
        if (::GlobalMemoryStatusEx(&mem)) {
            res.total_phys_memory = mem.ullTotalPhys;
            res.avail_phys_memory = mem.ullAvailPhys;
            res.total_page_file = mem.ullTotalPageFile;
        }
#elif defined (__linux__)
        size_t size = 0;
        std::ifstream fin;

        fin.open("/proc/self/status");
        if (!fin.good()) {
            return res;
        }

        std::string line;
        while (std::getline(fin, line)) {
            if (sscanf(line.c_str(), "VmRSS:%lu", &size) == 1) {
                res.resident = size * 1024;
            }
            else if (sscanf(line.c_str(), "VmHWM:%lu", &size) == 1) {
                res.resident_max = size * 1024;
            }
            else if (sscanf(line.c_str(), "VmSize:%lu", &size) == 1) {
                res.virt = size * 1024;
            }
            else if (sscanf(line.c_str(), "VmPeak:%lu", &size) == 1) {
                res.virt_max = size * 1024;
            }
        }
        fin.close();

        fin.open("/proc/meminfo");
        if (!fin.good()) {
            return res;
        }

        while (std::getline(fin, line)) {
            if (sscanf(line.c_str(), "MemTotal:%lu", &size) == 1) {
                res.total_phys_memory = size * 1024;
            }
            else if (sscanf(line.c_str(), "MemAvailable:%lu", &size) == 1) {
                res.avail_phys_memory = size * 1024;
            }
            else if (sscanf(line.c_str(), "SwapTotal:%lu", &size) == 1) {
                res.total_page_file = size * 1024;
            }
        }
        fin.close();

#elif defined(__APPLE__)
        mach_task_basic_info info = {};
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                reinterpret_cast<task_info_t>(&info), &infoCount);

        uint64_t total_phys = 0;
        {
            int mib[2] = { CTL_HW, HW_MEMSIZE };
            size_t len = sizeof(uint64_t);
            sysctl(mib, 2, &total_phys, &len, nullptr, 0);
        }

        res.resident = info.resident_size;
        res.resident_max = info.resident_size_max;
        res.virt = info.virtual_size;
        res.virt_max = 0; // Max virtual NYI for macOS
        res.total_phys_memory = total_phys; // Max virtual NYI for macOS
        res.total_page_file = 0; // Total page file NYI for macOS

        // Available memory detection: taken from the vm_stat utility sources.
        vm_size_t pageSize = 4096;
        const auto host = mach_host_self();
        host_page_size(host, &pageSize);
        vm_statistics64_data_t vm_stat;
        unsigned int count = HOST_VM_INFO64_COUNT;
        if (host_statistics64(host, HOST_VM_INFO64, (host_info64_t)&vm_stat,
                              &count) == KERN_SUCCESS) {
                res.avail_phys_memory =
                        (vm_stat.free_count - vm_stat.speculative_count) *
                        pageSize;
            }
#endif
        return res;
    }

    Optional<DiskKind> pathDiskKind(StringView path) override {
        return diskKindInternal(path);
    }
    Optional<DiskKind> diskKind(int fd) override {
        return diskKindInternal(fd);
    }

    std::vector<std::string> scanDirEntries(
            StringView dirPath,
            bool fullPath = false) const override {
        std::vector<std::string> result = scanDirInternal(dirPath);
        if (fullPath) {
            // Prepend |dirPath| to each entry.
            std::string prefix = PathUtils::addTrailingDirSeparator(dirPath);
            for (auto& entry : result) {
                entry.insert(0, prefix);
            }
        }
        return result;
    }

    std::string envGet(StringView varname) const override {
        return getEnvironmentVariable(varname);
    }

    void envSet(StringView varname, StringView varvalue) override {
        setEnvironmentVariable(varname, varvalue);
    }

    bool envTest(StringView varname) const override {
#ifdef _WIN32
        Win32UnicodeString varname_unicode(varname);
        const wchar_t* value = _wgetenv(varname_unicode.c_str());
        return value && value[0] != L'\0';
#else
        const char* value = getenv(c_str(varname));
        return value && value[0] != '\0';
#endif
    }

    std::vector<std::string> envGetAll() const override {
        std::vector<std::string> res;
        for (auto env = environ; env && *env; ++env) {
            res.push_back(*env);
        }
        return res;
    }

    bool isRemoteSession(std::string* sessionType) const final {
        if (envTest("NX_TEMP")) {
            if (sessionType) {
                *sessionType = "NX";
            }
            return true;
        }
        if (envTest("CHROME_REMOTE_DESKTOP_SESSION")) {
            if (sessionType) {
                *sessionType = "Chrome Remote Desktop";
            }
            return true;
        }
        if (!envGet("SSH_CONNECTION").empty() && !envGet("SSH_CLIENT").empty()) {
            // This can be a remote X11 session, let's check if DISPLAY is set
            // to something uncommon.
            if (envGet("DISPLAY").size() > 2) {
                if (sessionType) {
                    *sessionType = "X11 Forwarding";
                }
                return true;
            }
        }

#ifdef _WIN32

// https://docs.microsoft.com/en-us/windows/win32/termserv/detecting-the-terminal-services-environment
//
// "You should not use GetSystemMetrics(SM_REMOTESESSION) to determine if your
// application is running in a remote session in Windows 8 and later or Windows
// Server 2012 and later if the remote session may also be using the RemoteFX
// vGPU improvements to the Microsoft Remote Display Protocol (RDP). In this
// case, GetSystemMetrics(SM_REMOTESESSION) will identify the remote session as
// a local session."

#define TERMINAL_SERVER_KEY "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\"
#define GLASS_SESSION_ID    "GlassSessionId"

        BOOL fIsRemoteable = FALSE;

        if (GetSystemMetrics(SM_REMOTESESSION)) {
            fIsRemoteable = TRUE;
        } else {
            HKEY hRegKey = NULL;
            LONG lResult;

            lResult = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TERMINAL_SERVER_KEY,
                0, // ulOptions
                KEY_READ,
                &hRegKey);

            if (lResult == ERROR_SUCCESS) {
                DWORD dwGlassSessionId;
                DWORD cbGlassSessionId = sizeof(dwGlassSessionId);
                DWORD dwType;

                lResult = RegQueryValueEx(
                    hRegKey,
                    GLASS_SESSION_ID,
                    NULL, // lpReserved
                    &dwType,
                    (BYTE*) &dwGlassSessionId,
                    &cbGlassSessionId);

                if (lResult == ERROR_SUCCESS) {
                    DWORD dwCurrentSessionId;
                    if (ProcessIdToSessionId(GetCurrentProcessId(), &dwCurrentSessionId)) {
                        fIsRemoteable = (dwCurrentSessionId != dwGlassSessionId);
                    }
                }
            }

            if (hRegKey) {
                RegCloseKey(hRegKey);
            }
        }

        if (TRUE == fIsRemoteable && sessionType) {
            *sessionType = "Windows Remote Desktop";
        }

        if (TRUE == fIsRemoteable) {
            return true;
        }

#endif  // _WIN32
        return false;
    }

    bool pathExists(StringView path) const override {
        return pathExistsInternal(path);
    }

    bool pathIsFile(StringView path) const override {
        return pathIsFileInternal(path);
    }

    bool pathIsDir(StringView path) const override {
        return pathIsDirInternal(path);
    }

    bool pathIsLink(StringView path) const override {
        return pathIsLinkInternal(path);
    }

    bool pathCanRead(StringView path) const override {
        return pathCanReadInternal(path);
    }

    bool pathCanWrite(StringView path) const override {
        return pathCanWriteInternal(path);
    }

    bool pathCanExec(StringView path) const override {
        return pathCanExecInternal(path);
    }

    int pathOpen(const char *filename, int oflag, int pmode) const override {
        return pathOpenInternal(filename, oflag, pmode);
    }

    bool deleteFile(StringView path) const override {
        return deleteFileInternal(path);
    }

    bool pathFileSize(StringView path,
            FileSize* outFileSize) const override {
        return pathFileSizeInternal(path, outFileSize);
    }

    FileSize recursiveSize(StringView path) const override {
        return recursiveSizeInternal(path);
    }

    bool pathFreeSpace(StringView path, FileSize* spaceInBytes) const override {
        return pathFreeSpaceInternal(path, spaceInBytes);
    }

    bool fileSize(int fd, FileSize* outFileSize) const override {
        return fileSizeInternal(fd, outFileSize);
    }

    Optional<std::string> which(StringView command) const override {
#ifdef WIN32
      std::string cmd = command ;
      if (!extractFullPath(&cmd)) {
        return {};
      }
      return cmd;
#else
      if (PathUtils::isAbsolute(command)) {
        if (!pathCanExec(command))
          return {};

        return Optional<std::string>(command);
      }

      ScopedCPtr<char> exe(::path_search_exec(c_str(command)));
      if (exe && pathCanExec(exe.get())) {
        return Optional<std::string>(exe.get());
      }
      return {};
#endif
    }

    Optional<Duration> pathCreationTime(StringView path) const override {
        return pathCreationTimeInternal(path);
    }

    Optional<Duration> pathModificationTime(StringView path) const override {
        return pathModificationTimeInternal(path);
    }

    Times getProcessTimes() const override {
        Times res = {};

#ifdef _WIN32
        FILETIME creationTime = {};
        FILETIME exitTime = {};
        FILETIME kernelTime = {};
        FILETIME userTime = {};
        ::GetProcessTimes(::GetCurrentProcess(), &creationTime, &exitTime,
                &kernelTime, &userTime);

        // convert 100-ns intervals from a struct to int64_t milliseconds
        ULARGE_INTEGER kernelInt64;
        kernelInt64.LowPart = kernelTime.dwLowDateTime;
        kernelInt64.HighPart = kernelTime.dwHighDateTime;
        res.systemMs = static_cast<Duration>(kernelInt64.QuadPart / 10000);

        ULARGE_INTEGER userInt64;
        userInt64.LowPart = userTime.dwLowDateTime;
        userInt64.HighPart = userTime.dwHighDateTime;
        res.userMs = static_cast<Duration>(userInt64.QuadPart / 10000);
#else
        tms times = {};
        ::times(&times);
        // convert to milliseconds
        const long int ticksPerSec = ::sysconf(_SC_CLK_TCK);
        res.systemMs = (times.tms_stime * 1000ll) / ticksPerSec;
        res.userMs = (times.tms_utime * 1000ll) / ticksPerSec;
#endif
        res.wallClockMs =
            (kTickCount.getUs() - kTickCount.getStartTimeUs()) / 1000;

        return res;
    }

    time_t getUnixTime() const override {
        return time(NULL);
    }

    Duration getUnixTimeUs() const override {
        timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000000LL + tv.tv_usec;
    }

    WallDuration getHighResTimeUs() const override {
        return kTickCount.getUs();
    }

    void sleepMs(unsigned n) const override {
        Thread::sleepMs(n);
    }

    void sleepUs(unsigned n) const override {
        Thread::sleepUs(n);
    }

    void yield() const override {
        Thread::yield();
    }

#ifdef _MSC_VER
    static void msvcInvalidParameterHandler(const wchar_t* expression,
                                            const wchar_t* function,
                                            const wchar_t* file,
                                            unsigned int line,
                                            uintptr_t pReserved) {
        // Don't expect too much from actually getting these parameters..
        LOG(WARNING) << "Ignoring invalid parameter detected in function: "
                    << function << " file: " << file << ", line: " << line
                    << ", expression: " << expression;
    }
#endif

    void configureHost() const override {
    #ifdef _MSC_VER
        _set_invalid_parameter_handler(msvcInvalidParameterHandler);

        const HMODULE kernelDll = ::GetModuleHandleW(L"kernel32");
        if (kernelDll != NULL)  {
            // Windws 8 and above has fancy good timers, we want those..
            SystemTime pfnSys = (SystemTime)::GetProcAddress(kernelDll,
                                            "GetSystemTimePreciseAsFileTime");

            if (pfnSys) {
                getSystemTime = pfnSys;
            }
         }
    #endif
    }

    void cleanupWaitingPids() const override {
        // Only seeing this type of hang-on-exit for linux/mac.
#ifndef _WIN32
        std::lock_guard<std::mutex> lock(mMutex);
        for (auto pid : mWaitingPids) {
            LOG(VERBOSE) << "Force killing pid=" << pid;
            kill(pid, SIGKILL);
        }
#endif
    }

    Optional<std::string> runCommandWithResult(
            const std::vector<std::string>& commandLine,
            System::Duration timeoutMs = kInfinite,
            System::ProcessExitCode* outExitCode = nullptr) override {
        std::string tmp_dir = getTempDir();

        // Get temporary file path
        constexpr base::StringView temp_filename_pattern = "runCommand_XXXXXX";
        std::string temp_file_path =
                PathUtils::join(tmp_dir, temp_filename_pattern);

        auto tmpfd =
                android::base::ScopedFd(mkstemp((char*)temp_file_path.data()));
        if (!tmpfd.valid()) {
            return {};
        }
        //
        // We have to close the handle. On windows we will not be able to write
        // to this file, resulting in strange behavior.
        tmpfd.close();
        temp_file_path.resize(strlen(temp_file_path.c_str()));
        auto tmpFileDeleter = base::makeCustomScopedPtr(
               /* std::string* */ &temp_file_path,
               /* T? */ [](const std::string* name) { remove(name->c_str()); });

        if (!runCommand(commandLine,
                        RunOptions::WaitForCompletion |
                                RunOptions::TerminateOnTimeout |
                                RunOptions::DumpOutputToFile,
                        timeoutMs, outExitCode, nullptr, temp_file_path)) {
            return {};
        }

        // Extract stderr/stdout
        return android::readFileIntoString(temp_file_path).valueOr({});
    }

    bool runCommand(const std::vector<std::string>& commandLine,
            RunOptions options,
            System::Duration timeoutMs,
            System::ProcessExitCode* outExitCode,
            System::Pid* outChildPid,
            const std::string& outputFile) override {
        // Sanity check.
        if (commandLine.empty()) {
            return false;
        }

#ifdef _WIN32
        std::vector<std::string> commandLineCopy = commandLine;
        STARTUPINFOW startup = {};
        startup.cb = sizeof(startup);

        if (!extractFullPath(&commandLineCopy[0])) {
            return false;
        }

        bool useComspec = false;

        if ((options & RunOptions::ShowOutput) == 0 ||
                ((options & RunOptions::DumpOutputToFile) != 0 &&
                 !outputFile.empty())) {
            startup.dwFlags = STARTF_USESHOWWINDOW;

            // 'Normal' way of hiding console output is passing null std handles
            // to the CreateProcess() function and CREATE_NO_WINDOW as a flag.
            // Sadly, in this case Cygwin runtime goes completely mad - its
            // whole FILE* machinery just stops working. E.g., resize2fs always
            // creates corrupted images if you try doing it in a 'normal' way.
            // So, instead, we do the following: run the command in a cmd.exe
            // with stdout and stderr redirected to either nul (for no output)
            // or the specified file (for file output).

            // 1. Find the commmand-line interpreter - which hides behind the
            // %COMSPEC% environment variable
            std::string comspec = envGet("COMSPEC");
            if (comspec.empty()) {
                comspec = "cmd.exe";
            }

            if (!extractFullPath(&comspec)) {
                return false;
            }

            // 2. Now turn the command into the proper cmd command:
            //   cmd.exe /C "command" "arguments" ... >nul 2>&1
            // This executes a command with arguments passed and redirects
            // stdout to nul, stderr is attached to stdout (so it also
            // goes to nul)
            commandLineCopy.insert(commandLineCopy.begin(), "/C");
            commandLineCopy.insert(commandLineCopy.begin(), comspec);

            if ((options & RunOptions::DumpOutputToFile) != RunOptions::Empty) {
                commandLineCopy.push_back(">");
                commandLineCopy.push_back(outputFile);
                commandLineCopy.push_back("2>&1");
            } else if ((options & RunOptions::ShowOutput) == RunOptions::Empty) {
                commandLineCopy.push_back(">nul");
                commandLineCopy.push_back("2>&1");
            }

            useComspec = true;
        }

        PROCESS_INFORMATION pinfo = {};

        Win32UnicodeString commandUnicode(commandLineCopy[0]);
        Win32UnicodeString argsUnicode;

        if (useComspec) {
            std::string argsBeforeQuote = commandLineCopy[1];   // "/C"
            std::string argsToQuote;
            size_t i;
            for (i = 2; i < commandLineCopy.size() - 1; ++i) {
                argsToQuote += android::base::Win32Utils::quoteCommandLine(commandLineCopy[i]);
                argsToQuote += ' ';
            }
            argsToQuote += android::base::Win32Utils::quoteCommandLine(commandLineCopy[i]);
            argsUnicode = Win32UnicodeString(argsBeforeQuote + " \"" + argsToQuote + "\"");
        } else {
            std::string args = commandLineCopy[0];
            for (size_t i = 1; i < commandLineCopy.size(); ++i) {
                args += ' ';
                args += android::base::Win32Utils::quoteCommandLine(commandLineCopy[i]);
            }
            argsUnicode = Win32UnicodeString(args);
        }

        if (!::CreateProcessW(commandUnicode.c_str(), // program path
                    argsUnicode.data(), // command line args, has to be writable
                    nullptr,            // process handle is not inheritable
                    nullptr,            // thread handle is not inheritable
                    FALSE,              // no, don't inherit any handles
                    0,                  // default creation flags
                    nullptr,            // use parent's environment block
                    nullptr,            // use parent's starting directory
                    &startup,           // startup info, i.e. std handles
                    &pinfo)) {
            return false;
        }

        CloseHandle(pinfo.hThread);
        // make sure we close the process handle on exit
        const android::base::Win32Utils::ScopedHandle process(pinfo.hProcess);

        if (outChildPid) {
            *outChildPid = pinfo.dwProcessId;
        }

        if ((options & RunOptions::WaitForCompletion) == 0) {
            return true;
        }

        // We were requested to wait for the process to complete.
        DWORD ret = ::WaitForSingleObject(pinfo.hProcess,
                timeoutMs ? timeoutMs : INFINITE);
        if (ret == WAIT_FAILED || ret == WAIT_TIMEOUT) {
            if ((options & RunOptions::TerminateOnTimeout) != 0) {
                ::TerminateProcess(pinfo.hProcess, 1);
            }
            return false;
        }

        DWORD exitCode;
        auto exitCodeSuccess = ::GetExitCodeProcess(pinfo.hProcess, &exitCode);
        assert(exitCodeSuccess);
        (void)exitCodeSuccess;
        if (outExitCode) {
            *outExitCode = exitCode;
        }
        return true;

#else   // !_WIN32
        sigset_t oldset;
        sigset_t set;
        if (sigemptyset(&set) || sigaddset(&set, SIGCHLD) ||
                pthread_sigmask(SIG_UNBLOCK, &set, &oldset)) {
            return false;
        }
        auto result = runCommandPosix(commandLine, options, timeoutMs,
                outExitCode, outChildPid, outputFile);
        pthread_sigmask(SIG_SETMASK, &oldset, nullptr);
        return result;
#endif  // !_WIN32
    }

    std::string getTempDir() const override {
#ifdef _WIN32
        Win32UnicodeString path(PATH_MAX);
        DWORD retval = GetTempPathW(path.size(), path.data());
        if (retval > path.size()) {
            path.resize(static_cast<size_t>(retval));
            retval = GetTempPathW(path.size(), path.data());
        }
        if (!retval) {
            // Best effort!
            return std::string("C:\\Temp");
        }
        path.resize(retval);
        // The result of GetTempPath() is already user-dependent
        // so don't append the username or userid to the result.
        path.append(L"\\AndroidEmulator");
        ::_wmkdir(path.c_str());
        return path.toString();
#else   // !_WIN32
        std::string result;
        const char* tmppath = getenv("ANDROID_TMP");
        if (!tmppath) {
            const char* user = getenv("USER");
            if (user == NULL || user[0] == '\0') {
                user = "unknown";
            }
            result = "/tmp/android-";
            result += user;
        } else {
            result = tmppath;
        }
        ::android_mkdir(result.c_str(), 0744);
        return result;
#endif  // !_WIN32
    }

    bool getEnableCrashReporting() const override {
        const bool defaultValue = true;

        const std::string enableCrashReporting = envGet("ANDROID_EMU_ENABLE_CRASH_REPORTING");

        if (enableCrashReporting.empty()) {
            return defaultValue;
        } else {
            return parseBooleanValue(enableCrashReporting.c_str(), defaultValue);
        }
    }

#ifndef _WIN32
    bool runCommandPosix(const std::vector<std::string>& commandLine,
            RunOptions options,
            System::Duration timeoutMs,
            System::ProcessExitCode* outExitCode,
            System::Pid* outChildPid,
            const std::string& outputFile) {
        vector<char*> params;
        for (const auto& item : commandLine) {
            params.push_back(const_cast<char*>(item.c_str()));
        }
        params.push_back(nullptr);

        std::string cmd = "";
        if(LOG_IS_ON(VERBOSE)) {
            cmd = "|";
            for (const auto& param : commandLine) {
                cmd += param;
                cmd += " ";
            }
            cmd += "|";
        }

#if defined(__APPLE__)
        int pid = runViaPosixSpawn(commandLine[0].c_str(), params, options,
                outputFile);
#else
        int pid = runViaForkAndExec(commandLine[0].c_str(), params, options,
                outputFile);
#endif  // !defined(__APPLE__)

        if (pid < 0) {
            LOG(VERBOSE) << "Failed to fork for command " << cmd;
            return false;
        }

        if (outChildPid) {
            *outChildPid = pid;
        }

        if ((options & RunOptions::WaitForCompletion) == 0) {
            return true;
        }

        // We were requested to wait for the process to complete.
        int exitCode;
        // Do not use SIGCHLD here because we're not sure if we're
        // running on the main thread and/or what our sigmask is.
        if (timeoutMs == kInfinite) {
            // Let's just wait forever and hope that the child process
            // exits.
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mWaitingPids.insert(pid);
            }
            HANDLE_EINTR(waitpid(pid, &exitCode, 0));
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mWaitingPids.erase(pid);
            }

            if (outExitCode) {
                *outExitCode = WEXITSTATUS(exitCode);
            }
            return WIFEXITED(exitCode);
        }

        auto startTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::milliseconds::zero();
        while (elapsed.count() < timeoutMs) {
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mWaitingPids.insert(pid);
            }
            pid_t waitPid = HANDLE_EINTR(waitpid(pid, &exitCode, WNOHANG));
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mWaitingPids.erase(pid);
            }
            if (waitPid < 0) {
                auto local_errno = errno;
                LOG(VERBOSE) << "Error running command " << cmd
                    << ". waitpid failed with |"
                    << strerror(local_errno) << "|";
                return false;
            }

            if (waitPid > 0) {
                if (outExitCode) {
                    *outExitCode = WEXITSTATUS(exitCode);
                }
                return WIFEXITED(exitCode);
            }

            sleepMs(10);
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);
        }

        // Timeout occured.
        if ((options & RunOptions::TerminateOnTimeout) != 0) {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, WNOHANG);
        }
        LOG(VERBOSE) << "Timed out with running command " << cmd;
        return false;
    }

    int runViaForkAndExec(const char* command,
            const vector<char*>& params,
            RunOptions options,
            const string& outputFile) {
        // If an output file was requested, open it before forking, since
        // creating a file in the child of a multi-threaded process is sketchy.
        //
        // It will be immediately closed in the parent process, and dup2'd into
        // stdout and stderr in the child process.
        int outputFd = 0;
        if ((options & RunOptions::DumpOutputToFile) != RunOptions::Empty) {
            if (outputFile.empty()) {
                LOG(VERBOSE) << "Can not redirect output to empty file!";
                return -1;
            }

            // Ensure the umask doesn't get in the way while creating the
            // output file.
            mode_t old = umask(0);
            outputFd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                    0700);
            umask(old);
            if (outputFd < 0) {
                LOG(VERBOSE) << "Failed to open file to redirect stdout/stderr";
                return -1;
            }
        }

        int pid = fork();

        if (pid != 0) {
            if (outputFd > 0) {
                close(outputFd);
            }
            // Return the child's pid / error code to parent process.
            return pid;
        }

        // In the child process.
        // Do not do __anything__ except execve. That includes printing to
        // stdout/stderr. None of it is safe in the child process forked from a
        // parent with multiple threads.
        if ((options & RunOptions::DumpOutputToFile) != RunOptions::Empty) {
            dup2(outputFd, 1);
            dup2(outputFd, 2);
            close(outputFd);
        } else if ((options & RunOptions::ShowOutput) == RunOptions::Empty) {
            // We were requested to RunOptions::HideAllOutput
            int fd = open("/dev/null", O_WRONLY);
            if (fd > 0) {
                dup2(fd, 1);
                dup2(fd, 2);
                close(fd);
            }
        }

        // We never want to forward our stdin to the child process. On the other
        // hand, closing it can confuse some programs.
        int fd = open("/dev/null", O_RDONLY);
        if (fd > 0) {
            dup2(fd, 0);
            close(fd);
        }

        if (execvp(command, params.data()) == -1) {
            // emulator doesn't really like exit calls from a forked process
            // (it just hangs), so let's just kill it
            if (raise(SIGKILL) != 0) {
                exit(RunFailed);
            }
        }
        // Should not happen, but let's keep the compiler happy
        return -1;
    }

#if defined(__APPLE__)
    int runViaPosixSpawn(const char* command,
            const vector<char*>& params,
            RunOptions options,
            const string& outputFile) {
        posix_spawnattr_t attr;
        if (posix_spawnattr_init(&attr)) {
            LOG(VERBOSE) << "Failed to initialize spawnattr obj.";
            return -1;
        }
        // Automatically destroy the successfully initialized attr.
        auto attrDeleter = [](posix_spawnattr_t* t) {
            posix_spawnattr_destroy(t);
        };
        unique_ptr<posix_spawnattr_t, decltype(attrDeleter)> scopedAttr(
                &attr, attrDeleter);

        if (posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT)) {
            LOG(VERBOSE) << "Failed to request CLOEXEC.";
            return -1;
        }

        posix_spawn_file_actions_t fileActions;
        if (posix_spawn_file_actions_init(&fileActions)) {
            LOG(VERBOSE) << "Failed to initialize fileactions obj.";
            return -1;
        }
        // Automatically destroy the successfully initialized fileActions.
        auto fileActionsDeleter = [](posix_spawn_file_actions_t* t) {
            posix_spawn_file_actions_destroy(t);
        };
        unique_ptr<posix_spawn_file_actions_t, decltype(fileActionsDeleter)>
            scopedFileActions(&fileActions, fileActionsDeleter);

        if ((options & RunOptions::DumpOutputToFile) != RunOptions::Empty) {
            if (posix_spawn_file_actions_addopen(
                        &fileActions, 1, outputFile.c_str(),
                        O_WRONLY | O_CREAT | O_TRUNC, 0700) ||
                    posix_spawn_file_actions_addopen(
                        &fileActions, 2, outputFile.c_str(),
                        O_WRONLY | O_CREAT | O_TRUNC, 0700)) {
                LOG(VERBOSE) << "Failed to redirect child output to file "
                    << outputFile;
                return -1;
            }
        } else if ((options & RunOptions::ShowOutput) != RunOptions::Empty) {
            if (posix_spawn_file_actions_addinherit_np(&fileActions, 1) ||
                    posix_spawn_file_actions_addinherit_np(&fileActions, 2)) {
                LOG(VERBOSE) << "Failed to request child stdout/stderr to be "
                    "left intact";
                return -1;
            }
        } else {
            if (posix_spawn_file_actions_addopen(&fileActions, 1, "/dev/null",
                        O_WRONLY, 0700) ||
                    posix_spawn_file_actions_addopen(&fileActions, 2, "/dev/null",
                        O_WRONLY, 0700)) {
                LOG(VERBOSE) << "Failed to redirect child output to /dev/null";
                return -1;
            }
        }

        // We never want to forward our stdin to the child process. On the other
        // hand, closing it can confuse some programs.
        if (posix_spawn_file_actions_addopen(&fileActions, 0, "/dev/null",
                    O_RDONLY, 0700)) {
            LOG(VERBOSE) << "Failed to redirect child stdin from /dev/null";
            return -1;
        }

        // Posix spawn requires that argv[0] exists.
        assert(params[0] != nullptr);

        int pid;
        if (int error_code = posix_spawnp(&pid, command, &fileActions, &attr,
                    params.data(), environ)) {
            LOG(VERBOSE) << "posix_spawnp failed: " << strerror(error_code);
            return -1;
        }
        return pid;
    }
#endif  // defined(__APPLE__)
#endif  // !_WIN32

private:
    static void atexit_HostSystem();

    mutable std::mutex mMutex;
    std::unordered_set<int> mWaitingPids;

    mutable std::string mProgramDir;
    mutable std::string mLauncherDir;
    mutable std::string mHomeDir;
    mutable std::string mAppDataDir;
};

LazyInstance<HostSystem> sHostSystem = LAZY_INSTANCE_INIT;
System* sSystemForTesting = NULL;

// static
void HostSystem::atexit_HostSystem() {
    if (sHostSystem.hasInstance()) {
        hostSystem()->cleanupWaitingPids();
    }
}

#ifdef _WIN32
// Return |path| as a Unicode string, while discarding trailing separators.
Win32UnicodeString win32Path(StringView path) {
    Win32UnicodeString wpath(path);
    // Get rid of trailing directory separators, Windows doesn't like them.
    size_t size = wpath.size();
    while (size > 0U &&
           (wpath[size - 1U] == L'\\' || wpath[size - 1U] == L'/')) {
        size--;
    }
    if (size < wpath.size()) {
        wpath.resize(size);
    }
    return wpath;
}

using PathStat = struct _stat64;

#else  // _WIN32

using PathStat = struct stat;

#endif  // _WIN32

int pathStat(StringView path, PathStat* st) {
#ifdef _WIN32
    return _wstat64(win32Path(path).c_str(), st);
#else   // !_WIN32
    return HANDLE_EINTR(stat(c_str(path), st));
#endif  // !_WIN32
}

int fdStat(int fd, PathStat* st) {
#ifdef _WIN32
    return fstat64(fd, st);
#else   // !_WIN32
    return HANDLE_EINTR(fstat(fd, st));
#endif  // !_WIN32
}

#ifdef _WIN32
static int GetWin32Mode(int mode) {
    // Convert |mode| to win32 permission bits.
    int win32mode = 0x0;

    if ((mode & R_OK) || (mode & X_OK)) {
        win32mode |= 0x4;
    }
    if (mode & W_OK) {
        win32mode |= 0x2;
    }

    return win32mode;
}
#endif

int pathAccess(StringView path, int mode) {
#ifdef _WIN32
    return _waccess(win32Path(path).c_str(), GetWin32Mode(mode));
#else   // !_WIN32
    return HANDLE_EINTR(android_access(c_str(path), mode));
#endif  // !_WIN32
}

}  // namespace

// static
System* System::get() {
    System* result = sSystemForTesting;
    if (!result) {
        result = hostSystem();
    }
    return result;
}

#ifdef __x86_64__
// static
const char* System::kLibSubDir = "lib64";
// static
const char* System::kBinSubDir = "bin64";
#else
// static
const char* System::kLibSubDir = "lib";
// static
const char* System::kBinSubDir = "bin";
#endif

const char* System::kBin32SubDir = "bin";

// These need to be defined so one can take an address of them.
const int System::kProgramBitness;
const char System::kDirSeparator;
const char System::kPathSeparator;

#ifdef _WIN32
// static
const char* System::kLibrarySearchListEnvVarName = "PATH";
#elif defined(__APPLE__)
const char* System::kLibrarySearchListEnvVarName = "DYLD_LIBRARY_PATH";
#else
// static
const char* System::kLibrarySearchListEnvVarName = "LD_LIBRARY_PATH";
#endif

// static
System* System::setForTesting(System* system) {
    System* result = sSystemForTesting;
    sSystemForTesting = system;
    return result;
}

System* System::hostSystem() {
    return sHostSystem.ptr();
}

// static
std::vector<std::string> System::scanDirInternal(StringView dirPath) {
    std::vector<std::string> result;

    if (dirPath.empty()) {
        return result;
    }
#ifdef _WIN32
    auto root = PathUtils::addTrailingDirSeparator(dirPath);
    root += '*';
    Win32UnicodeString rootUnicode(root);
    struct _wfinddata_t findData;
    intptr_t findIndex = _wfindfirst(rootUnicode.c_str(), &findData);
    if (findIndex >= 0) {
        do {
            const wchar_t* name = findData.name;
            if (wcscmp(name, L".") != 0 && wcscmp(name, L"..") != 0) {
                result.push_back(Win32UnicodeString::convertToUtf8(name));
            }
        } while (_wfindnext(findIndex, &findData) >= 0);
        _findclose(findIndex);
    }
#else  // !_WIN32
    DIR* dir = ::opendir(c_str(dirPath));
    if (dir) {
        for (;;) {
            struct dirent* entry = ::readdir(dir);
            if (!entry) {
                break;
            }
            const char* name = entry->d_name;
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
                result.push_back(std::string(name));
            }
        }
        ::closedir(dir);
    }
#endif  // !_WIN32
    std::sort(result.begin(), result.end());
    return result;
}

// static
bool System::pathIsLinkInternal(StringView path) {
#ifdef _WIN32
    // Supposedly GetFileAttributes() and FindFirstFile()
    // can be used to detect symbolic links. In my tests,
    // a symbolic link looked exactly like a regular file.
    return false;
#else
    struct stat fileStatus;
    if (lstat(c_str(path), &fileStatus)) {
        return false;
    }
    return S_ISLNK(fileStatus.st_mode);
#endif
}

// static
bool System::pathExistsInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    int ret = pathAccess(path, F_OK);
    return (ret == 0) || (errno != ENOENT);
}

// static
bool System::pathIsFileInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    PathStat st;
    int ret = pathStat(path, &st);
    if (ret < 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

// static
bool System::pathIsDirInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    PathStat st;
    int ret = pathStat(path, &st);
    if (ret < 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

// static
bool System::pathCanReadInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    return pathAccess(path, R_OK) == 0;
}

// static
bool System::pathCanWriteInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    return pathAccess(path, W_OK) == 0;
}

// static
bool System::pathCanExecInternal(StringView path) {
    if (path.empty()) {
        return false;
    }
    return pathAccess(path, X_OK) == 0;
}

// static
int System::pathOpenInternal(const char *filename, int oflag, int perm) {
#ifdef _WIN32
    return _wopen(win32Path(filename).c_str(), GetWin32Mode(oflag), perm);
#else   // !_WIN32
    return ::open(filename, oflag, perm);
#endif  // !_WIN32
}

bool System::deleteFileInternal(StringView path) {
    if (!pathIsFileInternal(path)) {
        return false;
    }

    int remove_res = remove(c_str(path));

#ifdef _WIN32
    if (remove_res < 0) {
        // Windows sometimes just fails to delete a file
        // on the first try.
        // Sleep a little bit and try again here.
        System::get()->sleepMs(1);
        remove_res = remove(c_str(path));
    }
#endif

    if (remove_res != 0) {
        LOG(VERBOSE) << "Failed to delete file [" << path << "].";
    }

    return remove_res == 0;
}

bool System::pathFreeSpaceInternal(StringView path, FileSize* spaceInBytes) {
#ifdef _WIN32
    ULARGE_INTEGER freeBytesAvailableToUser;
    bool result = GetDiskFreeSpaceEx(c_str(path), &freeBytesAvailableToUser,
                                     NULL, NULL);
    if (!result) {
        return false;
    }
    *spaceInBytes = freeBytesAvailableToUser.QuadPart;
    return true;
#else
    struct statvfs fsStatus;
    int result = statvfs(c_str(path), &fsStatus);
    if (result != 0) {
        return false;
    }
    // Available space is (block size) * (# free blocks)
    *spaceInBytes = ((FileSize)fsStatus.f_frsize) * fsStatus.f_bavail;
    return true;
#endif
}

// static
bool System::pathFileSizeInternal(StringView path, FileSize* outFileSize) {
    if (path.empty() || !outFileSize) {
        return false;
    }
    PathStat st;
    int ret = pathStat(path, &st);
    if (ret < 0 || !S_ISREG(st.st_mode)) {
        return false;
    }
    // This is off_t on POSIX and a 32/64 bit integral type on windows based on
    // the host / compiler combination. We cast everything to 64 bit unsigned to
    // play safe.
    *outFileSize = static_cast<FileSize>(st.st_size);
    return true;
}

// static
System::FileSize System::recursiveSizeInternal(StringView path) {

    std::vector<std::string> fileList;
    fileList.push_back(path);

    FileSize totalSize = 0;

    while (fileList.size() > 0) {
        const auto currentPath = std::move(fileList.back());
        fileList.pop_back();
        if (pathIsFileInternal(currentPath) || pathIsLinkInternal(currentPath)) {
            // Regular file or link. Return its size.
            FileSize theSize;
            if (pathFileSizeInternal(currentPath, &theSize)) {
                totalSize += theSize;
            }
        } else if (pathIsDirInternal(currentPath)) {
            // Directory. Add its contents to the list.
            std::vector<std::string> includedFiles = scanDirInternal(currentPath);
            for (const auto& file : includedFiles) {
                fileList.push_back(PathUtils::join(currentPath, file));
            }
        }
    }
    return totalSize;
}

bool System::fileSizeInternal(int fd, System::FileSize* outFileSize) {
    if (fd < 0) {
        return false;
    }
    PathStat st;
    int ret = fdStat(fd, &st);
    if (ret < 0 || !S_ISREG(st.st_mode)) {
        return false;
    }
    // This is off_t on POSIX and a 32/64 bit integral type on windows based on
    // the host / compiler combination. We cast everything to 64 bit unsigned to
    // play safe.
    *outFileSize = static_cast<FileSize>(st.st_size);
    return true;
}

// static
Optional<System::Duration> System::pathCreationTimeInternal(StringView path) {
#if defined(__linux__) || (defined(__APPLE__) && !defined(_DARWIN_FEATURE_64_BIT_INODE))
    // TODO(zyy@): read the creation time directly from the ext4 attribute
    // on Linux.
    return {};
#else
    PathStat st;
    if (pathStat(path, &st)) {
        return {};
    }
#ifdef _WIN32
    return st.st_ctime * 1000000ll;
#else  // APPLE
    return st.st_birthtimespec.tv_sec * 1000000ll +
            st.st_birthtimespec.tv_nsec / 1000;
#endif  // WIN32 && APPLE
#endif  // Linux
}

// static
Optional<System::Duration> System::pathModificationTimeInternal(StringView path) {
    PathStat st;
    if (pathStat(path, &st)) {
        return {};
    }

#ifdef _WIN32
    return st.st_mtime * 1000000ll;
#elif defined(__linux__)
    return st.st_mtim.tv_sec * 1000000ll + st.st_mtim.tv_nsec / 1000;
#else   // Darwin
    return st.st_mtimespec.tv_sec * 1000000ll + st.st_mtimespec.tv_nsec / 1000;
#endif
}

static Optional<System::DiskKind> diskKind(const PathStat& st) {
#ifdef _WIN32

    auto volumeName = StringFormat(R"(\\?\%c:)", 'A' + st.st_dev);
    ScopedFileHandle volume(::CreateFileA(volumeName.c_str(), 0,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL, OPEN_EXISTING, 0, NULL));
    if (!volume.valid()) {
        return {};
    }

    VOLUME_DISK_EXTENTS volumeDiskExtents;
    DWORD bytesReturned = 0;
    if ((!::DeviceIoControl(volume.get(), IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                            NULL, 0, &volumeDiskExtents,
                            sizeof(volumeDiskExtents), &bytesReturned, NULL) &&
         ::GetLastError() != ERROR_MORE_DATA) ||
        bytesReturned != sizeof(volumeDiskExtents)) {
        return {};
    }
    if (volumeDiskExtents.NumberOfDiskExtents < 1) {
        return {};
    }

    auto deviceName =
            StringFormat(R"(\\?\PhysicalDrive%d)",
                         int(volumeDiskExtents.Extents[0].DiskNumber));
    ScopedFileHandle device(::CreateFileA(deviceName.c_str(), 0,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL, OPEN_EXISTING, 0, NULL));
    if (!device.valid()) {
        return {};
    }

    STORAGE_PROPERTY_QUERY spqTrim;
    spqTrim.PropertyId = (STORAGE_PROPERTY_ID)StorageDeviceTrimProperty;
    spqTrim.QueryType = PropertyStandardQuery;
    DEVICE_TRIM_DESCRIPTOR dtd = {0};
    if (::DeviceIoControl(device.get(), IOCTL_STORAGE_QUERY_PROPERTY, &spqTrim,
                          sizeof(spqTrim), &dtd, sizeof(dtd), &bytesReturned,
                          NULL) &&
        bytesReturned == sizeof(dtd)) {
        // Some SSDs don't support TRIM, so this can't be a sign of an HDD.
        if (dtd.TrimEnabled) {
            return System::DiskKind::Ssd;
        }
    }

    bytesReturned = 0;
    STORAGE_PROPERTY_QUERY spqSeekP;
    spqSeekP.PropertyId = (STORAGE_PROPERTY_ID)StorageDeviceSeekPenaltyProperty;
    spqSeekP.QueryType = PropertyStandardQuery;
    DEVICE_SEEK_PENALTY_DESCRIPTOR dspd = {0};
    if (::DeviceIoControl(device.get(), IOCTL_STORAGE_QUERY_PROPERTY, &spqSeekP,
                          sizeof(spqSeekP), &dspd, sizeof(dspd), &bytesReturned,
                          NULL) &&
        bytesReturned == sizeof(dspd)) {
        return dspd.IncursSeekPenalty ? System::DiskKind::Hdd
                                      : System::DiskKind::Ssd;
    }

    // TODO: figure out how to issue this query when not admin and not opening
    //  disk for write access.
#if 0
    bytesReturned = 0;

    // This struct isn't in the MinGW distribution headers.
    struct ATAIdentifyDeviceQuery {
        ATA_PASS_THROUGH_EX header;
        WORD data[256];
    };
    ATAIdentifyDeviceQuery id_query = {};
    id_query.header.Length = sizeof(id_query.header);
    id_query.header.AtaFlags = ATA_FLAGS_DATA_IN;
    id_query.header.DataTransferLength = sizeof(id_query.data);
    id_query.header.TimeOutValue = 5;  // Timeout in seconds
    id_query.header.DataBufferOffset =
            offsetof(ATAIdentifyDeviceQuery, data[0]);
    id_query.header.CurrentTaskFile[6] = 0xec;  // ATA IDENTIFY DEVICE
    if (::DeviceIoControl(device.get(), IOCTL_ATA_PASS_THROUGH, &id_query,
                          sizeof(id_query), &id_query, sizeof(id_query),
                          &bytesReturned, NULL) &&
        bytesReturned == sizeof(id_query)) {
        // Index of nominal media rotation rate
        // SOURCE:
        // http://www.t13.org/documents/UploadedDocuments/docs2009/d2015r1a-ATAATAPI_Command_Set_-_2_ACS-2.pdf
        //          7.18.7.81 Word 217
        // QUOTE: Word 217 indicates the nominal media rotation rate of the
        // device and is defined in table:
        //          Value           Description
        //          --------------------------------
        //          0000h           Rate not reported
        //          0001h           Non-rotating media (e.g., solid state
        //                          device)
        //          0002h-0400h     Reserved
        //          0401h-FFFEh     Nominal media rotation rate in rotations per
        //                          minute (rpm) (e.g., 7 200 rpm = 1C20h)
        //          FFFFh           Reserved
        unsigned rate = id_query.data[217];
        if (rate == 1) {
            return System::DiskKind::Ssd;
        } else if (rate >= 0x0401 && rate <= 0xFFFE) {
            return System::DiskKind::Hdd;
        }
    }
#endif

#elif defined __linux__

    // Parse /proc/partitions to find the corresponding device
    std::ifstream in("/proc/partitions");
    if (!in) {
        return {};
    }

    const auto maj = major(st.st_dev);
    const auto min = minor(st.st_dev);

    std::string line;
    std::string devName;

    std::unordered_set<std::string> devices;

    while (std::getline(in, line)) {
        unsigned curMaj, curMin;
        unsigned long blocks;
        char name[1024];
        if (sscanf(line.c_str(), "%u %u %lu %1023s", &curMaj, &curMin, &blocks,
                   name) == 4) {
            devices.insert(name);
            if (curMaj == maj && curMin == min) {
                devName = name;
                break;
            }
        }
    }
    if (devName.empty()) {
        return {};
    }
    in.close();

    if (maj == 8) {
        // get rid of the partition number for block devices.
        while (!devName.empty() && isdigit(devName.back())) {
            devName.pop_back();
        }
        if (devices.find(devName) == devices.end()) {
            return {};
        }
    }

    // Now, having a device name, let's parse
    // /sys/block/%device%X/queue/rotational to get the result.
    auto sysPath = StringFormat("/sys/block/%s/queue/rotational", devName);
    in.open(sysPath.c_str());
    if (!in) {
        return {};
    }
    char isRotational = 0;
    if (!(in >> isRotational)) {
        return {};
    }
    if (isRotational == '0') {
        return System::DiskKind::Ssd;
    } else if (isRotational == '1') {
        return System::DiskKind::Hdd;
    }

#else

    return nativeDiskKind(st.st_dev);

#endif

    // Sill got no idea.
    return {};
}

Optional<System::DiskKind> System::diskKindInternal(StringView path) {
    PathStat stat;
    if (pathStat(path, &stat)) {
        return {};
    }
    return android::base::diskKind(stat);
}

Optional<System::DiskKind> System::diskKindInternal(int fd) {
    PathStat stat;
    if (fdStat(fd, &stat)) {
        return {};
    }
    return android::base::diskKind(stat);
}

// static
void System::addLibrarySearchDir(StringView path) {
    System* system = System::get();
    const char* varName = kLibrarySearchListEnvVarName;

    std::string libSearchPath = system->envGet(varName);
    if (libSearchPath.size()) {
        libSearchPath =
                StringFormat("%s%c%s", path, kPathSeparator, libSearchPath);
    } else {
        libSearchPath = path;
    }
    system->envSet(varName, libSearchPath);
}

// static
std::string System::findBundledExecutable(StringView programName) {
    System* const system = System::get();
    const std::string executableName = PathUtils::toExecutableName(programName);

    // first, try the root launcher directory
    std::vector<std::string> pathList = {system->getLauncherDirectory(),
                                         executableName};
    std::string executablePath = PathUtils::recompose(pathList);
    if (system->pathIsFile(executablePath)) {
        return executablePath;
    }

    // it's not there - let's try the 'bin/' subdirectory
    assert(pathList.size() == 2);
    assert(pathList[1] == executableName.c_str());
    pathList[1] = kBinSubDir;
    pathList.push_back(executableName);
    executablePath = PathUtils::recompose(pathList);
    if (system->pathIsFile(executablePath)) {
        return executablePath;
    }

#if defined(_WIN32) && defined(__x86_64)
    // On Windows we don't have a x64 version e2fsprogs, so let's try
    // 32-bit directory if 64-bit lookup failed
    assert(pathList[1] == kBinSubDir);
    pathList[1] = kBin32SubDir;
    executablePath = PathUtils::recompose(pathList);
    if (system->pathIsFile(executablePath)) {
        return executablePath;
    }
#endif

    return std::string();
}

// static
int System::freeRamMb() {
    auto usage = get()->getMemUsage();
    uint64_t freePhysMb = usage.avail_phys_memory / (1024ULL * 1024ULL);
    return freePhysMb;
}

// static
bool System::isUnderMemoryPressure(int* freeRamMb_out) {
    uint64_t currentFreeRam = freeRamMb();

    if (freeRamMb_out) {
        *freeRamMb_out = currentFreeRam;
    }

    return currentFreeRam < kMemoryPressureLimitMb;
}

// static
bool System::isUnderDiskPressure(StringView path, System::FileSize* freeDisk) {
    System::FileSize availableSpace;
    bool success = System::get()->pathFreeSpace(path,
                                                &availableSpace);
    if (success && availableSpace < kDiskPressureLimitBytes) {
        if (freeDisk) *freeDisk = availableSpace;
        return true;
    }

    return false;
}

// static
System::FileSize System::getFilePageSizeForPath(StringView path) {
    System::FileSize pageSize;

#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    // Use dwAllocationGranularity
    // as that is what we need to align
    // the pointer to (64k on most systems)
    pageSize = (System::FileSize)sysinfo.dwAllocationGranularity;
#else // _WIN32

#ifdef __linux__

#define HUGETLBFS_MAGIC       0x958458f6

    struct statfs fsStatus;
    int ret;

    do {
        ret = statfs(c_str(path), &fsStatus);
    } while (ret != 0 && errno == EINTR);

    if (ret != 0) {
        LOG(VERBOSE) << "statvfs('" << path << "') failed: " << strerror(errno)
                     << "\n";
        pageSize = (System::FileSize)getpagesize();
    } else {
        if (fsStatus.f_type == HUGETLBFS_MAGIC) {
            fprintf(stderr, "hugepage detected. size: %lu\n",
                    fsStatus.f_bsize);
            /* It's hugepage, return the huge page size */
            pageSize = (System::FileSize)fsStatus.f_bsize;
        } else {
            pageSize = (System::FileSize)getpagesize();
        }
    }
#else // __linux
    pageSize = (System::FileSize)getpagesize();
#endif // !__linux__

#endif // !_WIN32

    return pageSize;
}

// static
System::FileSize System::getAlignedFileSize(System::FileSize align, System::FileSize size) {
#ifndef ROUND_UP
#define ROUND_UP(n, d) (((n) + (d) - 1) & -(0 ? (n) : (d)))
#endif

    return ROUND_UP(size, align);
}

// static
void System::setEnvironmentVariable(StringView varname, StringView varvalue) {
#ifdef _WIN32
    std::string envStr =
        StringFormat("%s=%s", varname, varvalue);
    // Note: this leaks the result of release().
    _wputenv(Win32UnicodeString(envStr).release());
#else
    if (varvalue.empty()) {
        unsetenv(c_str(varname));
    } else {
        setenv(c_str(varname), c_str(varvalue), 1);
    }
#endif
}

// static
std::string System::getEnvironmentVariable(StringView varname) {
#ifdef _WIN32
    Win32UnicodeString varname_unicode(varname);
    const wchar_t* value = _wgetenv(varname_unicode.c_str());
    if (!value) {
        return std::string();
    } else {
        return Win32UnicodeString::convertToUtf8(value);
    }
#else
    const char* value = getenv(c_str(varname));
    if (!value) {
        value = "";
    }
    return std::string(value);
#endif
}

// static
std::string System::getProgramDirectoryFromPlatform() {
    std::string res;
#if defined(__linux__)
    char path[1024];
    memset(path, 0, sizeof(path));  // happy valgrind!
    int len = readlink("/proc/self/exe", path, sizeof(path));
    if (len > 0 && len < (int)sizeof(path)) {
        char* x = ::strrchr(path, '/');
        if (x) {
            *x = '\0';
            res.assign(path);
        }
    }
#elif defined(__APPLE__)
    char s[PATH_MAX];
    auto pid = getpid();
    proc_pidpath(pid, s, sizeof(s));
    char* x = ::strrchr(s, '/');
    if (x) {
        // skip all slashes - there might be more than one
        while (x > s && x[-1] == '/') {
            --x;
        }
        *x = '\0';
        res.assign(s);
    } else {
        res.assign("<unknown-application-dir>");
    }
#elif defined(_WIN32)
    Win32UnicodeString appDir(PATH_MAX);
    int len = GetModuleFileNameW(0, appDir.data(), appDir.size());
    res.assign("<unknown-application-dir>");
    if (len > 0) {
        if (len > (int)appDir.size()) {
            appDir.resize(static_cast<size_t>(len));
            GetModuleFileNameW(0, appDir.data(), appDir.size());
        }
        std::string dir = appDir.toString();
        char* sep = ::strrchr(&dir[0], '\\');
        if (sep) {
            *sep = '\0';
            res.assign(dir.c_str());
        }
    }
#else
#error "Unsupported platform!"
#endif
    return res;
}

// static
System::WallDuration System::getSystemTimeUs() {
    return kTickCount.getUs();
}

std::string toString(OsType osType) {
    switch (osType) {
    case OsType::Windows:
        return "Windows";
    case OsType::Linux:
        return "Linux";
    case OsType::Mac:
        return "Mac";
    default:
        return "Unknown";
    }
}

#ifdef __APPLE__
// From http://mirror.informatimago.com/next/developer.apple.com/qa/qa2001/qa1123.html
typedef struct kinfo_proc kinfo_proc;

static int GetBSDProcessList(kinfo_proc **procList, size_t *procCount)
    // Returns a list of all BSD processes on the system.  This routine
    // allocates the list and puts it in *procList and a count of the
    // number of entries in *procCount.  You are responsible for freeing
    // this list (use "free" from System framework).
    // On success, the function returns 0.
    // On error, the function returns a BSD errno value.
{
    int                 err;
    kinfo_proc *        result;
    bool                done;
    static const int    name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    // Declaring name as const requires us to cast it when passing it to
    // sysctl because the prototype doesn't include the const modifier.
    size_t              length;

    assert( procList != NULL);
    assert(*procList == NULL);
    assert(procCount != NULL);

    *procCount = 0;

    // We start by calling sysctl with result == NULL and length == 0.
    // That will succeed, and set length to the appropriate length.
    // We then allocate a buffer of that size and call sysctl again
    // with that buffer.  If that succeeds, we're done.  If that fails
    // with ENOMEM, we have to throw away our buffer and loop.  Note
    // that the loop causes use to call sysctl with NULL again; this
    // is necessary because the ENOMEM failure case sets length to
    // the amount of data returned, not the amount of data that
    // could have been returned.

    result = NULL;
    done = false;
    do {
        assert(result == NULL);

        // Call sysctl with a NULL buffer.

        length = 0;
        err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
                      NULL, &length,
                      NULL, 0);
        if (err == -1) {
            err = errno;
        }

        // Allocate an appropriately sized buffer based on the results
        // from the previous call.

        if (err == 0) {
            result = (kinfo_proc*)malloc(length);
            if (result == NULL) {
                err = ENOMEM;
            }
        }

        // Call sysctl again with the new buffer.  If we get an ENOMEM
        // error, toss away our buffer and start again.

        if (err == 0) {
            err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
                          result, &length,
                          NULL, 0);
            if (err == -1) {
                err = errno;
            }
            if (err == 0) {
                done = true;
            } else if (err == ENOMEM) {
                assert(result != NULL);
                free(result);
                result = NULL;
                err = 0;
            }
        }
    } while (err == 0 && ! done);

    // Clean up and establish post conditions.

    if (err != 0 && result != NULL) {
        free(result);
        result = NULL;
    }
    *procList = result;
    if (err == 0) {
        *procCount = length / sizeof(kinfo_proc);
    }

    assert( (err == 0) == (*procList != NULL) );

    return err;
}

// From https://astojanov.wordpress.com/2011/11/16/mac-os-x-resolve-absolute-path-using-process-pid/
Optional<std::string> getPathOfProcessByPid(pid_t pid) {
    int ret;
    std::string result(PROC_PIDPATHINFO_MAXSIZE + 1, 0);
    ret = proc_pidpath(pid, (void*)result.data(), PROC_PIDPATHINFO_MAXSIZE);

    if ( ret <= 0 ) {
        return kNullopt;
    } else {
        return result;
    }
}

#endif

static bool sMultiStringMatch(StringView haystack,
                              const std::vector<StringView>& needles,
                              bool approxMatch) {
    bool found = false;

    for (auto needle : needles) {
        found = found ||
            approxMatch ?
                (haystack.find(needle) !=
                 std::string::npos) :
                (haystack == needle);
    }

    return found;
}

// static
std::vector<System::Pid> System::queryRunningProcessPids(
    const std::vector<StringView>& targets,
    bool approxMatch) {

    std::vector<System::Pid> pids;

// From https://stackoverflow.com/questions/20874381/get-a-process-id-in-c-by-name
#ifdef _WIN32
    // Take a snapshot of all processes in the system.
    ScopedFileHandle handle(
        CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));

    if (!handle.valid()) return {};

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process and exit if unsuccessful.
    if (!Process32First(handle.get(), &pe32)) {
        return {};
    }

    do {
        if (sMultiStringMatch(pe32.szExeFile, targets, approxMatch)) {
            pids.push_back((System::Pid)pe32.th32ProcessID);
        }
    } while (Process32Next(handle.get(), &pe32));
#else

#ifdef __APPLE__

    kinfo_proc* procs = nullptr;
    size_t procCount = 0;

    GetBSDProcessList(&procs, &procCount);

    for (size_t i = 0; i < procCount; i++) {
        pid_t pid = procs[i].kp_proc.p_pid;
        auto path = getPathOfProcessByPid(pid);

        if (!path) continue;

        auto filePart = PathUtils::pathWithoutDirs(*path);

        if (!filePart) continue;

        if (sMultiStringMatch(*filePart, targets, approxMatch)) {
            pids.push_back((System::Pid)pid);
        }
    }

#else
    // TODO: It seems quite error prone to try to list processes on Linux,
    // as opening and iterating through /proc/ seems needed.
    // We only want to use this for Windows anyway. YAGNI.
    fprintf(stderr, "FATAL: Process listing not implemented for Linux.\n");
    abort();
#endif

#endif
    return pids;
}

static bool looksLikeTempDir(StringView tempDir) {
    auto pathComponents = PathUtils::decompose(tempDir);

    if (pathComponents.empty()) return false;

    // basic validity checks
    for (auto pathComponent : pathComponents) {
        // Treat .. as not valid for a temp dir spec.
        // Too complex to handle in its full generality.
        if (pathComponent == "..") {
            return false;
        }
    }

    std::vector<StringView> neededPathComponents = {
#ifdef _WIN32
        "Users", "AppData", "Local", "Temp",
#else
        "tmp", "android-",
#endif
    };

    std::vector<size_t> foundPathComponentIndices;

    for (auto pathComponent: pathComponents)  {
        size_t index = 0;
        for (auto neededPathComponent : neededPathComponents) {
            if (pathComponent.find(
                    neededPathComponent.str()) !=
                std::string::npos) {
                foundPathComponentIndices.push_back(index);
                break;
            }
            ++index;
        }
    }

    if (foundPathComponentIndices.size() != neededPathComponents.size()) {
        return false;
    }

    for (size_t i = 0; i < foundPathComponentIndices.size(); i++) {
        if (foundPathComponentIndices[i] != i) return false;
    }

    return true;
}

// static
void System::deleteTempDir() {
    std::string tempDir = System::get()->getTempDir();

    std::vector<StringView> emulatorExePatterns = {
        "emulator",
        "qemu-system",
    };

#ifdef __linux__
    printf("Temp directory deletion not supported on Linux. Skipping.\n");
#else // !__linux__
    printf("Checking if path is safe for deletion: %s\n", tempDir.c_str());

    if (!looksLikeTempDir(tempDir)) {
        printf("WARNING: %s does not look like a standard temp directory.\n",
               tempDir.c_str());
        printf("Please remove files in there manually.\n");
        return;
    }

    printf("Checking for other emulator instances...");

    std::vector<System::Pid> emuPids =
        queryRunningProcessPids(emulatorExePatterns);

    if (!emuPids.empty()) {
#ifdef _WIN32
        printf("Detected running emulator processes.\n");
        printf("Marking existing files for deletion on reboot...\n");
        path_delete_dir_contents_on_reboot(tempDir.c_str());
        printf("Done.\n");
#else // !_WIN32
        printf("Detected running emulator processes. Skipping.\n");
#endif // !_WIN32
        return;
    }

    printf("Deleting emulator temp directory contents...");
    path_delete_dir_contents(tempDir.c_str());
    printf("done\n");
#endif // !__linux__
}

// static
void System::killProcess(System::Pid pid) {
#ifdef _WIN32
    ScopedFileHandle forKilling(OpenProcess(PROCESS_TERMINATE, false, pid));
    if (forKilling.valid()) {
        TerminateProcess(forKilling.get(), 1);
    }
#else
    kill(pid, SIGKILL);
#endif
}

// static
void System::stopAllEmulatorProcesses() {
// TODO: Implement process grepping for Linux.
#ifndef __linux__
    std::vector<StringView> emulatorExePatterns = {
        "emulator",
        "qemu-system",
    };

    std::vector<System::Pid> emuPids =
        queryRunningProcessPids(emulatorExePatterns);

    for (auto pid : emuPids) {
        System::killProcess(pid);
    }
#endif
}

// static
void System::disableCopyOnWriteForPath(StringView path) {
#ifdef __linux__
    std::vector<std::string> args = {
        "chattr", "+C", path.str(),
    };
    System::get()->runCommand(args,
                   RunOptions::WaitForCompletion |
                   RunOptions::TerminateOnTimeout,
                   1000 // timeout ms
    );
#endif
    // TODO: Disable CoW for Windows/Mac as well
}

#ifdef __APPLE__
void disableAppNap_macImpl(void);
void cpuUsageCurrentThread_macImpl(uint64_t* user, uint64_t* sys);
void hideDockIcon_macImpl(void);
#endif

// static
void System::disableAppNap() {
#ifdef __APPLE__
    disableAppNap_macImpl();
#endif
}

// static
void System::hideDockIcon() {
#ifdef __APPLE__
    hideDockIcon_macImpl();
#endif
}

// static
CpuTime System::cpuTime() {
    CpuTime res;

    res.wall_time_us = kTickCount.getUs();

#ifdef __APPLE__
    cpuUsageCurrentThread_macImpl(
        &res.user_time_us,
        &res.system_time_us);
#else

#ifdef __linux__
    struct rusage usage;
    getrusage(RUSAGE_THREAD, &usage);
    res.user_time_us =
        usage.ru_utime.tv_sec * 1000000ULL +
        usage.ru_utime.tv_usec;
    res.system_time_us =
        usage.ru_stime.tv_sec * 1000000ULL +
        usage.ru_stime.tv_usec;
#else // Windows
    FILETIME creation_time_struct;
    FILETIME exit_time_struct;
    FILETIME kernel_time_struct;
    FILETIME user_time_struct;
    GetThreadTimes(
        GetCurrentThread(),
        &creation_time_struct,
        &exit_time_struct,
        &kernel_time_struct,
        &user_time_struct);
    (void)creation_time_struct;
    (void)exit_time_struct;
    uint64_t user_time_100ns =
        user_time_struct.dwLowDateTime |
        ((uint64_t)user_time_struct.dwHighDateTime << 32);
    uint64_t system_time_100ns =
        kernel_time_struct.dwLowDateTime |
        ((uint64_t)kernel_time_struct.dwHighDateTime << 32);
    res.user_time_us = user_time_100ns / 10;
    res.system_time_us = system_time_100ns / 10;
#endif

#endif
    return res;
}

#ifdef _WIN32
// Based on chromium/src/base/file_version_info_win.cc's CreateFileVersionInfoWin
// Currently used to query Vulkan DLL's on the system and blacklist known
// problematic DLLs
// static

// Windows 10 funcs
typedef DWORD (*get_file_version_info_size_w_t)(LPCWSTR, LPDWORD);
typedef DWORD (*get_file_version_info_w_t)(LPCWSTR, DWORD, DWORD, LPVOID);

// Windows 8 funcs
typedef DWORD (*get_file_version_info_size_ex_w_t)(DWORD, LPCWSTR, LPDWORD);
typedef DWORD (*get_file_version_info_ex_w_t)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);

// common
typedef int (*ver_query_value_w_t)(LPCVOID, LPCWSTR, LPVOID, PUINT);

static get_file_version_info_size_w_t getFileVersionInfoSizeW_func = 0;
static get_file_version_info_w_t getFileVersionInfoW_func = 0;
static get_file_version_info_size_ex_w_t getFileVersionInfoSizeExW_func = 0;
static get_file_version_info_ex_w_t getFileVersionInfoExW_func = 0;
static ver_query_value_w_t verQueryValueW_func = 0;

static bool getFileVersionInfoFuncsAvailable = false;
static bool getFileVersionInfoExFuncsAvailable = false;
static bool canQueryFileVersion = false;

bool initFileVersionInfoFuncs() {
    LOG(VERBOSE) << "querying file version info API...";

    if (canQueryFileVersion) return true;

    HMODULE kernelLib = GetModuleHandle("kernelbase");

    if (!kernelLib) return false;

    LOG(VERBOSE) << "found kernelbase.dll";

    getFileVersionInfoSizeW_func =
        (get_file_version_info_size_w_t)GetProcAddress(kernelLib, "GetFileVersionInfoSizeW");

    if (!getFileVersionInfoSizeW_func) {
        LOG(VERBOSE) << "GetFileVersionInfoSizeW not found. Not on Windows 10?";
    } else {
        LOG(VERBOSE) << "GetFileVersionInfoSizeW found. On Windows 10?";
    }

    getFileVersionInfoW_func =
        (get_file_version_info_w_t)GetProcAddress(kernelLib, "GetFileVersionInfoW");

    if (!getFileVersionInfoW_func) {
        LOG(VERBOSE) << "GetFileVersionInfoW not found. Not on Windows 10?";
    } else {
        LOG(VERBOSE) << "GetFileVersionInfoW found. On Windows 10?";
    }

    getFileVersionInfoFuncsAvailable =
        getFileVersionInfoSizeW_func && getFileVersionInfoW_func;

    if (!getFileVersionInfoFuncsAvailable) {
        getFileVersionInfoSizeExW_func =
            (get_file_version_info_size_ex_w_t)GetProcAddress(kernelLib, "GetFileVersionInfoSizeExW");
        getFileVersionInfoExW_func =
            (get_file_version_info_ex_w_t)GetProcAddress(kernelLib, "GetFileVersionInfoExW");

        getFileVersionInfoExFuncsAvailable =
            getFileVersionInfoSizeExW_func && getFileVersionInfoExW_func;
    }

    if (!getFileVersionInfoFuncsAvailable &&
        !getFileVersionInfoExFuncsAvailable) {
        LOG(VERBOSE) << "Cannot get file version info funcs";
        return false;
    }

    verQueryValueW_func =
        (ver_query_value_w_t)GetProcAddress(kernelLib, "VerQueryValueW");

    if (!verQueryValueW_func) {
        LOG(VERBOSE) << "VerQueryValueW not found";
        return false;
    }

    LOG(VERBOSE) << "VerQueryValueW found. Can query file versions";
    canQueryFileVersion = true;

    return true;
}

bool System::queryFileVersionInfo(StringView path, int* major, int* minor, int* build_1, int* build_2) {

    if (!initFileVersionInfoFuncs()) return false;
    if (!canQueryFileVersion) return false;

    const Win32UnicodeString pathWide(path);
    DWORD dummy;
    DWORD length = 0;
    const DWORD fileVerGetNeutral = 0x02;

    if (getFileVersionInfoFuncsAvailable) {
        length = getFileVersionInfoSizeW_func(pathWide.c_str(), &dummy);
    } else if (getFileVersionInfoExFuncsAvailable) {
        length = getFileVersionInfoSizeExW_func(fileVerGetNeutral, pathWide.c_str(), &dummy);
    }

    if (length == 0) {
        LOG(VERBOSE) << "queryFileVersionInfo: path not found: " << path.str().c_str();
        return false;
    }

    std::vector<uint8_t> data(length, 0);

    if (getFileVersionInfoFuncsAvailable) {
        if (!getFileVersionInfoW_func(pathWide.c_str(), dummy, length, data.data())) {
            LOG(VERBOSE) << "GetFileVersionInfoW failed";
            return false;
        }
    } else if (getFileVersionInfoExFuncsAvailable) {
        if (!getFileVersionInfoExW_func(fileVerGetNeutral, pathWide.c_str(), dummy, length, data.data())) {
            LOG(VERBOSE) << "GetFileVersionInfoExW failed";
            return false;
        }
    }

    VS_FIXEDFILEINFO* fixedFileInfo = nullptr;
    UINT fixedFileInfoLength;

    if (!verQueryValueW_func(
            data.data(),
            L"\\",
            reinterpret_cast<void**>(&fixedFileInfo),
            &fixedFileInfoLength)) {
        LOG(VERBOSE) << "VerQueryValueW failed";
        return false;
    }

    if (major) *major = HIWORD(fixedFileInfo->dwFileVersionMS);
    if (minor) *minor = LOWORD(fixedFileInfo->dwFileVersionMS);
    if (build_1) *build_1 = HIWORD(fixedFileInfo->dwFileVersionLS);
    if (build_2) *build_2 = LOWORD(fixedFileInfo->dwFileVersionLS);

    return true;
}

#else


bool System::queryFileVersionInfo(StringView, int*, int*, int*, int*) {
    return false;
}

#endif // _WIN32

}  // namespace base
}  // namespaconExWs android
