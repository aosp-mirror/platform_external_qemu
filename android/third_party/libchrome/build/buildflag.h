// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a modified header specifically for the android emulator.
// It defines all the compiler flags as needed.

#ifndef BUILD_BUILDFLAG_H_
#define BUILD_BUILDFLAG_H_

// These macros un-mangle the names of the build flags in a way that looks
// natural, and gives errors if the flag is not defined. Normally in the
// preprocessor it's easy to make mistakes that interpret "you haven't done
// the setup to know what the flag is" as "flag is off". Normally you would
// include the generated header rather than include this file directly.
//
// This is for use with generated headers. See build/buildflag_header.gni.

// This dance of two macros does a concatenation of two preprocessor args using
// ## doubly indirectly because using ## directly prevents macros in that
// parameter from being expanded.
#define BUILDFLAG_CAT_INDIRECT(a, b) a##b
#define BUILDFLAG_CAT(a, b) BUILDFLAG_CAT_INDIRECT(a, b)

// Accessor for build flags.
//
// To test for a value, if the build file specifies:
//
//   ENABLE_FOOtrue
//
// Then you would check at build-time in source code with:
//
//   #include "foo_flags.h"  // The header the build file specified.
//
//   #if BUILDFLAG(ENABLE_FOO)
//     ...
//   #endif
//
// There will no #define called ENABLE_FOO so if you accidentally test for
// whether that is defined, it will always be negative. You can also use
// the value in expressions:
//
//   const char kSpamServerName[]  BUILDFLAG(SPAM_SERVER_NAME);
//
// Because the flag is accessed as a preprocessor macro with (), an error
// will be thrown if the proper header defining the internal flag value has
// not been included.
#define BUILDFLAG(flag) (BUILDFLAG_CAT(BUILDFLAG_INTERNAL_, flag)())

// Specific build flags for the android emulator
#define BUILDFLAG_INTERNAL_EXPENSIVE_DCHECKS_ARE_ON() (0)
#define BUILDFLAG_INTERNAL_IS_CHROMEOS_ASH() (0)
#define BUILDFLAG_INTERNAL_IS_CHROMEOS_LACROS() (0)
#define BUILDFLAG_INTERNAL_ENABLE_LLDBINIT_WARNING() (0)
#define BUILDFLAG_INTERNAL_ENABLE_BASE_TRACING() (0)
#define BUILDFLAG_INTERNAL_FROM_HERE_USES_LOCATION_BUILTINS() (1)
#define BUILDFLAG_INTERNAL_USE_BACKUP_REF_PTR() (0)
#define BUILDFLAG_INTERNAL_USE_PERFETTO_CLIENT_LIBRARY() (0)
#define BUILDFLAG_INTERNAL_REF_COUNT_AT_END_OF_ALLOCATION() (0)
#define BUILDFLAG_INTERNAL_ENABLE_RUNTIME_BACKUP_REF_PTR_CONTROL() (0)
#define BUILDFLAG_INTERNAL_ENABLE_LOG_ERROR_NOT_REACHED() (0)
#define BUILDFLAG_INTERNAL_USE_PARTITION_ALLOC() (0)
#define BUILDFLAG_INTERNAL_CAN_UNWIND_WITH_CFI_TABLE() (0)
#define BUILDFLAG_INTERNAL_USE_PARTITION_ALLOC_AS_MALLOC() (1)
#define BUILDFLAG_INTERNAL_ENABLE_ARM_CFI_TABLE() (0)
#define BUILDFLAG_INTERNAL_CLANG_PROFILING() (0)
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (0)
#define BUILDFLAG_INTERNAL_IS_CHROMECAST() (0)
#define BUILDFLAG_INTERNAL_IS_HWASAN() (0)
#define BUILDFLAG_INTERNAL_ENABLE_GDBINIT_WARNING() (0)

#define BUILDFLAG_INTERNAL_USE_TCMALLOC() (0)

#ifdef __linux__
#endif

#if defined(OS_WIN)
#define BUILDFLAG_INTERNAL_ENABLE_ATOMIC_ALIGNMENT_FIX() (1)
#define BUILDFLAG_INTERNAL_SINGLE_MODULE_MODE_HANDLE_VERIFIER() (0)
#define BUILDFLAG_INTERNAL_USE_TCMALLOC() (0)
#endif

#endif  // BUILD_BUILDFLAG_H_
