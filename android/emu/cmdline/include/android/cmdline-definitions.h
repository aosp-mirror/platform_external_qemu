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

#include <stdbool.h>

ANDROID_BEGIN_HEADER

/* a structure used to model a linked list of parameters
 */
typedef struct ParamList {
    char* param;
    struct ParamList* next;
} ParamList;

/* define a structure that will hold all option variables
 */
typedef struct AndroidOptions {
#define OPT_LIST(n, t, d) ParamList* n;
#define OPT_PARAM(n, t, d) char* n;
#define OPT_FLAG(n, d) int n;
#include "android/cmdline-options.h"
} AndroidOptions;

ANDROID_END_HEADER