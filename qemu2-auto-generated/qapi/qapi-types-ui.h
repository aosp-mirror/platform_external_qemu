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

#ifndef QAPI_TYPES_UI_H
#define QAPI_TYPES_UI_H

#include "qapi/qapi-builtin-types.h"
#include "qapi-types-sockets.h"

typedef struct q_obj_set_password_arg q_obj_set_password_arg;

typedef struct q_obj_expire_password_arg q_obj_expire_password_arg;

typedef struct q_obj_screendump_arg q_obj_screendump_arg;

typedef struct SpiceBasicInfo SpiceBasicInfo;

typedef struct SpiceServerInfo SpiceServerInfo;

typedef struct SpiceChannel SpiceChannel;

typedef enum SpiceQueryMouseMode {
    SPICE_QUERY_MOUSE_MODE_CLIENT = 0,
    SPICE_QUERY_MOUSE_MODE_SERVER = 1,
    SPICE_QUERY_MOUSE_MODE_UNKNOWN = 2,
    SPICE_QUERY_MOUSE_MODE__MAX = 3,
} SpiceQueryMouseMode;

#define SpiceQueryMouseMode_str(val) \
    qapi_enum_lookup(&SpiceQueryMouseMode_lookup, (val))

extern const QEnumLookup SpiceQueryMouseMode_lookup;

typedef struct SpiceChannelList SpiceChannelList;

typedef struct SpiceInfo SpiceInfo;

typedef struct q_obj_SPICE_CONNECTED_arg q_obj_SPICE_CONNECTED_arg;

typedef struct q_obj_SPICE_INITIALIZED_arg q_obj_SPICE_INITIALIZED_arg;

typedef struct q_obj_SPICE_DISCONNECTED_arg q_obj_SPICE_DISCONNECTED_arg;

typedef struct VncBasicInfo VncBasicInfo;

typedef struct VncServerInfo VncServerInfo;

typedef struct VncClientInfo VncClientInfo;

typedef struct VncClientInfoList VncClientInfoList;

typedef struct VncInfo VncInfo;

typedef enum VncPrimaryAuth {
    VNC_PRIMARY_AUTH_NONE = 0,
    VNC_PRIMARY_AUTH_VNC = 1,
    VNC_PRIMARY_AUTH_RA2 = 2,
    VNC_PRIMARY_AUTH_RA2NE = 3,
    VNC_PRIMARY_AUTH_TIGHT = 4,
    VNC_PRIMARY_AUTH_ULTRA = 5,
    VNC_PRIMARY_AUTH_TLS = 6,
    VNC_PRIMARY_AUTH_VENCRYPT = 7,
    VNC_PRIMARY_AUTH_SASL = 8,
    VNC_PRIMARY_AUTH__MAX = 9,
} VncPrimaryAuth;

#define VncPrimaryAuth_str(val) \
    qapi_enum_lookup(&VncPrimaryAuth_lookup, (val))

extern const QEnumLookup VncPrimaryAuth_lookup;

typedef enum VncVencryptSubAuth {
    VNC_VENCRYPT_SUB_AUTH_PLAIN = 0,
    VNC_VENCRYPT_SUB_AUTH_TLS_NONE = 1,
    VNC_VENCRYPT_SUB_AUTH_X509_NONE = 2,
    VNC_VENCRYPT_SUB_AUTH_TLS_VNC = 3,
    VNC_VENCRYPT_SUB_AUTH_X509_VNC = 4,
    VNC_VENCRYPT_SUB_AUTH_TLS_PLAIN = 5,
    VNC_VENCRYPT_SUB_AUTH_X509_PLAIN = 6,
    VNC_VENCRYPT_SUB_AUTH_TLS_SASL = 7,
    VNC_VENCRYPT_SUB_AUTH_X509_SASL = 8,
    VNC_VENCRYPT_SUB_AUTH__MAX = 9,
} VncVencryptSubAuth;

#define VncVencryptSubAuth_str(val) \
    qapi_enum_lookup(&VncVencryptSubAuth_lookup, (val))

extern const QEnumLookup VncVencryptSubAuth_lookup;

typedef struct VncServerInfo2 VncServerInfo2;

typedef struct VncServerInfo2List VncServerInfo2List;

typedef struct VncInfo2 VncInfo2;

typedef struct VncInfo2List VncInfo2List;

typedef struct q_obj_change_vnc_password_arg q_obj_change_vnc_password_arg;

