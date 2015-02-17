// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/opengl/EmuglBackendList.h"

#include "android/base/Log.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/opengl/EmuglBackendScanner.h"

#define DEBUG 0

#if DEBUG
#  include <stdio.h>
#  define D(...)  printf(__VA_ARGS__)
#else
#  define D(...)  ((void)0)
#endif

namespace android {
namespace opengl {

using android::base::String;
using android::base::StringVector;
using android::base::System;

EmuglBackendList::EmuglBackendList(const char* execDir,
                                   int hostBitness) :
        mDefaultName("auto"), mNames(), mHostBitness(0), mExecDir(execDir) {
    // Fix host bitness if needed.
    if (!hostBitness) {
        hostBitness = System::kProgramBitness;
    }
    mHostBitness = hostBitness;

    mNames = EmuglBackendScanner::scanDir(execDir, hostBitness);
}

bool EmuglBackendList::contains(const char* name) const {
    for (size_t n = 0; n < mNames.size(); ++n) {
        if (mNames[n] == name) {
            return true;
        }
    }
    return false;
}

String EmuglBackendList::getLibDirPath(const char* name) {
    return android::base::StringFormat(
            "%s/%s/gles_%s",
            mExecDir.c_str(),
            mHostBitness == 64 ? "lib64" : "lib",
            name);
}

bool EmuglBackendList::getBackendLibPath(const char* name,
                                         Library library,
                                         String* libPath) {
#ifdef _WIN32
    static const char kLibPrefix[] = "";
#else
    static const char kLibPrefix[] = "lib";
#endif
#ifdef _WIN32
    static const char kLibSuffix[] = ".dll";
#elif defined(__APPLE__)
    static const char kLibSuffix[] = ".dylib";
#else
    static const char kLibSuffix[] = ".so";
#endif

    const char* libraryName = NULL;
    if (library == LIBRARY_EGL) {
        libraryName = "EGL";
    } else if (library == LIBRARY_GLESv1) {
        libraryName = "GLES_CM";
    } else if (library == LIBRARY_GLESv2) {
        libraryName = "GLESv2";
    } else {
        // Should not happen.
        D("%s: Invalid library type: %d\n", __FUNCTION__, library);
        return false;
    }

    String path = android::base::StringFormat(
            "%s/%s%s%s",
            getLibDirPath(name).c_str(),
            kLibPrefix,
            libraryName,
            kLibSuffix);

    if (!System::get()->pathIsFile(path.c_str())) {
        D("%s(name=%s lib=%s): File does not exist: %s\n",
          __FUNCTION__, name, libraryName, path.c_str());
        return false;
    }
    *libPath = path;
    return true;
}

}  // namespace opengl
}  // namespace android
