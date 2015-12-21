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

#include "android/avd/hw-config.h"
#include "android/avd/info.h"
#include "android/cmdline-option.h"
#include "android/ui-emu-agent.h"
#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// Handle UI-related command-line options and AVD configuration.
// This function will update the content of |hw| based on the values
// found in |opts| and |avd|, so call this before writing hardware-qemu.img
// to disk. Return true on success, false otherwise.
bool handleEmulatorUiConfiguration(AndroidOptions* opts,
                                   AvdInfo* avd,
                                   AndroidHwConfig* hw);

// Initialize user interface. Return true on success, false on failure.
bool initEmulatorUi(const AndroidOptions* opts, const UiEmuAgent* uiEmuAgent);

// Finalize user interface. Call this on exit.
void finiEmulatorUi(void);

ANDROID_END_HEADER
