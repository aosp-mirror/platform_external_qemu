/* Copyright (C) 2010 The Android Open Source Project
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
#include "hw/input/goldfish_events.h"
#include "ui/console.h"

#include <stdbool.h>
#include <stdio.h>

// just a flag that qemu is initialized and it's ok to send user events
static void* eventDeviceHandle = NULL;

void qemu_control_setEventDevice(void* opaque) {
    assert(eventDeviceHandle == NULL);
    eventDeviceHandle = opaque;
}

static void user_event_keycodes(int* kcodes, int count) {
    int nn;
    for (nn = 0; nn < count; nn++) {
        kbd_put_keycode(kcodes[nn] & 0x1ff, (kcodes[nn] & 0x200) != 0);
    }
}

static void user_event_key(unsigned code, bool down) {
    if (code == 0) {
        return;
    }
    if (VERBOSE_CHECK(keys))
        printf(">> KEY [0x%03x,%s]\n", (code & 0x1ff), down ? "down" : " up ");

    gf_event_send(0x01, code, down);
}

static void user_event_keycode(int code) {
    kbd_put_keycode(code & 0x1ff, (code & 0x200) != 0);
}

static void user_event_generic(int type, int code, int value) {
    if (eventDeviceHandle) {
        gf_event_send(type, code, value);
    }
}

static void user_event_mouse(int dx, int dy, int dz, int buttonsState) {
    if (VERBOSE_CHECK(keys)) {
        printf(">> MOUSE [%d %d %d : 0x%04x]\n", dx, dy, dz, buttonsState);
    }

    kbd_mouse_event(dx, dy, dz, buttonsState);
}

static const QAndroidUserEventAgent sQAndroidUserEventAgent = {
        .sendKey = user_event_key,
        .sendKeyCode = user_event_keycode,
        .sendKeyCodes = user_event_keycodes,
        .sendMouseEvent = user_event_mouse,
        .sendGenericEvent = user_event_generic
};

const QAndroidUserEventAgent* const gQAndroidUserEventAgent =
        &sQAndroidUserEventAgent;
