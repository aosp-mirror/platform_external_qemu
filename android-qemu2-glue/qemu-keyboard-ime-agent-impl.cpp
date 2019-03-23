// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/control/keyboard_ime_agent.h"

#include "android/emulation/KeyboardIMEPipe.h"

using android::emulation::KeyboardIMEPipe;

static void set_guest_keyboard_ime_callback(void (*cb)(void*,
                                                    const uint8_t*,
                                                    size_t),
                                         void* context) {
    KeyboardIMEPipe::setGuestKeyboardIMECallback(
            [cb, context](const uint8_t* data, size_t size) {
                cb(context, data, size);
            });
}

static void set_guest_keyboard_ime_contents(const uint8_t* buf, size_t len) {
    const auto pipe = KeyboardIMEPipe::Service::getPipe();
    if (pipe) {
        pipe->setText(buf, len);
    }
}

static const QAndroidKeyboardIMEAgent keyboardIMEAgent = {
        .setEnabled = &KeyboardIMEPipe::setEnabled,
        .setGuestKeyboardIMECallback = set_guest_keyboard_ime_callback,
        .setGuestKeyboardIMEContents = set_guest_keyboard_ime_contents};

extern "C" const QAndroidKeyboardIMEAgent* const gQAndroidKeyboardIMEAgent =
        &keyboardIMEAgent;
