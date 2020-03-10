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
#include "android/skin/qt/emulator-qt-window.h"
#include "android/utils/debug.h"

static_assert(WINDOW_MESSAGE_GENERIC == int(Ui::OverlayMessageType::None),
              "Bad message type enum value (None)");
static_assert(WINDOW_MESSAGE_INFO == int(Ui::OverlayMessageType::Info),
              "Bad message type enum value (Info)");
static_assert(WINDOW_MESSAGE_WARNING == int(Ui::OverlayMessageType::Warning),
              "Bad message type enum value (Warning)");
static_assert(WINDOW_MESSAGE_ERROR == int(Ui::OverlayMessageType::Error),
              "Bad message type enum value (Error)");
static_assert(WINDOW_MESSAGE_OK == int(Ui::OverlayMessageType::Ok),
              "Bad message type enum value (Ok)");

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow = emulator_window_get,
        .rotate90Clockwise =
                [] {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        if (win->isMultiDisplayEnabled() == false) {
                            return emulator_window_rotate_90(true);
                        }
                    }
                    return false;
                },
        .rotate = emulator_window_rotate,
        .getRotation =
                [] {
                    EmulatorWindow* window = emulator_window_get();
                    if (!window)
                        return SKIN_ROTATION_0;
                    SkinLayout* layout = emulator_window_get_layout(window);
                    if (!layout)
                        return SKIN_ROTATION_0;
                    return layout->orientation;
                },
        .showMessage =
                [](const char* message, WindowMessageType type, int timeoutMs) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->showMessage(
                                QString::fromUtf8(message),
                                static_cast<Ui::OverlayMessageType>(type),
                                timeoutMs);
                    } else {
                        const auto printer =
                                (type == WINDOW_MESSAGE_ERROR)
                                        ? &derror
                                        : (type == WINDOW_MESSAGE_WARNING)
                                                  ? &dwarning
                                                  : &dprint;
                        printer("%s", message);
                    }
                },
        .showMessageWithDismissCallback =
                [](const char* message,
                   WindowMessageType type,
                   const char* dismissText,
                   void* context,
                   void (*func)(void*),
                   int timeoutMs) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->showMessageWithDismissCallback(
                                QString::fromUtf8(message),
                                static_cast<Ui::OverlayMessageType>(type),
                                QString::fromUtf8(dismissText),
                                [func, context] { if (func) func(context); },
                                timeoutMs);
                    } else {
                        const auto printer =
                                (type == WINDOW_MESSAGE_ERROR)
                                        ? &derror
                                        : (type == WINDOW_MESSAGE_WARNING)
                                                  ? &dwarning
                                                  : &dprint;
                        printer("%s", message);
                        // Don't necessarily perform the func since the
                        // user doesn't get a chance to dismiss.
                    }
                },
        .fold =
                [](bool is_fold) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        if (win->isFoldableConfigured()) {
                            QtUICommand cmd = is_fold ? QtUICommand::FOLD
                                                      : QtUICommand::UNFOLD;
                            win->runOnUiThread([win, cmd]() {
                                win->toolWindow()->handleUICommand(cmd);
                            });
                            return true;
                        }
                    }
                    return false;
                },
        .isFolded =
                [] {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        return win->isFolded();
                    }
                    return false;
                },
        .setUIDisplayRegion =
                [](int x, int y, int w, int h) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->runOnUiThread([win, x, y, w, h]() {
                            win->resizeAndChangeAspectRatio(x, y, w, h);
                        });
                    }
                },
        .setUIMultiDisplay =
                [](uint32_t id,
                   int32_t x,
                   int32_t y,
                   uint32_t w,
                   uint32_t h,
                   bool add,
                   uint32_t dpi = 0) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->setUIMultiDisplay(id, x, y, w, h, add, dpi);
                    }
                },
        .getMultiDisplay =
                [](uint32_t id,
                   int32_t* x,
                   int32_t* y,
                   uint32_t* w,
                   uint32_t* h,
                   uint32_t* dpi,
                   uint32_t* flag,
                   bool* enabled) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        return win->getMultiDisplay(id, x, y, w, h, dpi, flag, enabled);
                    }
                    return false;
                },
        .getMonitorRect =
                [](uint32_t* w, uint32_t* h) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        return win->getMonitorRect(w, h);
                    } else {
                        return false;
                    }
                },
        .setNoSkin =
                []() {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        return win->setNoSkin();
                    }
                },
        .restoreSkin =
                []() {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        return win->restoreSkin();
                    }
                },
        .switchMultiDisplay =
                [](bool add,
                   uint32_t id,
                   int32_t x,
                   int32_t y,
                   uint32_t w,
                   uint32_t h,
                   uint32_t dpi,
                   uint32_t flag)->bool {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        return win->switchMultiDisplay(add, id, x, y, w, h, dpi, flag);
                    }
                    return false;
                },
        .updateUIMultiDisplayPage =
                [](uint32_t id) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->updateUIMultiDisplayPage(id);
                    }
                }
};

const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent =
        &sQAndroidEmulatorWindowAgent;
