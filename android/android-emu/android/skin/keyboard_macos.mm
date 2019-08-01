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

const static struct {
    const char* appleKeyboardLayout;
    const char* androidKeyboardLayout;
} s_macos_keyboard_layout_to_android[] = {
      {"com.apple.keylayout.British", "keyboard_layout_english_uk"},
      {"com.apple.keylayout.US", "keyboard_layout_english_us"},
      {"com.apple.keylayout.USInternational-PC", "keyboard_layout_english_us_intl"},
      {"com.apple.keylayout.Colemak", "keyboard_layout_english_us_colemak"},
      {"com.apple.keylayout.Dvorak", "keyboard_layout_english_us_dvorak"},
      {"com.apple.keylayout.German", "keyboard_layout_german"},
      {"com.apple.keylayout.French", "keyboard_layout_french"},
      {"com.apple.keylayout.Canadian-CSA", "keyboard_layout_french_ca"},
      {"com.apple.keylayout.Russian", "keyboard_layout_russian"},
      {"com.apple.keylayout.Spanish", "keyboard_layout_spanish"},
      {"com.apple.keylayout.Spanish-ISO", "keyboard_layout_spanish_latin"},
      {"com.apple.keylayout.SwissFrench", "keyboard_layout_swiss_french"},
      {"com.apple.keylayout.SwissGerman", "keyboard_layout_swiss_german"},
      {"com.apple.keylayout.Belgian", "keyboard_layout_belgian"},
      {"com.apple.keylayout.Bulgarian", "keyboard_layout_bulgarian"},
      {"com.apple.keylayout.Italian-Pro", "keyboard_layout_italian"},
      {"com.apple.keylayout.Danish", "keyboard_layout_danish"},
      {"com.apple.keylayout.Norwegian", "keyboard_layout_norwegian"},
      {"com.apple.keylayout.Swedish", "keyboard_layout_swedish"},
      {"com.apple.keylayout.Finnish", "keyboard_layout_finnish"},
      {"com.apple.keylayout.Croatian", "keyboard_layout_croatian"},
      {"com.apple.keylayout.Czech", "keyboard_layout_czech"},
      {"com.apple.keylayout.Estonian", "keyboard_layout_estonian"},
      {"com.apple.keylayout.Hungarian", "keyboard_layout_hungarian"},
      {"com.apple.keylayout.Icelandic", "keyboard_layout_icelandic"},
      {"com.apple.keylayout.Brazilian", "keyboard_layout_brazilian"},
      {"com.apple.keylayout.Portuguese", "keyboard_layout_portuguese"},
      {"com.apple.keylayout.Slovak", "keyboard_layout_slovak"},
      {"com.apple.keylayout.Slovenian", "keyboard_layout_slovenian"},
      {"com.apple.keylayout.Turkish-Standard", "keyboard_layout_turkish"},
      {"com.apple.keylayout.Turkish-QWERTY-PC", "keyboard_layout_turkish"},
      {"com.apple.keylayout.Ukrainian", "keyboard_layout_ukrainian"},
      {"com.apple.keylayout.Latvian", "keyboard_layout_latvian"},
      {"com.apple.keylayout.Persian", "keyboard_layout_persian"},
      {"com.apple.keylayout.Azeri", "keyboard_layout_azerbaijani"},
      {"com.apple.keylayout.Polish", "keyboard_layout_polish"},
};

const char* skin_translate_host_kb_layout_to_android(const char* name) {
    for (size_t i = 0; i < ARRAY_SIZE(s_macos_keyboard_layout_to_android); i++) {
        if (strcmp(name, s_macos_keyboard_layout_to_android[i].appleKeyboardLayout) == 0) {
            return s_macos_keyboard_layout_to_android[i].androidKeyboardLayout;
        }
    }
    return NULL;
}