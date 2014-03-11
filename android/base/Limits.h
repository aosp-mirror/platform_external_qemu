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

#ifndef ANDROID_BASE_LIMITS_H
#define ANDROID_BASE_LIMITS_H

// In C++, <stdint.h> will only define macros like SIZE_MAX if you have
// defined __STDC_LIMIT_MACROS before including <stdint.h>. This header
// is used to do just that and verify that the macros are properly
// defined.
//
// NOTE: We have to define __STDC_FORMAT_MACROS in case the user wants
//       to use the corresponding macros as well.

#define __STDC_LIMIT_MACROS  1
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

#ifndef SIZE_MAX
#warning "<inttypes.h> has been included before this header."
#warning "This prevents the definition of useful macros."
#error "Please include <android/base/Limits.h> first!"
#endif

#endif  // ANDROID_BASE_LIMITS_H
