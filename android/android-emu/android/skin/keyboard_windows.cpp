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

#include "android/base/ArraySize.h"
#include "android/utils/debug.h"
#include "android/utils/system.h"

#include <windows.h>
#include <string>

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

// window keyboard layout is represented as ID defined in
// https://docs.microsoft.com/en-us/windows-hardware/manufacture/desktop/windows-language-pack-default-values
// Thus, we use direct mapping on the basis of best effort. Note that the text
// displayed on screen depends on the android key character mapping.
static const struct {
    const char* window_kb_layout_id;
    const char* android_kb_layout;
} s_windows_keyboard_layout_to_android[] = {
        {"0x00000809", "keyboard_layout_english_uk"},
        {"0x00000409", "keyboard_layout_english_us"},
        {"0x00020409", "keyboard_layout_english_us_intl"},
        //{ NULL, "keyboard_layout_english_us_colemak"},
        {"0x00010409", "keyboard_layout_english_us_dvorak"},
        //{ NULL , "keyboard_layout_english_us_workman"},
        {"0x00000407", "keyboard_layout_german"},
        {"0x0000040c", "keyboard_layout_french"},
        {"0x00001009", "keyboard_layout_french_ca"},
        {"0x00000419", "keyboard_layout_russian"},
        // { NULL , "keyboard_layout_russian_mac"},
        {"0x0000040a", "keyboard_layout_spanish"},
        {"0x0001040a", "keyboard_layout_spanish_latin"},
        {"0x0000100c", "keyboard_layout_swiss_french"},
        {"0x00000807", "keyboard_layout_swiss_german"},
        {"0x0001080c ", "keyboard_layout_belgian"},  // Belgian (Comma)
        {"0x00000813", "keyboard_layout_belgian"},   // Belgian (Period)
        {"0x0030402", "keyboard_layout_bulgarian"},
        {"0x00000410", "keyboard_layout_italian"},
        {"0x00000406", "keyboard_layout_danish"},
        {"0x00000414", "keyboard_layout_norwegian"},
        {"0x0000041d", "keyboard_layout_swedish"},
        {"0x0000040b", "keyboard_layout_finnish"},
        {"0x0000041a", "keyboard_layout_croatian"},
        {"0x00000405", "keyboard_layout_czech"},
        {"0x00000425", "keyboard_layout_estonian"},
        {"0x0000040e", "keyboard_layout_hungarian"},
        {"0x0000040f", "keyboard_layout_icelandic"},
        {"0x00000416",
         "keyboard_layout_brazilian"},  // Portuguese (Brazilian ABNT)
        {"0x00010416",
         "keyboard_layout_brazilian"},  // Portuguese (Brazilian ABNT2)
        {"0x00000816", "keyboard_layout_portuguese"},
        {"0x0000041b", "keyboard_layout_slovak"},
        {"0x00000424", "keyboard_layout_slovenian"},
        {"0x0001041f", "keyboard_layout_turkish"},  // Turkish F
        {"0x0000041f", "keyboard_layout_turkish"},  // Turkish Q
        {"0x00000422", "keyboard_layout_ukrainian"},
        {"0x00000401", "keyboard_layout_arabic"},  // Arabic (101)
        {"0x00010401", "keyboard_layout_arabic"},  // Arabic (102)
        {"0x00000408", "keyboard_layout_greek"},
        {"0x0000040d", "keyboard_layout_hebrew"},
        {"0x00010427", "keyboard_layout_lithuanian"},
        {"0x00020426", "keyboard_layout_latvian"},
        {"0x00000429", "keyboard_layout_persian"},
        {"0x0001042c", "keyboard_layout_azerbaijani"},
        {"0x00010415", "keyboard_layout_polish"},  // Polish (214)
};

extern const char* skin_translate_host_kb_layout_to_android(const char* name) {
    int keyboard_id = std::stoi(name, nullptr, 16);
    for (size_t i = 0; i < ARRAY_SIZE(s_windows_keyboard_layout_to_android);
         i++) {
        if(std::stoi(s_windows_keyboard_layout_to_android[i].window_kb_layout_id, nullptr, 16) ==
            keyboard_id) {
            return s_windows_keyboard_layout_to_android[i].android_kb_layout;
        }
    }
    D("No keyboard layout mapping is found for %s", name);
    return NULL;
}