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

#include "android/utils/bufprint.h"

#if defined(__APPLE__)
#include "android/skin/mac_keycodes.h"
#endif

#include <stddef.h>
#include <string.h>

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

int32_t skin_native_to_linux_keycode(int32_t scancode) {
#if defined(__linux__)
    // X11 scan code ranges from 8 to 255 with gaps in between
    // First listed by by "xmodmap -pke" then only include those keys
    // defined in frameworks/base/data/keyboards/qwerty2.kl
    static const struct {
        int scancode;
        int linuxKeyCode;
    } kTbl[] = {
        {   9, LINUX_KEY_ESC},  // Escape NoSymbol Escape
        {  10, LINUX_KEY_1},    // 1 exclam plus 1 exclam dead_tilde
        {  11, LINUX_KEY_2},    // 2 at ecaron 2 at dead_caron
        {  12, LINUX_KEY_3},  // 3 numbersign scaron 3 numbersign dead_circumflex
        {  13, LINUX_KEY_4},  // 4 dollar ccaron 4 dollar dead_breve
        {  14, LINUX_KEY_5},  // 5 percent rcaron 5 percent dead_abovering
        {  15, LINUX_KEY_6},  // 6 asciicircum zcaron 6 asciicircum dead_ogonek
        {  16, LINUX_KEY_7},  // 7 ampersand yacute 7 ampersand dead_grave
        {  17, LINUX_KEY_8},  // 8 asterisk aacute 8 asterisk dead_abovedot
        {  18, LINUX_KEY_9},  // 9 parenleft iacute 9 parenleft dead_acute
        {  19, LINUX_KEY_0},  // 0 parenright eacute 0 parenright dead_doubleacute
        {  20, LINUX_KEY_MINUS},  // minus underscore equal percent backslash
                                 // dead_diaeresis
        {  21, LINUX_KEY_EQUAL},  // equal plus dead_acute dead_caron dead_macron
                                 // dead_cedilla
        {  22, LINUX_KEY_BACKSPACE},   // BackSpace BackSpace BackSpace BackSpace
        {  23, LINUX_KEY_TAB},         // Tab ISO_Left_Tab Tab ISO_Left_Tab
        {  24, LINUX_KEY_Q},           // q Q q Q backslash Greek_OMEGA
        {  25, LINUX_KEY_W},           // w W w W bar Lstroke
        {  26, LINUX_KEY_E},           // e E e E EuroSign E
        {  27, LINUX_KEY_R},           // r R r R paragraph registered
        {  28, LINUX_KEY_T},           // t T t T tslash Tslash
        {  29, LINUX_KEY_Y},           // y Y y Y leftarrow yen
        {  30, LINUX_KEY_U},           // u U u U downarrow uparrow
        {  31, LINUX_KEY_I},           // i I i I rightarrow idotless
        {  32, LINUX_KEY_O},           // o O o O oslash Oslash
        {  33, LINUX_KEY_P},           // p P p P thorn THORN
        {  34, LINUX_KEY_LEFTBRACE},   // bracketleft braceleft uacute slash
                                     // bracketleft braceleft
        {  35, LINUX_KEY_RIGHTBRACE},  // bracketright braceright parenright
                                      // parenleft bracketright braceright
        {  36, LINUX_KEY_ENTER},       // Return NoSymbol Return
        {  37, LINUX_KEY_LEFTCTRL},    // Control_L NoSymbol Control_L
        {  38, LINUX_KEY_A},           // a A a A asciitilde AE
        {  39, LINUX_KEY_S},           // s S s S dstroke section
        {  40, LINUX_KEY_D},           // d D d D Dstroke ETH
        {  41, LINUX_KEY_F},           // f F f F bracketleft ordfeminine
        {  42, LINUX_KEY_G},           // g G g G bracketright ENG
        {  43, LINUX_KEY_H},           // h H h H grave Hstroke
        {  44, LINUX_KEY_J},           // j J j J apostrophe dead_horn
        {  45, LINUX_KEY_K},           // k K k K lstroke ampersand
        {  46, LINUX_KEY_L},           // l L l L Lstroke Lstroke
        {  47, LINUX_KEY_SEMICOLON},  // semicolon colon uring quotedbl semicolon
                                     // colon
        {  48, LINUX_KEY_APOSTROPHE},  // apostrophe quotedbl section exclam
                                      // apostrophe ssharp
        {  49, LINUX_KEY_GREEN},  // grave asciitilde semicolon dead_abovering
                                 // grave asciitilde
        {  50, LINUX_KEY_LEFTSHIFT},  // Shift_L NoSymbol Shift_L
        {  51, LINUX_KEY_BACKSLASH},  // backslash bar backslash bar slash bar
        {  52, LINUX_KEY_Z},          // z Z z Z degree less
        {  53, LINUX_KEY_X},          // x X x X numbersign greater
        {  54, LINUX_KEY_C},          // c C c C ampersand copyright
        {  55, LINUX_KEY_V},          // v V v V at leftsinglequotemark
        {  56, LINUX_KEY_B},          // b B b B braceleft rightsinglequotemark
        {  57, LINUX_KEY_N},          // n N n N braceright N
        {  58, LINUX_KEY_M},          // m M m M asciicircum masculine
        {  59, LINUX_KEY_COMMA},      // comma less comma question less multiply
        {  60, LINUX_KEY_DOT},    // period greater period colon greater division
        {  61, LINUX_KEY_SLASH},  // slash question minus underscore asterisk
                                 // dead_abovedot
        {  62, LINUX_KEY_RIGHTSHIFT},  // Shift_R NoSymbol Shift_R
        //{  63, LINUX_KEY_KPASTERISK},  // KP_Multiply KP_Multiply KP_Multiply
                                      // KP_Multiply KP_Multiply KP_Multiply
                                      // XF86ClearGrab KP_Multiply KP_Multiply
                                      // XF86ClearGrab
        {  64, LINUX_KEY_LEFTALT},   // Alt_L Meta_L Alt_L Meta_L
        {  65, LINUX_KEY_SPACE},     // space NoSymbol space space space space
        //{  66, LINUX_KEY_CAPSLOCK},  // Caps_Lock ISO_Next_Group Caps_Lock
                                    // ISO_Next_Group
        //{  77, LINUX_KEY_NUMLOCK},     // Num_Lock NoSymbol Num_Lock
        {  79, LINUX_KEY_KP7},         // KP_Home KP_7 KP_Home KP_7
        {  80, LINUX_KEY_KP8},         // KP_Up KP_8 KP_Up KP_8
        {  81, LINUX_KEY_KP9},         // KP_Prior KP_9 KP_Prior KP_9
        {  82, LINUX_KEY_KPMINUS},     // KP_Subtract KP_Subtract KP_Subtract
                                   // KP_Subtract KP_Subtract KP_Subtract
                                   // XF86Prev_VMode KP_Subtract KP_Subtract
                                   // XF86Prev_VMode
        {  83, LINUX_KEY_KP4},       // KP_Left KP_4 KP_Left KP_4
        {  84, LINUX_KEY_KP5},       // KP_Begin KP_5 KP_Begin KP_5
        {  85, LINUX_KEY_KP6},       // KP_Right KP_6 KP_Right KP_6
        {  86, LINUX_KEY_KPPLUS},    // KP_Add KP_Add KP_Add KP_Add KP_Add KP_Add
                                  // XF86Next_VMode KP_Add KP_Add XF86Next_VMode
        {  87, LINUX_KEY_KP1},       // KP_End KP_1 KP_End KP_1
        {  88, LINUX_KEY_KP2},       // KP_Down KP_2 KP_Down KP_2
        {  89, LINUX_KEY_KP3},       // KP_Next KP_3 KP_Next KP_3
        {  90, LINUX_KEY_KP0},       // KP_Insert KP_0 KP_Insert KP_0
        {  91, LINUX_KEY_KPDOT},     // KP_Delete KP_Decimal KP_Delete KP_Decimal
        {  92, LINUX_KEY_RIGHTALT},  // ISO_Level3_Shift NoSymbol
                                    // ISO_Level3_Shift
        //{  94, LINUX_KEY_KPPLUSMINUS},  // less greater backslash bar bar
                                       // brokenbar slash
        { 102, LINUX_KEY_MUHENKAN},   // Muhenkan NoSymbol Muhenkan
        { 104, LINUX_KEY_KPENTER},    // KP_Enter NoSymbol KP_Enter
        { 105, LINUX_KEY_RIGHTCTRL},  // Control_R NoSymbol Control_R
        { 106, LINUX_KEY_KPSLASH},    // KP_Divide KP_Divide KP_Divide KP_Divide
                                  // KP_Divide KP_Divide XF86Ungrab KP_Divide
                                  // KP_Divide XF86Ungrab
        { 108, LINUX_KEY_RIGHTALT},     // Alt_R Meta_R ISO_Level3_Shift
        { 110, LINUX_KEY_HOME},         // Home NoSymbol Home
        { 111, LINUX_KEY_UP},           // Up NoSymbol Up
        { 113, LINUX_KEY_LEFT},         // Left NoSymbol Left
        { 114, LINUX_KEY_RIGHT},        // Right NoSymbol Right
        //Do not include LINUX_KEY_END because it will be effectively an ENDCALL which turns off the screen.
        //{ 115, LINUX_KEY_END},          // End NoSymbol End
        { 116, LINUX_KEY_DOWN},         // Down NoSymbol Down
        { 122, LINUX_KEY_VOLUMEDOWN},   // XF86AudioLowerVolume NoSymbol
                                     // XF86AudioLowerVolume
        { 123, LINUX_KEY_VOLUMEUP},     // XF86AudioRaiseVolume NoSymbol
                                   // XF86AudioRaiseVolume
        { 124, LINUX_KEY_POWER},        // XF86PowerOff NoSymbol XF86PowerOff
        { 125, LINUX_KEY_KPEQUAL},      // KP_Equal NoSymbol KP_Equal
        { 126, LINUX_KEY_KPPLUSMINUS},  // plusminus NoSymbol plusminus
        { 129, LINUX_KEY_KPDOT},    // KP_Decimal KP_Decimal KP_Decimal KP_Decimal
        { 135, LINUX_KEY_MENU},          // Menu NoSymbol Menu
        { 150, LINUX_KEY_SLEEP},         // XF86Sleep NoSymbol XF86Sleep
        { 152, LINUX_KEY_WWW },          //XF86Explorer NoSymbol XF86Explorer
        { 153, LINUX_KEY_SEND},          // XF86Send NoSymbol XF86Send
        { 158, LINUX_KEY_WWW},           // XF86WWW NoSymbol XF86WWW
        { 163, LINUX_KEY_MAIL},          // XF86Mail NoSymbol XF86Mail
        { 166, LINUX_KEY_BACK},          // XF86Back NoSymbol XF86Back
        { 169, LINUX_KEY_EJECTCD},       // XF86Eject NoSymbol XF86Eject
        { 170, LINUX_KEY_EJECTCLOSECD},  // XF86Eject XF86Eject XF86Eject
                                       // XF86Eject
        { 171, LINUX_KEY_NEXTSONG},      // XF86AudioNext NoSymbol XF86AudioNext
        { 172, LINUX_KEY_PLAYPAUSE},     // XF86AudioPlay XF86AudioPause
                                    // XF86AudioPlay XF86AudioPause
        { 173, LINUX_KEY_PREVIOUSSONG},  // XF86AudioPrev NoSymbol XF86AudioPrev
        { 174, LINUX_KEY_STOPCD},        // XF86AudioStop XF86Eject XF86AudioStop
                                 // XF86Eject
        { 175, LINUX_KEY_RECORD},      // XF86AudioRecord NoSymbol XF86AudioRecord
        { 176, LINUX_KEY_REWIND},      // XF86AudioRewind NoSymbol XF86AudioRewind
        { 182, LINUX_KEY_EXIT},        // XF86Close NoSymbol XF86Close
        { 187, LINUX_KEY_KPLEFTPAREN},   // parenleft NoSymbol parenleft
        { 188, LINUX_KEY_KPRIGHTPAREN},  // parenright NoSymbol parenright
        { 204, LINUX_KEY_LEFTALT},   // NoSymbol Alt_L NoSymbol Alt_L
        { 208, LINUX_KEY_PLAYCD},    // XF86AudioPlay NoSymbol XF86AudioPlay
        { 209, LINUX_KEY_PAUSECD},   // XF86AudioPause NoSymbol XF86AudioPause
        { 213, LINUX_KEY_SUSPEND},   // XF86Suspend NoSymbol XF86Suspend
        { 215, LINUX_KEY_PLAY},      // XF86AudioPlay NoSymbol XF86AudioPlay
        { 216, LINUX_KEY_FASTFORWARD},     // XF86AudioForward NoSymbol
                                      // XF86AudioForward
        { 220, LINUX_KEY_CAMERA},          // XF86WebCam NoSymbol XF86WebCam
        { 223, LINUX_KEY_MAIL},           // XF86Mail NoSymbol XF86Mail
        { 225, LINUX_KEY_SEARCH},          // XF86Search NoSymbol XF86Search
    };

#elif defined(__APPLE__)
    //include those keys defined in frameworks/base/data/keyboards/qwerty2.kl
    static const struct {
        int scancode;
        int linuxKeyCode;
    } kTbl[] = {
        {kMacOS_VK_ANSI_A, LINUX_KEY_A},
        {kMacOS_VK_ANSI_S, LINUX_KEY_S},
        {kMacOS_VK_ANSI_D, LINUX_KEY_D},
        {kMacOS_VK_ANSI_F, LINUX_KEY_F},
        {kMacOS_VK_ANSI_H, LINUX_KEY_H},
        {kMacOS_VK_ANSI_G, LINUX_KEY_G},
        {kMacOS_VK_ANSI_Z, LINUX_KEY_Z},
        {kMacOS_VK_ANSI_X, LINUX_KEY_X},
        {kMacOS_VK_ANSI_C, LINUX_KEY_C},
        {kMacOS_VK_ANSI_V, LINUX_KEY_V},
        {kMacOS_VK_ANSI_B, LINUX_KEY_B},
        {kMacOS_VK_ANSI_Q, LINUX_KEY_Q},
        {kMacOS_VK_ANSI_W, LINUX_KEY_W},
        {kMacOS_VK_ANSI_E, LINUX_KEY_E},
        {kMacOS_VK_ANSI_R, LINUX_KEY_R},
        {kMacOS_VK_ANSI_Y, LINUX_KEY_Y},
        {kMacOS_VK_ANSI_T, LINUX_KEY_T},
        {kMacOS_VK_ANSI_1, LINUX_KEY_1},
        {kMacOS_VK_ANSI_2, LINUX_KEY_2},
        {kMacOS_VK_ANSI_3, LINUX_KEY_3},
        {kMacOS_VK_ANSI_4, LINUX_KEY_4},
        {kMacOS_VK_ANSI_6, LINUX_KEY_6},
        {kMacOS_VK_ANSI_5, LINUX_KEY_5},
        {kMacOS_VK_ANSI_Equal, LINUX_KEY_EQUAL},
        {kMacOS_VK_ANSI_9, LINUX_KEY_9},
        {kMacOS_VK_ANSI_7, LINUX_KEY_7},
        {kMacOS_VK_ANSI_Minus, LINUX_KEY_MINUS},
        {kMacOS_VK_ANSI_8, LINUX_KEY_8},
        {kMacOS_VK_ANSI_0, LINUX_KEY_0},
        {kMacOS_VK_ANSI_RightBracket, LINUX_KEY_RIGHTBRACE},
        {kMacOS_VK_ANSI_O, LINUX_KEY_0},
        {kMacOS_VK_ANSI_U, LINUX_KEY_U},
        {kMacOS_VK_ANSI_LeftBracket, LINUX_KEY_LEFTBRACE},
        {kMacOS_VK_ANSI_I, LINUX_KEY_I},
        {kMacOS_VK_ANSI_P, LINUX_KEY_P},
        {kMacOS_VK_ANSI_L, LINUX_KEY_L},
        {kMacOS_VK_ANSI_J, LINUX_KEY_J},
        {kMacOS_VK_ANSI_Quote, LINUX_KEY_APOSTROPHE},
        {kMacOS_VK_ANSI_K, LINUX_KEY_K},
        {kMacOS_VK_ANSI_Semicolon, LINUX_KEY_SEMICOLON},
        {kMacOS_VK_ANSI_Backslash, LINUX_KEY_BACKSLASH},
        {kMacOS_VK_ANSI_Comma, LINUX_KEY_COMMA},
        {kMacOS_VK_ANSI_Slash, LINUX_KEY_SLASH},
        {kMacOS_VK_ANSI_N, LINUX_KEY_N},
        {kMacOS_VK_ANSI_M, LINUX_KEY_M},
        {kMacOS_VK_ANSI_Period, LINUX_KEY_DOT},
        {kMacOS_VK_ANSI_Grave, LINUX_KEY_GREEN},
        {kMacOS_VK_ANSI_KeypadDecimal, LINUX_KEY_KPDOT},
        {kMacOS_VK_ANSI_KeypadMultiply, LINUX_KEY_KPASTERISK},
        {kMacOS_VK_ANSI_KeypadPlus, LINUX_KEY_KPPLUS},
        //{ kMacOS_VK_ANSI_KeypadClear, LINUX_KEY_UNKNOWN },
        {kMacOS_VK_ANSI_KeypadDivide, LINUX_KEY_KPSLASH},
        {kMacOS_VK_ANSI_KeypadEnter, LINUX_KEY_KPENTER},
        {kMacOS_VK_ANSI_KeypadMinus, LINUX_KEY_KPMINUS},
        {kMacOS_VK_ANSI_KeypadEquals, LINUX_KEY_KPEQUAL},
        {kMacOS_VK_ANSI_Keypad0, LINUX_KEY_KP0},
        {kMacOS_VK_ANSI_Keypad1, LINUX_KEY_KP1},
        {kMacOS_VK_ANSI_Keypad2, LINUX_KEY_KP2},
        {kMacOS_VK_ANSI_Keypad3, LINUX_KEY_KP3},
        {kMacOS_VK_ANSI_Keypad4, LINUX_KEY_KP4},
        {kMacOS_VK_ANSI_Keypad5, LINUX_KEY_KP5},
        {kMacOS_VK_ANSI_Keypad6, LINUX_KEY_KP6},
        {kMacOS_VK_ANSI_Keypad7, LINUX_KEY_KP7},
        {kMacOS_VK_ANSI_Keypad8, LINUX_KEY_KP8},
        {kMacOS_VK_ANSI_Keypad9, LINUX_KEY_KP9},
        {kMacOS_VK_Return, LINUX_KEY_ENTER},
        {kMacOS_VK_Tab, LINUX_KEY_TAB},
        {kMacOS_VK_Space, LINUX_KEY_SPACE},
        {kMacOS_VK_Delete, LINUX_KEY_BACKSPACE},
        {kMacOS_VK_Escape, LINUX_KEY_ESC},
        {kMacOS_VK_Command, LINUX_KEY_LEFTCTRL},
        {kMacOS_VK_Shift, LINUX_KEY_LEFTSHIFT},
        //{kMacOS_VK_CapsLock, LINUX_KEY_CAPSLOCK},
        {kMacOS_VK_Option, LINUX_KEY_LEFTALT},
        {kMacOS_VK_Control, LINUX_KEY_LEFTCTRL},
        {kMacOS_VK_RightCommand, LINUX_KEY_RIGHTCTRL},
        {kMacOS_VK_RightShift, LINUX_KEY_RIGHTSHIFT},
        {kMacOS_VK_RightOption, LINUX_KEY_RIGHTALT},
        {kMacOS_VK_RightControl, LINUX_KEY_RIGHTCTRL},
        //{ kMacOS_VK_Function     , LINUX_KEY_UNKNOWN },
        {kMacOS_VK_VolumeUp, LINUX_KEY_VOLUMEUP},
        {kMacOS_VK_VolumeDown, LINUX_KEY_VOLUMEDOWN},
        {kMacOS_VK_Home, LINUX_KEY_HOME},
        {kMacOS_VK_LeftArrow, LINUX_KEY_LEFT},
        {kMacOS_VK_RightArrow, LINUX_KEY_RIGHT},
        {kMacOS_VK_DownArrow, LINUX_KEY_DOWN},
        {kMacOS_VK_UpArrow, LINUX_KEY_UP},
    };
#else
    return -1;
#endif
    const size_t kTblSize = sizeof(kTbl) / sizeof(kTbl[0]);
    size_t nn;
    for (nn = 0; nn < kTblSize; ++nn) {
        if (scancode == kTbl[nn].scancode) {
            return kTbl[nn].linuxKeyCode;
        }
    }
    return -1;
}

bool skin_keycode_is_aplha(int32_t linux_key) {
    return (linux_key >= LINUX_KEY_Q && linux_key <= LINUX_KEY_P) ||
           (linux_key >= LINUX_KEY_A && linux_key <= LINUX_KEY_L) ||
           (linux_key >= LINUX_KEY_Z && linux_key <= LINUX_KEY_M);
}