typedef struct q_obj_VNC_CONNECTED_arg q_obj_VNC_CONNECTED_arg;

typedef struct q_obj_VNC_INITIALIZED_arg q_obj_VNC_INITIALIZED_arg;

typedef struct q_obj_VNC_DISCONNECTED_arg q_obj_VNC_DISCONNECTED_arg;

typedef struct MouseInfo MouseInfo;

typedef struct MouseInfoList MouseInfoList;

typedef enum QKeyCode {
    Q_KEY_CODE_UNMAPPED = 0,
    Q_KEY_CODE_SHIFT = 1,
    Q_KEY_CODE_SHIFT_R = 2,
    Q_KEY_CODE_ALT = 3,
    Q_KEY_CODE_ALT_R = 4,
    Q_KEY_CODE_CTRL = 5,
    Q_KEY_CODE_CTRL_R = 6,
    Q_KEY_CODE_MENU = 7,
    Q_KEY_CODE_ESC = 8,
    Q_KEY_CODE_1 = 9,
    Q_KEY_CODE_2 = 10,
    Q_KEY_CODE_3 = 11,
    Q_KEY_CODE_4 = 12,
    Q_KEY_CODE_5 = 13,
    Q_KEY_CODE_6 = 14,
    Q_KEY_CODE_7 = 15,
    Q_KEY_CODE_8 = 16,
    Q_KEY_CODE_9 = 17,
    Q_KEY_CODE_0 = 18,
    Q_KEY_CODE_MINUS = 19,
    Q_KEY_CODE_EQUAL = 20,
    Q_KEY_CODE_BACKSPACE = 21,
    Q_KEY_CODE_TAB = 22,
    Q_KEY_CODE_Q = 23,
    Q_KEY_CODE_W = 24,
    Q_KEY_CODE_E = 25,
    Q_KEY_CODE_R = 26,
    Q_KEY_CODE_T = 27,
    Q_KEY_CODE_Y = 28,
    Q_KEY_CODE_U = 29,
    Q_KEY_CODE_I = 30,
    Q_KEY_CODE_O = 31,
    Q_KEY_CODE_P = 32,
    Q_KEY_CODE_BRACKET_LEFT = 33,
    Q_KEY_CODE_BRACKET_RIGHT = 34,
    Q_KEY_CODE_RET = 35,
    Q_KEY_CODE_A = 36,
    Q_KEY_CODE_S = 37,
    Q_KEY_CODE_D = 38,
    Q_KEY_CODE_F = 39,
    Q_KEY_CODE_G = 40,
    Q_KEY_CODE_H = 41,
    Q_KEY_CODE_J = 42,
    Q_KEY_CODE_K = 43,
    Q_KEY_CODE_L = 44,
    Q_KEY_CODE_SEMICOLON = 45,
    Q_KEY_CODE_APOSTROPHE = 46,
    Q_KEY_CODE_GRAVE_ACCENT = 47,
    Q_KEY_CODE_BACKSLASH = 48,
    Q_KEY_CODE_Z = 49,
    Q_KEY_CODE_X = 50,
    Q_KEY_CODE_C = 51,
    Q_KEY_CODE_V = 52,
    Q_KEY_CODE_B = 53,
    Q_KEY_CODE_N = 54,
    Q_KEY_CODE_M = 55,
    Q_KEY_CODE_COMMA = 56,
    Q_KEY_CODE_DOT = 57,
    Q_KEY_CODE_SLASH = 58,
    Q_KEY_CODE_ASTERISK = 59,
    Q_KEY_CODE_SPC = 60,
    Q_KEY_CODE_CAPS_LOCK = 61,
    Q_KEY_CODE_F1 = 62,
    Q_KEY_CODE_F2 = 63,
    Q_KEY_CODE_F3 = 64,
    Q_KEY_CODE_F4 = 65,
    Q_KEY_CODE_F5 = 66,
    Q_KEY_CODE_F6 = 67,
    Q_KEY_CODE_F7 = 68,
    Q_KEY_CODE_F8 = 69,
    Q_KEY_CODE_F9 = 70,
    Q_KEY_CODE_F10 = 71,
    Q_KEY_CODE_NUM_LOCK = 72,
    Q_KEY_CODE_SCROLL_LOCK = 73,
    Q_KEY_CODE_KP_DIVIDE = 74,
    Q_KEY_CODE_KP_MULTIPLY = 75,
    Q_KEY_CODE_KP_SUBTRACT = 76,
    Q_KEY_CODE_KP_ADD = 77,
    Q_KEY_CODE_KP_ENTER = 78,
    Q_KEY_CODE_KP_DECIMAL = 79,
    Q_KEY_CODE_SYSRQ = 80,
    Q_KEY_CODE_KP_0 = 81,
    Q_KEY_CODE_KP_1 = 82,
    Q_KEY_CODE_KP_2 = 83,
    Q_KEY_CODE_KP_3 = 84,
    Q_KEY_CODE_KP_4 = 85,
    Q_KEY_CODE_KP_5 = 86,
    Q_KEY_CODE_KP_6 = 87,
    Q_KEY_CODE_KP_7 = 88,
    Q_KEY_CODE_KP_8 = 89,
    Q_KEY_CODE_KP_9 = 90,
    Q_KEY_CODE_LESS = 91,
    Q_KEY_CODE_F11 = 92,
    Q_KEY_CODE_F12 = 93,
    Q_KEY_CODE_PRINT = 94,
    Q_KEY_CODE_HOME = 95,
    Q_KEY_CODE_PGUP = 96,
    Q_KEY_CODE_PGDN = 97,
    Q_KEY_CODE_END = 98,
    Q_KEY_CODE_LEFT = 99,
    Q_KEY_CODE_UP = 100,
    Q_KEY_CODE_DOWN = 101,
    Q_KEY_CODE_RIGHT = 102,
    Q_KEY_CODE_INSERT = 103,
    Q_KEY_CODE_DELETE = 104,
    Q_KEY_CODE_STOP = 105,
    Q_KEY_CODE_AGAIN = 106,
    Q_KEY_CODE_PROPS = 107,
    Q_KEY_CODE_UNDO = 108,
    Q_KEY_CODE_FRONT = 109,
    Q_KEY_CODE_COPY = 110,
    Q_KEY_CODE_OPEN = 111,
    Q_KEY_CODE_PASTE = 112,
    Q_KEY_CODE_FIND = 113,
    Q_KEY_CODE_CUT = 114,
    Q_KEY_CODE_LF = 115,
    Q_KEY_CODE_HELP = 116,
    Q_KEY_CODE_META_L = 117,
    Q_KEY_CODE_META_R = 118,
    Q_KEY_CODE_COMPOSE = 119,
    Q_KEY_CODE_PAUSE = 120,
    Q_KEY_CODE_RO = 121,
    Q_KEY_CODE_HIRAGANA = 122,
    Q_KEY_CODE_HENKAN = 123,
    Q_KEY_CODE_YEN = 124,
    Q_KEY_CODE_MUHENKAN = 125,
    Q_KEY_CODE_KATAKANAHIRAGANA = 126,
    Q_KEY_CODE_KP_COMMA = 127,
    Q_KEY_CODE_KP_EQUALS = 128,
    Q_KEY_CODE_POWER = 129,
    Q_KEY_CODE_SLEEP = 130,
    Q_KEY_CODE_WAKE = 131,
    Q_KEY_CODE_AUDIONEXT = 132,
    Q_KEY_CODE_AUDIOPREV = 133,
    Q_KEY_CODE_AUDIOSTOP = 134,
    Q_KEY_CODE_AUDIOPLAY = 135,
    Q_KEY_CODE_AUDIOMUTE = 136,
    Q_KEY_CODE_VOLUMEUP = 137,
    Q_KEY_CODE_VOLUMEDOWN = 138,
    Q_KEY_CODE_MEDIASELECT = 139,
    Q_KEY_CODE_MAIL = 140,
    Q_KEY_CODE_CALCULATOR = 141,
    Q_KEY_CODE_COMPUTER = 142,
    Q_KEY_CODE_AC_HOME = 143,
    Q_KEY_CODE_AC_BACK = 144,
    Q_KEY_CODE_AC_FORWARD = 145,
    Q_KEY_CODE_AC_REFRESH = 146,
    Q_KEY_CODE_AC_BOOKMARKS = 147,
    Q_KEY_CODE__MAX = 148,
} QKeyCode;

