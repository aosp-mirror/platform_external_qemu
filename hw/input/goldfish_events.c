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

#include "hw/input/goldfish_events.h"

#define MAX_EVENTS (256 * 4)

typedef struct {
    const char *name;
    int value;
} GoldfishEventCodeInfo;

/* Absolute axes */
#define ABS_X                   0x00
#define ABS_Y                   0x01
#define ABS_Z                   0x02
#define ABS_MT_SLOT             0x2f    /* MT slot being modified */
#define ABS_MT_TOUCH_MAJOR      0x30    /* Major axis of touching ellipse */
#define ABS_MT_TOUCH_MINOR      0x31    /* Minor axis (omit if circular) */
#define ABS_MT_WIDTH_MAJOR      0x32    /* Major axis of approaching ellipse */
#define ABS_MT_WIDTH_MINOR      0x33    /* Minor axis (omit if circular) */
#define ABS_MT_ORIENTATION      0x34    /* Ellipse orientation */
#define ABS_MT_POSITION_X       0x35    /* Center X ellipse position */
#define ABS_MT_POSITION_Y       0x36    /* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE        0x37    /* Type of touching device */
#define ABS_MT_BLOB_ID          0x38    /* Group a set of packets as a blob */
#define ABS_MT_TRACKING_ID      0x39    /* Unique ID of initiated contact */
#define ABS_MT_PRESSURE         0x3a    /* Pressure on contact area */
#define ABS_MT_DISTANCE         0x3b    /* Contact hover distance */
#define ABS_MAX                 0x3f

#define BTN_TOUCH 0x14a
#define BTN_MOUSE 0x110

#define EV_CODE(_code)    {#_code, (_code)}
#define EV_CODE_END     {NULL, 0}
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

/* Relative axes */
#define REL_X                   0x00
#define REL_Y                   0x01
static const GoldfishEventCodeInfo ev_rel_codes_table[] = {
    EV_CODE(REL_X),
    EV_CODE(REL_Y),
    EV_CODE_END,
};

/* Switches */
#define SW_LID               0
#define SW_HEADPHONE_INSERT  2
#define SW_MICROPHONE_INSERT 4
static const GoldfishEventCodeInfo ev_sw_codes_table[] = {
    EV_CODE(SW_LID),
    EV_CODE(SW_HEADPHONE_INSERT),
    EV_CODE(SW_MICROPHONE_INSERT),
    EV_CODE_END,
};

#define BTN_MISC 0x100
#define BTN_0 0x100
#define BTN_1 0x101
#define BTN_2 0x102
#define BTN_3 0x103
#define BTN_4 0x104
#define BTN_5 0x105
#define BTN_6 0x106
#define BTN_7 0x107
#define BTN_8 0x108
#define BTN_9 0x109
#define BTN_MOUSE 0x110
#define BTN_LEFT 0x110
#define BTN_RIGHT 0x111
#define BTN_MIDDLE 0x112
#define BTN_SIDE 0x113
#define BTN_EXTRA 0x114
#define BTN_FORWARD 0x115
#define BTN_BACK 0x116
#define BTN_TASK 0x117
#define BTN_JOYSTICK 0x120
#define BTN_TRIGGER 0x120
#define BTN_THUMB 0x121
#define BTN_THUMB2 0x122
#define BTN_TOP 0x123
#define BTN_TOP2 0x124
#define BTN_PINKIE 0x125
#define BTN_BASE 0x126
#define BTN_BASE2 0x127
#define BTN_BASE3 0x128
#define BTN_BASE4 0x129
#define BTN_BASE5 0x12a
#define BTN_BASE6 0x12b
#define BTN_DEAD 0x12f
#define BTN_GAMEPAD 0x130
#define BTN_A 0x130
#define BTN_B 0x131
#define BTN_C 0x132
#define BTN_X 0x133
#define BTN_Y 0x134
#define BTN_Z 0x135
#define BTN_TL 0x136
#define BTN_TR 0x137
#define BTN_TL2 0x138
#define BTN_TR2 0x139
#define BTN_SELECT 0x13a
#define BTN_START 0x13b
#define BTN_MODE 0x13c
#define BTN_THUMBL 0x13d
#define BTN_THUMBR 0x13e
#define BTN_DIGI 0x140
#define BTN_TOOL_PEN 0x140
#define BTN_TOOL_RUBBER 0x141
#define BTN_TOOL_BRUSH 0x142
#define BTN_TOOL_PENCIL 0x143
#define BTN_TOOL_AIRBRUSH 0x144
#define BTN_TOOL_FINGER 0x145
#define BTN_TOOL_MOUSE 0x146
#define BTN_TOOL_LENS 0x147
#define BTN_TOUCH 0x14a
#define BTN_STYLUS 0x14b
#define BTN_STYLUS2 0x14c
#define BTN_TOOL_DOUBLETAP 0x14d
#define BTN_TOOL_TRIPLETAP 0x14e
#define BTN_WHEEL 0x150
#define BTN_GEAR_DOWN 0x150
#define BTN_GEAR_UP  0x151

