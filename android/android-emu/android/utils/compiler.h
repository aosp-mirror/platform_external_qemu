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

#pragma once

#ifdef __cplusplus
#define ANDROID_BEGIN_HEADER extern "C" {
#define ANDROID_END_HEADER   }
#else
#define ANDROID_BEGIN_HEADER /* nothing */
#define ANDROID_END_HEADER  /* nothing */
#endif

#ifdef _MSC_VER
  #define __ANDROID_NO_RETURN__
  #define __inline__ inline
  #define __ANDROID_NO_INLINE__
  #define __ANDROID_UNUSED__
  #define __ANDROID_ASSUME_ALINGED__(x, y) (x)
  #define __ANDROID_PREFETCH__(x) 
#else
  #define __ANDROID__NO_RETURN__ __attribute__((noreturn))
  #define __ANDROID_NO_INLINE__ __attribute__((noinline))
  #define __ANDROID_UNUSED__ __attribute__((unused))
  #define __ANDROID_ASSUME_ALINGED__(x, y) __builtin_assume_aligned((x), (y))
  #define __ANDROID_PREFETCH__(x)  __builtin_prefetch((x))
#endif
// ANDROID_GCC_PREREQ(<major>,<minor>) will evaluate to true
// iff the current version of GCC is <major>.<minor> or higher.
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
# define ANDROID_GCC_PREREQ(maj, min) \
         ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define ANDROID_GCC_PREREQ(maj, min) 0
#endif