#define QKeyCode_str(val) \
    qapi_enum_lookup(&QKeyCode_lookup, (val))

extern const QEnumLookup QKeyCode_lookup;

typedef struct q_obj_int_wrapper q_obj_int_wrapper;

typedef struct q_obj_QKeyCode_wrapper q_obj_QKeyCode_wrapper;

typedef enum KeyValueKind {
    KEY_VALUE_KIND_NUMBER = 0,
    KEY_VALUE_KIND_QCODE = 1,
    KEY_VALUE_KIND__MAX = 2,
} KeyValueKind;

#define KeyValueKind_str(val) \
    qapi_enum_lookup(&KeyValueKind_lookup, (val))

extern const QEnumLookup KeyValueKind_lookup;

typedef struct KeyValue KeyValue;

typedef struct KeyValueList KeyValueList;

typedef struct q_obj_send_key_arg q_obj_send_key_arg;

typedef enum InputButton {
    INPUT_BUTTON_LEFT = 0,
    INPUT_BUTTON_MIDDLE = 1,
    INPUT_BUTTON_RIGHT = 2,
    INPUT_BUTTON_WHEEL_UP = 3,
    INPUT_BUTTON_WHEEL_DOWN = 4,
    INPUT_BUTTON_SIDE = 5,
    INPUT_BUTTON_EXTRA = 6,
    INPUT_BUTTON__MAX = 7,
} InputButton;

