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

#pragma once

#ifndef _WIN32
// nothing's here for Posix
#else  // _WIN32

#include "android/base/Optional.h"

#include <memory>
#include <windows.h>

#include <string>
#include <string_view>

typedef enum {
    SVC_ERROR_OPENSCMANAGER = -3,
    SVC_ERROR_OPENSERVICE = -2,
    SVC_ERROR_QUERYSERVICE = -1,
    SVC_NOT_FOUND = 0,
    SVC_STOPPED = SERVICE_STOPPED,
    SVC_START_PENDING = SERVICE_START_PENDING,
    SVC_STOP_PENDING = SERVICE_STOP_PENDING,
    SVC_RUNNING = SERVICE_RUNNING,
    SVC_CONTINUE_PENDING = SERVICE_CONTINUE_PENDING,
    SVC_PAUSE_PENDING = SERVICE_PAUSE_PENDING,
    SVC_PAUSED = SERVICE_PAUSED,
} ServiceStatus;

namespace android {
namespace base {

// This class holds various utility functions for Windows host systems.
// All methods here must be static!
class Win32Utils {
public:
    // Quote a given command-line command or parameter so that it can be
    // sent to _execv() on Windows, and be received properly by the new
    // process.
    //
    // This is necessary to work-around an annoying issue on Windows, where
    // spawn() / exec() functions are mere wrappers around CreateProcess, which
    // always pass the command-line as a _single_ string made of the simple
    // concatenations of their arguments, while the new process will typically
    // use CommandLineToArgv to convert it into a list of command-line
    // arguments, expected the values to be quoted properly.
    //
    // For more details about this mess, read the MSDN blog post named
    // "Everyone quotes arguments the wrong way".
    //
    // |commandLine| is an input string, that may contain spaces, quotes or
    // backslashes. The function returns a new string that contains a version
    // of |commandLine| that can be decoded properly by CommandLineToArgv().
    // |commandLine| must be null-terminated.
    static std::string quoteCommandLine(std::string_view commandLine);

    // Creates a UTF-8 encoded error message string from a Windows System Error
    // Code.  String returned depends on current language id.  See
    // FormatMessage.
    static std::string getErrorString(DWORD error_code);

    // This function dynamically loads "Ntdll.dll" and calls RtlGetVersion in order
    // to properly identify the OS version. GetVersionEx will always return 6.2
    // for Windows 8 and later versions (unless the binary is manifested for a
    // specific OS version).
    static Optional<_OSVERSIONINFOEXW> getWindowsVersion();

    // A small handy struct for an automatic HANDLE management
    struct HandleCloser {
        void operator()(HANDLE h) const { ::CloseHandle(h); }
    };
    using ScopedHandle = std::unique_ptr<void, HandleCloser>;

    // This functions returns the service status givin the service name
    // negative values means error in calling related Win32 APIs.
    // 0 means the service does not exist
    // positive values are what one would expect from "sc query srcName"
    static ServiceStatus getServiceStatus(const char *srcName);
};

}  // namespace base
}  // namespace android

#endif  // _WIN32
