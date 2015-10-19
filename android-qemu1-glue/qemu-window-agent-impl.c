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

#include "android/emulation/control/window_agent.h"
#include "android/skin/ui.h"

static const QAndroidEmulatorWindowAgent sQAndroidEmulatorWindowAgent = {
    .getEmulatorWindow = emulator_window_get,
    .getCurrentKeyset = skin_ui_get_current_keyset
};

const QAndroidEmulatorWindowAgent* const gQAndroidEmulatorWindowAgent =
    &sQAndroidEmulatorWindowAgent;

