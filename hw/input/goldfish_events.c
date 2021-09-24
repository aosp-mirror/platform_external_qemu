/*
 * Goldfish 'events' device model
 *
 * Copyright (C) 2007-2008 The Android Open Source Project
 * Copyright (c) 2014 Linaro Limited
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "ui/input.h"
#include "ui/console.h"
#include "hw/input/android_keycodes.h"
#include "hw/input/linux_keycodes.h"
#include "hw/input/goldfish_events_common.h"

#include "hw/input/goldfish_events.h"


/* Range maximum value of the major and minor touch axis*/
#define MTS_TOUCH_AXIS_RANGE_MAX    0x7fffffff
/* Range maximum value of orientation, corresponds to 90 degrees*/
#define MTS_ORIENTATION_RANGE_MAX   90
/* Range maximum value of pressure, corresponds to 1024 levels of pressure */
#define MTS_PRESSURE_RANGE_MAX      0x400


/*
 * MT_TOOL types, duplicated from include\standard-headers\linux\input.h
 */
#define MT_TOOL_FINGER      0
#define MT_TOOL_PEN         1
#define MT_TOOL_MAX         0x0F


typedef struct {
    const char *name;
    int value;
} GoldfishEventCodeInfo;

static const GoldfishEventCodeInfo ev_abs_codes_table[] = {
    EV_CODE(ABS_X),
    EV_CODE(ABS_Y),
    EV_CODE(ABS_Z),
    EV_CODE(ABS_MT_SLOT),
    EV_CODE(ABS_MT_TOUCH_MAJOR),
    EV_CODE(ABS_MT_TOUCH_MINOR),
    EV_CODE(ABS_MT_WIDTH_MAJOR),
    EV_CODE(ABS_MT_WIDTH_MINOR),
    EV_CODE(ABS_MT_ORIENTATION),
    EV_CODE(ABS_MT_POSITION_X),
    EV_CODE(ABS_MT_POSITION_Y),
    EV_CODE(ABS_MT_TOOL_TYPE),
    EV_CODE(ABS_MT_BLOB_ID),
    EV_CODE(ABS_MT_TRACKING_ID),
    EV_CODE(ABS_MT_PRESSURE),
    EV_CODE(ABS_MT_DISTANCE),
    EV_CODE(ABS_MAX),
    EV_CODE_END,
};

static const GoldfishEventCodeInfo ev_rel_codes_table[] = {
    EV_CODE(REL_X),
    EV_CODE(REL_Y),
    EV_CODE_END,
};

static const GoldfishEventCodeInfo ev_sw_codes_table[] = {
    EV_CODE(SW_LID),
    EV_CODE(SW_TABLET_MODE),
    EV_CODE(SW_HEADPHONE_INSERT),
    EV_CODE(SW_MICROPHONE_INSERT),
    EV_CODE_END,
};

