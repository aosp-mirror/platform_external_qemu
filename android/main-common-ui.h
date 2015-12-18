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
#include "android/cmdline-option.h"
#include "android/skin/keyset.h"
#include "android/ui-emu-agent.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/** Emulator user configuration (e.g. last window position)
 **/

void user_config_init( void );
void user_config_done( void );

void user_config_get_window_pos( int *window_x, int *window_y );

extern SkinKeyset*  android_keyset;
void parse_keyset(const char*  keyset, AndroidOptions*  opts);
void write_default_keyset( void );

/* Find the skin corresponding to our options, and return an AConfig pointer
 * and the base path to load skin data from
 */
void parse_skin_files(const char*      skinDirPath,
                      const char*      skinName,
                      AndroidOptions*  opts,
                      AndroidHwConfig* hwConfig,
                      AConfig*        *skinConfig,
                      char*           *skinPath);

/* Returns the amount of pixels used by the default display. */
int64_t  get_screen_pixels(AConfig*  skinConfig);

void ui_init(AConfig*          skinConfig,
             const char*       skinPath,
             AndroidOptions*   opts,
             const UiEmuAgent* uiEmuAgent);

void ui_done(void);

ANDROID_END_HEADER
