/* Copyright (C) 2019 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android/skin/keycode.h"

#include "android/base/ArraySize.h"
#include "android/skin/macos_keycodes.h"

#include <Carbon/Carbon.h>

// Reference: HIToolbox.framework/Versions/A/Headers/Events.h
// Only include keys defined in frameworks/base/data/keyboards/qwerty.kl

bool skin_keycode_native_to_linux(int32_t* pVirtualKey, int32_t* pModifier) {
    static const struct {
        int virtualKey;
        int linuxKeyCode;
        bool addShiftMod;
    } kTbl[] = {
        { kVK_ANSI_A, LINUX_KEY_A },
        { kVK_ANSI_S, LINUX_KEY_S },
        { kVK_ANSI_D, LINUX_KEY_D },
        { kVK_ANSI_F, LINUX_KEY_F },
        { kVK_ANSI_H, LINUX_KEY_H },
        { kVK_ANSI_G, LINUX_KEY_G },
        { kVK_ANSI_Z, LINUX_KEY_Z },
        { kVK_ANSI_X, LINUX_KEY_X },
        { kVK_ANSI_C, LINUX_KEY_C },
        { kVK_ANSI_V, LINUX_KEY_V },
        { kVK_ANSI_B, LINUX_KEY_B },
        { kVK_ANSI_Q, LINUX_KEY_Q },
        { kVK_ANSI_W, LINUX_KEY_W },
        { kVK_ANSI_E, LINUX_KEY_E },
        { kVK_ANSI_R, LINUX_KEY_R },
        { kVK_ANSI_Y, LINUX_KEY_Y },
        { kVK_ANSI_T, LINUX_KEY_T },
        { kVK_ANSI_1, LINUX_KEY_1 },
        { kVK_ANSI_2, LINUX_KEY_2 },
        { kVK_ANSI_3, LINUX_KEY_3 },
        { kVK_ANSI_4, LINUX_KEY_4 },
        { kVK_ANSI_6, LINUX_KEY_6 },
        { kVK_ANSI_5, LINUX_KEY_5 },
        { kVK_ANSI_Equal, LINUX_KEY_EQUAL },
        { kVK_ANSI_9, LINUX_KEY_9 },
        { kVK_ANSI_7, LINUX_KEY_7 },
        { kVK_ANSI_Minus, LINUX_KEY_MINUS },
        { kVK_ANSI_8, LINUX_KEY_8 },
        { kVK_ANSI_0, LINUX_KEY_0 },
        { kVK_ANSI_RightBracket, LINUX_KEY_RIGHTBRACE },
        { kVK_ANSI_O, LINUX_KEY_O },
        { kVK_ANSI_U, LINUX_KEY_U },
        { kVK_ANSI_LeftBracket, LINUX_KEY_LEFTBRACE },
        { kVK_ANSI_I, LINUX_KEY_I },
        { kVK_ANSI_P, LINUX_KEY_P },
        { kVK_ANSI_L, LINUX_KEY_L },
        { kVK_ANSI_J, LINUX_KEY_J },
        { kVK_ANSI_Quote, LINUX_KEY_APOSTROPHE },
        { kVK_ANSI_K, LINUX_KEY_K },
        { kVK_ANSI_Semicolon, LINUX_KEY_SEMICOLON },
        { kVK_ANSI_Backslash, LINUX_KEY_BACKSLASH },
        { kVK_ANSI_Comma, LINUX_KEY_COMMA },
        { kVK_ANSI_Slash, LINUX_KEY_SLASH },
        { kVK_ANSI_N, LINUX_KEY_N },
        { kVK_ANSI_M, LINUX_KEY_M },
        { kVK_ANSI_Period, LINUX_KEY_DOT },
        { kVK_ANSI_Grave, LINUX_KEY_GREEN },
        { kVK_ANSI_KeypadDecimal, LINUX_KEY_KPDOT },
        { kVK_ANSI_KeypadMultiply, LINUX_KEY_8, true },
        { kVK_ANSI_KeypadPlus, LINUX_KEY_EQUAL, true },
        //{  kVK_ANSI_KeypadClear, LINUX_KEY_UNKNOWN  },
        { kVK_ANSI_KeypadDivide, LINUX_KEY_SLASH },
        { kVK_ANSI_KeypadEnter, LINUX_KEY_ENTER },
        { kVK_ANSI_KeypadMinus, LINUX_KEY_MINUS },
        { kVK_ANSI_KeypadEquals, LINUX_KEY_EQUAL },
        { kVK_ANSI_Keypad0, LINUX_KEY_KP0 },
        { kVK_ANSI_Keypad1, LINUX_KEY_KP1 },
        { kVK_ANSI_Keypad2, LINUX_KEY_KP2 },
        { kVK_ANSI_Keypad3, LINUX_KEY_KP3 },
        { kVK_ANSI_Keypad4, LINUX_KEY_KP4 },
        { kVK_ANSI_Keypad5, LINUX_KEY_KP5 },
        { kVK_ANSI_Keypad6, LINUX_KEY_KP6 },
        { kVK_ANSI_Keypad7, LINUX_KEY_KP7 },
        { kVK_ANSI_Keypad8, LINUX_KEY_KP8 },
        { kVK_ANSI_Keypad9, LINUX_KEY_KP9 },
        { kVK_Return, LINUX_KEY_ENTER },
        { kVK_Tab, LINUX_KEY_TAB },
        { kVK_Space, LINUX_KEY_SPACE },
        { kVK_Delete, LINUX_KEY_BACKSPACE },
        { kVK_Escape, LINUX_KEY_ESC },
        { kVK_Command, LINUX_KEY_LEFTCTRL },
        { kVK_Shift, LINUX_KEY_LEFTSHIFT },
        //{ kVK_CapsLock, LINUX_KEY_CAPSLOCK },
        { kVK_Option, LINUX_KEY_LEFTALT },
        { kVK_Control, LINUX_KEY_LEFTCTRL },
        { kVK_RightCommand, LINUX_KEY_RIGHTCTRL },
        { kVK_RightShift, LINUX_KEY_RIGHTSHIFT },
        { kVK_RightOption, LINUX_KEY_RIGHTALT },
        { kVK_RightControl, LINUX_KEY_RIGHTCTRL },
        //{  kVK_Function     , LINUX_KEY_UNKNOWN  },
        { kVK_VolumeUp, LINUX_KEY_VOLUMEUP },
        { kVK_VolumeDown, LINUX_KEY_VOLUMEDOWN },
        { kVK_Home, LINUX_KEY_HOME },
        { kVK_LeftArrow, LINUX_KEY_LEFT },
        { kVK_RightArrow, LINUX_KEY_RIGHT },
        { kVK_DownArrow, LINUX_KEY_DOWN },
        { kVK_UpArrow, LINUX_KEY_UP },
    };

    size_t nn;
    for (nn = 0; nn < android::base::arraySize(kTbl); ++nn) {
        if (*pVirtualKey == kTbl[nn].virtualKey) {
            *pVirtualKey = kTbl[nn].linuxKeyCode;
            if (kTbl[nn].addShiftMod) {
                *pModifier |= NSEventModifierFlagShift;
            }
            return true;
        }
    }
    return false;
}

static struct keypad_num_mapping {
    int32_t keypad;
    int32_t digit;
} sKeypadNumMapping[] = {
        {kVK_ANSI_Keypad7, kVK_ANSI_7},             // KP_7 --> 7
        {kVK_ANSI_Keypad8, kVK_ANSI_8},             // KP_8 --> 8
        {kVK_ANSI_Keypad9, kVK_ANSI_9},             // KP_9 --> 9
        {kVK_ANSI_Keypad4, kVK_ANSI_4},             // KP_4 --> 4
        {kVK_ANSI_Keypad5, kVK_ANSI_5},             // KP_5 --> 5
        {kVK_ANSI_Keypad6, kVK_ANSI_6},             // KP_6 --> 6
        {kVK_ANSI_Keypad1, kVK_ANSI_1},             // KP_1 --> 1
        {kVK_ANSI_Keypad2, kVK_ANSI_2},             // KP_2 --> 2
        {kVK_ANSI_Keypad3, kVK_ANSI_3},             // KP_3 --> 3
        {kVK_ANSI_Keypad0, kVK_ANSI_0},             // KP_0 --> 0
        {kVK_ANSI_KeypadDecimal, kVK_ANSI_Period},  // KP_DOT --> DOT
};

bool skin_keycode_native_is_keypad(int32_t virtualKey) {
    for (size_t nn = 0; nn < android::base::arraySize(sKeypadNumMapping);
         nn++) {
        if (virtualKey == sKeypadNumMapping[nn].keypad) {
            return true;
        }
    }
    return false;
}

int32_t skin_keycode_native_map_keypad(int32_t virtualKey) {
    for (size_t nn = 0; nn < android::base::arraySize(sKeypadNumMapping);
         nn++) {
        if (virtualKey == sKeypadNumMapping[nn].keypad) {
            return sKeypadNumMapping[nn].digit;
        }
    }
    return -1;
}
