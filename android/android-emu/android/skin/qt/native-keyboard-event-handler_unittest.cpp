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

#include "android/base/ArraySize.h"
#include "android/emulation/control/vm_operations.h"
#include "android/skin/event.h"
#include "android/skin/linux_keycodes.h"

#include <gtest/gtest.h>

extern "C" void qemu_system_shutdown_request(QemuShutdownCause reason) {}

namespace {

// Run keycode translation tests on Linux and MacOS only because windows
// requires win API call to getAsyncKeyState which is non-deterministic.

#if defined(__linux__)
const struct {
    int scancode;
    int inputModifier;
    int linuxKeyCode;
    int outputModifier;
} kKeycodeTestData[] = {
        {10, 0, LINUX_KEY_1, 0},          {11, 0, LINUX_KEY_2, 0},
        {12, 0, LINUX_KEY_3, 0},          {13, 0, LINUX_KEY_4, 0},
        {14, 0, LINUX_KEY_5, 0},          {15, 0, LINUX_KEY_6, 0},
        {16, 0, LINUX_KEY_7, 0},          {17, 0, LINUX_KEY_8, 0},
        {18, 0, LINUX_KEY_9, 0},          {19, 0, LINUX_KEY_0, 0},
        {20, 0, LINUX_KEY_MINUS, 0},      {21, 0, LINUX_KEY_EQUAL, 0},
        {22, 0, LINUX_KEY_BACKSPACE, 0},  {23, 0, LINUX_KEY_TAB, 0},
        {24, 0, LINUX_KEY_Q, 0},          {25, 0, LINUX_KEY_W, 0},
        {26, 0, LINUX_KEY_E, 0},          {27, 0, LINUX_KEY_R, 0},
        {28, 0, LINUX_KEY_T, 0},          {29, 0, LINUX_KEY_Y, 0},
        {30, 0, LINUX_KEY_U, 0},          {31, 0, LINUX_KEY_I, 0},
        {32, 0, LINUX_KEY_O, 0},          {33, 0, LINUX_KEY_P, 0},
        {34, 0, LINUX_KEY_LEFTBRACE, 0},

        {35, 0, LINUX_KEY_RIGHTBRACE, 0}, {36, 0, LINUX_KEY_ENTER, 0},
        {38, 0, LINUX_KEY_A, 0},          {39, 0, LINUX_KEY_S, 0},
        {40, 0, LINUX_KEY_D, 0},          {41, 0, LINUX_KEY_F, 0},
        {42, 0, LINUX_KEY_G, 0},          {43, 0, LINUX_KEY_H, 0},
        {44, 0, LINUX_KEY_J, 0},          {45, 0, LINUX_KEY_K, 0},
        {46, 0, LINUX_KEY_L, 0},          {47, 0, LINUX_KEY_SEMICOLON, 0},

        {48, 0, LINUX_KEY_APOSTROPHE, 0}, {49, 0, LINUX_KEY_GREEN, 0},
        {51, 0, LINUX_KEY_BACKSLASH, 0},  {52, 0, LINUX_KEY_Z, 0},
        {53, 0, LINUX_KEY_X, 0},          {54, 0, LINUX_KEY_C, 0},
        {55, 0, LINUX_KEY_V, 0},          {56, 0, LINUX_KEY_B, 0},
        {57, 0, LINUX_KEY_N, 0},          {58, 0, LINUX_KEY_M, 0},
        {59, 0, LINUX_KEY_COMMA, 0},      {60, 0, LINUX_KEY_DOT, 0},
        {61, 0, LINUX_KEY_SLASH, 0},
};
#elif defined(__APPLE__)
#include "android/skin/macos_keycodes.h"
const struct {
    int scancode;
    int inputModifier;
    int linuxKeyCode;
    int outputModifier;
} kKeycodeTestData[] =
        {
                {kVK_ANSI_A, 0, LINUX_KEY_A, 0},
                {kVK_ANSI_S, 0, LINUX_KEY_S, 0},
                {kVK_ANSI_D, 0, LINUX_KEY_D, 0},
                {kVK_ANSI_F, 0, LINUX_KEY_F, 0},
                {kVK_ANSI_H, 0, LINUX_KEY_H, 0},
                {kVK_ANSI_G, 0, LINUX_KEY_G, 0},
                {kVK_ANSI_Z, 0, LINUX_KEY_Z, 0},
                {kVK_ANSI_X, 0, LINUX_KEY_X, 0},
                {kVK_ANSI_C, 0, LINUX_KEY_C, 0},
                {kVK_ANSI_V, 0, LINUX_KEY_V, 0},
                {kVK_ANSI_B, 0, LINUX_KEY_B, 0},
                {kVK_ANSI_Q, 0, LINUX_KEY_Q, 0},
                {kVK_ANSI_W, 0, LINUX_KEY_W, 0},
                {kVK_ANSI_E, 0, LINUX_KEY_E, 0},
                {kVK_ANSI_R, 0, LINUX_KEY_R, 0},
                {kVK_ANSI_Y, 0, LINUX_KEY_Y, 0},
                {kVK_ANSI_T, 0, LINUX_KEY_T, 0},
                {kVK_ANSI_1, 0, LINUX_KEY_1, 0},
                {kVK_ANSI_2, 0, LINUX_KEY_2, 0},
                {kVK_ANSI_3, 0, LINUX_KEY_3, 0},
                {kVK_ANSI_4, 0, LINUX_KEY_4, 0},
                {kVK_ANSI_6, 0, LINUX_KEY_6, 0},
                {kVK_ANSI_5, 0, LINUX_KEY_5, 0},
                {kVK_ANSI_Equal, 0, LINUX_KEY_EQUAL, 0},
                {kVK_ANSI_9, 0, LINUX_KEY_9, 0},
                {kVK_ANSI_7, 0, LINUX_KEY_7, 0},
                {kVK_ANSI_Minus, 0, LINUX_KEY_MINUS, 0},
                {kVK_ANSI_8, 0, LINUX_KEY_8, 0},
                {kVK_ANSI_0, 0, LINUX_KEY_0, 0},
                {kVK_ANSI_RightBracket, 0, LINUX_KEY_RIGHTBRACE, 0},
                {kVK_ANSI_O, 0, LINUX_KEY_O, 0},
                {kVK_ANSI_U, 0, LINUX_KEY_U, 0},
                {kVK_ANSI_LeftBracket, 0, LINUX_KEY_LEFTBRACE, 0},
                {kVK_ANSI_I, 0, LINUX_KEY_I, 0},
                {kVK_ANSI_P, 0, LINUX_KEY_P, 0},
                {kVK_ANSI_L, 0, LINUX_KEY_L, 0},
                {kVK_ANSI_J, 0, LINUX_KEY_J, 0},
                {kVK_ANSI_Quote, 0, LINUX_KEY_APOSTROPHE, 0},
                {kVK_ANSI_K, 0, LINUX_KEY_K, 0},
                {kVK_ANSI_Semicolon, 0, LINUX_KEY_SEMICOLON, 0},
                {kVK_ANSI_Backslash, 0, LINUX_KEY_BACKSLASH, 0},
                {kVK_ANSI_Comma, 0, LINUX_KEY_COMMA, 0},
                {kVK_ANSI_Slash, 0, LINUX_KEY_SLASH, 0},
                {kVK_ANSI_N, 0, LINUX_KEY_N, 0},
                {kVK_ANSI_M, 0, LINUX_KEY_M, 0},
                {kVK_ANSI_Period, 0, LINUX_KEY_DOT, 0},
                {kVK_ANSI_Grave, 0, LINUX_KEY_GREEN, 0},
}
#endif

TEST(keyboardEventHandler, handleKeyEvent) {
    for (size_t i = 0; i < ARRAY_SIZE(kKeycodeTestData); i++) {
        NativeKeyboardEventHandler::KeyEvent keyEv;
        keyEv.scancode = kKeycodeTestData[i].scancode;
        keyEv.modifiers = kKeycodeTestData[i].inputModifier;
        SkinEvent* output =
                NativeKeyboardEventHandler::getInstance()->handleKeyEvent(
                        keyEv);
        EXPECT_NE(nullptr, output);
        EXPECT_EQ(kEventTextInput, output->type);
        EXPECT_EQ(kKeycodeTestData[i].linuxKeyCode, output->u.text.keycode);
        EXPECT_EQ(kKeycodeTestData[i].outputModifier, output->u.text.mod);
    }
}

}  // namespace
