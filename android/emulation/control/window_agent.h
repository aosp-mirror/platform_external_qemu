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

#include "android/emulator-window.h"
#include "android/skin/ui.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

typedef struct QAndroidEmulatorWindowAgent {
    // Get a pointer to the emulator window structure.
    EmulatorWindow* (*getEmulatorWindow)();

    // Get the current keyset from the given UI.
    SkinKeyset* (*getCurrentKeyset)(SkinUI*);
} QAndroidEmulatorWindowAgent;

ANDROID_END_HEADER
