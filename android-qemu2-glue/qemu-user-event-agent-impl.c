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
#include "android/skin/generic-event-buffer.h"
#include "android/utils/debug.h"

#include "qemu/osdep.h"
#include "hw/input/goldfish_events.h"
#include "hw/input/goldfish_events_common.h"
#include "hw/input/goldfish_rotary.h"
#include "ui/console.h"
#include "ui/input.h"

#include <stdbool.h>
#include <stdio.h>

static void user_event_key(unsigned code, bool down) {
    if (code == 0) {
        return;
    }
    if (VERBOSE_CHECK(keys))
        printf(">> %s KEY [0x%03x,%s]\n", __func__, (code & 0x3ff), down ? "down" : " up ");

    goldfish_event_send(0x01, code, down);
}

static void user_event_keycode(int code) {
    bool down = code & 0x400;
    if (VERBOSE_CHECK(keys))
        printf(">> %s KEY [0x%03x,%s]\n", __func__, (code & 0x3ff), down ? "down" : " up ");

    // Android already translates all the keycodes, so
    // we do not want to go through the Qemu keycode stack, as it will
    // end up confusing our goldfish drivers.
    //
    // Manually construct the key event
    KeyValue *key = g_new0(KeyValue, 1);
    key->type = KEY_VALUE_KIND_QCODE;
    key->u.qcode.data = code & 0x3ff;
    InputEvent *evt = g_new0(InputEvent, 1);
    evt->u.key.data = g_new0(InputKeyEvent, 1);
    evt->type = INPUT_EVENT_KIND_KEY;
    evt->u.key.data->key = key;
    evt->u.key.data->down = down;

    // Add to queue for the active console for processing
    QemuConsole *src = qemu_active_console();
    qemu_input_event_enqueue(src, evt);
}


static void user_event_keycodes(int* kcodes, int count) {
    int nn;
    for (nn = 0; nn < count; nn++) {
        user_event_keycode(kcodes[nn]);
    }
}

static void user_event_generic(int type, int code, int value) {
    goldfish_event_send(type, code, value);
}

static void user_event_generic_events(SkinGenericEventCode* events, int count) {
    int i;
    for (i = 0; i < count; i++) {
        goldfish_event_send(events[i].type, events[i].code,
                            events[i].value);
    }
}

static void user_event_mouse(int dx, int dy, int dz, int buttonsState) {
    if (VERBOSE_CHECK(keys)) {
        printf(">> MOUSE [%d %d %d : 0x%04x]\n", dx, dy, dz, buttonsState);
    }

    kbd_mouse_event(dx, dy, dz, buttonsState);
}

static void user_event_rotary(int delta) {
    VERBOSE_PRINT(keys, ">> ROTARY [%d]\n", delta);

    goldfish_rotary_send_rotate(delta);
}

static void on_new_event(void) {
    dpy_run_update(NULL);
}

static const QAndroidUserEventAgent sQAndroidUserEventAgent = {
        .sendKey = user_event_key,
        .sendKeyCode = user_event_keycode,
        .sendKeyCodes = user_event_keycodes,
        .sendMouseEvent = user_event_mouse,
        .sendRotaryEvent = user_event_rotary,
        .sendGenericEvent = user_event_generic,
        .sendGenericEvents = user_event_generic_events,
        .onNewUserEvent = on_new_event,
        .eventsDropped = goldfish_event_drop_count };


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
