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
#include <stdlib.h>

ANDROID_BEGIN_HEADER

// returns true if line matches "\s*auth\s+auth_token"
// auth_token is found in ~/.emulator_console_auth_token
bool android_console_auth_check_authorization_command(const char* line);

char* android_console_auth_token_path_dup();

bool android_console_auth_get_token(char* buf, size_t buf_len);

#define CONSOLE_AUTH_STATUS_REQUIRED \
    0  // ~/.emulator_console_auth_token contains an auth token
#define CONSOLE_AUTH_STATUS_DISABLED \
    1  // ~/.emulator_console_auth_token is empty
#define CONSOLE_AUTH_STATUS_ERROR \
    2  // ~/.emulator_console_auth_token is inaccessible

int android_console_auth_get_status();

ANDROID_END_HEADER
