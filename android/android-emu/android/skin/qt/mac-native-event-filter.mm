/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/skin/qt/mac-native-event-filter.h"

#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"

#import <AppKit/AppKit.h>

static void handleNativeKeyEvent(int keycode,
                                 int modifiers,
                                 SkinEventType eventType) {
    // Do no send modifier by itself.
    if (!skin_keycode_native_to_linux(&keycode, &modifiers) ||
        skin_keycode_is_modifier(keycode)) {
        return;
    }

    SkinEvent* skin_event_down =
            EmulatorQtWindow::getInstance()->createSkinEvent(kEventTextInput);
    SkinEventTextInputData& textInputData = skin_event_down->u.text;
    textInputData.down = true;
    textInputData.keycode = keycode;
    textInputData.mod = 0;
    if (modifiers & NSEventModifierFlagControl) {
        textInputData.mod |= kKeyModLCtrl;
    }
    if (modifiers & NSEventModifierFlagShift) {
        textInputData.mod |= kKeyModLShift;
    }
    // Map to right-alt instead of left-alt because Mac OS doesn't
    // differentiate between left or right alt but Linux does.
    // In Android, many key character map use right-alt as the alt-gr key.
    if (modifiers & NSEventModifierFlagOption) {
        textInputData.mod |= kKeyModRAlt;
    }
    if (modifiers & NSEventModifierFlagCapsLock) {
        textInputData.mod |= kKeyModCapsLock;
    }
    // TODO (wdu@) Use a guest feature flag to determine if the workaround
    // below needs to run.
    // Applicable to system images where caps locks is not accepted, in whic
    // case, substitute caps lock with shift. We simulate "Caps Lock" key
    // using "Shift" when "Alt" key is not pressed.
    if ((modifiers & NSEventModifierFlagCapsLock) &&
        skin_keycode_is_alpha(keycode) &&
        !(modifiers & NSEventModifierFlagOption)) {
        textInputData.mod |= kKeyModLShift;
    }

    EmulatorQtWindow::getInstance()->queueSkinEvent(skin_event_down);
}

bool NativeEventFilter::nativeEventFilter(const QByteArray& eventType,
                                             void* message,
                                             long*) {
    if (eventType.compare("mac_generic_NSEvent") == 0) {
        NSEvent* event = static_cast<NSEvent*>(message);
        if ([event type] == NSKeyDown) {
            if (EmulatorQtWindow::getInstance() != nullptr) {
                unsigned short keycode = [event keyCode];
                if (([event modifierFlags] & NSEventModifierFlagNumericPad) &&
                        skin_keycode_native_is_keypad(keycode)) {
                    keycode = skin_keycode_native_map_keypad(keycode);
                }
                if (EmulatorQtWindow::getInstance()->isActiveWindow()) {
                    handleNativeKeyEvent(keycode, [event modifierFlags],
                                         kEventKeyDown);
                }
            }
        }
    }
    return false;
}

