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

#include <stdbool.h>                      // for bool

#include "host-common/hw-config.h"        // for AndroidHwConfig
#include "android/avd/info.h"             // for AvdInfo
#include "android/cmdline-definitions.h"  // for AndroidOptions
#include "android/ui-emu-agent.h"         // for UiEmuAgent
#include "android/utils/aconfig-file.h"   // for AConfig
#include "android/utils/compiler.h"       // for ANDROID_BEGIN_HEADER, ANDRO...

ANDROID_BEGIN_HEADER

// Handle input-related command-line options, for example, keyboard events
// guest forwarding. Return true on success, false otherwise.
bool emulator_parseInputCommandLineOptions(AndroidOptions* opts);

// Handle UI-related command-line options and AVD configuration.
// This function will update the content of |hw| based on the values
// found in |opts| and |avd|, so call this before writing hardware-qemu.img
// to disk. Return true on success, false otherwise.
bool emulator_parseUiCommandLineOptions(AndroidOptions* opts,
                                        AvdInfo* avd,
                                        AndroidHwConfig* hw);

// Initialize user interface. Return true on success, false on failure.
bool emulator_initUserInterface(const AndroidOptions* opts,
                                const UiEmuAgent* uiEmuAgent);

// Finalize the user interface. Call this on exit.
void emulator_finiUserInterface(void);

// TODO(digit): Remove the deprecated declarations below once QEMU2 has been
//              ported to use emulator_parseUiCommandLineOptions() and
//              emulator_initUserInterface()

/** Emulator user configuration (e.g. last window position)
 **/

bool user_config_init( void );
void user_config_done( void );

void user_config_get_window_pos( int *window_x, int *window_y );

/* Find the skin corresponding to our options, and return an AConfig pointer
 * and the base path to load skin data from
 */
void parse_skin_files(const char*      skinDirPath,
                      const char*      skinName,
                      AndroidOptions*  opts,
                      AndroidHwConfig* hwConfig,
                      AConfig*        *skinConfig,
                      char*           *skinPath);

bool ui_init(const AConfig*    skinConfig,
             const char*       skinPath,
             const AndroidOptions* opts,
             const UiEmuAgent* uiEmuAgent);

void ui_done(void);

ANDROID_END_HEADER
