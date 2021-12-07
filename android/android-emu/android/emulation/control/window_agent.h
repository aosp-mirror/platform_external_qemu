/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#pragma once

#include <stdint.h>
#include "android/settings-agent.h"
#include "android/skin/qt/extended-window-styles.h"
#include "android/skin/rect.h"
#include "android/utils/compiler.h"
ANDROID_BEGIN_HEADER

// Window agent's possible message types
typedef enum {
    WINDOW_MESSAGE_GENERIC,
    WINDOW_MESSAGE_INFO,
    WINDOW_MESSAGE_WARNING,
    WINDOW_MESSAGE_ERROR,
    WINDOW_MESSAGE_OK,
} WindowMessageType;


typedef enum {
    LEFT,
    HCENTER,
    RIGHT,
} HorizontalAnchor;

typedef enum {
    TOP,
    VCENTER,
    BOTTOM,
} VerticalAnchor;

typedef struct {} MultiDisplayPageChangeEvent;

static const int kWindowMessageTimeoutInfinite = -1;

typedef struct EmulatorWindow EmulatorWindow;

typedef void (*UiUpdateFunc)(void* data);

typedef struct QAndroidEmulatorWindowAgent {
    // Get a pointer to the emulator window structure.
    EmulatorWindow* (*getEmulatorWindow)();

    // Rotate the screen clockwise by 90 degrees.
    // Returns true on success, false otherwise.
    bool (*rotate90Clockwise)(void);

    // Rotate to specific |rotation|
    bool (*rotate)(SkinRotation rotation);

    // Returns the current rotation.
    SkinRotation (*getRotation)(void);

    // Shows a message to the user.
    void (*showMessage)(const char* message,
                        WindowMessageType type,
                        int timeoutMs);

    // Shows a message to the user + custom dismiss op.
    void (*showMessageWithDismissCallback)(const char* message,
                                           WindowMessageType type,
                                           const char* dismissText,
                                           void* context,
                                           void (*func)(void*),
                                           int timeoutMs);
    // Fold/Unfold device
    bool (*fold)(bool is_fold);
    // Query folded state
    bool (*isFolded)(void);
    bool (*getFoldedArea)(int* x, int* y, int* w, int* h);

    // Update UI indicator which shows which foldable posture device is in
    void (*updateFoldablePostureIndicator)(bool confirmFoldedArea);
    // Set foldable device posture
    bool (*setPosture)(int posture);

    // Set the UI display region
    void (*setUIDisplayRegion)(int, int, int, int, bool);
    bool (*getMultiDisplay)(uint32_t,
                            int32_t*,
                            int32_t*,
                            uint32_t*,
                            uint32_t*,
                            uint32_t*,
                            uint32_t*,
                            bool*);
    void (*setNoSkin)(void);
    void (*restoreSkin)(void);
    void (*updateUIMultiDisplayPage)(uint32_t);
    bool (*addMultiDisplayWindow)(uint32_t, bool, uint32_t, uint32_t);
    bool (*paintMultiDisplayWindow)(uint32_t, uint32_t);
    bool (*getMonitorRect)(uint32_t*, uint32_t*);
    // moves the extended window to the given position if the window was never displayed. This does nothing
    // if the window has been show once during the lifetime of the avd.
    void (*moveExtendedWindow)(uint32_t x, uint32_t y, HorizontalAnchor horizonal, VerticalAnchor vertical);
    // start extended window and switch to the pane specified by the index.
    // return true if extended controls window's visibility has changed.
    // The window is not necessarily visible when this method returns.
    bool (*startExtendedWindow)(ExtendedWindowPane index);

    // Closes the extended window. At some point in time it will be gone.
    bool (*quitExtendedWindow)(void);

    // This will wait until the state of the visibility of the window has
    // changed to the given value. Calling show or close does not make
    // a qt frame immediately visible. Instead a series of events will be
    // fired when the frame is actually added to, or removed from the display.
    // so usually the pattern is showWindow, wait for visibility..
    //
    // Be careful to:
    //   - Not run this on a looper thread. The ui controls can post actions
    //     to the looper thread which can result in a deadlock
    //   - Not to run this on the Qt Message pump. You will deadlock.
    void (*waitForExtendedWindowVisibility)(bool);
    bool (*setUiTheme)(SettingsTheme type);
    void (*runOnUiThread)(UiUpdateFunc f, void* data, bool wait);
    bool (*isRunningInUiThread)(void);
    bool (*changeResizableDisplay)(int presetSize);
} QAndroidEmulatorWindowAgent;

ANDROID_END_HEADER
