/* Copyright 2015 The Android Open Source Project
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
#ifndef ANDROID_TELEMETRY_STUDIO_HELPER_H
#define ANDROID_TELEMETRY_STUDIO_HELPER_H

#include "android/utils/compiler.h"
#include <stdint.h>

ANDROID_BEGIN_HEADER

int get_android_studio_optins(char *);
int get_android_studio_installation_id(char **);

ANDROID_END_HEADER

#endif  // ANDROID_TELEMETRY_STUDIO_HELPER_H
