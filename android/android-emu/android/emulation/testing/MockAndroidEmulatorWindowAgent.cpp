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

#include "android/emulation/testing/MockAndroidEmulatorWindowAgent.h"
#include "android/base/Log.h"

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow = nullptr,  // Not currently mocked.
        .rotate90Clockwise = nullptr,  // Not currently mocked.
        .rotate = nullptr,             // Not currently mocked.
        .getRotation = nullptr,        // Not currently mocked.
        .showMessage =
                [](const char* message, WindowMessageType type, int timeoutMs) {
                    CHECK(android::MockAndroidEmulatorWindowAgent::mock);
                    return android::MockAndroidEmulatorWindowAgent::mock
                            ->showMessage(message, type, timeoutMs);
                },

        .showMessageWithDismissCallback = nullptr,  // Not currently mocked.
};

extern "C" const QAndroidEmulatorWindowAgent* const
        gQAndroidEmulatorWindowAgent = &sQAndroidEmulatorWindowAgent;

namespace android {

MockAndroidEmulatorWindowAgent* MockAndroidEmulatorWindowAgent::mock = nullptr;

}  // namespace android
