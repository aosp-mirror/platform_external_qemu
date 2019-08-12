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

#define DEBUG 1

#if DEBUG
#include "android/utils/debug.h"
#define D(...) VERBOSE_PRINT(surface, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

#ifdef __APPLE__

#import "android/skin/qt/mac-native-event-filter.h"

#elif defined(__linux__)

#include <xcb/xcb.h>

static void handleNativeKeyEvent(int keycode,
                                 int modifiers,
                                 SkinEventType eventType) {
    D("Raw keycode %d modifiers 0x%x", keycode, modifiers);

    // Do no send modifier by itself.
    if (!skin_keycode_native_to_linux(&keycode, &modifiers) ||
        skin_keycode_is_modifier(keycode)) {
        return;
    }

    SkinEvent* skin_event =
            EmulatorQtWindow::getInstance()->createSkinEvent(kEventTextInput);
    SkinEventTextInputData& textInputData = skin_event->u.text;
    textInputData.down = true;

    textInputData.keycode = keycode;
    textInputData.mod = 0;
    if (modifiers & XCB_MOD_MASK_SHIFT) {
        textInputData.mod |= kKeyModLShift;
    }
    if (modifiers & XCB_MOD_MASK_CONTROL) {
        textInputData.mod |= kKeyModLCtrl;
    }
    if (modifiers & XCB_MOD_MASK_1) {
        textInputData.mod |= kKeyModRAlt;
    }
    // Emulator uses qwerty2.kl in Android which doesn't recognize Caps lock
    // key. Thus, we simulate "Caps Lock" key using "Shift" when "Alt" key is
    // not pressed. Caution: Alt+Shift+base key will output different symbol
    // from Alt+Base under some keyboard layout.
    if ((modifiers & XCB_MOD_MASK_LOCK) && skin_keycode_is_alpha(keycode) &&
        !(modifiers & XCB_MOD_MASK_1)) {
        textInputData.mod |= kKeyModLShift;
    }
    D("Processed keycode %d modifiers 0x%x", keycode, textInputData.mod);
    EmulatorQtWindow::getInstance()->queueSkinEvent(skin_event);
}

class NativeEventFilter : public QAbstractNativeEventFilter {
public:
    bool nativeEventFilter(const QByteArray& eventType,
                           void* message,
                           long*) override {
        if (EmulatorQtWindow::getInstance()->isActiveWindow() &&
            eventType.compare("xcb_generic_event_t") == 0) {
            xcb_generic_event_t* ev =
                    static_cast<xcb_generic_event_t*>(message);
            if ((ev->response_type & ~0x80) == XCB_KEY_PRESS) {
                xcb_key_press_event_t* keyEv = (xcb_key_press_event_t*)ev;
                // Bug: 135141621 our own customized QT framework doesn't
                // handle num lock well. Thus, we translate keypad keycode
                // when num lock is on.
                if ((keyEv->state & XCB_MOD_MASK_2) &&
                    skin_keycode_native_is_keypad(keyEv->detail)) {
                    keyEv->detail =
                            skin_keycode_native_map_keypad(keyEv->detail);
                }
                handleNativeKeyEvent(keyEv->detail, keyEv->state,
                                     kEventKeyDown);
            }
        }
        return false;
    }
};

#else  // windows

#include <windows.h>

static void handleNativeKeyEvent(int keycode,
                                 int modifiers,
                                 SkinEventType eventType) {
    D("Raw keycode %d modifiers 0x%x", keycode, modifiers);

    // Do no send modifier by itself.
    if (!skin_keycode_native_to_linux(&keycode, &modifiers) ||
        skin_keycode_is_modifier(keycode)) {
        return;
    }

    SkinEvent* skin_event =
            EmulatorQtWindow::getInstance()->createSkinEvent(kEventTextInput);
    SkinEventTextInputData& textInputData = skin_event->u.text;
    textInputData.down = true;

    textInputData.keycode = keycode;
    textInputData.mod = 0;
    if (HIWORD(GetAsyncKeyState(VK_CONTROL)) != 0) {
        textInputData.mod |= kKeyModLCtrl;
    }
    if (HIWORD(GetAsyncKeyState(VK_SHIFT)) != 0 || (modifiers & MOD_SHIFT)) {
        textInputData.mod |= kKeyModLShift;
    }
    // Left menu will be omitted.
    if (HIWORD(GetAsyncKeyState(VK_RMENU)) != 0) {
        textInputData.mod |= kKeyModRAlt;
    }
    if (LOWORD(GetKeyState(VK_CAPITAL)) != 0 &&
        skin_keycode_is_alpha(keycode) && !(textInputData.mod & kKeyModRAlt)) {
        textInputData.mod |= kKeyModLShift;
    }
    D("Processed keycode %d modifiers 0x%x", keycode, textInputData.mod);
    EmulatorQtWindow::getInstance()->queueSkinEvent(skin_event);
}

class NativeEventFilter : public QAbstractNativeEventFilter {
public:
    bool nativeEventFilter(const QByteArray& eventType,
                           void* message,
                           long*) override {
        if (EmulatorQtWindow::getInstance()->isActiveWindow() &&
            eventType.compare("windows_generic_MSG") == 0) {
            MSG* ev = static_cast<MSG*>(message);
            if (ev->message == WM_KEYDOWN || ev->message == WM_SYSKEYDOWN) {
                if (LOWORD(GetKeyState(VK_NUMLOCK)) != 0 &&
                    skin_keycode_native_is_keypad(ev->wParam)) {
                    ev->wParam = skin_keycode_native_map_keypad(ev->wParam);
                }
                handleNativeKeyEvent(ev->wParam, 0, kEventKeyDown);
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
