/* Copyright (C) 2016 The Android Open Source Project
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
#include <stdint.h>

ANDROID_BEGIN_HEADER

// Get the access rules given by |name| if they exist, otherwise returns NULL
// caller should free the string
char* sim_get_access_rules(const char* name);

// Get the file control parameters assosicated with the file id if they exist,
// otherwise returns NULL
// caller should free the string
char* sim_get_fcp(uint16_t file_id);

ANDROID_END_HEADER

