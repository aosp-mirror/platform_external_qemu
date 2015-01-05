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

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#ifndef _WIN32
#include <dlfcn.h>
#include <stdlib.h>
#endif

namespace emugl {

#ifdef _WIN32

// static
SharedLibrary* SharedLibrary::open(const char* libraryName) {
    HMODULE lib = LoadLibrary(libraryName);
    return lib ? new SharedLibrary(lib) : NULL;
}

SharedLibrary::SharedLibrary(HandleType lib) : mLib(lib) {}

SharedLibrary::~SharedLibrary() {
    if (mLib) {
        FreeLibrary(mLib);
    }
}

SharedLibrary::FunctionPtr SharedLibrary::findSymbol(
        const char* symbolName) {
    if (!mLib || !symbolName) {
        return NULL;
    }
    return reinterpret_cast<FunctionPtr>(
                GetProcAddress(mLib, symbolName));
}

#else // !_WIN32

// static
SharedLibrary* SharedLibrary::open(const char* libraryName) {
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

#ifdef __APPLE__
    // On OSX, some libraries don't include an extension (notably OpenGL)
    // On OSX we try to open |libraryName| first.  If that doesn't exist,
    // we try |libraryName|.dylib
    void* lib = dlopen(libraryName, RTLD_NOW);
    if (lib == NULL) {
        lib = dlopen(libPath, RTLD_NOW);
    }
#else
    void* lib = dlopen(libPath, RTLD_NOW);
#endif

    if (path) {
        free(path);
    }

    return lib ? new SharedLibrary(lib) : NULL;
}

SharedLibrary::SharedLibrary(HandleType lib) : mLib(lib) {}

SharedLibrary::~SharedLibrary() {
    if (mLib) {
        dlclose(mLib);
    }
}

SharedLibrary::FunctionPtr SharedLibrary::findSymbol(
        const char* symbolName) {
    if (!mLib || !symbolName) {
        return NULL;
    }
    return reinterpret_cast<FunctionPtr>(dlsym(mLib, symbolName));
}

#endif  // !_WIN32

}  // namespace emugl
