/* Copyright (C) 2022 The Android Open Source Project
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

#include "android/utils/compiler.h"
#include "android/utils/aconfig-file.h"
#include "host-common/hw-config.h"
#include "android/cmdline-option.h"
#include <stdbool.h>

ANDROID_BEGIN_HEADER

/** Emulator user configuration (e.g. last window position)
 **/

bool user_config_init( void );
void user_config_done( void );

void user_config_get_window_pos( int *window_x, int *window_y );

AConfig* get_skin_config( void );
const char* get_skin_path( void );


/* Find the skin corresponding to our options, and return an AConfig pointer
 * and the base path to load skin data from
 */
void parse_skin_files(const char*      skinDirPath,
                      const char*      skinName,
                      AndroidOptions*  opts,
                      AndroidHwConfig* hwConfig,
                      AConfig*        *skinConfig,
                      char*           *skinPath);
ANDROID_END_HEADER
