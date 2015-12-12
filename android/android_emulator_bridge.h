// Copyright 2015 The Android Open Source Project
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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

#define AEB_COMMAND_WIDTH 1024
#define AEB_NUMCONST_WIDTH 17
#define AEB_BUFFER_SIZE (1024 * 1024 * 32)
#define AEB_PUSH_CHUNK_SIZE AEB_BUFFER_SIZE

void android_emulator_bridge_init(void);

void android_emulator_bridge_screencap(void);
void android_emulator_bridge_shutdown(void);
void android_emulator_bridge_push(const char* filename);
void android_emulator_bridge_pull(const char* filename);
void android_emulator_bridge_install(const char* filename);

void android_emulator_bridge_register_monitor(void* mon);
void android_emulator_bridge_register_print_callback(void (*monitor_printf_t)(void*, const char* fmt, ...)
);

ANDROID_END_HEADER
