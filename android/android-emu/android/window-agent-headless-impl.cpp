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

#include "android/emulation/control/window_agent.h"

#include "android/emulator-window.h"
#include "android/skin/qt/emulator-no-qt-no-window.h"
#include "android/utils/debug.h"

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow = emulator_window_get,
        .rotate90Clockwise =
                [] {
                    return emulator_window_rotate_90(true);
                },
        .rotate = emulator_window_rotate,
        .getRotation =
                [] {
                    EmulatorWindow* window = emulator_window_get();
                    if (!window) return SKIN_ROTATION_0;
                    SkinLayout* layout = emulator_window_get_layout(window);
                    if (!layout) return SKIN_ROTATION_0;
                    return layout->orientation;
                },
        .showMessage =
                [](const char* message, WindowMessageType type, int timeoutMs) {
                    const auto printer =
                            (type == WINDOW_MESSAGE_ERROR)
                                    ? &derror
                                    : (type == WINDOW_MESSAGE_WARNING)
                                              ? &dwarning
                                              : &dprint;
                    printer("%s", message);
                },
        .showMessageWithDismissCallback =
                [](const char* message, WindowMessageType type,
                   const char* dismissText, void* context,
                   void (*func)(void*), int timeoutMs) {
                    const auto printer =
                            (type == WINDOW_MESSAGE_ERROR)
                                    ? &derror
                                    : (type == WINDOW_MESSAGE_WARNING)
                                              ? &dwarning
                                              : &dprint;
                    printer("%s", message);
                    // Don't necessarily perform the func since the
                    // user doesn't get a chance to dismiss.
                },
        .fold =
                [](bool is_fold) {
                   if (const auto win = EmulatorNoQtNoWindow::getInstance()) {
                      if (is_fold)
                          win->fold();
                      else
                          win->unFold();
                   }
                },
        .isFolded =
                [] {
                    if (const auto win = EmulatorNoQtNoWindow::getInstance()) {
                        return win->isFolded();
                    }
                    return false;
                }
};

const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent =
        &sQAndroidEmulatorWindowAgent;