#define InputButton_str(val) \
    qapi_enum_lookup(&InputButton_lookup, (val))

extern const QEnumLookup InputButton_lookup;

typedef enum InputAxis {
    INPUT_AXIS_X = 0,
    INPUT_AXIS_Y = 1,
    INPUT_AXIS__MAX = 2,
} InputAxis;

#define InputAxis_str(val) \
    qapi_enum_lookup(&InputAxis_lookup, (val))

extern const QEnumLookup InputAxis_lookup;

typedef struct InputKeyEvent InputKeyEvent;

typedef struct InputBtnEvent InputBtnEvent;

typedef struct InputMoveEvent InputMoveEvent;

typedef struct q_obj_InputKeyEvent_wrapper q_obj_InputKeyEvent_wrapper;

typedef struct q_obj_InputBtnEvent_wrapper q_obj_InputBtnEvent_wrapper;

typedef struct q_obj_InputMoveEvent_wrapper q_obj_InputMoveEvent_wrapper;

typedef enum InputEventKind {
    INPUT_EVENT_KIND_KEY = 0,
    INPUT_EVENT_KIND_BTN = 1,
    INPUT_EVENT_KIND_REL = 2,
    INPUT_EVENT_KIND_ABS = 3,
    INPUT_EVENT_KIND__MAX = 4,
} InputEventKind;

#define InputEventKind_str(val) \
    qapi_enum_lookup(&InputEventKind_lookup, (val))

extern const QEnumLookup InputEventKind_lookup;

typedef struct InputEvent InputEvent;

typedef struct InputEventList InputEventList;

typedef struct q_obj_input_send_event_arg q_obj_input_send_event_arg;

typedef struct DisplayNoOpts DisplayNoOpts;

typedef struct DisplayGTK DisplayGTK;

typedef enum DisplayType {
    DISPLAY_TYPE_DEFAULT = 0,
    DISPLAY_TYPE_NONE = 1,
    DISPLAY_TYPE_GTK = 2,
    DISPLAY_TYPE_SDL = 3,
    DISPLAY_TYPE_EGL_HEADLESS = 4,
    DISPLAY_TYPE_CURSES = 5,
    DISPLAY_TYPE_COCOA = 6,
    DISPLAY_TYPE__MAX = 7,
} DisplayType;

#define DisplayType_str(val) \
    qapi_enum_lookup(&DisplayType_lookup, (val))

extern const QEnumLookup DisplayType_lookup;

typedef struct q_obj_DisplayOptions_base q_obj_DisplayOptions_base;

typedef struct DisplayOptions DisplayOptions;

struct q_obj_set_password_arg {
    char *protocol;
    char *password;
    bool has_connected;
    char *connected;
};

struct q_obj_expire_password_arg {
    char *protocol;
    char *time;
};

struct q_obj_screendump_arg {
    char *filename;
    bool has_device;
    char *device;
    bool has_head;
    int64_t head;
};

struct SpiceBasicInfo {
    char *host;
    char *port;
    NetworkAddressFamily family;
};

void qapi_free_SpiceBasicInfo(SpiceBasicInfo *obj);

struct SpiceServerInfo {
    /* Members inherited from SpiceBasicInfo: */
    char *host;
    char *port;
    NetworkAddressFamily family;
    /* Own members: */
    bool has_auth;
    char *auth;
};

