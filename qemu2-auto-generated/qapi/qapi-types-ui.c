/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI types
 *
 * Copyright IBM, Corp. 2011
 * Copyright (c) 2013-2018 Red Hat Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qapi/dealloc-visitor.h"
#include "qapi-types-ui.h"
#include "qapi-visit-ui.h"

void qapi_free_SpiceBasicInfo(SpiceBasicInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SpiceBasicInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SpiceServerInfo(SpiceServerInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SpiceServerInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SpiceChannel(SpiceChannel *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SpiceChannel(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup SpiceQueryMouseMode_lookup = {
    .array = (const char *const[]) {
        [SPICE_QUERY_MOUSE_MODE_CLIENT] = "client",
        [SPICE_QUERY_MOUSE_MODE_SERVER] = "server",
        [SPICE_QUERY_MOUSE_MODE_UNKNOWN] = "unknown",
    },
    .size = SPICE_QUERY_MOUSE_MODE__MAX
};

void qapi_free_SpiceChannelList(SpiceChannelList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SpiceChannelList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SpiceInfo(SpiceInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SpiceInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_VncBasicInfo(VncBasicInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VncBasicInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_VncServerInfo(VncServerInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VncServerInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_VncClientInfo(VncClientInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VncClientInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_VncClientInfoList(VncClientInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VncClientInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_VncInfo(VncInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VncInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup VncPrimaryAuth_lookup = {
    .array = (const char *const[]) {
        [VNC_PRIMARY_AUTH_NONE] = "none",
        [VNC_PRIMARY_AUTH_VNC] = "vnc",
        [VNC_PRIMARY_AUTH_RA2] = "ra2",
        [VNC_PRIMARY_AUTH_RA2NE] = "ra2ne",
        [VNC_PRIMARY_AUTH_TIGHT] = "tight",
        [VNC_PRIMARY_AUTH_ULTRA] = "ultra",
        [VNC_PRIMARY_AUTH_TLS] = "tls",
        [VNC_PRIMARY_AUTH_VENCRYPT] = "vencrypt",
        [VNC_PRIMARY_AUTH_SASL] = "sasl",
    },
    .size = VNC_PRIMARY_AUTH__MAX
};

const QEnumLookup VncVencryptSubAuth_lookup = {
    .array = (const char *const[]) {
        [VNC_VENCRYPT_SUB_AUTH_PLAIN] = "plain",
        [VNC_VENCRYPT_SUB_AUTH_TLS_NONE] = "tls-none",
        [VNC_VENCRYPT_SUB_AUTH_X509_NONE] = "x509-none",
        [VNC_VENCRYPT_SUB_AUTH_TLS_VNC] = "tls-vnc",
        [VNC_VENCRYPT_SUB_AUTH_X509_VNC] = "x509-vnc",
        [VNC_VENCRYPT_SUB_AUTH_TLS_PLAIN] = "tls-plain",
        [VNC_VENCRYPT_SUB_AUTH_X509_PLAIN] = "x509-plain",
        [VNC_VENCRYPT_SUB_AUTH_TLS_SASL] = "tls-sasl",
        [VNC_VENCRYPT_SUB_AUTH_X509_SASL] = "x509-sasl",
    },
    .size = VNC_VENCRYPT_SUB_AUTH__MAX
};

void qapi_free_VncServerInfo2(VncServerInfo2 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VncServerInfo2(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_VncServerInfo2List(VncServerInfo2List *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VncServerInfo2List(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_VncInfo2(VncInfo2 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VncInfo2(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_VncInfo2List(VncInfo2List *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VncInfo2List(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_MouseInfo(MouseInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MouseInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_MouseInfoList(MouseInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MouseInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup QKeyCode_lookup = {
    .array = (const char *const[]) {
        [Q_KEY_CODE_UNMAPPED] = "unmapped",
        [Q_KEY_CODE_SHIFT] = "shift",
        [Q_KEY_CODE_SHIFT_R] = "shift_r",
        [Q_KEY_CODE_ALT] = "alt",
        [Q_KEY_CODE_ALT_R] = "alt_r",
        [Q_KEY_CODE_CTRL] = "ctrl",
        [Q_KEY_CODE_CTRL_R] = "ctrl_r",
        [Q_KEY_CODE_MENU] = "menu",
        [Q_KEY_CODE_ESC] = "esc",
        [Q_KEY_CODE_1] = "1",
        [Q_KEY_CODE_2] = "2",
        [Q_KEY_CODE_3] = "3",
        [Q_KEY_CODE_4] = "4",
        [Q_KEY_CODE_5] = "5",
        [Q_KEY_CODE_6] = "6",
        [Q_KEY_CODE_7] = "7",
        [Q_KEY_CODE_8] = "8",
        [Q_KEY_CODE_9] = "9",
        [Q_KEY_CODE_0] = "0",
        [Q_KEY_CODE_MINUS] = "minus",
        [Q_KEY_CODE_EQUAL] = "equal",
        [Q_KEY_CODE_BACKSPACE] = "backspace",
        [Q_KEY_CODE_TAB] = "tab",
        [Q_KEY_CODE_Q] = "q",
        [Q_KEY_CODE_W] = "w",
        [Q_KEY_CODE_E] = "e",
        [Q_KEY_CODE_R] = "r",
        [Q_KEY_CODE_T] = "t",
        [Q_KEY_CODE_Y] = "y",
        [Q_KEY_CODE_U] = "u",
        [Q_KEY_CODE_I] = "i",
        [Q_KEY_CODE_O] = "o",
        [Q_KEY_CODE_P] = "p",
        [Q_KEY_CODE_BRACKET_LEFT] = "bracket_left",
        [Q_KEY_CODE_BRACKET_RIGHT] = "bracket_right",
        [Q_KEY_CODE_RET] = "ret",
        [Q_KEY_CODE_A] = "a",
        [Q_KEY_CODE_S] = "s",
        [Q_KEY_CODE_D] = "d",
        [Q_KEY_CODE_F] = "f",
        [Q_KEY_CODE_G] = "g",
        [Q_KEY_CODE_H] = "h",
        [Q_KEY_CODE_J] = "j",
        [Q_KEY_CODE_K] = "k",
        [Q_KEY_CODE_L] = "l",
        [Q_KEY_CODE_SEMICOLON] = "semicolon",
        [Q_KEY_CODE_APOSTROPHE] = "apostrophe",
        [Q_KEY_CODE_GRAVE_ACCENT] = "grave_accent",
        [Q_KEY_CODE_BACKSLASH] = "backslash",
        [Q_KEY_CODE_Z] = "z",
        [Q_KEY_CODE_X] = "x",
        [Q_KEY_CODE_C] = "c",
        [Q_KEY_CODE_V] = "v",
        [Q_KEY_CODE_B] = "b",
        [Q_KEY_CODE_N] = "n",
        [Q_KEY_CODE_M] = "m",
        [Q_KEY_CODE_COMMA] = "comma",
        [Q_KEY_CODE_DOT] = "dot",
        [Q_KEY_CODE_SLASH] = "slash",
        [Q_KEY_CODE_ASTERISK] = "asterisk",
        [Q_KEY_CODE_SPC] = "spc",
        [Q_KEY_CODE_CAPS_LOCK] = "caps_lock",
        [Q_KEY_CODE_F1] = "f1",
        [Q_KEY_CODE_F2] = "f2",
        [Q_KEY_CODE_F3] = "f3",
        [Q_KEY_CODE_F4] = "f4",
        [Q_KEY_CODE_F5] = "f5",
        [Q_KEY_CODE_F6] = "f6",
        [Q_KEY_CODE_F7] = "f7",
        [Q_KEY_CODE_F8] = "f8",
        [Q_KEY_CODE_F9] = "f9",
        [Q_KEY_CODE_F10] = "f10",
        [Q_KEY_CODE_NUM_LOCK] = "num_lock",
        [Q_KEY_CODE_SCROLL_LOCK] = "scroll_lock",
        [Q_KEY_CODE_KP_DIVIDE] = "kp_divide",
        [Q_KEY_CODE_KP_MULTIPLY] = "kp_multiply",
        [Q_KEY_CODE_KP_SUBTRACT] = "kp_subtract",
        [Q_KEY_CODE_KP_ADD] = "kp_add",
        [Q_KEY_CODE_KP_ENTER] = "kp_enter",
        [Q_KEY_CODE_KP_DECIMAL] = "kp_decimal",
        [Q_KEY_CODE_SYSRQ] = "sysrq",
        [Q_KEY_CODE_KP_0] = "kp_0",
        [Q_KEY_CODE_KP_1] = "kp_1",
        [Q_KEY_CODE_KP_2] = "kp_2",
        [Q_KEY_CODE_KP_3] = "kp_3",
        [Q_KEY_CODE_KP_4] = "kp_4",
        [Q_KEY_CODE_KP_5] = "kp_5",
        [Q_KEY_CODE_KP_6] = "kp_6",
        [Q_KEY_CODE_KP_7] = "kp_7",
        [Q_KEY_CODE_KP_8] = "kp_8",
        [Q_KEY_CODE_KP_9] = "kp_9",
        [Q_KEY_CODE_LESS] = "less",
        [Q_KEY_CODE_F11] = "f11",
        [Q_KEY_CODE_F12] = "f12",
        [Q_KEY_CODE_PRINT] = "print",
        [Q_KEY_CODE_HOME] = "home",
        [Q_KEY_CODE_PGUP] = "pgup",
        [Q_KEY_CODE_PGDN] = "pgdn",
        [Q_KEY_CODE_END] = "end",
        [Q_KEY_CODE_LEFT] = "left",
        [Q_KEY_CODE_UP] = "up",
        [Q_KEY_CODE_DOWN] = "down",
        [Q_KEY_CODE_RIGHT] = "right",
        [Q_KEY_CODE_INSERT] = "insert",
        [Q_KEY_CODE_DELETE] = "delete",
        [Q_KEY_CODE_STOP] = "stop",
        [Q_KEY_CODE_AGAIN] = "again",
        [Q_KEY_CODE_PROPS] = "props",
        [Q_KEY_CODE_UNDO] = "undo",
        [Q_KEY_CODE_FRONT] = "front",
        [Q_KEY_CODE_COPY] = "copy",
        [Q_KEY_CODE_OPEN] = "open",
        [Q_KEY_CODE_PASTE] = "paste",
        [Q_KEY_CODE_FIND] = "find",
        [Q_KEY_CODE_CUT] = "cut",
        [Q_KEY_CODE_LF] = "lf",
        [Q_KEY_CODE_HELP] = "help",
        [Q_KEY_CODE_META_L] = "meta_l",
        [Q_KEY_CODE_META_R] = "meta_r",
        [Q_KEY_CODE_COMPOSE] = "compose",
        [Q_KEY_CODE_PAUSE] = "pause",
        [Q_KEY_CODE_RO] = "ro",
        [Q_KEY_CODE_HIRAGANA] = "hiragana",
        [Q_KEY_CODE_HENKAN] = "henkan",
        [Q_KEY_CODE_YEN] = "yen",
        [Q_KEY_CODE_MUHENKAN] = "muhenkan",
        [Q_KEY_CODE_KATAKANAHIRAGANA] = "katakanahiragana",
        [Q_KEY_CODE_KP_COMMA] = "kp_comma",
        [Q_KEY_CODE_KP_EQUALS] = "kp_equals",
        [Q_KEY_CODE_POWER] = "power",
        [Q_KEY_CODE_SLEEP] = "sleep",
        [Q_KEY_CODE_WAKE] = "wake",
        [Q_KEY_CODE_AUDIONEXT] = "audionext",
        [Q_KEY_CODE_AUDIOPREV] = "audioprev",
        [Q_KEY_CODE_AUDIOSTOP] = "audiostop",
        [Q_KEY_CODE_AUDIOPLAY] = "audioplay",
        [Q_KEY_CODE_AUDIOMUTE] = "audiomute",
        [Q_KEY_CODE_VOLUMEUP] = "volumeup",
        [Q_KEY_CODE_VOLUMEDOWN] = "volumedown",
        [Q_KEY_CODE_MEDIASELECT] = "mediaselect",
        [Q_KEY_CODE_MAIL] = "mail",
        [Q_KEY_CODE_CALCULATOR] = "calculator",
        [Q_KEY_CODE_COMPUTER] = "computer",
        [Q_KEY_CODE_AC_HOME] = "ac_home",
        [Q_KEY_CODE_AC_BACK] = "ac_back",
        [Q_KEY_CODE_AC_FORWARD] = "ac_forward",
        [Q_KEY_CODE_AC_REFRESH] = "ac_refresh",
        [Q_KEY_CODE_AC_BOOKMARKS] = "ac_bookmarks",
    },
    .size = Q_KEY_CODE__MAX
};

const QEnumLookup KeyValueKind_lookup = {
    .array = (const char *const[]) {
        [KEY_VALUE_KIND_NUMBER] = "number",
        [KEY_VALUE_KIND_QCODE] = "qcode",
    },
    .size = KEY_VALUE_KIND__MAX
};

void qapi_free_KeyValue(KeyValue *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_KeyValue(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_KeyValueList(KeyValueList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_KeyValueList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup InputButton_lookup = {
    .array = (const char *const[]) {
        [INPUT_BUTTON_LEFT] = "left",
        [INPUT_BUTTON_MIDDLE] = "middle",
        [INPUT_BUTTON_RIGHT] = "right",
        [INPUT_BUTTON_WHEEL_UP] = "wheel-up",
        [INPUT_BUTTON_WHEEL_DOWN] = "wheel-down",
        [INPUT_BUTTON_SIDE] = "side",
        [INPUT_BUTTON_EXTRA] = "extra",
    },
    .size = INPUT_BUTTON__MAX
};

const QEnumLookup InputAxis_lookup = {
    .array = (const char *const[]) {
        [INPUT_AXIS_X] = "x",
        [INPUT_AXIS_Y] = "y",
    },
    .size = INPUT_AXIS__MAX
};

void qapi_free_InputKeyEvent(InputKeyEvent *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_InputKeyEvent(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_InputBtnEvent(InputBtnEvent *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_InputBtnEvent(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_InputMoveEvent(InputMoveEvent *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_InputMoveEvent(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup InputEventKind_lookup = {
    .array = (const char *const[]) {
        [INPUT_EVENT_KIND_KEY] = "key",
        [INPUT_EVENT_KIND_BTN] = "btn",
        [INPUT_EVENT_KIND_REL] = "rel",
        [INPUT_EVENT_KIND_ABS] = "abs",
    },
    .size = INPUT_EVENT_KIND__MAX
};

void qapi_free_InputEvent(InputEvent *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_InputEvent(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_InputEventList(InputEventList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_InputEventList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_DisplayNoOpts(DisplayNoOpts *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_DisplayNoOpts(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_DisplayGTK(DisplayGTK *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_DisplayGTK(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup DisplayType_lookup = {
    .array = (const char *const[]) {
        [DISPLAY_TYPE_DEFAULT] = "default",
        [DISPLAY_TYPE_NONE] = "none",
        [DISPLAY_TYPE_GTK] = "gtk",
        [DISPLAY_TYPE_SDL] = "sdl",
        [DISPLAY_TYPE_EGL_HEADLESS] = "egl-headless",
        [DISPLAY_TYPE_CURSES] = "curses",
        [DISPLAY_TYPE_COCOA] = "cocoa",
    },
    .size = DISPLAY_TYPE__MAX
};

void qapi_free_DisplayOptions(DisplayOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_DisplayOptions(v, NULL, &obj, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_ui_c;
