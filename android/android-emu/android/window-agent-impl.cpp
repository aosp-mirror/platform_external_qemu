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

#include "android/base/threads/Thread.h"
#include "android/emulation/MultiDisplay.h"
#include "android/emulator-window.h"
#include "android/globals.h"
#include "android/hw-sensors.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/winsys.h"
#include "android/utils/debug.h"

// Set this to true if you wish to enable debugging of the window position.
constexpr bool DEBUG_GRPC_ENDPOINT = false;
std::mutex s_extendedWindow;

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
                    if (const auto instance = android::MultiDisplay::getInstance()) {
                        if (instance && instance->isMultiDisplayEnabled() == false) {
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
                        switch(type) {
                            case WINDOW_MESSAGE_ERROR:
                                derror("%s", message);
                                break;
                            case WINDOW_MESSAGE_WARNING:
                                dwarning("%s", message);
                                break;
                            default:
                                dinfo("%s", message);
                        }
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
                        // Don't necessarily perform the func since the
                        // user doesn't get a chance to dismiss.
                        switch(type) {
                            case WINDOW_MESSAGE_ERROR:
                                derror("%s", message);
                                break;
                            case WINDOW_MESSAGE_WARNING:
                                dwarning("%s", message);
                                break;
                            default:
                                dinfo("%s", message);
                        }
                    }
                },
        .fold =
                [](bool is_fold) -> bool {
                    if (is_fold) {
                        return android_foldable_fold();
                    } else {
                        return android_foldable_unfold();
                    }
                },
        .isFolded =
                []() -> bool {
                    return android_foldable_is_folded();
                },
        .getFoldedArea =
                [](int* x, int* y, int* w, int* h) -> bool {
                    return android_foldable_get_folded_area(x, y, w, h);
                },
        .updateFoldablePostureIndicator =
                [](bool confirmFoldedArea) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        QtUICommand cmd = QtUICommand::UPDATE_FOLDABLE_POSTURE_INDICATOR;
                        win->runOnUiThread([win, cmd, confirmFoldedArea]() {
                            if (confirmFoldedArea) {
                                win->toolWindow()->handleUICommand(cmd, std::string("confirmFoldedArea"));
                            } else {
                                win->toolWindow()->handleUICommand(cmd);
                            }
                        });
                    }
                },
        .setPosture =
                [](int posture) -> bool {
                    return android_foldable_set_posture(posture);
                },
        .setUIDisplayRegion =
                [](int x, int y, int w, int h, bool ignoreOrientation) {
                    // The UI display region needs to be updated when we add additional displays, or when we
                    // resize the screen. In the embedded case we do not have a visible UI, so we do not need
                    // to update display regions.
                    if (android_cmdLineOptions->qt_hide_window) {
                        return;
                    }
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->setUIDisplayRegion(x, y, w, h, ignoreOrientation);
                    };
                },
        .getMultiDisplay =
                [](uint32_t id,
                   int32_t* x,
                   int32_t* y,
                   uint32_t* w,
                   uint32_t* h,
                   uint32_t* dpi,
                   uint32_t* flag,
                   bool* enabled) -> bool {
                    if (const auto ins = android::MultiDisplay::getInstance()) {
                        return ins->getMultiDisplay(id, x, y, w, h, dpi, flag, enabled);
                    }
                    return false;
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
        .updateUIMultiDisplayPage =
                [](uint32_t id) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->updateUIMultiDisplayPage(id);
                    }
                },
        .getMonitorRect =
                [](uint32_t* w, uint32_t* h) -> bool {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        return win->getMonitorRect(w, h);
                    } else {
                        return false;
                    }
                },
        .moveExtendedWindow =
                [](uint32_t x,
                   uint32_t y,
                   HorizontalAnchor horizontal,
                   VerticalAnchor vertical) {
                        int unused;
                        auto userConfig = aemu_get_userConfigPtr();
                        if (auserConfig_getExtendedControlsPos(
                                        userConfig, &unused, &unused, &unused,
                                        &unused) == 0 ||
                            DEBUG_GRPC_ENDPOINT) {
                                auserConfig_setExtendedControlsPos(
                                        userConfig, x, y, (int)horizontal,
                                        (int)vertical);
                    }
                },
        .startExtendedWindow =
                [](ExtendedWindowPane pane) {
                    std::lock_guard<std::mutex> extended(s_extendedWindow);
                    auto* win = EmulatorQtWindow::getInstance();
                    bool visibilityChanged = false;
                    if (win) {
                        QSemaphore completed;
                        visibilityChanged =
                                !win->toolWindow()->isExtendedWindowVisible();
                        win->runOnUiThread([win, pane]() {
                            win->toolWindow()->showExtendedWindow(pane);
                        }, &completed);
                        completed.acquire();
                    }
                    return visibilityChanged;
                },
        .quitExtendedWindow =
                []() {
                    std::lock_guard<std::mutex> extended(s_extendedWindow);
                    auto* win = EmulatorQtWindow::getInstance();
                    bool visibilityChanged = false;
                    if (win) {
                        QSemaphore completed;
                        visibilityChanged =
                                win->toolWindow()->isExtendedWindowVisible();
                        win->runOnUiThread([win]() {
                            win->toolWindow()->hideExtendedWindow();
                        }, &completed);
                        completed.acquire();
                    }
                    return visibilityChanged;
                },
        .waitForExtendedWindowVisibility =
                [](bool visible) {
                    auto* win = EmulatorQtWindow::getInstance();
                    if (win) {
                        win->toolWindow()->waitForExtendedWindowVisibility(
                                visible);
                        assert(visible ==
                               win->toolWindow()->isExtendedWindowVisible());
                    }
                },
        .setUiTheme = [](SettingsTheme type) {
                    skin_winsys_touch_qt_extended_virtual_sensors();
                    if (auto* win = EmulatorQtWindow::getInstance()) {
                        win->runOnUiThread([win, type]() {
                            win->toolWindow()->setUiTheme(type, false);
                        });
                        return true;
                    } else {
                        return false;
                    }
                },
        .runOnUiThread =
                [](UiUpdateFunc f, void* data, bool wait) {
                    skin_winsys_run_ui_update(f, data, wait);
                },
        .isRunningInUiThread =
                []() {
                    return android::base::isRunningInUiThread();
                },
        .changeResizableDisplay =
                [](int presetSize) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->runOnUiThread([win, presetSize]() {
                            win->toolWindow()->presetSizeAdvance((PresetEmulatorSizeType)presetSize);
                        });
                    }
                },
};

extern "C" const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent =
        &sQAndroidEmulatorWindowAgent;
