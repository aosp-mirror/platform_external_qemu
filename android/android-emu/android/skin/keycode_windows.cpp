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

#include <windows.h>

// Virtual-Key Codes. Reference: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
// only include those keys defined in frameworks/base/data/keyboards/qwerty.kl
bool skin_keycode_native_to_linux(int32_t* pVirtualKey, int32_t* pModifier) {
    static const struct {
        int virtualKey;
        int linuxKeyCode;
        bool addShiftMod;
    } kTbl[] = {
        // Do not distinguish left or right for modifier keys
        { VK_SHIFT, LINUX_KEY_LEFTSHIFT },
        { VK_MENU, LINUX_KEY_LEFTALT },
        { VK_CONTROL, LINUX_KEY_LEFTCTRL },

        // Other keys are reported properly
        { VK_APPS, LINUX_KEY_MENU },
        { VK_OEM_1, LINUX_KEY_SEMICOLON },
        { VK_OEM_2, LINUX_KEY_SLASH },
        { VK_OEM_PLUS, LINUX_KEY_EQUAL },
        { VK_OEM_MINUS, LINUX_KEY_MINUS },
        { VK_OEM_4, LINUX_KEY_LEFTBRACE },
        { VK_OEM_6, LINUX_KEY_RIGHTBRACE },
        { VK_OEM_COMMA, LINUX_KEY_COMMA },
        { VK_OEM_PERIOD, LINUX_KEY_DOT },
        { VK_OEM_7, LINUX_KEY_APOSTROPHE },
        { VK_OEM_5, LINUX_KEY_BACKSLASH },
        { VK_OEM_3, LINUX_KEY_GREEN },
        { VK_ESCAPE, LINUX_KEY_ESC },
        { VK_SPACE, LINUX_KEY_SPACE },
        { VK_RETURN, LINUX_KEY_ENTER },
        { VK_BACK, LINUX_KEY_BACKSPACE },
        { VK_TAB, LINUX_KEY_TAB },
        // { VK_PRIOR, LINUX_KEY_PAGEUP },
        // { VK_NEXT, LINUX_KEY_PAGEDOWN },
        //Do not include LINUX_KEY_END because it will be effectively an ENDCALL which turns off the screen.
        // { VK_END, LINUX_KEY_END },
        { VK_HOME, LINUX_KEY_HOME },
        // { VK_INSERT, LINUX_KEY_INSERT },
        // { VK_DELETE, LINUX_KEY_DEL },
        { VK_SUBTRACT, LINUX_KEY_MINUS },
        { VK_PAUSE, LINUX_KEY_PAUSE },
        { VK_LEFT, LINUX_KEY_LEFT },
        { VK_RIGHT, LINUX_KEY_RIGHT },
        { VK_UP, LINUX_KEY_UP },
        { VK_DOWN, LINUX_KEY_DOWN },
        { VK_NUMPAD0, LINUX_KEY_KP0 },
        { VK_NUMPAD1, LINUX_KEY_KP1 },
        { VK_NUMPAD2, LINUX_KEY_KP2 },
        { VK_NUMPAD3, LINUX_KEY_KP3 },
        { VK_NUMPAD4, LINUX_KEY_KP4 },
        { VK_NUMPAD5, LINUX_KEY_KP5 },
        { VK_NUMPAD6, LINUX_KEY_KP6 },
        { VK_NUMPAD7, LINUX_KEY_KP7 },
        { VK_NUMPAD8, LINUX_KEY_KP8 },
        { VK_NUMPAD9, LINUX_KEY_KP9 },
        { VK_ADD, LINUX_KEY_EQUAL, true},
        { VK_MULTIPLY, LINUX_KEY_8, true},
        { VK_DECIMAL, LINUX_KEY_DOT },
        { VK_DIVIDE, LINUX_KEY_SLASH },
        { 'A', LINUX_KEY_A },
        { 'Z', LINUX_KEY_Z },
        { 'E', LINUX_KEY_E },
        { 'R', LINUX_KEY_R },
        { 'T', LINUX_KEY_T },
        { 'Y', LINUX_KEY_Y },
        { 'U', LINUX_KEY_U },
        { 'I', LINUX_KEY_I },
        { 'O', LINUX_KEY_O },
        { 'P', LINUX_KEY_P },
        { 'Q', LINUX_KEY_Q },
        { 'S', LINUX_KEY_S },
        { 'D', LINUX_KEY_D },
        { 'F', LINUX_KEY_F },
        { 'G', LINUX_KEY_G },
        { 'H', LINUX_KEY_H },
        { 'J', LINUX_KEY_J },
        { 'K', LINUX_KEY_K },
        { 'L', LINUX_KEY_L },
        { 'M', LINUX_KEY_M },
        { 'W', LINUX_KEY_W },
        { 'X', LINUX_KEY_X },
        { 'C', LINUX_KEY_C },
        { 'V', LINUX_KEY_V },
        { 'B', LINUX_KEY_B },
        { 'N', LINUX_KEY_N },
        { '0', LINUX_KEY_0 },
        { '1', LINUX_KEY_1 },
        { '2', LINUX_KEY_2 },
        { '3', LINUX_KEY_3 },
        { '4', LINUX_KEY_4 },
        { '5', LINUX_KEY_5 },
        { '6', LINUX_KEY_6 },
        { '7', LINUX_KEY_7 },
        { '8', LINUX_KEY_8 },
        { '9', LINUX_KEY_9 },
    };
    size_t nn;
    for (nn = 0; nn < android::base::arraySize(kTbl); ++nn) {
        if (*pVirtualKey == kTbl[nn].virtualKey) {
            *pVirtualKey = kTbl[nn].linuxKeyCode;
            if (kTbl[nn].addShiftMod) {
                *pModifier |= MOD_SHIFT;
            }
            return true;
        }
    }
    return false;
}