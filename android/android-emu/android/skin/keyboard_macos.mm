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

#include "android/skin/qt/emulator-qt-window.h"
#include "android/utils/system.h"

#include <Cocoa/Cocoa.h>

const char* skin_keyboard_current_active_layout() {
    EmulatorQtWindow* emuWin = EmulatorQtWindow::getInstance();
    if (!emuWin)    return NULL;

    NSWindow* pWin = (NSWindow*)emuWin->getWindowId();
    if (!pWin)    return NULL;
    NSTextInputContext* context = [[pWin contentView] inputContext];
    NSTextInputSourceIdentifier curLayout =
            [context selectedKeyboardInputSource];
    return ASTRDUP([curLayout UTF8String]);

}

static const char* s_android_kb_layout_file [] = {
    "keyboard_layout_english_uk", 
    "keyboard_layout_english_us",
    "keyboard_layout_english_us_intl",
    "keyboard_layout_english_us_colemak",
    "keyboard_layout_english_us_dvorak",
    "keyboard_layout_english_us_workman",
    "keyboard_layout_german",
    "keyboard_layout_french",
    "keyboard_layout_french_ca",
    "keyboard_layout_russian",
    "keyboard_layout_russian_mac",
    "keyboard_layout_spanish",
    "keyboard_layout_swiss_french",
    "keyboard_layout_swiss_german",
    "keyboard_layout_belgian",
    "keyboard_layout_bulgarian",
    "keyboard_layout_italian",
    "keyboard_layout_danish",
    "keyboard_layout_norwegian",
    "keyboard_layout_swedish",
    "keyboard_layout_finnish",
    "keyboard_layout_croatian",
    "keyboard_layout_czech",
    "keyboard_layout_estonian",
    "keyboard_layout_hungarian",
    "keyboard_layout_icelandic",
    "keyboard_layout_brazilian",
    "keyboard_layout_portuguese",
    "keyboard_layout_slovak",
    "keyboard_layout_slovenian",
    "keyboard_layout_turkish",
    "keyboard_layout_ukrainian",
    "keyboard_layout_arabic",
    "keyboard_layout_greek",
    "keyboard_layout_hebrew",
    "keyboard_layout_lithuanian",
    "keyboard_layout_spanish_latin",
    "keyboard_layout_latvian",
    "keyboard_layout_persian",
    "keyboard_layout_azerbaijani",
    "keyboard_layout_polish",
};

const char* skin_translate_host_kb_layout_to_android(const char* name) {
    int i;
    for (i = 0; i < sizeof(s_android_kb_layout_file) /
            sizeof(s_android_kb_layout_file[0]); i++) {
        if (strstr(s_android_kb_layout_file[i], name)) {
            return s_android_kb_layout_file[i];
        }
    }
    return NULL;
}