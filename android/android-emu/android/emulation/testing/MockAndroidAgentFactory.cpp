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

