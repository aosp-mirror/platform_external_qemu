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

#include "android/base/EintrWrapper.h"
#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/StringFormat.h"

#ifdef _WIN32
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#endif

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#include <shlobj.h>
#endif

#ifdef __APPLE__
#import <Carbon/Carbon.h>
#endif  // __APPLE__

#include <array>

#ifndef _WIN32
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace android {
namespace base {

namespace {

class HostSystem : public System {
public:
    HostSystem() : mProgramDir(), mHomeDir(), mAppDataDir() {}

    virtual ~HostSystem() {}

    virtual const String& getProgramDirectory() const {
        if (mProgramDir.empty()) {
#if defined(__linux__)
            char path[1024];
            memset(path, 0, sizeof(path));  // happy valgrind!
            int len = readlink("/proc/self/exe", path, sizeof(path));
            if (len > 0 && len < (int)sizeof(path)) {
                char* x = ::strrchr(path, '/');
                if (x) {
                    *x = '\0';
                    mProgramDir.assign(path);
                }
            }
#elif defined(__APPLE__)
            ProcessSerialNumber psn;
            GetCurrentProcess(&psn);
            CFDictionaryRef dict =
                    ProcessInformationCopyDictionary(&psn, 0xffffffff);
            CFStringRef value = (CFStringRef)CFDictionaryGetValue(
                    dict, CFSTR("CFBundleExecutable"));
            char s[PATH_MAX];
            CFStringGetCString(value, s, PATH_MAX - 1, kCFStringEncodingUTF8);
            char* x = ::strrchr(s, '/');
            if (x) {
                // skip all slashes - there might be more than one
                while (x > s && x[-1] == '/') {
                    --x;
                }
                *x = '\0';
                mProgramDir.assign(s);
            } else {
                mProgramDir.assign("<unknown-application-dir>");
            }
#elif defined(_WIN32)
            Win32UnicodeString appDir(PATH_MAX);
            int len = GetModuleFileNameW(0, appDir.data(), appDir.size());
            mProgramDir.assign("<unknown-application-dir>");
            if (len > 0) {
                if (len > (int)appDir.size()) {
                    appDir.resize(static_cast<size_t>(len));
                    GetModuleFileNameW(0, appDir.data(), appDir.size());
                }
                String dir = appDir.toString();
                char* sep = ::strrchr(&dir[0], '\\');
                if (sep) {
                    *sep = '\0';
                    mProgramDir.assign(dir.c_str());
                }
            }
#else
#error "Unsupported platform!"
#endif
        }
        return mProgramDir;
    }

    virtual String getCurrentDirectory() const {
#if defined(_WIN32)
        int currentLen = GetCurrentDirectoryW(0, nullptr);
        if (currentLen < 0) {
            // Could not get size of working directory. Something is really
            // fishy here, return an empty string.
            return String();
        }
        wchar_t* currentDir =
                static_cast<wchar_t*>(calloc(currentLen + 1, sizeof(wchar_t)));
        if (!GetCurrentDirectoryW(currentLen + 1, currentDir)) {
            // Again, some unexpected problem. Can't do much here.
            // Make the string empty.
            currentDir[0] = L'0';
        }

        String result = Win32UnicodeString::convertToUtf8(currentDir);
        ::free(currentDir);
        return result;
#else   // !_WIN32
        char currentDir[PATH_MAX];
        if (!getcwd(currentDir, sizeof(currentDir))) {
            return String();
        }
        return String(currentDir);
#endif  // !_WIN32
    }

    virtual const String& getLauncherDirectory() const {
        if (mLauncherDir.empty()) {
            String programDir = getProgramDirectory();

            String launcherName("emulator");
#ifdef _WIN32
            launcherName += ".exe";
#endif
            StringVector pathList;
            pathList.push_back(programDir);
            pathList.push_back(launcherName);
            String launcherPath = PathUtils::recompose(pathList);

            if (pathIsFile(launcherPath.c_str())) {
                mLauncherDir = programDir;
                return mLauncherDir;
            }

            // we are probably executing a qemu2 binary, which live in
            // <launcher-dir>/qemu/<os>-<arch>/
            // look for the launcher in grandparent directory
            StringVector programDirVector =
                    PathUtils::decompose(programDir.c_str());
            if (programDirVector.size() >= 2) {
                programDirVector.resize(programDirVector.size() - 2);
                String grandparentDir = PathUtils::recompose(programDirVector);
                programDirVector.push_back(launcherName);
                String launcherPath = PathUtils::recompose(programDirVector);
                if (pathIsFile(launcherPath.c_str())) {
                    mLauncherDir = grandparentDir;
                    return mLauncherDir;
                }
            }

            mLauncherDir.assign("<unknown-launcher-dir>");
        }
        return mLauncherDir;
    }

    virtual const String& getHomeDirectory() const {
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
                String homedrive = envGet("HOMEDRIVE");
                String homepath = envGet("HOMEPATH");
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

    virtual const String& getAppDataDirectory() const {
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


    virtual int getHostBitness() const {
#ifdef _WIN32

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

#else // !_WIN32
    /*
        This function returns 64 if host is running 64-bit OS, or 32 otherwise.

        It uses the same technique in ndk/build/core/ndk-common.sh.
        Here are comments from there:

        ## On Linux or Darwin, a 64-bit kernel (*) doesn't mean that the
        ## user-land is always 32-bit, so use "file" to determine the bitness
        ## of the shell that invoked us. The -L option is used to de-reference
        ## symlinks.
        ##
        ## Note that on Darwin, a single executable can contain both x86 and
        ## x86_64 machine code, so just look for x86_64 (darwin) or x86-64
        ## (Linux) in the output.

        (*) ie. The following code doesn't always work:
            struct utsname u;
            int host_runs_64bit_OS = (uname(&u) == 0 &&
                                     strcmp(u.machine, "x86_64") == 0);
    */
        if (system("file -L \"$SHELL\" | grep -q \"x86[_-]64\"") == 0) {
                return 64;
        } else if (system("file -L \"$SHELL\" > /dev/null")) {
            fprintf(stderr, "WARNING: Cannot decide host bitness because "
                    "$SHELL is not properly defined; 32 bits assumed.\n");
        }
        return 32;

#endif // !_WIN32
    }

    virtual StringVector scanDirEntries(const char* dirPath,
                                        bool fullPath = false) const {
        StringVector result = scanDirInternal(dirPath);
        if (fullPath) {
            // Pre-pend |dirPath| to each entry.
            String prefix =
                    PathUtils::addTrailingDirSeparator(String(dirPath));
            for (size_t n = 0; n < result.size(); ++n) {
                String path = prefix;
                path.append(result[n]);
                result[n] = path;
            }
        }
        return result;
    }

    virtual String envGet(const char* varname) const {
#ifdef _WIN32
        Win32UnicodeString varname_unicode(varname);
        const wchar_t* value = _wgetenv(varname_unicode.c_str());
        if (!value) {
            return String();
        } else {
            return Win32UnicodeString::convertToUtf8(value);
        }
#else
        const char* value = getenv(varname);
        if (!value) {
            value = "";
        }
        return String(value);
#endif
    }

    virtual void envSet(const char* varname, const char* varvalue) {
#ifdef _WIN32
        if (!varvalue || !varvalue[0]) {
            varvalue = "";
        }
        String envStr = StringFormat("%s=%s", varname, varvalue);
        // Note: this leaks the result of release().
        _wputenv(Win32UnicodeString(envStr).release());
#else
        if (!varvalue || !varvalue[0]) {
            unsetenv(varname);
        } else {
            setenv(varname, varvalue, 1);
        }
#endif
    }

    virtual bool envTest(const char* varname) const {
#ifdef _WIN32
        Win32UnicodeString varname_unicode(varname);
        const wchar_t* value = _wgetenv(varname_unicode.c_str());
        return value && value[0] != L'\0';
#else
        const char* value = getenv(varname);
        return value && value[0] != '\0';
#endif
    }

    virtual bool isRemoteSession(String* sessionType) const {
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
#ifdef _WIN32
        if (GetSystemMetrics(SM_REMOTESESSION)) {
            if (sessionType) {
                *sessionType = "Windows Remote Desktop";
            }
            return true;
        }
#endif  // _WIN32
        return false;
    }

    virtual bool pathExists(const char* path) const {
        return pathExistsInternal(path);
    }

    virtual bool pathIsFile(const char* path) const {
        return pathIsFileInternal(path);
    }

    virtual bool pathIsDir(const char* path) const {
        return pathIsDirInternal(path);
    }

    virtual bool pathCanRead(const char* path) const override {
        return pathCanReadInternal(path);
    }

    virtual bool pathCanWrite(const char* path) const override {
        return pathCanWriteInternal(path);
    }

    virtual bool pathCanExec(const char* path) const override {
        return pathCanExecInternal(path);
    }

    Times getProcessTimes() const {
        Times res = {};

#ifdef _WIN32
        FILETIME creationTime = {};
        FILETIME exitTime = {};
        FILETIME kernelTime = {};
        FILETIME userTime = {};
        ::GetProcessTimes(::GetCurrentProcess(),
            &creationTime, &exitTime, &kernelTime, &userTime);

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

        return res;
    }

    time_t getUnixTime() const {
        return time(NULL);
    }

    bool runSilentCommand(const StringVector& commandLine, bool wait) {
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
        String command = "/C";
        for (size_t n = 0; n < commandLine.size(); ++n) {
            command += " ";
            command += android::base::Win32Utils::quoteCommandLine(commandLine[n].c_str());
        }

        Win32UnicodeString command_unicode(command);

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

        CloseHandle(pinfo.hThread);

        if (wait) {
            ::WaitForSingleObject(pinfo.hProcess, INFINITE);
        }

        CloseHandle(pinfo.hProcess);

        return true;
#else  // !_WIN32
        char** params = new char*[commandLine.size()];
        for (size_t n = 0; n < commandLine.size(); ++n) {
            params[n] = (char*)commandLine[n].c_str();
        }

        int pid = fork();
        if (pid < 0) {
            return false;
        }
        if (pid != 0) {
            // Parent process returns.
            delete [] params;
            if (wait) {
                int status;
                HANDLE_EINTR(waitpid(pid, &status, 0));
            }
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

    virtual String getTempDir() const {
#ifdef _WIN32
        Win32UnicodeString path(PATH_MAX);
        DWORD retval = GetTempPathW(path.size(), path.data());
        if (!retval) {
            // Best effort!
            return String("C:\\Temp");
        }
        if (retval > path.size()) {
            path.resize(static_cast<size_t>(retval));
            GetTempPathW(path.size(), path.data());
        }
        // The result of GetTempPath() is already user-dependent
        // so don't append the username or userid to the result.
        path.append(L"\\AndroidEmulator");
        ::_wmkdir(path.c_str());
        return path.toString();
#else   // !_WIN32
        String result;
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
        ::mkdir(result.c_str(), 0744);
        return result;
#endif  // !_WIN32
    }

private:
    mutable String mProgramDir;
    mutable String mLauncherDir;
    mutable String mHomeDir;
    mutable String mAppDataDir;
};

LazyInstance<HostSystem> sHostSystem = LAZY_INSTANCE_INIT;
System* sSystemForTesting = NULL;

#ifdef _WIN32
// Return |path| as a Unicode string, while discarding trailing separators.
Win32UnicodeString win32Path(const char* path) {
    Win32UnicodeString wpath(path, ::strlen(path));
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

using PathStat = struct _stat;

#else  // _WIN32

using PathStat = struct stat;

#endif  // _WIN32

int pathStat(const char* path, PathStat* st) {
#ifdef _WIN32
    return _wstat(win32Path(path).c_str(), st);
#else   // !_WIN32
    return HANDLE_EINTR(stat(path, st));
#endif  // !_WIN32
}

int pathAccess(const char* path, int mode) {
#ifdef _WIN32
    return _waccess(win32Path(path).c_str(), mode);
#else   // !_WIN32
    return HANDLE_EINTR(access(path, mode));
#endif  // !_WIN32
}

}  // namespace

// static
System* System::get() {
    System* result = sSystemForTesting;
    if (!result) {
        result = sHostSystem.ptr();
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

// static
StringVector System::scanDirInternal(const char* dirPath) {
    StringVector result;

    if (!dirPath) {
        return result;
    }
#ifdef _WIN32
    String root(dirPath);
    root = PathUtils::addTrailingDirSeparator(root);
    root += '*';
    Win32UnicodeString rootUnicode(root);
    struct _wfinddata_t findData;
    intptr_t findIndex = _wfindfirst(rootUnicode.c_str(), &findData);
    if (findIndex >= 0) {
        do {
            const wchar_t* name = findData.name;
            if (wcscmp(name, L".") != 0 && wcscmp(name, L"..") != 0) {
                result.append(Win32UnicodeString::convertToUtf8(name));
            }
        } while (_wfindnext(findIndex, &findData) >= 0);
        _findclose(findIndex);
    }
#else  // !_WIN32
    DIR* dir = ::opendir(dirPath);
    if (dir) {
        for (;;) {
            struct dirent* entry = ::readdir(dir);
            if (!entry) {
                break;
            }
            const char* name = entry->d_name;
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
                result.append(String(name));
            }
        }
        ::closedir(dir);
    }
#endif  // !_WIN32
    sortStringVector(&result);
    return result;
}

// static
bool System::pathExistsInternal(const char* path) {
    if (!path) {
        return false;
    }
    int ret = pathAccess(path, F_OK);
    return (ret == 0) || (errno != ENOENT);
}

// static
bool System::pathIsFileInternal(const char* path) {
    if (!path) {
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
bool System::pathIsDirInternal(const char* path) {
    if (!path) {
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
bool System::pathCanReadInternal(const char* path) {
    if (!path) {
        return false;
    }
    return pathAccess(path, R_OK) == 0;
}

// static
bool System::pathCanWriteInternal(const char* path) {
    if (!path) {
        return false;
    }
    return pathAccess(path, W_OK) == 0;
}

// static
bool System::pathCanExecInternal(const char* path) {
    if (!path) {
        return false;
    }
    return pathAccess(path, X_OK) == 0;
}

// static
void System::addLibrarySearchDir(const char* path) {
    System* system = System::get();
    const char* varName = kLibrarySearchListEnvVarName;

    String libSearchPath = system->envGet(varName);
    if (libSearchPath.size()) {
        libSearchPath = StringFormat("%s%c%s",
                                     path,
                                     kPathSeparator,
                                     libSearchPath.c_str());
    } else {
        libSearchPath = path;
    }
    system->envSet(varName, libSearchPath.c_str());
}

// static
String System::findBundledExecutable(const char* programName) {
    System* system = System::get();

    String executableName(programName);
#ifdef _WIN32
    executableName += ".exe";
#endif
    StringVector pathList;
    pathList.push_back(system->getLauncherDirectory());
    pathList.push_back(kBinSubDir);
    pathList.push_back(executableName);

    String executablePath = PathUtils::recompose(pathList);
    if (!system->pathIsFile(executablePath.c_str())) {
#if defined(_WIN32) && defined(__x86_64)
        // On Windows we don't have a x64 version e2fsprogs, so let's try
        // 32-bit directory if 64-bit lookup failed
        assert(pathList[1] == kBinSubDir);
        pathList[1] = kBin32SubDir;
        executablePath = PathUtils::recompose(pathList);
        if (!system->pathIsFile(executablePath.c_str())) {
            executablePath.clear();
        }
#else
        executablePath.clear();
#endif
    }

    return executablePath;
}

void System::sleepMs(unsigned n) {
#ifdef _WIN32
    ::Sleep(n);
#else
    usleep(n * 1000);
#endif
}

}  // namespace base
}  // namespace android
