
// Copyright (C) 2022 Google Inc.
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

#include "android/base/Process.h"

#if defined(__linux__)
#include <fstream>
#endif
#include <string>

namespace android {
namespace base {
namespace guest {

std::string getProcessName(void)
{
    // Support for "getprogname" function in bionic was introduced in L (API level 21)
    std::string processName;
#if defined(__ANDROID__) && __ANDROID_API__ >= 21
        processName = std::string(getprogname());
#elif defined(__linux__)
        {
            std::ifstream stream("/proc/self/cmdline");
            if (stream.is_open()) {
                processName = std::string(std::istreambuf_iterator<char>(stream),
                                          std::istreambuf_iterator<char>());
            }
        }
#endif
    return processName;
}

}  // namespace guest
}  // namespace base
}  // namespace android