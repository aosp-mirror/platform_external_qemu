/* Copyright (C) 2019 The Android Open Source Project
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

#include "android/base/ArraySize.h"

#include <xcb/xcb.h>
// X11 scan code ranges from 8 to 255 with gaps in between
// Reference: run "xmodmap -pke" in terminal
// then only include those keys defined in frameworks/base/data/keyboards/qwerty.kl

bool skin_keycode_native_to_linux(int32_t* pVirtualKey, int32_t* pModifier) {
    static const struct {
        int virtualKey;
        int linuxKeyCode;
        bool addShiftMod = false;
    } kTbl[] = {
        { 9, LINUX_KEY_ESC },  // Escape NoSymbol Escape
        { 10, LINUX_KEY_1 },    // 1 exclam plus 1 exclam dead_tilde
        { 11, LINUX_KEY_2 },    // 2 at ecaron 2 at dead_caron
        { 12, LINUX_KEY_3 },  // 3 numbersign scaron 3 numbersign dead_circumflex
        { 13, LINUX_KEY_4 },  // 4 dollar ccaron 4 dollar dead_breve
        { 14, LINUX_KEY_5 },  // 5 percent rcaron 5 percent dead_abovering
        { 15, LINUX_KEY_6 },  // 6 asciicircum zcaron 6 asciicircum dead_ogonek
        { 16, LINUX_KEY_7 },  // 7 ampersand yacute 7 ampersand dead_grave
        { 17, LINUX_KEY_8 },  // 8 asterisk aacute 8 asterisk dead_abovedot
        { 18, LINUX_KEY_9 },  // 9 parenleft iacute 9 parenleft dead_acute
        { 19, LINUX_KEY_0 },  // 0 parenright eacute 0 parenright dead_doubleacute
        { 20, LINUX_KEY_MINUS },  // minus underscore equal percent backslash
                                 // dead_diaeresis
        { 21, LINUX_KEY_EQUAL },  // equal plus dead_acute dead_caron dead_macron
                                 // dead_cedilla
        { 22, LINUX_KEY_BACKSPACE },   // BackSpace BackSpace BackSpace BackSpace
        { 23, LINUX_KEY_TAB },         // Tab ISO_Left_Tab Tab ISO_Left_Tab
        { 24, LINUX_KEY_Q },           // q Q q Q backslash Greek_OMEGA
        { 25, LINUX_KEY_W },           // w W w W bar Lstroke
        { 26, LINUX_KEY_E },           // e E e E EuroSign E
        { 27, LINUX_KEY_R },           // r R r R paragraph registered
        { 28, LINUX_KEY_T },           // t T t T tslash Tslash
        { 29, LINUX_KEY_Y },           // y Y y Y leftarrow yen
        { 30, LINUX_KEY_U },           // u U u U downarrow uparrow
        { 31, LINUX_KEY_I },           // i I i I rightarrow idotless
        { 32, LINUX_KEY_O },           // o O o O oslash Oslash
        { 33, LINUX_KEY_P },           // p P p P thorn THORN
        { 34, LINUX_KEY_LEFTBRACE },   // bracketleft braceleft uacute slash
                                     // bracketleft braceleft
        { 35, LINUX_KEY_RIGHTBRACE },  // bracketright braceright parenright
                                      // parenleft bracketright braceright
        { 36, LINUX_KEY_ENTER },       // Return NoSymbol Return
        { 37, LINUX_KEY_LEFTCTRL },    // Control_L NoSymbol Control_L
        { 38, LINUX_KEY_A },           // a A a A asciitilde AE
        { 39, LINUX_KEY_S },           // s S s S dstroke section
        { 40, LINUX_KEY_D },           // d D d D Dstroke ETH
        { 41, LINUX_KEY_F },           // f F f F bracketleft ordfeminine
        { 42, LINUX_KEY_G },           // g G g G bracketright ENG
        { 43, LINUX_KEY_H },           // h H h H grave Hstroke
        { 44, LINUX_KEY_J },           // j J j J apostrophe dead_horn
        { 45, LINUX_KEY_K },           // k K k K lstroke ampersand
        { 46, LINUX_KEY_L },           // l L l L Lstroke Lstroke
        { 47, LINUX_KEY_SEMICOLON },  // semicolon colon uring quotedbl semicolon
                                     // colon
        { 48, LINUX_KEY_APOSTROPHE },  // apostrophe quotedbl section exclam
                                      // apostrophe ssharp
        { 49, LINUX_KEY_GREEN },  // grave asciitilde semicolon dead_abovering
                                 // grave asciitilde
        { 50, LINUX_KEY_LEFTSHIFT },  // Shift_L NoSymbol Shift_L
        { 51, LINUX_KEY_BACKSLASH },  // backslash bar backslash bar slash bar
        { 52, LINUX_KEY_Z },          // z Z z Z degree less
        { 53, LINUX_KEY_X },          // x X x X numbersign greater
        { 54, LINUX_KEY_C },          // c C c C ampersand copyright
        { 55, LINUX_KEY_V },          // v V v V at leftsinglequotemark
        { 56, LINUX_KEY_B },          // b B b B braceleft rightsinglequotemark
        { 57, LINUX_KEY_N },          // n N n N braceright N
        { 58, LINUX_KEY_M },          // m M m M asciicircum masculine
        { 59, LINUX_KEY_COMMA },      // comma less comma question less multiply
        { 60, LINUX_KEY_DOT },    // period greater period colon greater division
        { 61, LINUX_KEY_SLASH },  // slash question minus underscore asterisk
                                 // dead_abovedot
        { 62, LINUX_KEY_RIGHTSHIFT },  // Shift_R NoSymbol Shift_R
        { 63, LINUX_KEY_8, true },  // KP_Multiply KP_Multiply KP_Multiply
                                      // KP_Multiply KP_Multiply KP_Multiply
                                      // XF86ClearGrab KP_Multiply KP_Multiply
                                      // XF86ClearGrab
        { 64, LINUX_KEY_LEFTALT },   // Alt_L Meta_L Alt_L Meta_L
        { 65, LINUX_KEY_SPACE },     // space NoSymbol space space space space
        //{ 66, LINUX_KEY_CAPSLOCK },  // Caps_Lock ISO_Next_Group Caps_Lock
                                    // ISO_Next_Group
        //{ 77, LINUX_KEY_NUMLOCK },     // Num_Lock NoSymbol Num_Lock
        { 79, LINUX_KEY_KP7 },         // KP_Home KP_7 KP_Home KP_7
        { 80, LINUX_KEY_KP8 },         // KP_Up KP_8 KP_Up KP_8
        { 81, LINUX_KEY_KP9 },         // KP_Prior KP_9 KP_Prior KP_9
        { 82, LINUX_KEY_MINUS },     // KP_Subtract KP_Subtract KP_Subtract
                                   // KP_Subtract KP_Subtract KP_Subtract
                                   // XF86Prev_VMode KP_Subtract KP_Subtract
                                   // XF86Prev_VMode
        { 83, LINUX_KEY_KP4 },       // KP_Left KP_4 KP_Left KP_4
        { 84, LINUX_KEY_KP5 },       // KP_Begin KP_5 KP_Begin KP_5
        { 85, LINUX_KEY_KP6 },       // KP_Right KP_6 KP_Right KP_6
        { 86, LINUX_KEY_EQUAL, true },    // KP_Add KP_Add KP_Add KP_Add KP_Add KP_Add
                                  // XF86Next_VMode KP_Add KP_Add XF86Next_VMode
        { 87, LINUX_KEY_KP1 },       // KP_End KP_1 KP_End KP_1
        { 88, LINUX_KEY_KP2 },       // KP_Down KP_2 KP_Down KP_2
        { 89, LINUX_KEY_KP3 },       // KP_Next KP_3 KP_Next KP_3
        { 90, LINUX_KEY_KP0 },       // KP_Insert KP_0 KP_Insert KP_0
        { 91, LINUX_KEY_DELETE },     // KP_Delete KP_Decimal KP_Delete KP_Decimal
        { 92, LINUX_KEY_RIGHTALT },  // ISO_Level3_Shift NoSymbol
                                    // ISO_Level3_Shift
        //{  94, LINUX_KEY_KPPLUSMINUS },  // less greater backslash bar bar
                                       // brokenbar slash
        { 104, LINUX_KEY_ENTER },    // KP_Enter NoSymbol KP_Enter
        { 105, LINUX_KEY_RIGHTCTRL },  // Control_R NoSymbol Control_R
        { 106, LINUX_KEY_SLASH },    // KP_Divide KP_Divide KP_Divide KP_Divide
                                  // KP_Divide KP_Divide XF86Ungrab KP_Divide
                                  // KP_Divide XF86Ungrab
        { 108, LINUX_KEY_RIGHTALT },     // Alt_R Meta_R ISO_Level3_Shift
        { 110, LINUX_KEY_HOME },         // Home NoSymbol Home
        { 111, LINUX_KEY_UP },           // Up NoSymbol Up
        { 113, LINUX_KEY_LEFT },         // Left NoSymbol Left
        { 114, LINUX_KEY_RIGHT },        // Right NoSymbol Right
        //Do not include LINUX_KEY_END because it will be effectively an ENDCALL which turns off the screen.
        //{ 115, LINUX_KEY_END },          // End NoSymbol End
        { 116, LINUX_KEY_DOWN },         // Down NoSymbol Down
        { 122, LINUX_KEY_VOLUMEDOWN },   // XF86AudioLowerVolume NoSymbol
                                     // XF86AudioLowerVolume
        { 123, LINUX_KEY_VOLUMEUP },     // XF86AudioRaiseVolume NoSymbol
                                   // XF86AudioRaiseVolume
        { 124, LINUX_KEY_POWER },        // XF86PowerOff NoSymbol XF86PowerOff
        { 125, LINUX_KEY_KPEQUAL },      // KP_Equal NoSymbol KP_Equal
        { 126, LINUX_KEY_KPPLUSMINUS },  // plusminus NoSymbol plusminus
        { 129, LINUX_KEY_KPDOT },    // KP_Decimal KP_Decimal KP_Decimal KP_Decimal
        { 135, LINUX_KEY_MENU },          // Menu NoSymbol Menu
        { 150, LINUX_KEY_SLEEP },         // XF86Sleep NoSymbol XF86Sleep
        { 152, LINUX_KEY_WWW  },          //XF86Explorer NoSymbol XF86Explorer
        { 153, LINUX_KEY_SEND },          // XF86Send NoSymbol XF86Send
        { 158, LINUX_KEY_WWW },           // XF86WWW NoSymbol XF86WWW
        { 163, LINUX_KEY_MAIL },          // XF86Mail NoSymbol XF86Mail
        { 166, LINUX_KEY_BACK },          // XF86Back NoSymbol XF86Back
        { 169, LINUX_KEY_EJECTCD },       // XF86Eject NoSymbol XF86Eject
        { 170, LINUX_KEY_EJECTCLOSECD },  // XF86Eject XF86Eject XF86Eject
                                       // XF86Eject
        { 171, LINUX_KEY_NEXTSONG },      // XF86AudioNext NoSymbol XF86AudioNext
        { 172, LINUX_KEY_PLAYPAUSE },     // XF86AudioPlay XF86AudioPause
                                    // XF86AudioPlay XF86AudioPause
        { 173, LINUX_KEY_PREVIOUSSONG },  // XF86AudioPrev NoSymbol XF86AudioPrev
        { 174, LINUX_KEY_STOPCD },        // XF86AudioStop XF86Eject XF86AudioStop
                                 // XF86Eject
        { 175, LINUX_KEY_RECORD },      // XF86AudioRecord NoSymbol XF86AudioRecord
        { 176, LINUX_KEY_REWIND },      // XF86AudioRewind NoSymbol XF86AudioRewind
        { 182, LINUX_KEY_EXIT },        // XF86Close NoSymbol XF86Close
        { 187, LINUX_KEY_KPLEFTPAREN },   // parenleft NoSymbol parenleft
        { 188, LINUX_KEY_KPRIGHTPAREN },  // parenright NoSymbol parenright
        { 204, LINUX_KEY_LEFTALT },   // NoSymbol Alt_L NoSymbol Alt_L
        { 208, LINUX_KEY_PLAYCD },    // XF86AudioPlay NoSymbol XF86AudioPlay
        { 209, LINUX_KEY_PAUSECD },   // XF86AudioPause NoSymbol XF86AudioPause
        { 213, LINUX_KEY_SUSPEND },   // XF86Suspend NoSymbol XF86Suspend
        { 215, LINUX_KEY_PLAY },      // XF86AudioPlay NoSymbol XF86AudioPlay
        { 216, LINUX_KEY_FASTFORWARD },     // XF86AudioForward NoSymbol
                                      // XF86AudioForward
        { 220, LINUX_KEY_CAMERA },          // XF86WebCam NoSymbol XF86WebCam
        { 223, LINUX_KEY_MAIL },           // XF86Mail NoSymbol XF86Mail
        { 225, LINUX_KEY_SEARCH },          // XF86Search NoSymbol XF86Search
    };
    size_t nn;
    for (nn = 0; nn < android::base::arraySize(kTbl); ++nn) {
        if (*pVirtualKey == kTbl[nn].virtualKey) {
            *pVirtualKey = kTbl[nn].linuxKeyCode;
            if (kTbl[nn].addShiftMod) {
                *pModifier |= XCB_MOD_MASK_SHIFT;
            }
            return true;
        }
    }
    return false;
}

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

bool skin_keycode_native_is_keypad(int32_t virtualKey) {
    for (size_t nn = 0; nn < android::base::arraySize(sKeypadNumMapping);
         nn++) {
        if (virtualKey == sKeypadNumMapping[nn].keypad) {
            return true;
        }
    }
    return false;
}

int32_t skin_keycode_native_map_keypad(int32_t virtualKey) {
    for (size_t nn = 0; nn < android::base::arraySize(sKeypadNumMapping);
         nn++) {
        if (virtualKey == sKeypadNumMapping[nn].keypad) {
            return sKeypadNumMapping[nn].digit;
        }
    }
    return -1;
}
