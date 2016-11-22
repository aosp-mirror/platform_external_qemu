/* Copyright (C) 2010-2015 The Android Open Source Project
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
#include "android-qemu1-glue/qemu-control-impl.h"

#include "android/utils/debug.h"
#include "hw/android/goldfish/device.h"
#include "ui/console.h"

#include <stdbool.h>
#include <stdio.h>

void* eventDeviceHandle = NULL;
void qemu_control_setEventDevice(void* opaque) {
    eventDeviceHandle = opaque;
}

static void user_event_keycodes(int* kcodes, int count) {
    int nn;
    for (nn = 0; nn < count; nn++)
        kbd_put_keycode(kcodes[nn]);
}

static void user_event_key(unsigned code, bool down) {
    if (code == 0) {
        return;
    }
    if (VERBOSE_CHECK(keys))
        printf(">> KEY [0x%03x,%s]\n", (code & 0x3ff), down ? "down" : " up ");

    kbd_put_keycode((code & 0x3ff) | (down ? 0x400 : 0));
}

static void user_event_generic(int type, int code, int value) {
    if (eventDeviceHandle) {
        events_put_generic(eventDeviceHandle, type, code, value);
    }
}

const static QAndroidUserEventAgent sQAndroidUserEventAgent = {
        .sendKey = user_event_key,
        .sendKeyCode = kbd_put_keycode,
        .sendKeyCodes = user_event_keycodes,
        .sendMouseEvent = kbd_mouse_event,
        .sendGenericEvent = user_event_generic};
const QAndroidUserEventAgent* const gQAndroidUserEventAgent =
        &sQAndroidUserEventAgent;
