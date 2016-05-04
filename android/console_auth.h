// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "android/utils/compiler.h"
#include <stdlib.h>

ANDROID_BEGIN_HEADER

// returns true if line matches "\s*auth\s*auth_token"
// auth_token is found in ~/.emulator_console_auth_token
bool android_console_auth_check_authorization_command(const char* line);

void android_console_auth_get_token_path(char* buf, size_t buf_len);

bool android_console_auth_get_token(char* buf, size_t buf_len);

#define CONSOLE_AUTH_STATUS_REQUIRED 0
#define CONSOLE_AUTH_STATUS_DISABLED 1
#define CONSOLE_AUTH_STATUS_ERROR 2

int android_console_auth_get_status();

ANDROID_END_HEADER
