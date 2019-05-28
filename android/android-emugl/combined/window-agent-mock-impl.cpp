// Copyright 2015-2017 The Android Open Source Project
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

#include <stdio.h>
#include "android/emulation/control/window_agent.h"

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
                    printf("window-agent-mock-impl: .showMessage %s\n", message);
                },
        .showMessageWithDismissCallback =
                [](const char* message, WindowMessageType type,
                   const char* dismissText, void* context,
                   void (*func)(void*), int timeoutMs) {
                    printf("window-agent-mock-impl: .showMessageWithDismissCallback %s\n", message);
                },
        .fold =
                [](bool is_fold) {
                    printf("window-agent-mock-impl: .fold %d\n", is_fold);
                    sIsFolded = is_fold;
                },
        .isFolded =
                [](void) -> bool {
                    printf("window-agent-mock-impl: .isFolded ? %d\n", sIsFolded);
                    return sIsFolded;
                },
        .setUIDisplayRegion =
                [](int x_offset, int y_offset, int w, int h) {
                    printf("window-agent-mock-impl: .setUIDisplayRegion %d %d %dx%d\n",
                           x_offset, y_offset, w, h);
                },
        .setMultiDisplay =
                [](int id, int x, int y, int w, int h, bool add) {
                    printf("window-agent-mock-impl: .setMultiDisplay id %d %d %d %dx%d %s\n",
                           id, x, y, w, h, add ? "add" : "del");
                },
};

const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent =
        &sQAndroidEmulatorWindowAgent;
