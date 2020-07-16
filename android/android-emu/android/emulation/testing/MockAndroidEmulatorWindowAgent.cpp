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

static bool sIsFolded = false;
static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow =
                [](void) {
                    printf("window-agent-mock-impl: .getEmulatorWindow\n");
                    return (EmulatorWindow*)nullptr;
                },
        .rotate90Clockwise =
                [](void) {
                    printf("window-agent-mock-impl: .rotate90Clockwise\n");
                    return true;
                },
        .rotate =
                [](SkinRotation rotation) {
                    printf("window-agent-mock-impl: .rotate90Clockwise\n");
                    return true;
                },
        .getRotation =
                [](void) {
                    printf("window-agent-mock-impl: .getRotation\n");
                    return SKIN_ROTATION_0;
                },
        .showMessage =
                [](const char* message, WindowMessageType type, int timeoutMs) {
                    CHECK(android::MockAndroidEmulatorWindowAgent::mock);
                    return android::MockAndroidEmulatorWindowAgent::mock
                            ->showMessage(message, type, timeoutMs);
                },

        .showMessageWithDismissCallback =
                [](const char* message,
                   WindowMessageType type,
                   const char* dismissText,
                   void* context,
                   void (*func)(void*),
                   int timeoutMs) {
                    printf("window-agent-mock-impl: "
                           ".showMessageWithDismissCallback %s\n",
                           message);
                },
        .fold =
                [](bool is_fold) {
                    printf("window-agent-mock-impl: .fold %d\n", is_fold);
                    sIsFolded = is_fold;
                    return true;
                },
        .isFolded = [](void) -> bool {
            printf("window-agent-mock-impl: .isFolded ? %d\n", sIsFolded);
            return sIsFolded;
        },
        .setUIDisplayRegion =
                [](int x_offset, int y_offset, int w, int h) {
                    printf("window-agent-mock-impl: .setUIDisplayRegion %d %d "
                           "%dx%d\n",
                           x_offset, y_offset, w, h);
                },
        .getMultiDisplay = 0,
        .setNoSkin = [](void) {},
        .restoreSkin = [](void) {},
        .updateUIMultiDisplayPage =
                [](uint32_t id) { printf("updateMultiDisplayPage\n"); },
        .getMonitorRect =
                [](uint32_t* w, uint32_t* h) {
                    if (w)
                        *w = 2500;
                    if (h)
                        *h = 1600;
                    return true;
                },

};

extern "C" const QAndroidEmulatorWindowAgent* const
        gMockQAndroidEmulatorWindowAgent = &sQAndroidEmulatorWindowAgent;

namespace android {

MockAndroidEmulatorWindowAgent* MockAndroidEmulatorWindowAgent::mock = nullptr;

}  // namespace android
