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

#include <stdbool.h>

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/** Emulator user configuration (e.g. last window position)
 **/
typedef struct AndroidOptions AndroidOptions;
typedef struct AndroidHwConfig AndroidHwConfig;
typedef struct AConfig AConfig;

bool user_config_init(void);
void user_config_done(void);

void user_config_get_window_pos(int* window_x, int* window_y);
double user_config_get_window_scale();
void user_config_set_window_scale(double scale);
int user_config_get_resizable_config();
void user_config_set_resizable_config(int configid);

AConfig* get_skin_config(void);
const char* get_skin_path(void);
const char* android_skin_net_speed(void);
const char* android_skin_net_delay(void);

/* Find the skin corresponding to our options, and return an AConfig pointer
 * and the base path to load skin data from
 */
void parse_skin_files(const char* skinDirPath,
                      const char* skinName,
                      AndroidOptions* opts,
                      AndroidHwConfig* hwConfig,
                      AConfig** skinConfig,
                      char** skinPath);
ANDROID_END_HEADER