#define KEY_CODE(_name, _val) {#_name, _val}
#define BTN_CODE(_code) {#_code, (_code)}
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

/* Event types (as per Linux input event layer) */
#define EV_SYN                  0x00
#define EV_KEY                  0x01
#define EV_REL                  0x02
#define EV_ABS                  0x03
#define EV_MSC                  0x04
#define EV_SW                   0x05
#define EV_LED                  0x11
#define EV_SND                  0x12
#define EV_REP                  0x14
#define EV_FF                   0x15
#define EV_PWR                  0x16
#define EV_FF_STATUS            0x17
#define EV_MAX                  0x1f

#define EV_TYPE(_type, _codes)    {#_type, (_type), _codes}
#define EV_TYPE_END     {NULL, 0}
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

enum {
    REG_READ        = 0x00,
    REG_SET_PAGE    = 0x00,
    REG_LEN         = 0x04,
    REG_DATA        = 0x08,

    PAGE_NAME       = 0x00000,
    PAGE_EVBITS     = 0x10000,
    PAGE_ABSDATA    = 0x20000 | EV_ABS,
};

/* These corresponds to the state of the driver.
 * Unfortunately, we have to buffer events coming
 * from the UI, since the kernel driver is not
 * capable of receiving them until XXXXXX
 */
enum {
    STATE_INIT = 0,  /* The device is initialized */
    STATE_BUFFERED,  /* Events have been buffered, but no IRQ raised yet */
    STATE_LIVE       /* Events can be sent directly to the kernel */
};

/* NOTE: The ev_bits arrays are used to indicate to the kernel
 *       which events can be sent by the emulated hardware.
 */

#define TYPE_GOLDFISHEVDEV "goldfish-events"
#define GOLDFISHEVDEV(obj) \
    OBJECT_CHECK(GoldfishEvDevState, (obj), TYPE_GOLDFISHEVDEV)

typedef struct GoldfishEvDevState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    MemoryRegion iomem;
    qemu_irq irq;

    /* Device properties */
    bool have_dpad;
    bool have_trackball;
    bool have_camera;
    bool have_keyboard;
    bool have_keyboard_lid;
    bool have_touch;
    bool have_multitouch;

    /* Actual device state */
    int32_t page;
    uint32_t events[MAX_EVENTS];
    uint32_t first;
    uint32_t last;
    uint32_t state;

    uint32_t modifier_state;

    /* All data below here is set up at realize and not modified thereafter */

    const char *name;

    struct {
        size_t   len;
        uint8_t *bits;
    } ev_bits[EV_MAX + 1];

    int32_t *abs_info;
    size_t abs_info_count;
} GoldfishEvDevState;

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

static const VMStateDescription vmstate_goldfish_evdev = {
    .name = "goldfish-events",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_INT32(page, GoldfishEvDevState),
        VMSTATE_UINT32_ARRAY(events, GoldfishEvDevState, MAX_EVENTS),
        VMSTATE_UINT32(first, GoldfishEvDevState),
        VMSTATE_UINT32(last, GoldfishEvDevState),
        VMSTATE_UINT32(state, GoldfishEvDevState),
        VMSTATE_UINT32(modifier_state, GoldfishEvDevState),
        VMSTATE_END_OF_LIST()
    }
};

