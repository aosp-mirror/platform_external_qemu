/* Copyright (C) 2018 The Android Open Source Project
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

ANDROID_BEGIN_HEADER
typedef void (*meminfo_callback_t)(const char* msg, int msgLen);

/* initialize meminfo qemud service */
extern void android_qemu_meminfo_init(void);

/* query meminfo qemud client in guest and callback will be expected to be
 * invoked when result is sent back*/
extern bool android_query_meminfo(meminfo_callback_t callback);
ANDROID_END_HEADER
