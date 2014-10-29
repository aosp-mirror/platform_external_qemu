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

#ifndef ANDROID_UTILS_UTF8_UTILS_H
#define ANDROID_UTILS_UTF8_UTILS_H

// Simple C wrapper for android/base/misc/Utf8Utils.h

#include "android/utils/compiler.h"

#include <stdbool.h>
#include <stddef.h>

ANDROID_BEGIN_HEADER

bool android_utf8_is_valid(const char* text, size_t text_len);

ANDROID_END_HEADER

#endif  // ANDROID_UTILS_UTF8_UTILS_H
