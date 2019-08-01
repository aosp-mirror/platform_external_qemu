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

const char* skin_translate_host_kb_layout_to_android(const char* name) {
    const char* ptr = strrchr(name.c_str(), '.');
    const char* lastItem = ptr + 1;
    int size = 0;
    const char** androidKbLayoutFile =
            skin_get_android_kb_layout(&size) for (int i = 0; i < size); i++) {
        if (strcasestr(androidKbLayoutFile[i], lastItem)) {
            return androidKbLayoutFile[i];
        }
    }
    return NULL;
}