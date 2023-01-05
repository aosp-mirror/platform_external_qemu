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
#pragma once

#include "android/emulation/control/AndroidAgentFactory.h"  // for AndroidCo...
#include "android/emulation/control/globals_agent.h"        // for QAndroidG...
#include "host-common/multi_display_agent.h"  // for QAndroidM...
#include "host-common/vm_operations.h"        // for QAndroidV...
#include "host-common/window_agent.h"         // for QAndroidE...

namespace android {
namespace emulation {


// Creates a series of Mock Agents that can be used by the unit tests.
//
// Most of the agents are not defined, add your agents here if you need
// access to addition mock agents. They will be automatically injected
// at the start of the unit tests.
class MockAndroidConsoleFactory : public AndroidConsoleFactory {
public:
    const QAndroidVmOperations* const android_get_QAndroidVmOperations()
            const override;

    const QAndroidMultiDisplayAgent* const
    android_get_QAndroidMultiDisplayAgent() const override;

    const QAndroidEmulatorWindowAgent* const
    android_get_QAndroidEmulatorWindowAgent() const override;

    const QAndroidGlobalVarsAgent* const android_get_QAndroidGlobalVarsAgent()
            const override;
};

#ifdef _WIN32
// A set of flags that are only relevant for windows based unit tests
class WindowsFlags {
public:
    static bool sIsParentProcess;
    static HANDLE sChildRead;
    static HANDLE sChildWrite;
    static char sFileLockPath[MAX_PATH];
};
#endif

}  // namespace emulation
}  // namespace android
