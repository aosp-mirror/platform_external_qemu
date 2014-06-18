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

#ifndef ANDROID_CRASH_HANDLER_GOOGLE_BREAKPAD_H
#define ANDROID_CRASH_HANDLER_GOOGLE_BREAKPAD_H

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Setup Google Breakpad client to catch any crashes and write
// a minidump file for them in |minidumps_dir| directory.
//
// This function tries to create the directory if it does not exist.
// Returns 0 on success, -errno on failure.
//
int android_setupGoogleBreakpad(const char* minidumps_dir);

ANDROID_END_HEADER

#endif  // ANDROID_CRASH_HANDLER_GOOGLE_BREAKPAD_H
