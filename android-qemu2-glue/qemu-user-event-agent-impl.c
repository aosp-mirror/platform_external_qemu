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
#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/multitouch-screen.h"
#include "android/utils/debug.h"

#include "qemu/osdep.h"
#include "hw/input/goldfish_events.h"
#include "ui/console.h"

#include <stdbool.h>
#include <stdio.h>

static void user_event_keycodes(int* kcodes, int count) {
    int nn;
    for (nn = 0; nn < count; nn++) {
        kbd_put_keycode(kcodes[nn] & 0x3ff, (kcodes[nn] & 0x400) != 0);
    }
}

static void user_event_key(unsigned code, bool down) {
    if (code == 0) {
        return;
    }
    if (VERBOSE_CHECK(keys))
        printf(">> KEY [0x%03x,%s]\n", (code & 0x3ff), down ? "down" : " up ");

    goldfish_event_send(0x01, code, down);
}

static void user_event_keycode(int code) {
    kbd_put_keycode(code & 0x3ff, (code & 0x400) != 0);
}

static void user_event_generic(int type, int code, int value) {
    goldfish_event_send(type, code, value);
}

static void user_event_mouse(int dx, int dy, int dz, int buttonsState) {
    if (VERBOSE_CHECK(keys)) {
        printf(">> MOUSE [%d %d %d : 0x%04x]\n", dx, dy, dz, buttonsState);
    }

    kbd_mouse_event(dx, dy, dz, buttonsState);
}

static void on_new_event(void) {
    dpy_run_update(NULL);
}

static const QAndroidUserEventAgent sQAndroidUserEventAgent = {
        .sendKey = user_event_key,
        .sendKeyCode = user_event_keycode,
        .sendKeyCodes = user_event_keycodes,
        .sendMouseEvent = user_event_mouse,
        .sendGenericEvent = user_event_generic,
        .onNewUserEvent = on_new_event
};

const QAndroidUserEventAgent* const gQAndroidUserEventAgent =
        &sQAndroidUserEventAgent;

static void translate_mouse_event(int x,
                                  int y,
                                  int buttons_state) {
    int pressure = multitouch_is_touch_down(buttons_state) ? 0x81 : 0;
    int finger = multitouch_is_second_finger(buttons_state);
    multitouch_update_pointer(MTES_DEVICE, finger, x, y, pressure,
                              multitouch_should_skip_sync(buttons_state));
}

const GoldfishEventMultitouchFuncs qemu2_goldfish_event_multitouch_funcs = {
    .get_max_slot = multitouch_get_max_slot,
    .translate_mouse_event = translate_mouse_event,
};
