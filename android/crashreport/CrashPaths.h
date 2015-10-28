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

#include "android/base/misc/StringUtils.h"
#include "android/base/String.h"

#include <string>

struct CrashPaths {
    CrashPaths();

    // Return predefined location of crash dump directory
    const std::string& getCrashDirectory(void);
#ifdef _WIN32
    // Return predefined wide format location of crash dump directory
    const std::wstring& getWCrashDirectory(void);
#endif

    // Return predefined location of emulator launcher executable
    const std::string& getEmulatorPath(void);

    // Returns predefined path to bundle required for curl init
    const std::string& getCaBundlePath(void);

    // Return crash command line to execute crash uploader in StringVector
    // format
    //
    // Takes path to crash dump as argument
    const ::android::base::StringVector getCrashCmdLine(::android::base::String dumpPath);

    // Returns true if all required paths to directories, executables, files
    // are valid.
    bool validatePaths(void);

    // Returns instance pointer to global structure
    static CrashPaths* get(void);

    // Returns true if path is a valid dump file
    //
    // Takes path to crash dump as argument
    static bool isDump(const std::string &str);

    static const char* kCrashURL;
#ifdef _WIN32
    static const wchar_t* kWPipeName;
    static const char* kPipeName;
#endif

private:
    static const char* kEmulatorCrashArg;

    static const char* kEmulatorBinary;
    static const char* kCrashSubDir;

    static const std::string kDumpSuffix;
    std::string mCaBundlePath;
    std::string mEmulatorPath;
    ::android::base::StringVector mCrashCmdLine;
    std::string mCrashDir;
#ifdef _WIN32
    std::wstring mWCrashDir;
#endif
};
