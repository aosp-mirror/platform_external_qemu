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

#include "android/base/Compiler.h"
#include "android/base/misc/StringUtils.h"
#include "android/base/String.h"

#include <string>

namespace android {
namespace crashreport {
// Singleton class constructs paths and provides crash related utility
//   functions
// Result from validatePaths will determine whether crash reporting
//   can be enabled on the system
// Paths checked:
//   Checks if crash dump directory exists, if not tries to create
//   Checks if caBundlePath for libcurl exists
//   checks if crash-service binary exists
class CrashSystem {
public:
    CrashSystem() {}

    virtual ~CrashSystem() {}

    // Return predefined location of crash dump directory
    virtual const std::string& getCrashDirectory(void) = 0;
#ifdef _WIN32
    // Return predefined wide format location of crash dump directory
    virtual const std::wstring getWCrashDirectory(void) = 0;
#endif

    // Returns predefined path to bundle required for curl init
    virtual const std::string& getCaBundlePath(void) = 0;

    // Returns predefined path to crash-service
    virtual const std::string& getCrashServicePath(void) = 0;

    // Returns URL for crash server
    virtual const std::string& getCrashURL(void) = 0;

    // Return crash command line to execute crash service in StringVector
    // format
    // Takes a pipe identfier and a current proc identifier
    virtual const ::android::base::StringVector getCrashServiceCmdLine(
            const std::string& pipe,
            const std::string& proc) = 0;

    // Returns pipe identifier based on a process identifier
    virtual const std::string getPipeName(int ident) = 0;

    // Returns true if all required paths to directories, executables, files
    // are valid.
    virtual bool validatePaths(void) = 0;

    // Returns instance pointer to global structure
    static CrashSystem* get(void);

    // Returns true if path is a valid dump file
    // Takes path to crash dump as argument
    static bool isDump(const std::string& str);

protected:
    static CrashSystem* setForTesting(CrashSystem* crashsystem);

private:
    DISALLOW_COPY_AND_ASSIGN(CrashSystem);
};
}  // namespace crashreport
}  // namespace android
