/* Copyright (C) 2009-2015 The Android Open Source Project
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
#include "android/skin/keycode.h"

#include <stddef.h>
#include <string.h>

#ifdef __APPLE__
#import <Carbon/Carbon.h>
#endif
#include "android/utils/bufprint.h"

SkinKeyCode skin_keycode_rotate(SkinKeyCode code, int  rotation) {
    static const SkinKeyCode  wheel[4] = { kKeyCodeDpadUp,
                                           kKeyCodeDpadRight,
                                           kKeyCodeDpadDown,
                                           kKeyCodeDpadLeft };
    int  index;

    for (index = 0; index < 4; index++) {
        if (code == wheel[index]) {
            index = (index + rotation) & 3;
            code  = wheel[index];
            break;
        }
    }
    return code;
}

#define ARRAYLEN(x) (sizeof(x) / sizeof(x[0]))

#define _KEYSYM1_(x) _KEYSYM_(x, x)
#define _KEYSYM1_ANDROID_(x) _KEYSYM_ANDROID_(x, x)

#define _KEYSYM_LIST                      \
    _KEYSYM_(ESC, Escape)                 \
    _KEYSYM_(BACKSPACE, Backspace)        \
    _KEYSYM_(TAB, Tab)                    \
    _KEYSYM_(CLEAR, Clear)                \
    _KEYSYM_(ENTER, Return)               \
    _KEYSYM_(EQUAL, Equal)                \
    _KEYSYM_(PAUSE, Pause)                \
    _KEYSYM_(ESC, Esc)                    \
    _KEYSYM_(SPACE, Space)                \
    _KEYSYM_(PLUS, Plus)                  \
    _KEYSYM_(COMMA, Comma)                \
    _KEYSYM_(MINUS, Minus)                \
    _KEYSYM_(SLASH, Slash)                \
    _KEYSYM1_(0)                          \
    _KEYSYM1_(1)                          \
    _KEYSYM1_(2)                          \
    _KEYSYM1_(3)                          \
    _KEYSYM1_(4)                          \
    _KEYSYM1_(5)                          \
    _KEYSYM1_(6)                          \
    _KEYSYM1_(7)                          \
    _KEYSYM1_(8)                          \
    _KEYSYM1_(9)                          \
    _KEYSYM1_(SEMICOLON)                  \
    _KEYSYM1_(QUESTION)                   \
    _KEYSYM1_(BACKSLASH)                  \
    _KEYSYM1_(A)                          \
    _KEYSYM1_(B)                          \
    _KEYSYM1_(C)                          \
    _KEYSYM1_(D)                          \
    _KEYSYM1_(E)                          \
    _KEYSYM1_(F)                          \
    _KEYSYM1_(G)                          \
    _KEYSYM1_(H)                          \
    _KEYSYM1_(I)                          \
    _KEYSYM1_(J)                          \
    _KEYSYM1_(K)                          \
    _KEYSYM1_(L)                          \
    _KEYSYM1_(M)                          \
    _KEYSYM1_(N)                          \
    _KEYSYM1_(O)                          \
    _KEYSYM1_(P)                          \
    _KEYSYM1_(Q)                          \
    _KEYSYM1_(R)                          \
    _KEYSYM1_(S)                          \
    _KEYSYM1_(T)                          \
    _KEYSYM1_(U)                          \
    _KEYSYM1_(V)                          \
    _KEYSYM1_(W)                          \
    _KEYSYM1_(X)                          \
    _KEYSYM1_(Y)                          \
    _KEYSYM1_(Z)                          \
    _KEYSYM_(COMMA, Comma)                \
    _KEYSYM_(KPPLUS, Keypad_Plus)         \
    _KEYSYM_(KPMINUS, Keypad_Minus)       \
    _KEYSYM_(KPASTERISK, Keypad_Multiply) \
    _KEYSYM_(KPSLASH, Keypad_Divide)      \
    _KEYSYM_(KPENTER, Keypad_Enter)       \
    _KEYSYM_(KPDOT, Keypad_Period)        \
    _KEYSYM_(KPEQUAL, Keypad_Equals)      \
    _KEYSYM_(NUMLOCK, NumLock)            \
    _KEYSYM_(KP1, Keypad_1)               \
    _KEYSYM_(KP2, Keypad_2)               \
    _KEYSYM_(KP3, Keypad_3)               \
    _KEYSYM_(KP4, Keypad_4)               \
    _KEYSYM_(KP5, Keypad_5)               \
    _KEYSYM_(KP6, Keypad_6)               \
    _KEYSYM_(KP7, Keypad_7)               \
    _KEYSYM_(KP8, Keypad_8)               \
    _KEYSYM_(KP9, Keypad_9)               \
    _KEYSYM_(KP0, Keypad_0)               \
    _KEYSYM_(UP, Up)                      \
    _KEYSYM_(DOWN, Down)                  \
    _KEYSYM_(RIGHT, Right)                \
    _KEYSYM_(LEFT, Left)                  \
    _KEYSYM_(INSERT, Insert)              \
    _KEYSYM_(DELETE, Delete)              \
    _KEYSYM_(HOME, Home)                  \
    _KEYSYM_(HOMEPAGE, HomePage)          \
    _KEYSYM_(END, End)                    \
    _KEYSYM_(PAGEUP, PageUp)              \
    _KEYSYM_(PAGEDOWN, PageDown)          \
    _KEYSYM1_(F1)                         \
    _KEYSYM1_(F2)                         \
    _KEYSYM1_(F3)                         \
    _KEYSYM1_(F4)                         \
    _KEYSYM1_(F5)                         \
    _KEYSYM1_(F6)                         \
    _KEYSYM1_(F7)                         \
    _KEYSYM1_(F8)                         \
    _KEYSYM1_(F9)                         \
    _KEYSYM1_(F10)                        \
    _KEYSYM1_(F11)                        \
    _KEYSYM1_(F12)                        \
    _KEYSYM1_(F13)                        \
    _KEYSYM1_(F14)                        \
    _KEYSYM1_(F15)                        \
    _KEYSYM1_(PRINT)                      \
    _KEYSYM1_(BREAK)                      \
    _KEYSYM1_(PLAYPAUSE)                  \
    _KEYSYM1_(REWIND)                     \
    _KEYSYM1_(FASTFORWARD)                \
    _KEYSYM1_(SLEEP)                      \
    _KEYSYM1_ANDROID_(STEM_1)             \
    _KEYSYM1_ANDROID_(STEM_2)             \
    _KEYSYM1_ANDROID_(STEM_3)             \
    _KEYSYM1_ANDROID_(STEM_PRIMARY)       \
    _KEYSYM1_ANDROID_(APPSWITCH)

#define _KEYSYM_(x, y) {LINUX_KEY_##x, #y},
#define _KEYSYM_ANDROID_(x, y) {KEY_##x, #y},
static const struct { int  _sym; const char*  _str; }  keysym_names[] =
{
    _KEYSYM_LIST
};
#undef _KEYSYM_

static const int keysym_names_size =
        (int)(sizeof(keysym_names) / sizeof(keysym_names[0]));

int skin_key_code_count( void )
{
    return keysym_names_size;
}

const char*
skin_key_code_str( int  index )
{
    if (index >= 0 && index < keysym_names_size) {
        return keysym_names[index]._str;
    }
    return NULL;
}

const char* skin_key_pair_to_string(uint32_t keycode, uint32_t mod) {
    static char  temp[32];
    char*        p = temp;
    char*        end = p + sizeof(temp);
    int          nn;

    if ((mod & kKeyModLCtrl) != 0) {
        p = bufprint(p, end, "Ctrl-");
    }
    if ((mod & kKeyModRCtrl) != 0) {
        p = bufprint(p, end, "RCtrl-");
    }
    if ((mod & kKeyModLShift) != 0) {
        p = bufprint(p, end, "Shift-");
    }
    if ((mod & kKeyModRShift) != 0) {
        p = bufprint(p, end, "RShift-");
    }
    if ((mod & kKeyModLAlt) != 0) {
        p = bufprint(p, end, "Alt-");
    }
    if ((mod & kKeyModRAlt) != 0) {
        p = bufprint(p, end, "RAlt-");
    }
    if ((mod & kKeyModNumLock) != 0) {
        p = bufprint(p, end, "NumLock");
    }
    for (nn = 0; nn < keysym_names_size; ++nn) {
        if (keysym_names[nn]._sym == keycode) {
            p = bufprint(p, end, "%s", keysym_names[nn]._str);
            return temp;;
        }
    }

    if (keycode >= 32 && keycode <= 255) {
        p = bufprint(p, end, "%c", (int)keycode);
        return temp;
    }

    return NULL;
}

// Convert a textual key description |str| (e.g. "Ctrl-K") into a pair
// if (SkinKeyCode,SkinKeyMod) values. On success, return true and sets
// |*keycode| and |*mod|. On failure, return false.
bool skin_key_pair_from_string(const char* str,
                               uint32_t* pkeycode,
                               uint32_t* pmod) {
    uint32_t     mod = 0;
    int          match = 1;
    int          nn;
    static const struct { const char*  prefix; int  mod; }  mods[] =
    {
        { "^",      kKeyModLCtrl },
        { "Ctrl",   kKeyModLCtrl },
        { "ctrl",   kKeyModLCtrl },
        { "RCtrl",  kKeyModRCtrl },
        { "rctrl",  kKeyModRCtrl },
        { "Alt",    kKeyModLAlt },
        { "alt",    kKeyModLAlt },
        { "RAlt",   kKeyModRAlt },
        { "ralt",   kKeyModRAlt },
        { "Shift",  kKeyModLShift },
        { "shift",  kKeyModLShift },
        { "RShift", kKeyModRShift },
        { "rshift", kKeyModRShift },
        { NULL, 0 }
    };

    while (match) {
        match = 0;
        for (nn = 0; mods[nn].mod != 0; ++nn) {
            const char*  prefix = mods[nn].prefix;
            int          len    = strlen(prefix);

            if ( !strncmp(str, prefix, len) ) {
                str  += len;
                match = 1;
                mod  |= mods[nn].mod;
                if (str[0] == '-' && str[1] != 0)
                    str++;
                break;
            }
        }
    }

    for (nn = 0; nn < keysym_names_size; ++nn) {
#ifdef _WIN32
        if ( !stricmp(str, keysym_names[nn]._str) )
#else
        if ( !strcasecmp(str, keysym_names[nn]._str) )
#endif
        {
            *pkeycode = keysym_names[nn]._sym;
            *pmod = mod;
            return true;
        }
    }
    return false;
}

// the linux key code is alphabetical if it is within the range of the 3 rows on
// Qwerty keyboard.
bool skin_keycode_is_alpha(int32_t linux_key) {
    return (linux_key >= LINUX_KEY_Q && linux_key <= LINUX_KEY_P) ||
           (linux_key >= LINUX_KEY_A && linux_key <= LINUX_KEY_L) ||
           (linux_key >= LINUX_KEY_Z && linux_key <= LINUX_KEY_M);
}

bool skin_keycode_is_modifier(int32_t linux_key) {
    switch (linux_key) {
        case LINUX_KEY_LEFTCTRL:
        case LINUX_KEY_RIGHTCTRL:
        case LINUX_KEY_LEFTSHIFT:
        case LINUX_KEY_RIGHTSHIFT:
        case LINUX_KEY_LEFTALT:
        case LINUX_KEY_RIGHTALT:
        case LINUX_KEY_CAPSLOCK:
        case LINUX_KEY_NUMLOCK:
            return true;
        default:
            return false;
    }
}

typedef struct {
    // Contains one of the following:
    //  On Linux: XKB scancode
    //  On Windows: Windows OEM scancode
    //  On Mac: Mac keycode
    int native_scancode;
    int linux_keycode;
} LinuxKeyCodeMapEntry;

#define LINUX_KEYMAP_DECLARATION \
    const LinuxKeyCodeMapEntry linux_keycode_map[] =
#if defined(__linux__)
#define LINUX_KEYMAP(xkb, win, mac, code) \
    { xkb, code }
#elif defined(__APPLE__)
#define LINUX_KEYMAP(xkb, win, mac, code) \
    { mac, code }
#else
#define LINUX_KEYMAP(xkb, win, mac, code) \
    { win, code }
#endif
#include "android/skin/keycode_converter_data.inc"

#undef LINUX_KEYMAP
#undef LINUX_KEYMAP_DECLARATION

int skin_native_scancode_to_linux(int32_t scancode) {
    size_t nn;
    for (nn = 0; nn < ARRAYLEN(linux_keycode_map); nn++) {
        if (linux_keycode_map[nn].native_scancode == scancode) {
            return linux_keycode_map[nn].linux_keycode;
        }
    }
    return LINUX_KEY_RESERVED;  // 0
}

#if defined(__linux__)
static struct keypad_num_mapping {
    int32_t keypad;
    int32_t digit;
} sKeypadNumMapping[] = {
        {79, 16},  // KP_7 --> 7
        {80, 17},  // KP_8 --> 8
        {81, 18},  // KP_9 --> 9
        {83, 13},  // KP_4 --> 4
        {84, 14},  // KP_5 --> 5
        {85, 15},  // KP_6 --> 6
        {87, 10},  // KP_1 --> 1
        {88, 11},  // KP_2 --> 2
        {89, 12},  // KP_3 --> 3
        {90, 19},  // KP_0 --> 0
        {91, 60},  // KP_DOT --> DOT
};

#elif defined(__APPLE__)

static struct keypad_num_mapping {
    int32_t keypad;
    int32_t digit;
} sKeypadNumMapping[] = {
        {kVK_ANSI_Keypad7, kVK_ANSI_7},             // KP_7 --> 7
        {kVK_ANSI_Keypad8, kVK_ANSI_8},             // KP_8 --> 8
        {kVK_ANSI_Keypad9, kVK_ANSI_9},             // KP_9 --> 9
        {kVK_ANSI_Keypad4, kVK_ANSI_4},             // KP_4 --> 4
        {kVK_ANSI_Keypad5, kVK_ANSI_5},             // KP_5 --> 5
        {kVK_ANSI_Keypad6, kVK_ANSI_6},             // KP_6 --> 6
        {kVK_ANSI_Keypad1, kVK_ANSI_1},             // KP_1 --> 1
        {kVK_ANSI_Keypad2, kVK_ANSI_2},             // KP_2 --> 2
        {kVK_ANSI_Keypad3, kVK_ANSI_3},             // KP_3 --> 3
        {kVK_ANSI_Keypad0, kVK_ANSI_0},             // KP_0 --> 0
        {kVK_ANSI_KeypadDecimal, kVK_ANSI_Period},  // KP_DOT --> DOT
};

#else

static struct keypad_num_mapping {
    int32_t keypad;
    int32_t digit;
} sKeypadNumMapping[] = {
        {0x0047, 0x0008},  // KP_7 --> 7
        {0x0048, 0x0009},  // KP_8 --> 8
        {0x0049, 0x000a},  // KP_9 --> 9
        {0x004b, 0x0005},  // KP_4 --> 4
        {0x004c, 0x0006},  // KP_5 --> 5
        {0x004d, 0x0007},  // KP_6 --> 6
        {0x004f, 0x0002},  // KP_1 --> 1
        {0x0050, 0x0003},  // KP_2 --> 2
        {0x0051, 0x0004},  // KP_3 --> 3
        {0x0052, 0x000b},  // KP_0 --> 0
        {0x0053, 0x0034},  // KP_DOT --> DOT
};

#endif

bool skin_keycode_native_is_keypad(int32_t scancode) {
    size_t nn;
    for (nn = 0; nn < ARRAYLEN(sKeypadNumMapping); nn++) {
        if (scancode == sKeypadNumMapping[nn].keypad) {
            return true;
        }
    }
    return false;
}

int32_t skin_keycode_native_map_keypad(int32_t scancode) {
    size_t nn;
    for (nn = 0; nn < ARRAYLEN(sKeypadNumMapping); nn++) {
        if (scancode == sKeypadNumMapping[nn].keypad) {
            return sKeypadNumMapping[nn].digit;
        }
    }
    return LINUX_KEY_RESERVED;
}
