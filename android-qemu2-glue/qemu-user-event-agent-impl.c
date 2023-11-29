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
#include <stdbool.h>
#include <stdio.h>

#include "android-qemu2-glue/emulation/virtio-input-multi-touch.h"
#include "android-qemu2-glue/emulation/virtio-input-rotary.h"
#include "android-qemu2-glue/qemu-control-impl.h"
#include "android/avd/info.h"
#include "android/console.h"
#include "android/multitouch-screen.h"
#include "android/skin/event.h"
#include "android/skin/generic-event-buffer.h"
#include "android/utils/debug.h"
#include "host-common/feature_control.h"
#include "hw/input/goldfish_events.h"
#include "hw/input/goldfish_events_common.h"
#include "hw/input/goldfish_rotary.h"
#include "qemu/osdep.h"
#include "ui/console.h"
#include "ui/input.h"

static void user_event_keycode(int code) {
    bool down = code & 0x400;
    if (VERBOSE_CHECK(keys))
        dprint(">> %s KEY [0x%03x,%s]", __func__, (code & 0x3ff), down ? "down" : " up ");

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

static void user_event_key(unsigned code, bool down) {
    if (code == 0) {
        return;
    }
    if (VERBOSE_CHECK(keys))
        dprint(">> %s KEY [0x%03x,%s]", __func__, (code & 0x3ff), down ? "down" : " up ");

    if (down) {
        code |= 0x400;
    }
    user_event_keycode(code);
}

/*
 * Both goldfish_events and virtio_input_multi_touch are maintained in order to
 * be backward compatible. When feature Virtio is enabled, makes sure
 * multi-touch events (type == EV_ABS or type == EV_SYN) are sent to virtio
 * input multi touch device. For qemu2 and API 10+, multi-touch is always
 * supported by default.
 */
static void user_event_generic(SkinGenericEventCode event) {
    bool sent = false;
    if (feature_is_enabled(kFeature_VirtioInput)) {
        sent = android_virtio_input_send(event.type, event.code, event.value,
                                         event.displayId);
    }

    if (!sent) {
        goldfish_event_send(event.type, event.code, event.value);
    }
}

static void user_event_generic_events(SkinGenericEventCode* events, int count) {
    int i;
    for (i = 0; i < count; i++) {
        user_event_generic(events[i]);
    }
}

static void user_event_touch(const SkinEvent * const data,
                             int displayId) {
    android_virtio_touch_event(data, displayId);
}

static void user_event_mouse(int dx,
                             int dy,
                             int dz,
                             int buttonsState,
                             int displayId) {
    if (VERBOSE_CHECK(keys))
        dprint(">> MOUSE [%d %d %d : 0x%04x]", dx, dy, dz, buttonsState);
    if (feature_is_enabled(kFeature_VirtioInput) &&
        !feature_is_enabled(kFeature_VirtioMouse) &&
        !feature_is_enabled(kFeature_VirtioTablet))
        android_virtio_kbd_mouse_event(dx, dy, dz, buttonsState, displayId);
    else {
        if (feature_is_enabled(kFeature_VirtioTablet)) {
            kbd_put_tablet_button_state(buttonsState);
        }
        if (qemu_input_is_absolute()) {
            int w = surface_width(qemu_console_surface(qemu_active_console()));
            int h = surface_height(qemu_console_surface(qemu_active_console()));
            bool isUdcOrHigher =
                    avdInfo_getApiLevel(
                            getConsoleAgents()->settings->avdInfo()) >= 34;
            // Bug(b/309667960): Stop-gap solution to resolve the difference
            // between tablet size and display viewport size.
            if (isUdcOrHigher) {
                int size = (w > h) ? w : h;
                kbd_mouse_event_absolute(dx, dy, dz, buttonsState, size, size);
            } else
                kbd_mouse_event_absolute(dx, dy, dz, buttonsState, w, h);
        } else {
            kbd_mouse_event(dx, dy, dz, buttonsState);
        }
    }
}

static void user_event_pen(int dx,
                           int dy,
                           const SkinEvent* ev,
                           int buttonsState,
                           int displayId) {
    if (VERBOSE_CHECK(keys)) {
        dprint(">> PEN [%d %d : 0x%04x]", dx, dy, buttonsState);
    }
    android_virtio_pen_event(dx, dy, ev, buttonsState, displayId);
}

// Scales v by a factor of 1/d. If the result is zero, returns 1 or -1 instead
// to retain v's sign. d must be a positive integer.
static int scale_with_retaining_sign(int v, int d) {
    if (v == 0) return 0;
    else if (v > 0 && v < d) return 1;
    else if (v < 0 && v > -d) return -1;
    else return v / d;
}

static void user_event_mouse_wheel(int dx, int dy, int displayId) {
    if (VERBOSE_CHECK(keys)) {
        dprint(">> MOUSE WHEEL [%d %d]", dx, dy);
    }
    if (!feature_is_enabled(kFeature_VirtioMouse) &&
            !feature_is_enabled(kFeature_VirtioTablet)) {
        return;
    }
    kbd_mouse_wheel_event(
        scale_with_retaining_sign(dx, 120), scale_with_retaining_sign(dy, 120));
}

static void user_event_rotary(int delta) {
    VERBOSE_PRINT(keys, ">> ROTARY [%d]\n", delta);

    if (feature_is_enabled(kFeature_VirtioInput)) {
        virtio_send_rotary_event(delta);
    } else {
        goldfish_rotary_send_rotate(delta);
    }
}

static void on_new_event(void) {
    dpy_run_update(NULL);
}

static const QAndroidUserEventAgent sQAndroidUserEventAgent = {
        .sendTouchEvents = user_event_touch,
        .sendKey = user_event_key,
        .sendKeyCode = user_event_keycode,
        .sendKeyCodes = user_event_keycodes,
        .sendMouseEvent = user_event_mouse,
        .sendPenEvent = user_event_pen,
        .sendMouseWheelEvent = user_event_mouse_wheel,
        .sendRotaryEvent = user_event_rotary,
        .sendGenericEvent = user_event_generic,
        .sendGenericEvents = user_event_generic_events,
        .onNewUserEvent = on_new_event,
        .eventsDropped = goldfish_event_drop_count};

const QAndroidUserEventAgent* const gQAndroidUserEventAgent =
        &sQAndroidUserEventAgent;

static void translate_mouse_event(int x,
                                  int y,
                                  int buttons_state) {
    int pressure = multitouch_is_touch_down(buttons_state) ?
                                                MTS_PRESSURE_RANGE_MAX : 0;
    int finger = multitouch_is_second_finger(buttons_state);
    multitouch_update_pointer(MTES_MOUSE, finger, x, y, pressure,
                              multitouch_should_skip_sync(buttons_state));
}

const GoldfishEventMultitouchFuncs qemu2_goldfish_event_multitouch_funcs = {
    .get_max_slot = multitouch_get_max_slot,
    .translate_mouse_event = translate_mouse_event,
};
