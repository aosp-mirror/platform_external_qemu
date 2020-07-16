// Copyright (C) 2020 The Android Open Source Project
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
#include "android/emulation/testing/MockAndroidAgentFactory.h"

#include <stdio.h>

#include "gtest/gtest.h"

#ifdef _WIN32
using android::emulation::WindowsFlags;
bool WindowsFlags::sIsParentProcess = true;
HANDLE WindowsFlags::sChildRead = nullptr;
HANDLE WindowsFlags::sChildWrite = nullptr;
char WindowsFlags::sFileLockPath[MAX_PATH] = {};
#endif

extern "C" const QAndroidVmOperations* const gMockQAndroidVmOperations;
extern "C" const QAndroidEmulatorWindowAgent* const
        gMockQAndroidEmulatorWindowAgent;
extern "C" const QAndroidMultiDisplayAgent* const
        gMockQAndroidMultiDisplayAgent;

namespace android {
namespace emulation {

const QAndroidVmOperations* const
MockAndroidConsoleFactory::android_get_QAndroidVmOperations() const {
    return gMockQAndroidVmOperations;
}

const QAndroidMultiDisplayAgent* const
MockAndroidConsoleFactory::android_get_QAndroidMultiDisplayAgent() const {
    return gMockQAndroidMultiDisplayAgent;
}

const QAndroidEmulatorWindowAgent* const
MockAndroidConsoleFactory::android_get_QAndroidEmulatorWindowAgent() const {
    return gMockQAndroidEmulatorWindowAgent;
}
}  // namespace emulation
}  // namespace android


// The gtest entry point that will configure the mock agents.
int main(int argc, char** argv) {
#ifdef _WIN32
#define _READ_STR "--read"
#define _WRITE_STR "--write"
#define _FILE_PATH_STR "--file-lock-path"
    for (int i = 1; i < argc; i++) {
        if (!strcmp("--child", argv[i])) {
            WindowsFlags::sIsParentProcess = false;
        } else if (!strncmp(_READ_STR, argv[i], strlen(_READ_STR))) {
            sscanf(argv[i], _READ_STR "=%p", &WindowsFlags::sChildRead);
        } else if (!strncmp(_WRITE_STR, argv[i], strlen(_WRITE_STR))) {
            sscanf(argv[i], _WRITE_STR "=%p", &WindowsFlags::sChildWrite);
        } else if (!strncmp(_FILE_PATH_STR, argv[i], strlen(_FILE_PATH_STR))) {
            sscanf(argv[i], _FILE_PATH_STR "=%s",
                   WindowsFlags::sFileLockPath);
        }
    }
#endif
    ::testing::InitGoogleTest(&argc, argv);
    fprintf(stderr, " -- Injecting mock agents. -- \n");
    android::emulation::injectConsoleAgents(
            android::emulation::MockAndroidConsoleFactory());
    return RUN_ALL_TESTS();
}
