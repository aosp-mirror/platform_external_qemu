// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifdef ANDROID_IO
#include "android/utils/file_io.h"
#else
#define android_fopen fopen
#define android_popen popen
#define android_stat stat
#define android_lstat lstat
#define android_access access
#define android_mkdir mkdir
#define android_creat creat
#define android_unlink android_unlink
#define android_chmod chmod
#endif