static inline SpiceBasicInfo *qapi_SpiceServerInfo_base(const SpiceServerInfo *obj)
{
    return (SpiceBasicInfo *)obj;
}

void qapi_free_SpiceServerInfo(SpiceServerInfo *obj);

struct SpiceChannel {
    /* Members inherited from SpiceBasicInfo: */
    char *host;
    char *port;
    NetworkAddressFamily family;
    /* Own members: */
    int64_t connection_id;
    int64_t channel_type;
    int64_t channel_id;
    bool tls;
};

static inline SpiceBasicInfo *qapi_SpiceChannel_base(const SpiceChannel *obj)
{
    return (SpiceBasicInfo *)obj;
}

void qapi_free_SpiceChannel(SpiceChannel *obj);

struct SpiceChannelList {
    SpiceChannelList *next;
    SpiceChannel *value;
};

void qapi_free_SpiceChannelList(SpiceChannelList *obj);

struct SpiceInfo {
    bool enabled;
    bool migrated;
    bool has_host;
    char *host;
    bool has_port;
    int64_t port;
    bool has_tls_port;
    int64_t tls_port;
    bool has_auth;
    char *auth;
    bool has_compiled_version;
    char *compiled_version;
    SpiceQueryMouseMode mouse_mode;
    bool has_channels;
    SpiceChannelList *channels;
};

void qapi_free_SpiceInfo(SpiceInfo *obj);

struct q_obj_SPICE_CONNECTED_arg {
    SpiceBasicInfo *server;
    SpiceBasicInfo *client;
};

struct q_obj_SPICE_INITIALIZED_arg {
    SpiceServerInfo *server;
    SpiceChannel *client;
};

struct q_obj_SPICE_DISCONNECTED_arg {
    SpiceBasicInfo *server;
    SpiceBasicInfo *client;
};

struct VncBasicInfo {
    char *host;
    char *service;
    NetworkAddressFamily family;
    bool websocket;
};

void qapi_free_VncBasicInfo(VncBasicInfo *obj);

struct VncServerInfo {
    /* Members inherited from VncBasicInfo: */
    char *host;
    char *service;
    NetworkAddressFamily family;
    bool websocket;
    /* Own members: */
    bool has_auth;
    char *auth;
};

static inline VncBasicInfo *qapi_VncServerInfo_base(const VncServerInfo *obj)
{
    return (VncBasicInfo *)obj;
}

void qapi_free_VncServerInfo(VncServerInfo *obj);

struct VncClientInfo {
    /* Members inherited from VncBasicInfo: */
    char *host;
    char *service;
    NetworkAddressFamily family;
    bool websocket;
    /* Own members: */
    bool has_x509_dname;
    char *x509_dname;
    bool has_sasl_username;
    char *sasl_username;
};

static inline VncBasicInfo *qapi_VncClientInfo_base(const VncClientInfo *obj)
{
    return (VncBasicInfo *)obj;
}

void qapi_free_VncClientInfo(VncClientInfo *obj);

struct VncClientInfoList {
    VncClientInfoList *next;
    VncClientInfo *value;
};

void qapi_free_VncClientInfoList(VncClientInfoList *obj);

struct VncInfo {
    bool enabled;
    bool has_host;
    char *host;
    bool has_family;
    NetworkAddressFamily family;
    bool has_service;
    char *service;
    bool has_auth;
    char *auth;
    bool has_clients;
    VncClientInfoList *clients;
};

void qapi_free_VncInfo(VncInfo *obj);

struct VncServerInfo2 {
    /* Members inherited from VncBasicInfo: */
    char *host;
    char *service;
    NetworkAddressFamily family;
    bool websocket;
    /* Own members: */
    VncPrimaryAuth auth;
    bool has_vencrypt;
    VncVencryptSubAuth vencrypt;
};

static inline VncBasicInfo *qapi_VncServerInfo2_base(const VncServerInfo2 *obj)
{
    return (VncBasicInfo *)obj;
}

void qapi_free_VncServerInfo2(VncServerInfo2 *obj);

struct VncServerInfo2List {
    VncServerInfo2List *next;
    VncServerInfo2 *value;
};

void qapi_free_VncServerInfo2List(VncServerInfo2List *obj);

