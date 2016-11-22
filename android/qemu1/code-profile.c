/* Copyright (C) 2014-2015 The Android Open Source Project
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

#include "exec/code-profile.h"

// IMPORTANT: Initializers are required to ensure that these
// variables are properly linked into the final executables on
// Darwin. Otherwise, the build will succeed, but trying to run
// a program will result in a dyld failure.
//
CodeProfileRecordFunc code_profile_record_func = NULL;

const char *code_profile_dirname = NULL;