static const GoldfishEventCodeInfo ev_key_codes_table[] = {
    KEY_CODE(KEY_ESC, LINUX_KEY_ESC),
    KEY_CODE(KEY_1, LINUX_KEY_1),
    KEY_CODE(KEY_2, LINUX_KEY_2),
    KEY_CODE(KEY_3, LINUX_KEY_3),
    KEY_CODE(KEY_4, LINUX_KEY_4),
    KEY_CODE(KEY_5, LINUX_KEY_5),
    KEY_CODE(KEY_6, LINUX_KEY_6),
    KEY_CODE(KEY_7, LINUX_KEY_7),
    KEY_CODE(KEY_8, LINUX_KEY_8),
    KEY_CODE(KEY_9, LINUX_KEY_9),
    KEY_CODE(KEY_0, LINUX_KEY_0),
    KEY_CODE(KEY_MINUS, LINUX_KEY_MINUS),
    KEY_CODE(KEY_EQUAL, LINUX_KEY_EQUAL),
    KEY_CODE(KEY_BACKSPACE, LINUX_KEY_BACKSPACE),
    KEY_CODE(KEY_TAB, LINUX_KEY_TAB),
    KEY_CODE(KEY_Q, LINUX_KEY_Q),
    KEY_CODE(KEY_W, LINUX_KEY_W),
    KEY_CODE(KEY_E, LINUX_KEY_E),
    KEY_CODE(KEY_R, LINUX_KEY_R),
    KEY_CODE(KEY_T, LINUX_KEY_T),
    KEY_CODE(KEY_Y, LINUX_KEY_Y),
    KEY_CODE(KEY_U, LINUX_KEY_U),
    KEY_CODE(KEY_I, LINUX_KEY_I),
    KEY_CODE(KEY_O, LINUX_KEY_O),
    KEY_CODE(KEY_P, LINUX_KEY_P),
    KEY_CODE(KEY_LEFTBRACE, LINUX_KEY_LEFTBRACE),
    KEY_CODE(KEY_RIGHTBRACE, LINUX_KEY_RIGHTBRACE),
    KEY_CODE(KEY_ENTER, LINUX_KEY_ENTER),
    KEY_CODE(KEY_LEFTCTRL, LINUX_KEY_LEFTCTRL),
    KEY_CODE(KEY_A, LINUX_KEY_A),
    KEY_CODE(KEY_S, LINUX_KEY_S),
    KEY_CODE(KEY_D, LINUX_KEY_D),
    KEY_CODE(KEY_F, LINUX_KEY_F),
    KEY_CODE(KEY_G, LINUX_KEY_G),
    KEY_CODE(KEY_H, LINUX_KEY_H),
    KEY_CODE(KEY_J, LINUX_KEY_J),
    KEY_CODE(KEY_K, LINUX_KEY_K),
    KEY_CODE(KEY_L, LINUX_KEY_L),
    KEY_CODE(KEY_SEMICOLON, LINUX_KEY_SEMICOLON),
    KEY_CODE(KEY_APOSTROPHE, LINUX_KEY_APOSTROPHE),
    KEY_CODE(KEY_GRAVE, LINUX_KEY_GRAVE),
    KEY_CODE(KEY_LEFTSHIFT, LINUX_KEY_LEFTSHIFT),
    KEY_CODE(KEY_BACKSLASH, LINUX_KEY_BACKSLASH),
    KEY_CODE(KEY_Z, LINUX_KEY_Z),
    KEY_CODE(KEY_X, LINUX_KEY_X),
    KEY_CODE(KEY_C, LINUX_KEY_C),
    KEY_CODE(KEY_V, LINUX_KEY_V),
    KEY_CODE(KEY_B, LINUX_KEY_B),
    KEY_CODE(KEY_N, LINUX_KEY_N),
    KEY_CODE(KEY_M, LINUX_KEY_M),
    KEY_CODE(KEY_COMMA, LINUX_KEY_COMMA),
    KEY_CODE(KEY_DOT, LINUX_KEY_DOT),
    KEY_CODE(KEY_SLASH, LINUX_KEY_SLASH),
    KEY_CODE(KEY_RIGHTSHIFT, LINUX_KEY_RIGHTSHIFT),
    KEY_CODE(KEY_KPASTERISK, LINUX_KEY_KPASTERISK),
    KEY_CODE(KEY_LEFTALT, LINUX_KEY_LEFTALT),
    KEY_CODE(KEY_SPACE, LINUX_KEY_SPACE),
    KEY_CODE(KEY_CAPSLOCK, LINUX_KEY_CAPSLOCK),
    KEY_CODE(KEY_F1, LINUX_KEY_F1),
    KEY_CODE(KEY_F2, LINUX_KEY_F2),
    KEY_CODE(KEY_F3, LINUX_KEY_F3),
    KEY_CODE(KEY_F4, LINUX_KEY_F4),
    KEY_CODE(KEY_F5, LINUX_KEY_F5),
    KEY_CODE(KEY_F6, LINUX_KEY_F6),
    KEY_CODE(KEY_F7, LINUX_KEY_F7),
    KEY_CODE(KEY_F8, LINUX_KEY_F8),
    KEY_CODE(KEY_F9, LINUX_KEY_F9),
    KEY_CODE(KEY_F10, LINUX_KEY_F10),
    KEY_CODE(KEY_NUMLOCK, LINUX_KEY_NUMLOCK),
    KEY_CODE(KEY_SCROLLLOCK, LINUX_KEY_SCROLLLOCK),
    KEY_CODE(KEY_KP7, LINUX_KEY_KP7),
    KEY_CODE(KEY_KP8, LINUX_KEY_KP8),
    KEY_CODE(KEY_KP9, LINUX_KEY_KP9),
    KEY_CODE(KEY_KPMINUS, LINUX_KEY_KPMINUS),
    KEY_CODE(KEY_KP4, LINUX_KEY_KP4),
    KEY_CODE(KEY_KP5, LINUX_KEY_KP5),
    KEY_CODE(KEY_KP6, LINUX_KEY_KP6),
    KEY_CODE(KEY_KPPLUS, LINUX_KEY_KPPLUS),
    KEY_CODE(KEY_KP1, LINUX_KEY_KP1),
    KEY_CODE(KEY_KP2, LINUX_KEY_KP2),
    KEY_CODE(KEY_KP3, LINUX_KEY_KP3),
    KEY_CODE(KEY_KP0, LINUX_KEY_KP0),
    KEY_CODE(KEY_KPDOT, LINUX_KEY_KPDOT),
    KEY_CODE(KEY_ZENKAKUHANKAKU, LINUX_KEY_ZENKAKUHANKAKU),
    KEY_CODE(KEY_102ND, LINUX_KEY_102ND),
    KEY_CODE(KEY_F11, LINUX_KEY_F11),
    KEY_CODE(KEY_F12, LINUX_KEY_F12),
    KEY_CODE(KEY_RO, LINUX_KEY_RO),
    KEY_CODE(KEY_KATAKANA, LINUX_KEY_KATAKANA),
    KEY_CODE(KEY_HIRAGANA, LINUX_KEY_HIRAGANA),
    KEY_CODE(KEY_HENKAN, LINUX_KEY_HENKAN),
    KEY_CODE(KEY_KATAKANAHIRAGANA, LINUX_KEY_KATAKANAHIRAGANA),
    KEY_CODE(KEY_MUHENKAN, LINUX_KEY_MUHENKAN),
    KEY_CODE(KEY_KPJPCOMMA, LINUX_KEY_KPJPCOMMA),
    KEY_CODE(KEY_KPENTER, LINUX_KEY_KPENTER),
    KEY_CODE(KEY_RIGHTCTRL, LINUX_KEY_RIGHTCTRL),
    KEY_CODE(KEY_KPSLASH, LINUX_KEY_KPSLASH),
    KEY_CODE(KEY_SYSRQ, LINUX_KEY_SYSRQ),
    KEY_CODE(KEY_RIGHTALT, LINUX_KEY_RIGHTALT),
    KEY_CODE(KEY_LINEFEED, LINUX_KEY_LINEFEED),
    KEY_CODE(KEY_HOME, LINUX_KEY_HOME),
    KEY_CODE(KEY_UP, LINUX_KEY_UP),
    KEY_CODE(KEY_PAGEUP, LINUX_KEY_PAGEUP),
    KEY_CODE(KEY_LEFT, LINUX_KEY_LEFT),
    KEY_CODE(KEY_RIGHT, LINUX_KEY_RIGHT),
    KEY_CODE(KEY_END, LINUX_KEY_END),
    KEY_CODE(KEY_DOWN, LINUX_KEY_DOWN),
    KEY_CODE(KEY_PAGEDOWN, LINUX_KEY_PAGEDOWN),
    KEY_CODE(KEY_INSERT, LINUX_KEY_INSERT),
    KEY_CODE(KEY_DELETE, LINUX_KEY_DELETE),
    KEY_CODE(KEY_MACRO, LINUX_KEY_MACRO),
    KEY_CODE(KEY_MUTE, LINUX_KEY_MUTE),
    KEY_CODE(KEY_VOLUMEDOWN, LINUX_KEY_VOLUMEDOWN),
    KEY_CODE(KEY_VOLUMEUP, LINUX_KEY_VOLUMEUP),
    KEY_CODE(KEY_POWER, LINUX_KEY_POWER),
    KEY_CODE(KEY_KPEQUAL, LINUX_KEY_KPEQUAL),
    KEY_CODE(KEY_KPPLUSMINUS, LINUX_KEY_KPPLUSMINUS),
    KEY_CODE(KEY_PAUSE, LINUX_KEY_PAUSE),
    KEY_CODE(KEY_KPCOMMA, LINUX_KEY_KPCOMMA),
    KEY_CODE(KEY_HANGEUL, LINUX_KEY_HANGEUL),
    KEY_CODE(KEY_HANJA, LINUX_KEY_HANJA),
    KEY_CODE(KEY_YEN, LINUX_KEY_YEN),
    KEY_CODE(KEY_LEFTMETA, LINUX_KEY_LEFTMETA),
    KEY_CODE(KEY_RIGHTMETA, LINUX_KEY_RIGHTMETA),
    KEY_CODE(KEY_COMPOSE, LINUX_KEY_COMPOSE),
    KEY_CODE(KEY_AGAIN, LINUX_KEY_AGAIN),
    KEY_CODE(KEY_PROPS, LINUX_KEY_PROPS),
    KEY_CODE(KEY_UNDO, LINUX_KEY_UNDO),
    KEY_CODE(KEY_FRONT, LINUX_KEY_FRONT),
    KEY_CODE(KEY_COPY, LINUX_KEY_COPY),
    KEY_CODE(KEY_OPEN, LINUX_KEY_OPEN),
    KEY_CODE(KEY_PASTE, LINUX_KEY_PASTE),
    KEY_CODE(KEY_FIND, LINUX_KEY_FIND),
    KEY_CODE(KEY_CUT, LINUX_KEY_CUT),
    KEY_CODE(KEY_HELP, LINUX_KEY_HELP),
    KEY_CODE(KEY_MENU, LINUX_KEY_MENU),
    KEY_CODE(KEY_CALC, LINUX_KEY_CALC),
    KEY_CODE(KEY_SETUP, LINUX_KEY_SETUP),
    KEY_CODE(KEY_SLEEP, LINUX_KEY_SLEEP),
    KEY_CODE(KEY_WAKEUP, LINUX_KEY_WAKEUP),
    KEY_CODE(KEY_FILE, LINUX_KEY_FILE),
    KEY_CODE(KEY_SENDFILE, LINUX_KEY_SENDFILE),
    KEY_CODE(KEY_DELETEFILE, LINUX_KEY_DELETEFILE),
    KEY_CODE(KEY_XFER, LINUX_KEY_XFER),
    KEY_CODE(KEY_PROG1, LINUX_KEY_PROG1),
    KEY_CODE(KEY_PROG2, LINUX_KEY_PROG2),
    KEY_CODE(KEY_WWW, LINUX_KEY_WWW),
    KEY_CODE(KEY_MSDOS, LINUX_KEY_MSDOS),
    KEY_CODE(KEY_COFFEE, LINUX_KEY_COFFEE),
    KEY_CODE(KEY_DIRECTION, LINUX_KEY_DIRECTION),
    KEY_CODE(KEY_CYCLEWINDOWS, LINUX_KEY_CYCLEWINDOWS),
    KEY_CODE(KEY_MAIL, LINUX_KEY_MAIL),
    KEY_CODE(KEY_BOOKMARKS, LINUX_KEY_BOOKMARKS),
    KEY_CODE(KEY_COMPUTER, LINUX_KEY_COMPUTER),
    KEY_CODE(KEY_BACK, LINUX_KEY_BACK),
    KEY_CODE(KEY_FORWARD, LINUX_KEY_FORWARD),
    KEY_CODE(KEY_CLOSECD, LINUX_KEY_CLOSECD),
    KEY_CODE(KEY_EJECTCD, LINUX_KEY_EJECTCD),
    KEY_CODE(KEY_EJECTCLOSECD, LINUX_KEY_EJECTCLOSECD),
    KEY_CODE(KEY_NEXTSONG, LINUX_KEY_NEXTSONG),
    KEY_CODE(KEY_PLAYPAUSE, LINUX_KEY_PLAYPAUSE),
    KEY_CODE(KEY_PREVIOUSSONG, LINUX_KEY_PREVIOUSSONG),
    KEY_CODE(KEY_STOPCD, LINUX_KEY_STOPCD),
    KEY_CODE(KEY_RECORD, LINUX_KEY_RECORD),
    KEY_CODE(KEY_REWIND, LINUX_KEY_REWIND),
    KEY_CODE(KEY_PHONE, LINUX_KEY_PHONE),
    KEY_CODE(KEY_ISO, LINUX_KEY_ISO),
    KEY_CODE(KEY_CONFIG, LINUX_KEY_CONFIG),
    KEY_CODE(KEY_HOMEPAGE, LINUX_KEY_HOMEPAGE),
    KEY_CODE(KEY_REFRESH, LINUX_KEY_REFRESH),
    KEY_CODE(KEY_EXIT, LINUX_KEY_EXIT),
    KEY_CODE(KEY_MOVE, LINUX_KEY_MOVE),
    KEY_CODE(KEY_EDIT, LINUX_KEY_EDIT),
    KEY_CODE(KEY_SCROLLUP, LINUX_KEY_SCROLLUP),
    KEY_CODE(KEY_SCROLLDOWN, LINUX_KEY_SCROLLDOWN),
    KEY_CODE(KEY_KPLEFTPAREN, LINUX_KEY_KPLEFTPAREN),
    KEY_CODE(KEY_KPRIGHTPAREN, LINUX_KEY_KPRIGHTPAREN),
    KEY_CODE(KEY_NEW, LINUX_KEY_NEW),
    KEY_CODE(KEY_REDO, LINUX_KEY_REDO),
    KEY_CODE(KEY_F13, LINUX_KEY_F13),
    KEY_CODE(KEY_F14, LINUX_KEY_F14),
    KEY_CODE(KEY_F15, LINUX_KEY_F15),
    KEY_CODE(KEY_F16, LINUX_KEY_F16),
    KEY_CODE(KEY_F17, LINUX_KEY_F17),
    KEY_CODE(KEY_F18, LINUX_KEY_F18),
    KEY_CODE(KEY_F19, LINUX_KEY_F19),
    KEY_CODE(KEY_F20, LINUX_KEY_F20),
    KEY_CODE(KEY_F21, LINUX_KEY_F21),
    KEY_CODE(KEY_F22, LINUX_KEY_F22),
    KEY_CODE(KEY_F23, LINUX_KEY_F23),
    KEY_CODE(KEY_F24, LINUX_KEY_F24),
    KEY_CODE(KEY_PLAYCD, LINUX_KEY_PLAYCD),
    KEY_CODE(KEY_PAUSECD, LINUX_KEY_PAUSECD),
    KEY_CODE(KEY_PROG3, LINUX_KEY_PROG3),
    KEY_CODE(KEY_PROG4, LINUX_KEY_PROG4),
    KEY_CODE(KEY_SUSPEND, LINUX_KEY_SUSPEND),
    KEY_CODE(KEY_CLOSE, LINUX_KEY_CLOSE),
    KEY_CODE(KEY_PLAY, LINUX_KEY_PLAY),
    KEY_CODE(KEY_FASTFORWARD, LINUX_KEY_FASTFORWARD),
    KEY_CODE(KEY_BASSBOOST, LINUX_KEY_BASSBOOST),
    KEY_CODE(KEY_PRINT, LINUX_KEY_PRINT),
    KEY_CODE(KEY_HP, LINUX_KEY_HP),
    KEY_CODE(KEY_CAMERA, LINUX_KEY_CAMERA),
    KEY_CODE(KEY_SOUND, LINUX_KEY_SOUND),
    KEY_CODE(KEY_QUESTION, LINUX_KEY_QUESTION),
    KEY_CODE(KEY_EMAIL, LINUX_KEY_EMAIL),
    KEY_CODE(KEY_CHAT, LINUX_KEY_CHAT),
    KEY_CODE(KEY_SEARCH, LINUX_KEY_SEARCH),
    KEY_CODE(KEY_CONNECT, LINUX_KEY_CONNECT),
    KEY_CODE(KEY_FINANCE, LINUX_KEY_FINANCE),
    KEY_CODE(KEY_SPORT, LINUX_KEY_SPORT),
    KEY_CODE(KEY_SHOP, LINUX_KEY_SHOP),
    KEY_CODE(KEY_ALTERASE, LINUX_KEY_ALTERASE),
    KEY_CODE(KEY_CANCEL, LINUX_KEY_CANCEL),
    KEY_CODE(KEY_BRIGHTNESSDOWN, LINUX_KEY_BRIGHTNESSDOWN),
    KEY_CODE(KEY_BRIGHTNESSUP, LINUX_KEY_BRIGHTNESSUP),
    KEY_CODE(KEY_MEDIA, LINUX_KEY_MEDIA),
    KEY_CODE(KEY_STAR, LINUX_KEY_STAR),
    KEY_CODE(KEY_SHARP, LINUX_KEY_SHARP),
    KEY_CODE(KEY_SOFT1, LINUX_KEY_SOFT1),
    KEY_CODE(KEY_SOFT2, LINUX_KEY_SOFT2),
    KEY_CODE(KEY_CENTER, LINUX_KEY_CENTER),
    KEY_CODE(KEY_HEADSETHOOK, LINUX_KEY_HEADSETHOOK),
    KEY_CODE(KEY_0_5, LINUX_KEY_0_5),
    KEY_CODE(KEY_2_5, LINUX_KEY_2_5),
    KEY_CODE(KEY_SWITCHVIDEOMODE, LINUX_KEY_SWITCHVIDEOMODE),
    KEY_CODE(KEY_KBDILLUMTOGGLE, LINUX_KEY_KBDILLUMTOGGLE),
    KEY_CODE(KEY_KBDILLUMDOWN, LINUX_KEY_KBDILLUMDOWN),
    KEY_CODE(KEY_KBDILLUMUP, LINUX_KEY_KBDILLUMUP),
    KEY_CODE(KEY_SEND, LINUX_KEY_SEND),
    KEY_CODE(KEY_REPLY, LINUX_KEY_REPLY),
    KEY_CODE(KEY_FORWARDMAIL, LINUX_KEY_FORWARDMAIL),
    KEY_CODE(KEY_SAVE, LINUX_KEY_SAVE),
    KEY_CODE(KEY_DOCUMENTS, LINUX_KEY_DOCUMENTS),
    KEY_CODE(KEY_BATTERY, LINUX_KEY_BATTERY),
    KEY_CODE(KEY_UNKNOWN, LINUX_KEY_UNKNOWN),
    KEY_CODE(KEY_NUM, LINUX_KEY_NUM),
    KEY_CODE(KEY_FOCUS, LINUX_KEY_FOCUS),
    KEY_CODE(KEY_PLUS, LINUX_KEY_PLUS),
    KEY_CODE(KEY_NOTIFICATION, LINUX_KEY_NOTIFICATION),

    KEY_CODE(KEY_APPSWITCH, ANDROID_KEY_APPSWITCH),
    KEY_CODE(KEY_STEM_PRIMARY, ANDROID_KEY_STEM_PRIMARY),
    KEY_CODE(KEY_STEM_1, ANDROID_KEY_STEM_1),
    KEY_CODE(KEY_STEM_2, ANDROID_KEY_STEM_2),
    KEY_CODE(KEY_STEM_3, ANDROID_KEY_STEM_3),

    BTN_CODE(BTN_MISC),
    BTN_CODE(BTN_0),
    BTN_CODE(BTN_1),
    BTN_CODE(BTN_2),
    BTN_CODE(BTN_3),
    BTN_CODE(BTN_4),
    BTN_CODE(BTN_5),
    BTN_CODE(BTN_6),
    BTN_CODE(BTN_7),
    BTN_CODE(BTN_8),
    BTN_CODE(BTN_9),
    BTN_CODE(BTN_MOUSE),
    BTN_CODE(BTN_LEFT),
    BTN_CODE(BTN_RIGHT),
    BTN_CODE(BTN_MIDDLE),
    BTN_CODE(BTN_SIDE),
    BTN_CODE(BTN_EXTRA),
    BTN_CODE(BTN_FORWARD),
    BTN_CODE(BTN_BACK),
    BTN_CODE(BTN_TASK),
    BTN_CODE(BTN_JOYSTICK),
    BTN_CODE(BTN_TRIGGER),
    BTN_CODE(BTN_THUMB),
    BTN_CODE(BTN_THUMB2),
    BTN_CODE(BTN_TOP),
    BTN_CODE(BTN_TOP2),
    BTN_CODE(BTN_PINKIE),
    BTN_CODE(BTN_BASE),
    BTN_CODE(BTN_BASE2),
    BTN_CODE(BTN_BASE3),
    BTN_CODE(BTN_BASE4),
    BTN_CODE(BTN_BASE5),
    BTN_CODE(BTN_BASE6),
    BTN_CODE(BTN_DEAD),
    BTN_CODE(BTN_GAMEPAD),
    BTN_CODE(BTN_A),
    BTN_CODE(BTN_B),
    BTN_CODE(BTN_C),
    BTN_CODE(BTN_X),
    BTN_CODE(BTN_Y),
    BTN_CODE(BTN_Z),
    BTN_CODE(BTN_TL),
    BTN_CODE(BTN_TR),
    BTN_CODE(BTN_TL2),
    BTN_CODE(BTN_TR2),
    BTN_CODE(BTN_SELECT),
    BTN_CODE(BTN_START),
    BTN_CODE(BTN_MODE),
    BTN_CODE(BTN_THUMBL),
    BTN_CODE(BTN_THUMBR),
    BTN_CODE(BTN_DIGI),
    BTN_CODE(BTN_TOOL_PEN),
    BTN_CODE(BTN_TOOL_RUBBER),
    BTN_CODE(BTN_TOOL_BRUSH),
    BTN_CODE(BTN_TOOL_PENCIL),
    BTN_CODE(BTN_TOOL_AIRBRUSH),
    BTN_CODE(BTN_TOOL_FINGER),
    BTN_CODE(BTN_TOOL_MOUSE),
    BTN_CODE(BTN_TOOL_LENS),
    BTN_CODE(BTN_TOUCH),
    BTN_CODE(BTN_STYLUS),
    BTN_CODE(BTN_STYLUS2),
    BTN_CODE(BTN_TOOL_DOUBLETAP),
    BTN_CODE(BTN_TOOL_TRIPLETAP),
    BTN_CODE(BTN_WHEEL),
    BTN_CODE(BTN_GEAR_DOWN),
    BTN_CODE(BTN_GEAR_UP),
    EV_CODE_END,
};

