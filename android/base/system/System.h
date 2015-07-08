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

#ifndef ANDROID_BASE_SYSTEM_SYSTEM_H
#define ANDROID_BASE_SYSTEM_SYSTEM_H

#include "android/base/Compiler.h"

#include "android/base/String.h"
#include "android/base/containers/StringVector.h"

namespace android {
namespace base {

// Interface class to the underlying operating system.
class System {
public:
    // Call this function to get the instance
    static System* get();

    // Default constructor doesn't do anything.
    System() {}

    // Default destructor is empty but virtual.
    virtual ~System() {}

    // Return the path of the current program's directory.
    virtual const String& getProgramDirectory() const = 0;

    // Return the host bitness as an integer, either 32 or 64.
    // Note that this is different from the program's bitness. I.e. if
    // a 32-bit program runs under a 64-bit host, getProgramBitness()
    // shall return 32, byt getHostBitness() shall return 64.
    virtual int getHostBitness() const = 0;

    // Return the program bitness as an integer, either 32 or 64.
#ifdef __x86_64__
    static const int kProgramBitness = 64;
#else
    static const int kProgramBitness = 32;
#endif

    // The character used to separator directories in path-related
    // environment variables.
#ifdef _WIN32
    static const char kPathSeparator = ';';
#else
    static const char kPathSeparator = ':';
#endif

    // Environment variable name corresponding to the library search
    // list for shared libraries.
    static const char* kLibrarySearchListEnvVarName;

    // Return the name of the sub-directory containing libraries
    // for the current platform, i.e. "lib" or "lib64" depending
    // on the value of kProgramBitness.
    static const char* kLibSubDir;

    // Return the name of the sub-directory containing executables
    // for the current platform, i.e. "bin" or "bin64" depending
    // on the value of kProgramBitness.
    static const char* kBinSubDir;

    // Return program's bitness, either 32 or 64.
    static int getProgramBitness() { return kProgramBitness; }

    // Prepend a new directory to the system's library search path. This
    // only alters an environment variable like PATH or LD_LIBRARY_PATH,
    // and thus typically takes effect only after spawning/executing a new
    // process.
    static void addLibrarySearchDir(const char* dirPath);

    // Find a bundled executable named |programName|, it must appear in the
    // kBinSubDir of getProgramDirectory(). The name should not include the
    // executable extension (.exe) on Windows.
    // Return an empty string if the file doesn't exist.
    static String findBundledExecutable(const char* programName);

    // Retrieve the value of a given environment variable.
    // Equivalent to getenv().
    virtual const char* envGet(const char* varname) const = 0;

    // Set the value of a given environment variable.
    // If |varvalue| is NULL or empty, this unsets the variable.
    // Equivalent to setenv().
    virtual void envSet(const char* varname, const char* varvalue) = 0;

    // Return true iff |path| exists on the file system.
    virtual bool pathExists(const char* path) = 0;

    // Return true iff |path| exists and is a regular file on the file system.
    virtual bool pathIsFile(const char* path) = 0;

    // Return true iff |path| exists and is a directory on the file system.
    virtual bool pathIsDir(const char* path) = 0;

    // Scan directory |dirPath| for entries, and return them as a sorted
    // vector or entries. If |fullPath| is true, then each item of the
    // result vector contains a full path.
    virtual StringVector scanDirEntries(const char* dirPath,
                                        bool fullPath = false) = 0;

    // Checks the system to see if it is running under a remoting session
    // like Nomachine's NX, Chrome Remote Desktop or Windows Terminal Services.
    // On success, return true and sets |*sessionType| to the detected
    // session type. Otherwise, just return false.
    virtual bool isRemoteSession(String* sessionType) const = 0;

protected:
    static System* setForTesting(System* system);

    // Internal implementation of scanDirEntries() that can be used by
    // mock implementation using a fake file system rooted into a temporary
    // directory or something like that. Always returns short paths.
    static StringVector scanDirInternal(const char* dirPath);

    static bool pathExistsInternal(const char* path);
    static bool pathIsFileInternal(const char* path);
    static bool pathIsDirInternal(const char* path);

private:
    DISALLOW_COPY_AND_ASSIGN(System);
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_SYSTEM_SYSTEM_H
