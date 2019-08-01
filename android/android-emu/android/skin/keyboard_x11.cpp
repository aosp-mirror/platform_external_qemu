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

#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include <iostream>
#include <regex>

#define DEBUG 1

#if DEBUG
#define D(...) VERBOSE_PRINT(keys, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

static Display* s_display = nullptr;

const char* skin_keyboard_current_active_layout() {
    if (!s_display) {
        s_display = XOpenDisplay(NULL);        
    }
    if (!s_display) {
        D("Failed to X11 open display");
        return NULL;
    }
    XkbStateRec state;
    XkbGetState(s_display, XkbUseCoreKbd, &state);

    XkbDescPtr desc =
            XkbGetKeyboard(s_display, XkbAllComponentsMask, XkbUseCoreKbd);
    return XGetAtomName(s_display, desc->names->groups[state.group]);
}

extern const char* skin_translate_host_kb_layout_to_android(const char* name) {
    std::string s(name);
    std::vector<std::string> tokens;
    std::regex re("\\b[[:alpha:]]+\\b");
    std::sregex_iterator next(s.begin(), s.end(), re);
    std::sregex_iterator end;
    while (next != end) {
        std::smatch match = *next;
        tokens.push_back(match.str());
        next++;
    }
    int size = 0;
    const char** android_kb_layout_file = skin_get_android_kb_layout(&size);
    for (int i = 0; i < size; i++) {
        bool isMatch = true;
        for (auto& s : tokens) {
            if (!strcasestr(android_kb_layout_file[i], s.c_str())) {
                isMatch = false;
                break;
            }
        }
        if (isMatch)
            return android_kb_layout_file[i];
    }

    return NULL;
}