typedef struct {
    const char *name;
    int value;
    const GoldfishEventCodeInfo *codes;
} GoldfishEventTypeInfo;

static const GoldfishEventTypeInfo ev_type_table[] = {
    EV_TYPE(EV_SYN, NULL),
    EV_TYPE(EV_KEY, ev_key_codes_table),
    EV_TYPE(EV_REL, ev_rel_codes_table),
    EV_TYPE(EV_ABS, ev_abs_codes_table),
    EV_TYPE(EV_MSC, NULL),
    EV_TYPE(EV_SW,  ev_sw_codes_table),
    EV_TYPE(EV_LED, NULL),
    EV_TYPE(EV_SND, NULL),
    EV_TYPE(EV_REP, NULL),
    EV_TYPE(EV_FF, NULL),
    EV_TYPE(EV_PWR, NULL),
    EV_TYPE(EV_FF_STATUS, NULL),
    EV_TYPE(EV_MAX, NULL),
    EV_TYPE_END
};

/* NOTE: The ev_bits arrays are used to indicate to the kernel
 *       which events can be sent by the emulated hardware.
 */

#define TYPE_GOLDFISHEVDEV "goldfish-events"

/* Bitfield meanings for modifier_state. */
#define MODSTATE_SHIFT (1 << 0)
#define MODSTATE_CTRL (1 << 1)
#define MODSTATE_ALT (1 << 2)
#define MODSTATE_MASK (MODSTATE_SHIFT | MODSTATE_CTRL | MODSTATE_ALT)

