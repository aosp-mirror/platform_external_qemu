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

#ifndef ANDROID_BASE_SYSTEM_WIN32UTILS_H
#define ANDROID_BASE_SYSTEM_WIN32UTILS_H

#include "android/base/String.h"

#ifndef _WIN32
#error "Only include this file when compiling for Windows!"
#endif

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
// use CommandLineToArgv to convert it into a list of command-line arguments,
// expected the values to be quoted properly.
//
// For more details about this mess, read the MSDN blog post named
// "Everyone quotes arguments the wrong way".
//
// |commandLine| is an input string, that may contain spaces, quotes or
// backslashes. The function returns a new String that contains a version
// of |commandLine| that can be decoded properly by CommandLineToArgv().
static String quoteCommandLine(const char* commandLine);

};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_SYSTEM_WIN32UTILS_H
