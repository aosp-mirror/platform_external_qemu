// Copyright 2018 The Android Open Source Project
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

#include "android/console.h"
#include "android/emulation/testing/MockAndroidConsoleAgent.h"
#include <stdio.h>

extern "C" const QAndroidVmOperations* const gQAndroidVmOperations;
extern "C" const QAndroidEmulatorWindowAgent* const
        gQAndroidEmulatorWindowAgent;
namespace android {

static const AndroidConsoleAgents* getConsoleAgents() {
    // Add the mocks you need here, and call register_mock_agents in your test.
    static const AndroidConsoleAgents consoleAgents = {
            nullptr /* gQAndroidBatteryAgent */,
            nullptr /* gQAndroidDisplayAgent */,
            gQAndroidEmulatorWindowAgent /* gQAndroidEmulatorWindowAgent */,
            nullptr /* gQAndroidFingerAgent */,
            nullptr /* gQAndroidLocationAgent */,
            nullptr /* gQAndroidHttpProxyAgent */,
            nullptr /* gQAndroidRecordScreenAgent */,
            nullptr /* gQAndroidTelephonyAgent */,
            nullptr /* gQAndroidUserEventAgent */,
            gQAndroidVmOperations /* gQAndroidVmOperations */,
            nullptr /* gQAndroidNetAgent */,
            nullptr /* gQAndroidLibuiAgent */,
            nullptr /* gQCarDataAgent */,
            nullptr /* gAndroidProxyCB */,
    };
    return &consoleAgents;
}

static bool registered = false;

extern "C" {
void register_mock_agents() {
    android_register_console_agents(getConsoleAgents());
}
}

}  // namespace android
