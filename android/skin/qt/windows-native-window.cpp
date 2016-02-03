/* Copyright (C) 2016 The Android Open Source Project
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

#include <windows.h>
#include "android/skin/qt/windows-native-window.h"

int numHeldMouseButtons() {
    int numHeld = 0;
    if (GetKeyState(VK_LBUTTON) & 0x80) {
        numHeld++;
    }
    if (GetKeyState(VK_RBUTTON) & 0x80) {
        numHeld++;
    }
    return numHeld;
}
