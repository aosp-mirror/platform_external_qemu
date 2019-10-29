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

#include "android/skin/qt/native-keyboard-event-handler.h"

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

#include "android/skin/macos_keycodes.h"

class MacOSKeyboardEventHandler : public NativeKeyboardEventHandler {
    int translateModifierState(int keycode, int modifiers) override {
        int result = 0;
        if (modifiers & NSEventModifierFlagControl) {
            result |= kKeyModLCtrl;
        }
        if (modifiers & NSEventModifierFlagShift) {
            result |= kKeyModLShift;
        }
        // Map to right-alt instead of left-alt because Mac OS doesn't
        // differentiate between left or right alt but Android does.
        // In Android, many key character map use right-alt as the alt-gr key.
        if (modifiers & NSEventModifierFlagOption) {
            result |= kKeyModRAlt;
        }
        if (modifiers & NSEventModifierFlagCapsLock) {
            result |= kKeyModCapsLock;
        }
        return result;
    }
};

static android::base::LazyInstance<MacOSKeyboardEventHandler> sInstance =
        LAZY_INSTANCE_INIT;

#elif defined(__linux__)

#include <X11/XKBlib.h>
#include <xcb/xcb.h>

class X11KeyboardEventHandler : public NativeKeyboardEventHandler {
public:
    X11KeyboardEventHandler()
        : mAltMask(XCB_MOD_MASK_1), mAltGrMask(XCB_MOD_MASK_5) {
        // Alt and AltGr modifiers could be mapped to any one of MOD_MASK1 -
        // MOD_MASK5 depending on system configurations. We should use virtual
        // modifier to to find which real modifier it is mapped to.
        Display* display = XOpenDisplay(NULL);

        if (!display) {
            fprintf(stderr, "%s: warning: cannot get x11 display info\n",
                    __func__);
            return;
        }

        XkbDescPtr xkb =
                XkbGetMap(display, XkbAllComponentsMask, XkbUseCoreKbd);

        if (!xkb) {
            fprintf(stderr, "%s: warning: cannot get mod mask info\n",
                    __func__);
            return;
        }

        if (XkbGetNames(display, XkbKeycodesNameMask, xkb) != 0) {
            fprintf(stderr, "%s: warning: cannot get xkb keycodes names\n",
                    __func__);
            return;
        }

        for (int i = 0; i < XkbNumVirtualMods; i++) {
            char* name = NULL;
            unsigned int realMod = 0;
            if (xkb->names->vmods[i]) {
                name = XGetAtomName(display, xkb->names->vmods[i]);
                if (!strcmp(name, "Alt")) {
                    XkbVirtualModsToReal(xkb, 1 << i, &realMod);
                    mAltMask = realMod;
                } else if (!strcmp(name, "AltGr")) {
                    XkbVirtualModsToReal(xkb, 1 << i, &realMod);
                    mAltGrMask = realMod;
                }
            }
        }
        XkbFreeClientMap(xkb, XkbAllComponentsMask, true);
        XCloseDisplay(display);
    }

    int translateModifierState(int keycode, int modifiers) override {
        int result = 0;
        if (modifiers & XCB_MOD_MASK_CONTROL) {
            result |= kKeyModLCtrl;
        }

        if (modifiers & XCB_MOD_MASK_SHIFT) {
            result |= kKeyModLShift;
        }

        if (modifiers & XCB_MOD_MASK_LOCK) {
            result |= kKeyModCapsLock;
        }

        if (modifiers & mAltMask) {
            result |= kKeyModLAlt;
        }

        if (modifiers & mAltGrMask) {
            result |= kKeyModRAlt;
        }

        // For older system image that doesn't accept CAPS_LOCK, we need to
        // simulate the CAPS_LOCK by using shift when the key is alphabetical.
        // BUG: 141318682
        if ((modifiers & XCB_MOD_MASK_LOCK) && skin_keycode_is_alpha(keycode) &&
            !(modifiers & mAltMask)) {
            result |= kKeyModLShift;
        }
        return result;
    }

private:
    unsigned int mAltMask;
    unsigned int mAltGrMask;
};

static android::base::LazyInstance<X11KeyboardEventHandler> sInstance =
        LAZY_INSTANCE_INIT;
#else
#include <windows.h>

class WindowsKeyboardEventHandler : public NativeKeyboardEventHandler {
    int translateModifierState(int keycode, int modifiers) override {
        int result = 0;
        if (HIWORD(GetAsyncKeyState(VK_CONTROL)) != 0) {
            result |= kKeyModLCtrl;
        }
        if (HIWORD(GetAsyncKeyState(VK_SHIFT)) != 0 ||
            (modifiers & MOD_SHIFT)) {
            result |= kKeyModLShift;
        }
        if (HIWORD(GetAsyncKeyState(VK_LMENU)) != 0) {
            result |= kKeyModLAlt;
        }
        if (HIWORD(GetAsyncKeyState(VK_RMENU)) != 0) {
            result |= kKeyModRAlt;
        }
        if (LOWORD(GetKeyState(VK_CAPITAL)) != 0) {
            result |= kKeyModCapsLock;
        }
        return result;
    }
};

static android::base::LazyInstance<WindowsKeyboardEventHandler> sInstance =
        LAZY_INSTANCE_INIT;

#endif

SkinEvent* NativeKeyboardEventHandler::handleKeyEvent(
        NativeKeyboardEventHandler::KeyEvent keyEv) {
    D("Raw scancode %d modifiers 0x%x", keyEv.scancode, keyEv.modifiers);
    int scancode = keyEv.scancode;
    int modifiers = keyEv.modifiers;
    int linuxKeycode = skin_native_scancode_to_linux(scancode);
    if (linuxKeycode == 0 || skin_keycode_is_modifier(linuxKeycode)) {
        return nullptr;
    }

    SkinEvent* skinEvent =
            EmulatorQtWindow::getInstance()->createSkinEvent(kEventTextInput);
    SkinEventTextInputData& textInputData = skinEvent->u.text;
    textInputData.keycode = linuxKeycode;
    textInputData.mod =
            translateModifierState(textInputData.keycode, modifiers);

    D("Processed keycode %d modifiers 0x%x", textInputData.keycode,
      skinEvent->u.text.mod);
    return skinEvent;
}

NativeKeyboardEventHandler* NativeKeyboardEventHandler::getInstance() {
    return sInstance.ptr();
}
