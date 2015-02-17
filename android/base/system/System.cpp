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

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#import <Carbon/Carbon.h>
#endif  // __APPLE__

#ifndef _WIN32
#include <dirent.h>
#endif
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
    HostSystem() : mProgramDir() {}

    virtual ~HostSystem() {}

    virtual const String& getProgramDirectory() const {
        if (mProgramDir.empty()) {
#if defined(__linux__)
            char path[1024];
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
                *x = '\0';
                mProgramDir.assign(s);
            } else {
                mProgramDir.assign("<unknown-application-dir>");
            }
#elif defined(_WIN32)
            char appDir[MAX_PATH];
            int len = GetModuleFileName(0, appDir, sizeof(appDir)-1);
            mProgramDir.assign("<unknown-application-dir>");
            if (len > 0 && len < (int)sizeof(appDir)) {
                appDir[len] = 0;
                char* sep = ::strrchr(appDir, '\\');
                if (sep) {
                    *sep = '\0';
                    mProgramDir.assign(appDir);
                }
            }
#else
#error "Unsupported platform!"
#endif
        }
        return mProgramDir;
    }

    virtual int getHostBitness() const {
#ifdef _WIN32
        char directory[900];

        // Retrieves the path of the WOW64 system directory, which doesn't
        // exist on 32-bit systems.
        unsigned len = GetSystemWow64Directory(directory, sizeof(directory));
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
        ## user-landis always 32-bit, so use "file" to determine the bitness
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
        return system("file -L \"$SHELL\" | grep -q \"x86[_-]64\"") == 0 ?
                64 : 32;
#endif // !_WIN32
    }

    virtual StringVector scanDirEntries(const char* dirPath,
                                        bool fullPath = false) {
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

    virtual const char* envGet(const char* varname) const {
        return getenv(varname);
    }

    virtual void envSet(const char* varname, const char* varvalue) {
#ifdef _WIN32
        if (!varvalue || !varvalue[0]) {
            varvalue = "";
        }
        size_t length = ::strlen(varname) + ::strlen(varvalue) + 2;
        char* string = static_cast<char*>(malloc(length));
        snprintf(string, length, "%s=%s", varname, varvalue);
        putenv(string);
#else
        if (!varvalue || !varvalue[0]) {
            unsetenv(varname);
        } else {
            setenv(varname, varvalue, 1);
        }
#endif
    }

    virtual bool pathExists(const char* path) {
        return pathExistsInternal(path);
    }

    virtual bool pathIsFile(const char* path) {
        return pathIsFileInternal(path);
    }

    virtual bool pathIsDir(const char* path) {
        return pathIsDirInternal(path);
    }

private:
    mutable String mProgramDir;
};

LazyInstance<HostSystem> sHostSystem = LAZY_INSTANCE_INIT;
System* sSystemForTesting = NULL;

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
#else
// static
const char* System::kLibSubDir = "lib";
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
    struct _finddata_t findData;
    intptr_t findIndex = _findfirst(root.c_str(), &findData);
    if (findIndex >= 0) {
        do {
            const char* name = findData.name;
            if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
                result.append(String(name));
            }
        } while (_findnext(findIndex, &findData) >= 0);
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
    int ret = HANDLE_EINTR(access(path, F_OK));
    return (ret == 0) || (errno != ENOENT);
}

// static
bool System::pathIsFileInternal(const char* path) {
    if (!path) {
        return false;
    }
    struct stat st;
    int ret = HANDLE_EINTR(stat(path, &st));
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
    struct stat st;
    int ret = HANDLE_EINTR(stat(path, &st));
    if (ret < 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

}  // namespace base
}  // namespace android