static void enqueue_event(GoldfishEvDevState *s,
                          unsigned int type, unsigned int code, int value)
{
    int  enqueued = s->last - s->first;

    if (enqueued < 0) {
        enqueued += MAX_EVENTS;
    }

    if (enqueued + 3 > MAX_EVENTS) {
        fprintf(stderr, "##KBD: Full queue, lose event\n");
        return;
    }

    if (s->first == s->last) {
        if (s->state == STATE_LIVE) {
            qemu_irq_raise(s->irq);
        } else {
            s->state = STATE_BUFFERED;
        }
    }

    s->events[s->last] = type;
    s->last = (s->last + 1) & (MAX_EVENTS-1);
    s->events[s->last] = code;
    s->last = (s->last + 1) & (MAX_EVENTS-1);
    s->events[s->last] = value;
    s->last = (s->last + 1) & (MAX_EVENTS-1);

}

static unsigned dequeue_event(GoldfishEvDevState *s)
{
    unsigned n;

    if (s->first == s->last) {
        return 0;
    }

    n = s->events[s->first];

    s->first = (s->first + 1) & (MAX_EVENTS - 1);

    if (s->first == s->last) {
        qemu_irq_lower(s->irq);
    }
#ifdef TARGET_I386
    /*
     * Adding the logic to handle edge-triggered interrupts for x86
     * because the exisiting goldfish events device basically provides
     * level-trigger interrupts only.
     *
     * Logic: When an event (including the type/code/value) is fetched
     * by the driver, if there is still another event in the event
     * queue, the goldfish event device will re-assert the IRQ so that
     * the driver can be notified to fetch the event again.
     */
    else if (((s->first + 2) & (MAX_EVENTS - 1)) < s->last ||
               (s->first & (MAX_EVENTS - 1)) > s->last) {
        /* if there still is an event */
        qemu_irq_lower(s->irq);
        qemu_irq_raise(s->irq);
    }
#endif
    return n;
}

static int get_page_len(GoldfishEvDevState *s)
{
    int page = s->page;
    if (page == PAGE_NAME) {
        const char *name = s->name;
        return strlen(name);
    }
    if (page >= PAGE_EVBITS && page <= PAGE_EVBITS + EV_MAX) {
        return s->ev_bits[page - PAGE_EVBITS].len;
    }
    if (page == PAGE_ABSDATA) {
        return s->abs_info_count * sizeof(s->abs_info[0]);
    }
    return 0;
}

static int get_page_data(GoldfishEvDevState *s, int offset)
{
    int page_len = get_page_len(s);
    int page = s->page;
    if (offset > page_len) {
        return 0;
    }
    if (page == PAGE_NAME) {
        const char *name = s->name;
        return name[offset];
    }
    if (page >= PAGE_EVBITS && page <= PAGE_EVBITS + EV_MAX) {
        return s->ev_bits[page - PAGE_EVBITS].bits[offset];
    }
    if (page == PAGE_ABSDATA) {
        return s->abs_info[offset / sizeof(s->abs_info[0])];
    }
    return 0;
}

int goldfish_event_send(int type, int code, int value)
{
    GoldfishEvDevState *dev = s_evdev;

    if (dev) {
        enqueue_event(dev, type, code, value);
    }

    return 0;
}

static uint64_t events_read(void *opaque, hwaddr offset, unsigned size)
{
    GoldfishEvDevState *s = (GoldfishEvDevState *)opaque;

    /* This gross hack below is used to ensure that we
     * only raise the IRQ when the kernel driver is
     * properly ready! If done before this, the driver
     * becomes confused and ignores all input events
     * as soon as one was buffered!
     */
    if (offset == REG_LEN && s->page == PAGE_ABSDATA) {
        if (s->state == STATE_BUFFERED) {
            qemu_irq_raise(s->irq);
        }
        s->state = STATE_LIVE;
    }

    switch (offset) {
    case REG_READ:
        return dequeue_event(s);
    case REG_LEN:
        return get_page_len(s);
    default:
        if (offset >= REG_DATA) {
            return get_page_data(s, offset - REG_DATA);
        }
        qemu_log_mask(LOG_GUEST_ERROR,
                      "goldfish events device read: bad offset %x\n",
                      (int)offset);
        return 0;
    }
}

static void events_write(void *opaque, hwaddr offset,
                         uint64_t val, unsigned size)
{
    GoldfishEvDevState *s = (GoldfishEvDevState *)opaque;
    switch (offset) {
    case REG_SET_PAGE:
        s->page = val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "goldfish events device write: bad offset %x\n",
                      (int)offset);
        break;
    }
}

