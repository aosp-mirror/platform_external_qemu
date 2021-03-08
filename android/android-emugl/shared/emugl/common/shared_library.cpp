// Copyright (C) 2014 The Android Open Source Project
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

#include "emugl/common/shared_library.h"

#include "android/base/files/PathUtils.h"
#include "android/base/FunctionView.h"
#include "android/base/memory/LazyInstance.h"

#include "emugl/common/logging.h"

#include <functional>
#include <vector>

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#ifndef _WIN32
#include <dlfcn.h>
#include <stdlib.h>
#endif

using android::base::FunctionView;
using android::base::LazyInstance;
using android::base::PathUtils;

namespace emugl {

class LibrarySearchPaths {
public:
    LibrarySearchPaths() = default;

    void addPath(const char* path) {
        mPaths.push_back(path);
    }

    void forEachPath(FunctionView<void(const std::string&)> func) {
        for (const auto& path: mPaths) {
            func(path);
        }
    }

private:
    std::vector<std::string> mPaths;
};

static LazyInstance<LibrarySearchPaths> sSearchPaths = LAZY_INSTANCE_INIT;

SharedLibrary::LibraryMap SharedLibrary::s_libraryMap = LibraryMap();

// static
SharedLibrary* SharedLibrary::open(const char* libraryName) {
    GL_LOG("SharedLibrary::open for [%s]\n", libraryName);
    char error[1];
    return open(libraryName, error, sizeof(error));
}

SharedLibrary* SharedLibrary::open(const char* libraryName,
                                   char* error,
                                   size_t errorSize) {
    auto lib = s_libraryMap.find(libraryName);

    if (lib == s_libraryMap.end()) {
        GL_LOG("SharedLibrary::open for [%s]: not found in map, open for the first time\n", libraryName);
        SharedLibrary* load = do_open(libraryName, error, errorSize);
        if (load != nullptr) {
            s_libraryMap[libraryName] = std::move(
                    std::unique_ptr<SharedLibrary, SharedLibrary::Deleter>(
                            load));
        }
        return load;
    }

    return lib->second.get();
}

#ifdef _WIN32

// static
SharedLibrary* SharedLibrary::do_open(const char* libraryName,
                                   char* error,
                                   size_t errorSize) {
    GL_LOG("SharedLibrary::open for [%s] (win32): call LoadLibrary\n", libraryName);
    HMODULE lib = LoadLibrary(libraryName);

    // Try a bit harder to find the shared library if we cannot find it.
    if (!lib) {
        GL_LOG("SharedLibrary::open for [%s] can't find in default path. Searching alternatives...\n",
               libraryName);
        sSearchPaths->forEachPath([&lib, libraryName](const std::string& path) {
            if (!lib) {
                auto libName = PathUtils::join(path, libraryName);
                GL_LOG("SharedLibrary::open for [%s]: trying [%s]\n",
                       libraryName, libName.c_str());
                lib = LoadLibrary(libName.c_str());
                GL_LOG("SharedLibrary::open for [%s]: trying [%s]. found? %d\n",
                       libraryName, libName.c_str(), lib != nullptr);
            }
        });
    }

    if (lib) {
        constexpr size_t kMaxPathLength = 2048;
        char fullPath[kMaxPathLength];
        GetModuleFileNameA(lib, fullPath, kMaxPathLength);
        GL_LOG("SharedLibrary::open succeeded for [%s]. File name: [%s]\n",
               libraryName, fullPath);
        return new SharedLibrary(lib);
    }

    if (errorSize == 0) {
        GL_LOG("SharedLibrary::open for [%s] failed, but no error\n",
               libraryName);
        return NULL;
    }

    // Convert error into human-readable message.
    DWORD errorCode = ::GetLastError();
    LPSTR message = NULL;
    size_t messageLen = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR) &message,
            0,
            NULL);

    int ret = snprintf(error, errorSize, "%.*s", (int)messageLen, message);
    if (ret < 0 || ret == static_cast<int>(errorSize)) {
        // snprintf() on Windows doesn't behave as expected by C99,
        // this path is to ensure that the result is always properly
        // zero-terminated.
        ret = static_cast<int>(errorSize - 1);
        error[ret] = '\0';
    }
    // Remove any trailing \r\n added by FormatMessage
    if (ret > 0 && error[ret - 1] == '\n') {
        error[--ret] = '\0';
    }
    if (ret > 0 && error[ret - 1] == '\r') {
        error[--ret] = '\0';
    }
    GL_LOG("Failed to load [%s]. Error string: [%s]\n",
           libraryName, error);

    return NULL;
}

