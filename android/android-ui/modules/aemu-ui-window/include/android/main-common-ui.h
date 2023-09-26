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

#include "android/avd/info.h"             // for AvdInfo
#include "android/cmdline-definitions.h"  // for AndroidOptions
#include "android/skin/backend-defs.h"
#include "android/ui-emu-agent.h"         // for UiEmuAgent
#include "android/utils/aconfig-file.h"   // for AConfig
#include "android/utils/compiler.h"
#include "host-common/hw-config.h"        // for AndroidHwConfig
#include "host-common/opengl/emugl_config.h"  // for AndroidGlesEmulationMode

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

// First, there is a struct to hold outputs.
typedef struct {
    AndroidGlesEmulationMode glesMode;
    int openglAlive;
    int bootPropOpenglesVersion;
    int glFramebufferSizeBytes;
    SelectedRenderer selectedRenderer;
} RendererConfig;
// Function itself:
bool configAndStartRenderer(
        enum WinsysPreferredGlesBackend uiPreferredBackend,
        RendererConfig* config_out);


// stopRenderer() - stop all the render threads and wait until their exit.
// NOTE: It is only safe to stop the OpenGL ES renderer after the main loop
//   has exited. This is not necessarily before |skin_window_free| is called,
//   especially on Windows!
void stopRenderer(void);


// After configAndStartRenderer is called, one can query last output values:
RendererConfig getLastRendererConfig(void);

// TODO(digit): Remove the deprecated declarations below once QEMU2 has been
//              ported to use emulator_parseUiCommandLineOptions() and
//              emulator_initUserInterface()

/** Emulator user configuration (e.g. last window position)
 **/

bool user_config_init( void );
void user_config_done( void );

void user_config_get_window_pos( int *window_x, int *window_y );

bool ui_init(const AConfig*    skinConfig,
             const char*       skinPath,
             const AndroidOptions* opts,
             const UiEmuAgent* uiEmuAgent);

void ui_done(void);

ANDROID_END_HEADER