/* An entry in the array of ABS_XXX values */
typedef struct ABSEntry {
    /* Minimum ABS_XXX value. */
    uint32_t    min;
    /* Maximum ABS_XXX value. */
    uint32_t    max;
    /* 'fuzz;, and 'flat' ABS_XXX values are always zero here. */
    uint32_t    fuzz;
    uint32_t    flat;
} ABSEntry;

/* Pointer to the global device instance. Also serves as an initialization
 * flag in goldfish_event_send() to filter-out events that are sent from
 * the UI before the device was properly realized.
 */
static GoldfishEvDevState* s_evdev = NULL;

static const GoldfishEventMultitouchFuncs* s_multitouch_funcs = NULL;

void goldfish_events_enable_multitouch(
        const GoldfishEventMultitouchFuncs* funcs) {
    s_multitouch_funcs = funcs;
}

static const VMStateDescription vmstate_goldfish_evdev =
        GOLDFISHEVDEV_VM_STATE_DESCRIPTION("goldfish-events");

int goldfish_event_send(int type, int code, int value)
{
    GoldfishEvDevState *dev = s_evdev;

    if (dev) {
        goldfish_enqueue_event(dev, type, code, value);
    }

    return 0;
}

static const MemoryRegionOps goldfish_evdev_ops = {
    .read = goldfish_events_read,
    .write = goldfish_events_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void goldfish_evdev_put_mouse(void *opaque,
                               int dx, int dy, int dz, int buttons_state)
{
    GoldfishEvDevState *s = (GoldfishEvDevState *)opaque;

    /* Note that, like the "classic" Android emulator, we
     * have dz == 0 for touchscreen, == 1 for trackball
     */
    if (s->have_multitouch  &&  dz == 0) {
        if (s_multitouch_funcs) {
            s_multitouch_funcs->translate_mouse_event(dx, dy, buttons_state);
            return;
        }
    }

    if (s->have_touch  &&  dz == 0) {
        goldfish_enqueue_event(s, EV_ABS, ABS_X, dx);
        goldfish_enqueue_event(s, EV_ABS, ABS_Y, dy);
        goldfish_enqueue_event(s, EV_ABS, ABS_Z, dz);
        goldfish_enqueue_event(s, EV_KEY, BTN_TOUCH, buttons_state & 1);
        goldfish_enqueue_event(s, EV_SYN, 0, 0);
        return;
    }
    if (s->have_trackball  &&  dz == 1) {
        goldfish_enqueue_event(s, EV_REL, REL_X, dx);
        goldfish_enqueue_event(s, EV_REL, REL_Y, dy);
        goldfish_enqueue_event(s, EV_SYN, 0, 0);
        return;
    }
}

static const int dpad_map[Q_KEY_CODE__MAX] = {
    [Q_KEY_CODE_KP_4] = LINUX_KEY_LEFT,
    [Q_KEY_CODE_KP_6] = LINUX_KEY_RIGHT,
    [Q_KEY_CODE_KP_8] = LINUX_KEY_UP,
    [Q_KEY_CODE_KP_2] = LINUX_KEY_DOWN,
    [Q_KEY_CODE_KP_5] = LINUX_KEY_CENTER,
};

static void goldfish_evdev_handle_keyevent(DeviceState *dev, QemuConsole *src,
                                           InputEvent *evt)
{
    GoldfishEvDevState *s = GOLDFISHEVDEV(dev, TYPE_GOLDFISHEVDEV);
    int lkey = 0;
    int mod;

    assert(evt->type == INPUT_EVENT_KIND_KEY);

    KeyValue* kv = evt->u.key.data->key;

    int qcode = kv->u.qcode.data;

    goldfish_enqueue_event(s, EV_KEY, qcode, evt->u.key.data->down);

    int qemu2_qcode = qemu_input_key_value_to_qcode(evt->u.key.data->key);

    /* Keep our modifier state up to date */
    switch (qemu2_qcode) {
    case Q_KEY_CODE_SHIFT:
    case Q_KEY_CODE_SHIFT_R:
        mod = MODSTATE_SHIFT;
        break;
    case Q_KEY_CODE_ALT:
    case Q_KEY_CODE_ALT_R:
        mod = MODSTATE_ALT;
        break;
    case Q_KEY_CODE_CTRL:
    case Q_KEY_CODE_CTRL_R:
        mod = MODSTATE_CTRL;
        break;
    default:
        mod = 0;
        break;
    }

    if (mod) {
        if (evt->u.key.data->down) {
            s->modifier_state |= mod;
        } else {
            s->modifier_state &= ~mod;
        }
    }

    if (!lkey && s->have_dpad && s->modifier_state == 0) {
        lkey = dpad_map[qemu2_qcode];
    }

    if (lkey) {
        goldfish_enqueue_event(s, EV_KEY, lkey, evt->u.key.data->down);
    }
}

static const GoldfishEventTypeInfo *goldfish_get_event_type(const char *typename)
{
    const GoldfishEventTypeInfo *type = NULL;
    int count = 0;

    /* Find the type descriptor by doing a name match */
    while (ev_type_table[count].name != NULL) {
        if (typename && !strcmp(ev_type_table[count].name, typename)) {
            type = &ev_type_table[count];
            break;
        }
        count++;
    }

    return type;
}

int goldfish_get_event_type_count(void)
{
    int count = 0;

    while (ev_type_table[count].name != NULL) {
        count++;
    }

    return count;
}

int goldfish_get_event_type_name(int type, char *buf)
{
    g_stpcpy(buf, ev_type_table[type].name);

    return 0;
}

int goldfish_get_event_type_value(char *typename)
{
    const GoldfishEventTypeInfo *type = goldfish_get_event_type(typename);
    int ret = -1;

    if (type) {
        ret = type->value;
    }

    return ret;
}

static const GoldfishEventCodeInfo *goldfish_get_event_code(int typeval,
                                                      const char *codename)
{
    const GoldfishEventTypeInfo *type = &ev_type_table[typeval];
    const GoldfishEventCodeInfo *code = NULL;
    int count = 0;

    /* Find the type descriptor by doing a name match */
    while (type->codes[count].name != NULL) {
        if (codename && !strcmp(type->codes[count].name, codename)) {
            code = &type->codes[count];
            break;
        }
        count++;
    }

    return code;
}

int goldfish_get_event_code_count(const char *typename)
{
    const GoldfishEventTypeInfo *type = NULL;
    const GoldfishEventCodeInfo *codes;
    int count = -1;     /* Return -1 if type not found */

    type = goldfish_get_event_type(typename);

    /* Count the number of codes for the specified type if found */
    if (type) {
        count = 0;
        codes = type->codes;
        if (codes) {
            while (codes[count].name != NULL) {
                count++;
            }
        }
    }

    return count;
}

int goldfish_get_event_code_name(const char *typename, unsigned int code, char *buf)
{
    const GoldfishEventTypeInfo *type = goldfish_get_event_type(typename);
    int ret = -1;   /* Return -1 if type not found */

    if (type && type->codes && code < goldfish_get_event_code_count(typename)) {
        g_stpcpy(buf, type->codes[code].name);
        ret = 0;
    }

    return ret;
}

int goldfish_get_event_code_value(int typeval, char *codename)
{
    const GoldfishEventCodeInfo *code = goldfish_get_event_code(typeval, codename);
    int ret = -1;

    if (code) {
        ret = code->value;
    }

    return ret;
}

static QemuInputHandler goldfish_evdev_key_input_handler = {
    .name = "goldfish event device key handler",
    .mask = INPUT_EVENT_MASK_KEY,
    .event = goldfish_evdev_handle_keyevent,
};

static void goldfish_evdev_init(Object *obj)
{
    GoldfishEvDevState *s = GOLDFISHEVDEV(obj, TYPE_GOLDFISHEVDEV);
    DeviceState *dev = DEVICE(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &goldfish_evdev_ops, s,
                          "goldfish-events", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    qemu_input_handler_register(dev, &goldfish_evdev_key_input_handler);
    // Register the mouse handler for both absolute and relative position
    // reports. (Relative reports are used in trackball mode.)
    qemu_add_mouse_event_handler(goldfish_evdev_put_mouse, s, 1, "goldfish-events");
    qemu_add_mouse_event_handler(goldfish_evdev_put_mouse, s, 0, "goldfish-events-rel");
}

static void goldfish_evdev_realize(DeviceState *dev, Error **errp)
{

    GoldfishEvDevState *s = GOLDFISHEVDEV(dev, TYPE_GOLDFISHEVDEV);

    /* Initialize the device ID so the event dev can be looked up duringi
     * monitor commands.
     */
    dev->id = g_strdup(TYPE_GOLDFISHEVDEV);

    /* now set the events capability bits depending on hardware configuration */
    /* apparently, the EV_SYN array is used to indicate which other
     * event classes to consider.
     */

    /* XXX PMM properties ? */
    s->name = "qwerty2";

    /* configure EV_KEY array
     *
     * All Android devices must have the following keys:
     *   KEY_HOME, KEY_BACK, KEY_SEND (Call), KEY_END (EndCall),
     *   KEY_SOFT1 (Menu), VOLUME_UP, VOLUME_DOWN
     *
     *   Note that previous models also had a KEY_SOFT2,
     *   and a KEY_POWER  which we still support here.
     *
     *   Newer models have a KEY_SEARCH key, which we always
     *   enable here.
     *
     * A Dpad will send: KEY_DOWN / UP / LEFT / RIGHT / CENTER
     *
     * The KEY_CAMERA button isn't very useful if there is no camera.
     *
     * BTN_MOUSE is sent when the trackball is pressed
     * BTN_TOUCH is sent when the touchscreen is pressed
     */
    goldfish_events_set_bit(s, EV_SYN, EV_KEY);

    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_HOME);
    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_BACK);
    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_SEND);
    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_END);
    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_SOFT1);
    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_VOLUMEUP);
    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_VOLUMEDOWN);
    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_SOFT2);
    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_POWER);
    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_SEARCH);
    goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_SLEEP);

    goldfish_events_set_bit(s, EV_KEY, ANDROID_KEY_APPSWITCH);
    goldfish_events_set_bit(s, EV_KEY, ANDROID_KEY_STEM_PRIMARY);
    goldfish_events_set_bit(s, EV_KEY, ANDROID_KEY_STEM_1);
    goldfish_events_set_bit(s, EV_KEY, ANDROID_KEY_STEM_2);
    goldfish_events_set_bit(s, EV_KEY, ANDROID_KEY_STEM_3);

    if (s->have_dpad) {
        goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_DOWN);
        goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_UP);
        goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_LEFT);
        goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_RIGHT);
        goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_CENTER);
    }

    if (s->have_trackball) {
        goldfish_events_set_bit(s, EV_KEY, BTN_MOUSE);
    }
    if (s->have_touch) {
        goldfish_events_set_bit(s, EV_KEY, BTN_TOUCH);
    }

    if (s->have_camera) {
        /* Camera emulation is enabled. */
        goldfish_events_set_bit(s, EV_KEY, LINUX_KEY_CAMERA);
    }

    if (s->have_keyboard) {
        /* since we want to implement Unicode reverse-mapping
         * allow any kind of key, even those not available on
         * the skin.
         *
         * the previous code did set the [1..0x1ff] range, but
         * we don't want to enable certain bits in the middle
         * of the range that are registered for mouse/trackball/joystick
         * events.
         *
         * see "linux_keycodes.h" for the list of events codes.
         */
        goldfish_events_set_bits(s, EV_KEY, 1, 0xff);
        goldfish_events_set_bits(s, EV_KEY, 0x160, 0x1ff);

        /* If there is a keyboard, but no DPad, we need to clear the
         * corresponding bit. Doing this is simpler than trying to exclude
         * the DPad values from the ranges above.
         *
         * LINUX_KEY_UP/DOWN/LEFT/RIGHT should be left set so the keyboard
         * arrow keys are still usable, even though a typical device hard
         * keyboard would not include those keys.
         */
        if (!s->have_dpad) {
            goldfish_events_clr_bit(s, EV_KEY, LINUX_KEY_CENTER);
        }
    }

    /* configure EV_REL array
     *
     * EV_REL events are sent when the trackball is moved
     */
    if (s->have_trackball) {
        goldfish_events_set_bit(s, EV_SYN, EV_REL);
        goldfish_events_set_bits(s, EV_REL, REL_X, REL_Y);
    }

    /* configure EV_ABS array.
     *
     * EV_ABS events are sent when the touchscreen is pressed
     */
    if (s->have_touch || s->have_multitouch) {
        ABSEntry *abs_values;

        goldfish_events_set_bit(s, EV_SYN, EV_ABS);
        goldfish_events_set_bits(s, EV_ABS, ABS_X, ABS_Z);
        /* Allocate the absinfo to report the min/max bounds for each
         * absolute dimension. The array must contain 3, or ABS_MAX tuples
         * of (min,max,fuzz,flat) 32-bit values.
         *
         * min and max are the bounds
         * fuzz corresponds to the device's fuziness, we set it to 0
         * flat corresponds to the flat position for JOEYDEV devices,
         * we also set it to 0.
         *
         * There is no need to save/restore this array in a snapshot
         * since the values only depend on the hardware configuration.
         */
        s->abs_info_count = s->have_multitouch ? ABS_MAX * 4 : 3 * 4;
        s->abs_info = g_new0(int32_t, s->abs_info_count);
        abs_values = (ABSEntry *)s->abs_info;

        /* QEMU provides absolute coordinates in the [0,0x7fff] range
         * regardless of the display resolution.
         */
        abs_values[ABS_X].max = INPUT_EVENT_ABS_MAX;
        abs_values[ABS_Y].max = INPUT_EVENT_ABS_MAX;
        abs_values[ABS_Z].max = 1;

        if (s->have_multitouch) {
            /*
             * Setup multitouch.
             */
            goldfish_events_set_bit(s, EV_ABS, ABS_MT_SLOT);
            goldfish_events_set_bit(s, EV_ABS, ABS_MT_TOUCH_MAJOR);
            goldfish_events_set_bit(s, EV_ABS, ABS_MT_TOUCH_MINOR);
            goldfish_events_set_bit(s, EV_ABS, ABS_MT_ORIENTATION);
            goldfish_events_set_bit(s, EV_ABS, ABS_MT_POSITION_X);
            goldfish_events_set_bit(s, EV_ABS, ABS_MT_POSITION_Y);
            goldfish_events_set_bit(s, EV_ABS, ABS_MT_TOOL_TYPE);
            goldfish_events_set_bit(s, EV_ABS, ABS_MT_TRACKING_ID);
            goldfish_events_set_bit(s, EV_ABS, ABS_MT_PRESSURE);

            goldfish_events_set_bit(s, EV_SYN, EV_KEY);
            goldfish_events_set_bit(s, EV_KEY, BTN_TOOL_RUBBER);
            goldfish_events_set_bit(s, EV_KEY, BTN_STYLUS);

            if (s_multitouch_funcs) {
                abs_values[ABS_MT_SLOT].max =
                                        s_multitouch_funcs->get_max_slot();
                abs_values[ABS_MT_TOUCH_MAJOR].max = MTS_TOUCH_AXIS_RANGE_MAX;
                abs_values[ABS_MT_TOUCH_MINOR].max = MTS_TOUCH_AXIS_RANGE_MAX;
                abs_values[ABS_MT_ORIENTATION].max = MTS_ORIENTATION_RANGE_MAX;
                abs_values[ABS_MT_POSITION_X].max  = abs_values[ABS_X].max;
                abs_values[ABS_MT_POSITION_Y].max  = abs_values[ABS_Y].max;
                abs_values[ABS_MT_TOOL_TYPE].max   = MT_TOOL_MAX;
                abs_values[ABS_MT_TRACKING_ID].max =
                                            abs_values[ABS_MT_SLOT].max + 1;
                abs_values[ABS_MT_PRESSURE].max    = MTS_PRESSURE_RANGE_MAX;
            }
        }
    }

    /* configure EV_SW array
     *
     * EV_SW events are sent to indicate that headphones
     * were pluged or unplugged.
     * If hw.keyboard.lid is true, EV_SW events are also
     * used to indicate if the keyboard lid was opened or
     * closed (done when we switch layouts through KP-7
     * or KP-9).
     */
    goldfish_events_set_bit(s, EV_SYN, EV_SW);
    goldfish_events_set_bit(s, EV_SW, SW_HEADPHONE_INSERT);
    goldfish_events_set_bit(s, EV_SW, SW_MICROPHONE_INSERT);

    if (s->have_tablet_mode) {
        goldfish_events_set_bit(s, EV_SW, SW_TABLET_MODE);
    }

    if (s->have_keyboard_lid) {
        goldfish_events_set_bit(s, EV_SW, SW_LID);
    }

