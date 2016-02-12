// Copyright (C) 2016 The Android Open Source Project
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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// An error code similar to errno that is set if a failure during QEMU
// initilization occurs. If this is zero then no error has occurred.
extern int android_init_error_code;
// An error string that describes the error (if any) in android_init_error_code
// This may be NULL if there is no associated message or if no error occurred.
// Note that this will be free'd when the emulator exits so make sure you malloc
// it if you create an error message.
extern char* android_init_error_message;

// Error message to explain why haxm's vcpu sync failed
extern const char* const kHaxVcpuSyncFailed;

// Error message indicating that some unknown error occurred during startup
extern const char* const kUnknownInitError;

// Format string for an error message indicating not enough RAM for QEMU guest
extern const char* const kNotEnoughMemForGuestError;

ANDROID_END_HEADER
