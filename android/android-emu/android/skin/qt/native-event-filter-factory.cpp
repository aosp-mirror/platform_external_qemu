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

#include "android/skin/qt/native-event-filter-factory.h"

#include "android/base/memory/LazyInstance.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"

#ifdef __APPLE__

#import "android/skin/qt/mac-native-event-filter.h"

#elif defined(__linux__)

#include <xcb/xcb.h>

class NativeEventFilter : public QAbstractNativeEventFilter {
public:
    bool nativeEventFilter(const QByteArray& eventType,
                           void* message,
                           long*) override {
        if (eventType.compare("xcb_generic_event_t") == 0) {
            xcb_generic_event_t* ev =
                    static_cast<xcb_generic_event_t*>(message);
            if ((ev->response_type & ~0x80) == XCB_KEY_PRESS) {
                xcb_key_press_event_t* keyEv = (xcb_key_press_event_t*)ev;
                if (EmulatorQtWindow::getInstance() != nullptr) {
                    // Bug: 135141621 our own customized QT framework doesn't
                    // handle num lock well. Thus, we translate keypad keycode
                    // when num lock is on.
                    if ((keyEv->state & XCB_MOD_MASK_2) &&
                        skin_keycode_native_is_keypad(keyEv->detail)) {
                        keyEv->detail = skin_keycode_native_map_keypad(keyEv->detail);
                    }
                    EmulatorQtWindow::getInstance()->handleNativeKeyEvent(
                            keyEv->detail, keyEv->state, kEventKeyDown);
                }
            }
        }
        return false;
    }
};

#else  // windows

#include <windows.h>

class NativeEventFilter : public QAbstractNativeEventFilter {
public:
    bool nativeEventFilter(const QByteArray& eventType,
                           void* message,
                           long*) override {
        if (eventType.compare("windows_generic_MSG") == 0) {
            MSG* ev = static_cast<MSG*>(message);
            if (ev->message == WM_KEYDOWN || ev->message == WM_SYSKEYDOWN) {
                if (EmulatorQtWindow::getInstance() != nullptr) {
                    if (LOWORD(GetKeyState(VK_NUMLOCK)) != 0 &&
                        skin_keycode_native_is_keypad(ev->wParam)) {
                        ev->wParam = skin_keycode_native_map_keypad(ev->wParam);
                    }
                    // modifiers can be queried by GetAsyncKeyState()
                    EmulatorQtWindow::getInstance()->handleNativeKeyEvent(
                            ev->wParam, 0, kEventKeyDown);
                }
            }
        }
        return false;
    }
};

#endif

static android::base::LazyInstance<NativeEventFilter> sInstance = LAZY_INSTANCE_INIT;
QAbstractNativeEventFilter* NativeEventFilterFactory::getEventFilter() {
    return sInstance.ptr();
}
