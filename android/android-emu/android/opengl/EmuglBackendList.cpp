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
#include "android/utils/path.h"

#define DEBUG 0

#if DEBUG
#  include <stdio.h>
#  define D(...)  printf(__VA_ARGS__)
#else
#  define D(...)  ((void)0)
#endif

namespace android {
namespace opengl {

using android::base::System;

EmuglBackendList::EmuglBackendList(const char* execDir,
                                   int programBitness) :
        mDefaultName("auto"), mNames(), mProgramBitness(0), mExecDir(execDir) {
    // Fix host bitness if needed.
    if (!programBitness) {
        programBitness = System::get()->getProgramBitness();
    }
    mProgramBitness = programBitness;

    mNames = EmuglBackendScanner::scanDir(execDir, programBitness);
}

EmuglBackendList::EmuglBackendList(int programBitness,
                                   const std::vector<std::string>& names) :
        mDefaultName("auto"), mNames(names), mProgramBitness(programBitness) { }

bool EmuglBackendList::contains(const char* name) const {
    for (size_t n = 0; n < mNames.size(); ++n) {
        if (mNames[n] == name) {
            return true;
        }
    }
    return false;
}

std::string EmuglBackendList::getLibDirPath(const char* name) {
    // remove the "_indirect" suffix
    static constexpr android::base::StringView suffix("_indirect");
    std::string nameNoSuffix(name);
    int nameNoSuffixLen = (int)nameNoSuffix.size() - (int)suffix.size();
    if (nameNoSuffixLen > 0 &&
        suffix == nameNoSuffix.c_str() + nameNoSuffixLen) {
        nameNoSuffix.erase(nameNoSuffixLen);
    }
    return android::base::StringFormat(
            "%s" PATH_SEP "%s" PATH_SEP "gles_%s",
            mExecDir,
            mProgramBitness == 64 ? "lib64" : "lib",
            nameNoSuffix.c_str());
}

#ifdef _WIN32
static const char kLibSuffix[] = ".dll";
#elif defined(__APPLE__)
static const char kLibSuffix[] = ".dylib";
#else
static const char kLibSuffix[] = ".so";
#endif

bool EmuglBackendList::getBackendLibPath(const char* name,
                                         Library library,
                                         std::string* libPath) {

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

    std::string path = android::base::StringFormat(
            "%s" PATH_SEP "lib%s%s",
            getLibDirPath(name),
            libraryName,
            kLibSuffix);

    if (!System::get()->pathIsFile(path)) {
        D("%s(name=%s lib=%s): File does not exist: %s\n",
          __FUNCTION__, name, libraryName, path.c_str());
        return false;
    }
    *libPath = path;
    return true;
}

}  // namespace opengl
}  // namespace android
