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

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <QSemaphore>
#include <QString>
#include <array>
#include <initializer_list>
#include <mutex>
#include <string>
#include <utility>

#include "aemu/base/logging/CLog.h"
#include "aemu/base/threads/Thread.h"
#include "android/cmdline-definitions.h"
#include "android/console.h"
#include "android/emulation/control/globals_agent.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/resizable_display_config.h"
#include "android/emulator-window.h"
#include "android/hw-sensors.h"
#include "android/skin/qt/OverlayMessageCenter.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/qt-ui-commands.h"
#include "android/skin/qt/tool-window.h"
#include "android/skin/winsys.h"
#include "android/user-config.h"
#include "glm/detail/func_geometric.hpp"
#include "glm/detail/type_vec.hpp"
#include "glm/detail/type_vec3.hpp"
#include "host-common/MultiDisplay.h"
#include "host-common/misc.h"
#include "host-common/qt_ui_defs.h"
#include "host-common/window_agent.h"

/* set >0 for very verbose debugging */
// #define DEBUG 1
#ifndef DEBUG
#define DD(...) (void)0
#else
#define DD(fmt, ...) \
    dinfo("window-agent-impl: %s:%d| " fmt, __func__, __LINE__, ##__VA_ARGS__)
#endif

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

static bool gUserSettingIsDontSaveSnapshot = true;
static QSemaphore gWindowPosSemaphore;

static SkinRotation getRotation() {
    const QAndroidSensorsAgent* sensorsAgent = getConsoleAgents()->sensors;
    if (!sensorsAgent)
        return SKIN_ROTATION_0;
    sensorsAgent->advanceTime();
    glm::vec3 device_accelerometer;
    auto out = {&device_accelerometer.x, &device_accelerometer.y,
                &device_accelerometer.z};
    sensorsAgent->getSensor(ANDROID_SENSOR_ACCELERATION, out.begin(),
                            out.size());
    glm::vec3 normalized_accelerometer = glm::normalize(device_accelerometer);

    static const std::array<std::pair<glm::vec3, SkinRotation>, 4> directions{
            std::make_pair(glm::vec3(0.0f, 1.0f, 0.0f), SKIN_ROTATION_0),
            std::make_pair(glm::vec3(-1.0f, 0.0f, 0.0f), SKIN_ROTATION_90),
            std::make_pair(glm::vec3(0.0f, -1.0f, 0.0f), SKIN_ROTATION_180),
            std::make_pair(glm::vec3(1.0f, 0.0f, 0.0f), SKIN_ROTATION_270)};
    SkinRotation coarse_orientation = SKIN_ROTATION_0;
    for (const auto& v : directions) {
        if (fabs(glm::dot(normalized_accelerometer, v.first) - 1.f) < 0.1f) {
            coarse_orientation = v.second;
            break;
        }
    }
    return coarse_orientation;
}

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
        .getEmulatorWindow = emulator_window_get,
        .rotate90Clockwise =
                [] {
                    if (const auto instance =
                                android::MultiDisplay::getInstance()) {
                        if (instance &&
                            instance->isMultiDisplayEnabled() == false) {
                            return emulator_window_rotate_90(true);
                        }
                    }
                    return false;
                },
        .rotate = emulator_window_rotate,
        .getRotation = []() -> int { return getRotation(); },
        .showMessage =
                [](const char* message, WindowMessageType type, int timeoutMs) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->showMessage(
                                QString::fromUtf8(message),
                                static_cast<Ui::OverlayMessageType>(type),
                                timeoutMs);
                    } else {
                        switch (type) {
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
                                [func, context] {
                                    if (func)
                                        func(context);
                                },
                                timeoutMs);
                    } else {
                        // Don't necessarily perform the func since the
                        // user doesn't get a chance to dismiss.
                        switch (type) {
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
        .fold = [](bool is_fold) -> bool {
            if (is_fold) {
                return android_foldable_fold();
            } else {
                return android_foldable_unfold();
            }
        },
        .isFolded = []() -> bool { return android_foldable_is_folded(); },
        .getFoldedArea = [](int* x, int* y, int* w, int* h) -> bool {
            return android_foldable_get_folded_area(x, y, w, h);
        },
        .updateFoldablePostureIndicator =
                [](bool confirmFoldedArea) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        QtUICommand cmd =
                                QtUICommand::UPDATE_FOLDABLE_POSTURE_INDICATOR;
                        win->runOnUiThread([win, cmd, confirmFoldedArea]() {
                            if (confirmFoldedArea) {
                                win->toolWindow()->handleUICommand(
                                        cmd, std::string("confirmFoldedArea"));
                            } else {
                                win->toolWindow()->handleUICommand(cmd);
                            }
                        });
                    }
                },
        .setPosture = [](int posture) -> bool {
            return android_foldable_set_posture(posture);
        },
        .setUIDisplayRegion =
                [](int x, int y, int w, int h, bool ignoreOrientation) {
                    // The UI display region needs to be updated when we add
                    // additional displays, or when we resize the screen. In the
                    // embedded case we do not have a visible UI, so we do not
                    // need to update display regions.
                    if (getConsoleAgents()
                                ->settings->android_cmdLineOptions()
                                ->qt_hide_window) {
                        return;
                    }
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->setUIDisplayRegion(x, y, w, h, ignoreOrientation);
                    };
                },
        .getMultiDisplay = [](uint32_t id,
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
        .addMultiDisplayWindow =
                [](uint32_t id, bool add, uint32_t w, uint32_t h) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->runOnUiThread([win, id, add, w, h]() {
                            win->addMultiDisplayWindow(id, add, w, h);
                            return;
                        });
                        return true;
                    }
                    return false;
                },
        .paintMultiDisplayWindow =
                [](uint32_t id, uint32_t texture) {
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->runOnUiThread([win, id, texture]() {
                            win->paintMultiDisplayWindow(id, texture);
                        });
                        return true;
                    }
                    return false;
                },
        .getMonitorRect = [](uint32_t* w, uint32_t* h) -> bool {
            SkinRect rect{0};
            skin_winsys_get_monitor_rect(&rect);
            if (w) {
                *w = rect.size.w;
            }
            if (h) {
                *h = rect.size.h;
            }
            return true;
        },
        .moveExtendedWindow =
                [](uint32_t x, uint32_t y, int horizontal, int vertical) {
                    int unused;
                    auto userConfig =
                            getConsoleAgents()->settings->userConfig();
                    if (auserConfig_getExtendedControlsPos(userConfig, &unused,
                                                           &unused, &unused,
                                                           &unused) == 0 ||
                        DEBUG_GRPC_ENDPOINT) {
                        auserConfig_setExtendedControlsPos(
                                userConfig, x, y, horizontal, vertical);
                    }
                },
        .startExtendedWindow =
                [](int pane) {
                    std::lock_guard<std::mutex> extended(s_extendedWindow);
                    auto* win = EmulatorQtWindow::getInstance();
                    bool visibilityChanged = false;
                    if (win) {
                        QSemaphore completed;
                        visibilityChanged =
                                !win->toolWindow()->isExtendedWindowVisible();
                        win->runOnUiThread(
                                [win, pane]() {
                                    win->toolWindow()->showExtendedWindow(
                                            (ExtendedWindowPane)pane);
                                },
                                &completed);
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
                        win->runOnUiThread(
                                [win]() {
                                    win->toolWindow()->hideExtendedWindow();
                                },
                                &completed);
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
        .setUiTheme =
                [](int type) {
                    skin_winsys_touch_qt_extended_virtual_sensors();
                    if (auto* win = EmulatorQtWindow::getInstance()) {
                        win->runOnUiThread([win, type]() {
                            win->toolWindow()->setUiTheme((SettingsTheme)type,
                                                          false);
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
                []() { return android::base::isRunningInUiThread(); },
        .changeResizableDisplay =
                [](int presetSize) {
                    if (!resizableEnabled()) {
                        return false;
                    }
                    if (const auto win = EmulatorQtWindow::getInstance()) {
                        win->runOnUiThread([win, presetSize]() {
                            win->toolWindow()->presetSizeAdvance(
                                    (PresetEmulatorSizeType)presetSize);
                        });
                        return true;
                    }
                    return false;
                },
        .getLayout =
                []() {
                    SkinLayout* layout = nullptr;
                    auto win = emulator_window_get();
                    if (win) {
                        layout = emulator_window_get_layout(win);
                    }
                    return (void*)layout;
                },
        .getWindowPosition = [](int* x,
                                int* y) { skin_winsys_get_window_pos(x, y); },
        .hasWindow = [] { return true; },
        .userSettingIsDontSaveSnapshot =
                []() { return gUserSettingIsDontSaveSnapshot; },
        .setUserSettingIsDontSaveSnapshot =
                [](bool val) { gUserSettingIsDontSaveSnapshot = val; },
};

const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent =
        &sQAndroidEmulatorWindowAgent;

const QAndroidEmulatorWindowAgent* const getEmulatorWindowAgent() {
    return gQAndroidEmulatorWindowAgent;
}
