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

static const int kWindowMessageTimeoutInfinite = -1;

typedef struct EmulatorWindow EmulatorWindow;

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
    void (*fold)(bool is_fold);
    // Query folded state
    bool (*isFolded)(void);

    // Set the UI display region
    void (*setUIDisplayRegion)(int, int, int, int);
    // Inform UI creation/modification/deletion of multi-display window
    void (*setMultiDisplay)(int, int, int, int, int, bool);
    bool (*getMultiDisplay)(int, int*, int*, int*, int*);

} QAndroidEmulatorWindowAgent;

// Defined in android/window-agent-impl.cpp
extern const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent;

ANDROID_END_HEADER
