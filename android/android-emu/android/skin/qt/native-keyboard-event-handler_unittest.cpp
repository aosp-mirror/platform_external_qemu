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

#include "android/skin/qt/native-keyboard-event-handler.h"

#include <gtest/gtest.h>
#include <stddef.h>

#include "android/skin/event.h"
#include "android/skin/keycode.h"
#include "android/skin/linux_keycodes.h"

#define ARRAYLEN(x) (sizeof(x) / sizeof(x[0]))

namespace {

#if defined(__linux__)
#include <X11/XKBlib.h>
#include <xcb/xcb.h>

const struct {
    int scancode;
    int linuxKeyCode;
} kHandleKeyEventTestData[] = {
        {10, LINUX_KEY_1},          {11, LINUX_KEY_2},
        {12, LINUX_KEY_3},          {13, LINUX_KEY_4},
        {14, LINUX_KEY_5},          {15, LINUX_KEY_6},
        {16, LINUX_KEY_7},          {17, LINUX_KEY_8},
        {18, LINUX_KEY_9},          {19, LINUX_KEY_0},
        {20, LINUX_KEY_MINUS},      {21, LINUX_KEY_EQUAL},
        {22, LINUX_KEY_BACKSPACE},  {23, LINUX_KEY_TAB},
        {24, LINUX_KEY_Q},          {25, LINUX_KEY_W},
        {26, LINUX_KEY_E},          {27, LINUX_KEY_R},
        {28, LINUX_KEY_T},          {29, LINUX_KEY_Y},
        {30, LINUX_KEY_U},          {31, LINUX_KEY_I},
        {32, LINUX_KEY_O},          {33, LINUX_KEY_P},
        {34, LINUX_KEY_LEFTBRACE},

        {35, LINUX_KEY_RIGHTBRACE}, {36, LINUX_KEY_ENTER},
        {38, LINUX_KEY_A},          {39, LINUX_KEY_S},
        {40, LINUX_KEY_D},          {41, LINUX_KEY_F},
        {42, LINUX_KEY_G},          {43, LINUX_KEY_H},
        {44, LINUX_KEY_J},          {45, LINUX_KEY_K},
        {46, LINUX_KEY_L},          {47, LINUX_KEY_SEMICOLON},

        {48, LINUX_KEY_APOSTROPHE}, {49, LINUX_KEY_GREEN},
        {51, LINUX_KEY_BACKSLASH},  {52, LINUX_KEY_Z},
        {53, LINUX_KEY_X},          {54, LINUX_KEY_C},
        {55, LINUX_KEY_V},          {56, LINUX_KEY_B},
        {57, LINUX_KEY_N},          {58, LINUX_KEY_M},
        {59, LINUX_KEY_COMMA},      {60, LINUX_KEY_DOT},
        {61, LINUX_KEY_SLASH},
};

const struct {
    int inputModifier;
    int outputModifer;
} kModifierTestData[] = {{XCB_MOD_MASK_SHIFT, kKeyModLShift},
                         {XCB_MOD_MASK_LOCK, kKeyModCapsLock},
                         {XCB_MOD_MASK_CONTROL, kKeyModLCtrl},
                         {XCB_MOD_MASK_SHIFT | XCB_MOD_MASK_LOCK,
                          kKeyModLShift | kKeyModCapsLock}};

#elif defined(__APPLE__)

#include "android/skin/macos_keycodes.h"

#include <Carbon/Carbon.h>
const struct {
    int scancode;
    int linuxKeyCode;
} kHandleKeyEventTestData[] = {
        {kVK_ANSI_A, LINUX_KEY_A},
        {kVK_ANSI_S, LINUX_KEY_S},
        {kVK_ANSI_D, LINUX_KEY_D},
        {kVK_ANSI_F, LINUX_KEY_F},
        {kVK_ANSI_H, LINUX_KEY_H},
        {kVK_ANSI_G, LINUX_KEY_G},
        {kVK_ANSI_Z, LINUX_KEY_Z},
        {kVK_ANSI_X, LINUX_KEY_X},
        {kVK_ANSI_C, LINUX_KEY_C},
        {kVK_ANSI_V, LINUX_KEY_V},
        {kVK_ANSI_B, LINUX_KEY_B},
        {kVK_ANSI_Q, LINUX_KEY_Q},
        {kVK_ANSI_W, LINUX_KEY_W},
        {kVK_ANSI_E, LINUX_KEY_E},
        {kVK_ANSI_R, LINUX_KEY_R},
        {kVK_ANSI_Y, LINUX_KEY_Y},
        {kVK_ANSI_T, LINUX_KEY_T},
        {kVK_ANSI_1, LINUX_KEY_1},
        {kVK_ANSI_2, LINUX_KEY_2},
        {kVK_ANSI_3, LINUX_KEY_3},
        {kVK_ANSI_4, LINUX_KEY_4},
        {kVK_ANSI_6, LINUX_KEY_6},
        {kVK_ANSI_5, LINUX_KEY_5},
        {kVK_ANSI_Equal, LINUX_KEY_EQUAL},
        {kVK_ANSI_9, LINUX_KEY_9},
        {kVK_ANSI_7, LINUX_KEY_7},
        {kVK_ANSI_Minus, LINUX_KEY_MINUS},
        {kVK_ANSI_8, LINUX_KEY_8},
        {kVK_ANSI_0, LINUX_KEY_0},
        {kVK_ANSI_RightBracket, LINUX_KEY_RIGHTBRACE},
        {kVK_ANSI_O, LINUX_KEY_O},
        {kVK_ANSI_U, LINUX_KEY_U},
        {kVK_ANSI_LeftBracket, LINUX_KEY_LEFTBRACE},
        {kVK_ANSI_I, LINUX_KEY_I},
        {kVK_ANSI_P, LINUX_KEY_P},
        {kVK_ANSI_L, LINUX_KEY_L},
        {kVK_ANSI_J, LINUX_KEY_J},
        {kVK_ANSI_Quote, LINUX_KEY_APOSTROPHE},
        {kVK_ANSI_K, LINUX_KEY_K},
        {kVK_ANSI_Semicolon, LINUX_KEY_SEMICOLON},
        {kVK_ANSI_Backslash, LINUX_KEY_BACKSLASH},
        {kVK_ANSI_Comma, LINUX_KEY_COMMA},
        {kVK_ANSI_Slash, LINUX_KEY_SLASH},
        {kVK_ANSI_N, LINUX_KEY_N},
        {kVK_ANSI_M, LINUX_KEY_M},
        {kVK_ANSI_Period, LINUX_KEY_DOT},
        {kVK_ANSI_Grave, LINUX_KEY_GREEN},
};

const struct {
    int inputModifier;
    int outputModifer;
} kModifierTestData[] = {
        {NSEventModifierFlagShift, kKeyModLShift},
        {NSEventModifierFlagCapsLock, kKeyModCapsLock},
        {NSEventModifierFlagControl, kKeyModLCtrl},
        {NSEventModifierFlagShift | NSEventModifierFlagCapsLock,
         kKeyModLShift | kKeyModCapsLock}};

#else  // Windows

const struct {
    int scancode;
    int linuxKeyCode;
} kHandleKeyEventTestData[] = {
        {0x001e, LINUX_KEY_A},  // aA
        {0x0030, LINUX_KEY_B},  // bB
        {0x002e, LINUX_KEY_C},  // cC
        {0x0020, LINUX_KEY_D},  // dD

        {0x0012, LINUX_KEY_E},  // eE
        {0x0021, LINUX_KEY_F},  // fF
        {0x0022, LINUX_KEY_G},  // gG
        {0x0023, LINUX_KEY_H},  // hH
        {0x0017, LINUX_KEY_I},  // iI
        {0x0024, LINUX_KEY_J},  // jJ
        {0x0025, LINUX_KEY_K},  // kK
        {0x0026, LINUX_KEY_L},  // lL

        {0x0032, LINUX_KEY_M},  // mM
        {0x0031, LINUX_KEY_N},  // nN
        {0x0018, LINUX_KEY_O},  // oO
        {0x0019, LINUX_KEY_P},  // pP
        {0x0010, LINUX_KEY_Q},  // qQ
        {0x0013, LINUX_KEY_R},  // rR
        {0x001f, LINUX_KEY_S},  // sS
        {0x0014, LINUX_KEY_T},  // tT

        {0x0016, LINUX_KEY_U},  // uU
        {0x002f, LINUX_KEY_V},  // vV
        {0x0011, LINUX_KEY_W},  // wW
        {0x002d, LINUX_KEY_X},  // xX
        {0x0015, LINUX_KEY_Y},  // yY
        {0x002c, LINUX_KEY_Z},  // zZ
        {0x0002, LINUX_KEY_1},  // 1!
        {0x0003, LINUX_KEY_2},  // 2@

        {0x0004, LINUX_KEY_3},  // 3#
        {0x0005, LINUX_KEY_4},  // 4$
        {0x0006, LINUX_KEY_5},  // 5%
        {0x0007, LINUX_KEY_6},  // 6^
        {0x0008, LINUX_KEY_7},  // 7&
        {0x0009, LINUX_KEY_8},  // 8*
        {0x000a, LINUX_KEY_9},  // 9(
        {0x000b, LINUX_KEY_0},  // 0)
};

#endif

// On Windows, translateModifierState() function invokes winAPI getAsyncState()
// and is not reliable for testing.

#if defined(__linux__) || defined(__APPLE__)
TEST(native_keyboard_event_handler, handleKeyEvent) {
    for (size_t i = 0; i < ARRAYLEN(kHandleKeyEventTestData); i++) {
        NativeKeyboardEventHandler::KeyEvent keyEv;
        keyEv.scancode = kHandleKeyEventTestData[i].scancode;
        keyEv.modifiers = 0;
        SkinEvent* output =
                NativeKeyboardEventHandler::getInstance()->handleKeyEvent(
                        keyEv);
        EXPECT_NE(nullptr, output);
        EXPECT_EQ(kEventTextInput, output->type);
        EXPECT_EQ(kHandleKeyEventTestData[i].linuxKeyCode,
                  output->u.text.keycode);
    }
}


TEST(native_keyboard_event_handler, translateModifierState) {
    for (size_t i = 0; i < ARRAYLEN(kModifierTestData); i++) {
        int modifer = NativeKeyboardEventHandler::getInstance()
                              ->translateModifierState(
                                      0, kModifierTestData[i].inputModifier);
        EXPECT_EQ(kModifierTestData[i].outputModifer, modifer);
    }
}
#endif
}  // namespace
