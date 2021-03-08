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

#include "android/globals.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/native-keyboard-event-handler.h"

#import <AppKit/AppKit.h>

bool NativeEventFilter::nativeEventFilter(const QByteArray& eventType,
                                             void* message,
                                             long*) {
    if (eventType.compare("mac_generic_NSEvent") == 0) {
        NSEvent* event = static_cast<NSEvent*>(message);
        if ([event type] == NSKeyDown) {
            EmulatorQtWindow* emuWindow = EmulatorQtWindow::getInstance();
            if (emuWindow != nullptr) {
                unsigned short keycode = [event keyCode];
                if (([event modifierFlags] & NSEventModifierFlagNumericPad) &&
                        skin_keycode_native_is_keypad(keycode)) {
                    keycode = skin_keycode_native_map_keypad(keycode);
                }
                if (use_keycode_forwarding) {
                    if (emuWindow->isActiveWindow()) {
                        NativeKeyboardEventHandler* handler =
                                NativeKeyboardEventHandler::getInstance();
                        NativeKeyboardEventHandler::KeyEvent ev;
                        ev.scancode = keycode;
                        ev.modifiers = [event modifierFlags];
                        ev.eventType = kEventKeyDown;
                        SkinEvent* event =
                                NativeKeyboardEventHandler::getInstance()
                                        ->handleKeyEvent(ev);
                        if (event)
                            emuWindow->queueSkinEvent(event);
                    }
                }
            }
        }
    }
    return false;
}