static const MemoryRegionOps goldfish_evdev_ops = {
    .read = events_read,
    .write = events_write,
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
        enqueue_event(s, EV_ABS, ABS_X, dx);
        enqueue_event(s, EV_ABS, ABS_Y, dy);
        enqueue_event(s, EV_ABS, ABS_Z, dz);
        enqueue_event(s, EV_KEY, BTN_TOUCH, buttons_state & 1);
        enqueue_event(s, EV_SYN, 0, 0);
        return;
    }
    if (s->have_trackball  &&  dz == 1) {
        enqueue_event(s, EV_REL, REL_X, dx);
        enqueue_event(s, EV_REL, REL_Y, dy);
        enqueue_event(s, EV_SYN, 0, 0);
        return;
    }
}

/* set bits [bitl..bith] in the ev_bits[type] array
 */
static void
events_set_bits(GoldfishEvDevState *s, int type, int bitl, int bith)
{
    uint8_t *bits;
    uint8_t maskl, maskh;
    int il, ih;
    il = bitl / 8;
    ih = bith / 8;
    if (ih >= s->ev_bits[type].len) {
        bits = g_malloc0(ih + 1);
        if (bits == NULL) {
            return;
        }
        memcpy(bits, s->ev_bits[type].bits, s->ev_bits[type].len);
        g_free(s->ev_bits[type].bits);
        s->ev_bits[type].bits = bits;
        s->ev_bits[type].len = ih + 1;
    } else {
        bits = s->ev_bits[type].bits;
    }
    maskl = 0xffU << (bitl & 7);
    maskh = 0xffU >> (7 - (bith & 7));
    if (il >= ih) {
        maskh &= maskl;
    } else {
        bits[il] |= maskl;
        while (++il < ih) {
            bits[il] = 0xff;
        }
    }
    bits[ih] |= maskh;
}

static void
events_set_bit(GoldfishEvDevState *s, int  type, int  bit)
{
    events_set_bits(s, type, bit, bit);
}