#ifdef _WIN32
    InitializeSRWLock(&s->lock);
#else
    pthread_mutex_init(&s->lock, 0);
#endif

    s->measure_latency = false;
    {
        char* android_emu_trace_env_var =
            getenv("ANDROID_EMU_TRACING");
        s->measure_latency =
            android_emu_trace_env_var &&
            !strcmp("1", android_emu_trace_env_var);
    }

    /* Register global variable. */
    assert(s_evdev == NULL);
    assert(s->state == 0);
    s_evdev = s;
}

static void goldfish_evdev_reset(DeviceState *dev)
{
    GoldfishEvDevState *s = GOLDFISHEVDEV(dev, TYPE_GOLDFISHEVDEV);

    s->first = 0;
    s->last = 0;
}

static Property goldfish_evdev_props[] = {
    DEFINE_PROP_BOOL("have-dpad", GoldfishEvDevState, have_dpad, false),
    DEFINE_PROP_BOOL("have-trackball", GoldfishEvDevState,
                     have_trackball, false),
    DEFINE_PROP_BOOL("have-camera", GoldfishEvDevState, have_camera, false),
    DEFINE_PROP_BOOL("have-keyboard", GoldfishEvDevState, have_keyboard, false),
    DEFINE_PROP_BOOL("have-lidswitch", GoldfishEvDevState,
                     have_keyboard_lid, false),
    DEFINE_PROP_BOOL("have-tabletmode", GoldfishEvDevState,
                     have_tablet_mode, false),
    DEFINE_PROP_BOOL("have-touch", GoldfishEvDevState,
                     have_touch, false),
    DEFINE_PROP_BOOL("have-multitouch", GoldfishEvDevState,
                     have_multitouch, true),
    DEFINE_PROP_END_OF_LIST()
};

static void goldfish_evdev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_evdev_realize;
    dc->reset = goldfish_evdev_reset;
    dc->props = goldfish_evdev_props;
    dc->vmsd = &vmstate_goldfish_evdev;
}

static const TypeInfo goldfish_evdev_info = {
    .name = TYPE_GOLDFISHEVDEV,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(GoldfishEvDevState),
    .instance_init = goldfish_evdev_init,
    .class_init = goldfish_evdev_class_init,
};

static void goldfish_evdev_register_types(void)
{
    type_register_static(&goldfish_evdev_info);
}

type_init(goldfish_evdev_register_types)