SharedLibrary::SharedLibrary(HandleType lib) : mLib(lib) {}

SharedLibrary::~SharedLibrary() {
    if (mLib) {
        // BUG: 66013149
        // In windows it sometimes hang on exit when destroying s_libraryMap.
        // Let's skip freeing the library, since pretty much the only situation
        // we need to do it, is on exit.
        //FreeLibrary(mLib);
    }
}

SharedLibrary::FunctionPtr SharedLibrary::findSymbol(
        const char* symbolName) const {
    if (!mLib || !symbolName) {
        return NULL;
    }
    return reinterpret_cast<FunctionPtr>(
                GetProcAddress(mLib, symbolName));
}

#else // !_WIN32

// static
SharedLibrary* SharedLibrary::do_open(const char* libraryName,
                                   char* error,
                                   size_t errorSize) {
    GL_LOG("SharedLibrary::open for [%s] (posix): begin\n", libraryName);

    const char* libPath = libraryName;
    char* path = NULL;

    const char* libBaseName = strrchr(libraryName, '/');
    if (!libBaseName) {
        libBaseName = libraryName;
    }

    if (!strchr(libBaseName, '.')) {
        // There is no extension in this library name, so append one.
#ifdef __APPLE__
        static const char kDllExtension[] = ".dylib";
#else
        static const char kDllExtension[] = ".so";
#endif
        size_t pathLen = strlen(libraryName) + sizeof(kDllExtension);
        path = static_cast<char*>(malloc(pathLen));
        snprintf(path, pathLen, "%s%s", libraryName, kDllExtension);
        libPath = path;
    }

    dlerror();  // clear error.

#ifdef __APPLE__
    // On OSX, some libraries don't include an extension (notably OpenGL)
    // On OSX we try to open |libraryName| first.  If that doesn't exist,
    // we try |libraryName|.dylib
    GL_LOG("SharedLibrary::open for [%s] (posix,darwin): call dlopen\n", libraryName);
    void* lib = dlopen(libraryName, RTLD_NOW);
    if (lib == NULL) {
        GL_LOG("SharedLibrary::open for [%s] (posix,darwin): failed, "
               "try again with [%s]\n", libraryName, libPath);
        lib = dlopen(libPath, RTLD_NOW);

        sSearchPaths->forEachPath([&lib, libraryName, libPath](const std::string& path) {
            if (!lib) {
                auto libName = PathUtils::join(path, libraryName);
                GL_LOG("SharedLibrary::open for [%s] (posix,darwin): still failed, "
                       "try [%s]\n", libraryName, libName.c_str());
                lib = dlopen(libName.c_str(), RTLD_NOW);
                if (!lib) {
                    auto libPathName = PathUtils::join(path, libPath);
                    GL_LOG("SharedLibrary::open for [%s] (posix,darwin): still failed, "
                           "try [%s]\n", libraryName, libPathName.c_str());
                    lib = dlopen(libPathName.c_str(), RTLD_NOW);
                }
            }
        });
    }
#else
    GL_LOG("SharedLibrary::open for [%s] (posix,linux): call dlopen on [%s]\n",
           libraryName, libPath);
    void* lib = dlopen(libPath, RTLD_NOW);
#endif

    sSearchPaths->forEachPath([&lib, libPath, libraryName](const std::string& path) {
        if (!lib) {
            auto libPathName = PathUtils::join(path, libPath);
            GL_LOG("SharedLibrary::open for [%s] (posix): try again with %s\n",
                   libraryName,
                   libPathName.c_str());
            lib = dlopen(libPathName.c_str(), RTLD_NOW);
        }
    });

    if (path) {
        free(path);
    }

    if (lib) {
        return new SharedLibrary(lib);
    }

    snprintf(error, errorSize, "%s", dlerror());
    GL_LOG("SharedLibrary::open for [%s] failed (posix). dlerror: [%s]\n",
           libraryName, error);
    return NULL;
}

SharedLibrary::SharedLibrary(HandleType lib) : mLib(lib) {}

SharedLibrary::~SharedLibrary() {
    if (mLib) {
        dlclose(mLib);
    }
}

SharedLibrary::FunctionPtr SharedLibrary::findSymbol(
        const char* symbolName) const {
    if (!mLib || !symbolName) {
        return NULL;
    }
    return reinterpret_cast<FunctionPtr>(dlsym(mLib, symbolName));
}

#endif  // !_WIN32

// static
void SharedLibrary::addLibrarySearchPath(const char* path) {
    sSearchPaths->addPath(path);
}

}  // namespace emugl
