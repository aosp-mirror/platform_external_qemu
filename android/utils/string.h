// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_UTILS_STRING_H
#define ANDROID_UTILS_STRING_H

#include "android/utils/compiler.h"

#include <stddef.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <string.h>
#endif

ANDROID_BEGIN_HEADER

#if !defined(__APPLE__) && !defined(__FreeBSD__)
size_t strlcpy(char* dst, const char * src, size_t size);
#endif

ANDROID_END_HEADER

#endif  // ANDROID_UTILS_STRING_H