struct VncInfo2 {
    char *id;
    VncServerInfo2List *server;
    VncClientInfoList *clients;
    VncPrimaryAuth auth;
    bool has_vencrypt;
    VncVencryptSubAuth vencrypt;
    bool has_display;
    char *display;
};

void qapi_free_VncInfo2(VncInfo2 *obj);

struct VncInfo2List {
    VncInfo2List *next;
    VncInfo2 *value;
};

void qapi_free_VncInfo2List(VncInfo2List *obj);

struct q_obj_change_vnc_password_arg {
    char *password;
};

struct q_obj_VNC_CONNECTED_arg {
    VncServerInfo *server;
    VncBasicInfo *client;
};

struct q_obj_VNC_INITIALIZED_arg {
    VncServerInfo *server;
    VncClientInfo *client;
};

struct q_obj_VNC_DISCONNECTED_arg {
    VncServerInfo *server;
    VncClientInfo *client;
};

struct MouseInfo {
    char *name;
    int64_t index;
    bool current;
    bool absolute;
};

void qapi_free_MouseInfo(MouseInfo *obj);

struct MouseInfoList {
    MouseInfoList *next;
    MouseInfo *value;
};

void qapi_free_MouseInfoList(MouseInfoList *obj);

struct q_obj_int_wrapper {
    int64_t data;
};

struct q_obj_QKeyCode_wrapper {
    QKeyCode data;
};

struct KeyValue {
    KeyValueKind type;
    union { /* union tag is @type */
        q_obj_int_wrapper number;
        q_obj_QKeyCode_wrapper qcode;
    } u;
};

void qapi_free_KeyValue(KeyValue *obj);

struct KeyValueList {
    KeyValueList *next;
    KeyValue *value;
};

void qapi_free_KeyValueList(KeyValueList *obj);

struct q_obj_send_key_arg {
    KeyValueList *keys;
    bool has_hold_time;
    int64_t hold_time;
};

struct InputKeyEvent {
    KeyValue *key;
    bool down;
};

void qapi_free_InputKeyEvent(InputKeyEvent *obj);

struct InputBtnEvent {
    InputButton button;
    bool down;
};

void qapi_free_InputBtnEvent(InputBtnEvent *obj);

struct InputMoveEvent {
    InputAxis axis;
    int64_t value;
};

void qapi_free_InputMoveEvent(InputMoveEvent *obj);

struct q_obj_InputKeyEvent_wrapper {
    InputKeyEvent *data;
};

struct q_obj_InputBtnEvent_wrapper {
    InputBtnEvent *data;
};

struct q_obj_InputMoveEvent_wrapper {
    InputMoveEvent *data;
};

struct InputEvent {
    InputEventKind type;
    union { /* union tag is @type */
        q_obj_InputKeyEvent_wrapper key;
        q_obj_InputBtnEvent_wrapper btn;
        q_obj_InputMoveEvent_wrapper rel;
        q_obj_InputMoveEvent_wrapper abs;
    } u;
};

void qapi_free_InputEvent(InputEvent *obj);

struct InputEventList {
    InputEventList *next;
    InputEvent *value;
};

void qapi_free_InputEventList(InputEventList *obj);

struct q_obj_input_send_event_arg {
    bool has_device;
    char *device;
    bool has_head;
    int64_t head;
    InputEventList *events;
};

struct DisplayNoOpts {
    char qapi_dummy_for_empty_struct;
};

void qapi_free_DisplayNoOpts(DisplayNoOpts *obj);

struct DisplayGTK {
    bool has_grab_on_hover;
    bool grab_on_hover;
};

void qapi_free_DisplayGTK(DisplayGTK *obj);

struct q_obj_DisplayOptions_base {
    DisplayType type;
    bool has_full_screen;
    bool full_screen;
    bool has_window_close;
    bool window_close;
    bool has_gl;
    bool gl;
};

struct DisplayOptions {
    DisplayType type;
    bool has_full_screen;
    bool full_screen;
    bool has_window_close;
    bool window_close;
    bool has_gl;
    bool gl;
    union { /* union tag is @type */
        DisplayNoOpts q_default;
        DisplayNoOpts none;
        DisplayGTK gtk;
        DisplayNoOpts sdl;
        DisplayNoOpts egl_headless;
        DisplayNoOpts curses;
        DisplayNoOpts cocoa;
    } u;
};

void qapi_free_DisplayOptions(DisplayOptions *obj);

#endif /* QAPI_TYPES_UI_H */