static void
events_clr_bit(GoldfishEvDevState *s, int type, int bit)
{
    int ii = bit / 8;
    if (ii < s->ev_bits[type].len) {
        uint8_t *bits = s->ev_bits[type].bits;
        uint8_t  mask = 0x01U << (bit & 7);
        bits[ii] &= ~mask;
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

    GoldfishEvDevState *s = GOLDFISHEVDEV(dev);
    int lkey = 0;
    int mod;

    assert(evt->type == INPUT_EVENT_KIND_KEY);

    KeyValue* kv = evt->u.key.data->key;

    int qcode = kv->u.qcode.data;

    enqueue_event(s, EV_KEY, qcode, evt->u.key.data->down);

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
        enqueue_event(s, EV_KEY, lkey, evt->u.key.data->down);
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
    GoldfishEvDevState *s = GOLDFISHEVDEV(obj);
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

    GoldfishEvDevState *s = GOLDFISHEVDEV(dev);

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
    events_set_bit(s, EV_SYN, EV_KEY);

    events_set_bit(s, EV_KEY, LINUX_KEY_HOME);
    events_set_bit(s, EV_KEY, LINUX_KEY_BACK);
    events_set_bit(s, EV_KEY, LINUX_KEY_SEND);
    events_set_bit(s, EV_KEY, LINUX_KEY_END);
    events_set_bit(s, EV_KEY, LINUX_KEY_SOFT1);
    events_set_bit(s, EV_KEY, LINUX_KEY_VOLUMEUP);
    events_set_bit(s, EV_KEY, LINUX_KEY_VOLUMEDOWN);
    events_set_bit(s, EV_KEY, LINUX_KEY_SOFT2);
    events_set_bit(s, EV_KEY, LINUX_KEY_POWER);
    events_set_bit(s, EV_KEY, LINUX_KEY_SEARCH);

    events_set_bit(s, EV_KEY, ANDROID_KEY_APPSWITCH);

    if (s->have_dpad) {
        events_set_bit(s, EV_KEY, LINUX_KEY_DOWN);
        events_set_bit(s, EV_KEY, LINUX_KEY_UP);
        events_set_bit(s, EV_KEY, LINUX_KEY_LEFT);
        events_set_bit(s, EV_KEY, LINUX_KEY_RIGHT);
        events_set_bit(s, EV_KEY, LINUX_KEY_CENTER);
    }

    if (s->have_trackball) {
        events_set_bit(s, EV_KEY, BTN_MOUSE);
    }
    if (s->have_touch) {
        events_set_bit(s, EV_KEY, BTN_TOUCH);
    }

    if (s->have_camera) {
        /* Camera emulation is enabled. */
        events_set_bit(s, EV_KEY, LINUX_KEY_CAMERA);
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
        events_set_bits(s, EV_KEY, 1, 0xff);
        events_set_bits(s, EV_KEY, 0x160, 0x1ff);

        /* If there is a keyboard, but no DPad, we need to clear the
         * corresponding bit. Doing this is simpler than trying to exclude
         * the DPad values from the ranges above.
         *
         * LINUX_KEY_UP/DOWN/LEFT/RIGHT should be left set so the keyboard
         * arrow keys are still usable, even though a typical device hard
         * keyboard would not include those keys.
         */
        if (!s->have_dpad) {
            events_clr_bit(s, EV_KEY, LINUX_KEY_CENTER);
        }
    }

    /* configure EV_REL array
     *
     * EV_REL events are sent when the trackball is moved
     */
    if (s->have_trackball) {
        events_set_bit(s, EV_SYN, EV_REL);
        events_set_bits(s, EV_REL, REL_X, REL_Y);
    }

    /* configure EV_ABS array.
     *
     * EV_ABS events are sent when the touchscreen is pressed
     */
    if (s->have_touch || s->have_multitouch) {
        ABSEntry *abs_values;

        events_set_bit(s, EV_SYN, EV_ABS);
        events_set_bits(s, EV_ABS, ABS_X, ABS_Z);
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
        abs_values[ABS_X].max = 0x7fff;
        abs_values[ABS_Y].max = 0x7fff;
        abs_values[ABS_Z].max = 1;

        if (s->have_multitouch) {
            /*
             * Setup multitouch.
             */
            events_set_bit(s, EV_ABS, ABS_MT_SLOT);
            events_set_bit(s, EV_ABS, ABS_MT_POSITION_X);
            events_set_bit(s, EV_ABS, ABS_MT_POSITION_Y);
            events_set_bit(s, EV_ABS, ABS_MT_TRACKING_ID);
            events_set_bit(s, EV_ABS, ABS_MT_TOUCH_MAJOR);
            events_set_bit(s, EV_ABS, ABS_MT_PRESSURE);

            if (s_multitouch_funcs) {
                abs_values[ABS_MT_SLOT].max =
                        s_multitouch_funcs->get_max_slot();
                abs_values[ABS_MT_TRACKING_ID].max =
                        abs_values[ABS_MT_SLOT].max + 1;
                abs_values[ABS_MT_POSITION_X].max = abs_values[ABS_X].max;
                abs_values[ABS_MT_POSITION_Y].max = abs_values[ABS_Y].max;
                /* TODO : make next 2 less random */
                abs_values[ABS_MT_TOUCH_MAJOR].max = 0x7fffffff;
                abs_values[ABS_MT_PRESSURE].max = 0x100;
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
    events_set_bit(s, EV_SYN, EV_SW);
    events_set_bit(s, EV_SW, SW_HEADPHONE_INSERT);
    events_set_bit(s, EV_SW, SW_MICROPHONE_INSERT);

    if (s->have_keyboard && s->have_keyboard_lid) {
        events_set_bit(s, EV_SW, SW_LID);
    }

    /* Register global variable. */
    assert(s_evdev == NULL);
    s_evdev = s;
}

static void goldfish_evdev_reset(DeviceState *dev)
{
    GoldfishEvDevState *s = GOLDFISHEVDEV(dev);

    s->state = STATE_INIT;
    s->first = 0;
    s->last = 0;
    s->state = 0;
}

static Property goldfish_evdev_props[] = {
    DEFINE_PROP_BOOL("have-dpad", GoldfishEvDevState, have_dpad, false),
    DEFINE_PROP_BOOL("have-trackball", GoldfishEvDevState,
                     have_trackball, false),
    DEFINE_PROP_BOOL("have-camera", GoldfishEvDevState, have_camera, false),
    DEFINE_PROP_BOOL("have-keyboard", GoldfishEvDevState, have_keyboard, false),
    DEFINE_PROP_BOOL("have-lidswitch", GoldfishEvDevState,
                     have_keyboard_lid, false),
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
