/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/skin/keyboard.h"

#include "android/utils/debug.h"
#include "android/utils/system.h"

#include <windows.h>

#define DEBUG 1

#if DEBUG
#define D(...) VERBOSE_PRINT(keys, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

const char* skin_keyboard_current_active_layout() {
	char temp[64];
	memset(temp, 0, 64);
	if(GetKeyboardLayoutNameA(temp)) {
		return ASTRDUP(temp);
	} else {
		D("Failed to retrieve keyboard layout name.");
		return NULL;
	}
}
