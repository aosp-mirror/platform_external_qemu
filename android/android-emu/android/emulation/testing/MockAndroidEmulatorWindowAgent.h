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

#include "android/emulation/control/window_agent.h"

#include <gmock/gmock.h>

namespace android {

#define WINDOW_MOCK(MOCK_MACRO, name)              \
    MOCK_MACRO(name, std::remove_pointer<decltype( \
                             QAndroidEmulatorWindowAgent::name)>::type)

class MockAndroidEmulatorWindowAgent {
public:
    static MockAndroidEmulatorWindowAgent* mock;

    // void showMessage(const char* message, WindowMessageType type, int
    //                  timeoutMs)
    WINDOW_MOCK(MOCK_METHOD3, showMessage);
};

}  // namespace android
