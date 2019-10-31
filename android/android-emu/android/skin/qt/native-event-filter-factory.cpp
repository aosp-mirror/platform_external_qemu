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

#include "android/base/memory/LazyInstance.h"         // for LazyInstance

class QAbstractNativeEventFilter;

#define DEBUG 1

#if DEBUG
#include "android/utils/debug.h"                      // for VERBOSE_PRINT

#define D(...) VERBOSE_PRINT(surface, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

#ifdef __APPLE__

#import "android/skin/qt/mac-native-event-filter.h"

#elif defined(__linux__)
#include <cstdio>

#include "android/globals.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/native-keyboard-event-handler.h"
#include <QAbstractNativeEventFilter>
#include <QByteArray>
#include <X11/XKBlib.h>
#include <xcb/xcb.h>

class NativeEventFilter : public QAbstractNativeEventFilter {
public:
    NativeEventFilter() : mNumLockMask(XCB_MOD_MASK_2) {
        // NumLock could be mapped to any one of MOD_MASK1 - MOD_MASK5
        // depending on system configurations. We should use virtual modifier to
        // to find which real modifier it is mapped to.
        Display* display = XOpenDisplay(NULL);

        if (!display) {
            fprintf(stderr, "%s: warning: cannot get x11 display info\n", __func__);
            return;
        }

        XkbDescPtr xkb =
                XkbGetMap(display, XkbAllComponentsMask, XkbUseCoreKbd);

        if (!xkb) {
            fprintf(stderr, "%s: warning: cannot get mod mask info\n", __func__);
            return;
        }

        if (XkbGetNames(display, XkbKeycodesNameMask, xkb) != 0) {
            fprintf(stderr, "%s: warning: cannot get xkb keycodes names\n", __func__);
            return;
        }

        for (int i = 0; i < XkbNumVirtualMods; i++) {
            char* name = NULL;
            unsigned int realMod = 0;
            if (xkb->names->vmods[i]) {
                name = XGetAtomName(display, xkb->names->vmods[i]);
                if (!strcmp(name, "NumLock")) {
                    XkbVirtualModsToReal(xkb, 1 << i, &realMod);
                    mNumLockMask = realMod;
                    break;
                }
            }
        }
        XkbFreeClientMap(xkb, XkbAllComponentsMask, true);
        XCloseDisplay(display);
    }

    bool nativeEventFilter(const QByteArray& eventType,
                           void* message,
                           long*) override {
        if (eventType.compare("xcb_generic_event_t") == 0) {
            xcb_generic_event_t* ev =
                    static_cast<xcb_generic_event_t*>(message);
            int evType = (ev->response_type & ~0x80);
            if (evType == XCB_KEY_PRESS) {
                xcb_key_press_event_t* keyEv = (xcb_key_press_event_t*)ev;
                if (use_keycode_forwarding) {
                    if (EmulatorQtWindow::getInstance()->isActiveWindow()) {
                        NativeKeyboardEventHandler::KeyEvent inputEv;
                        inputEv.scancode = keyEv->detail;
                        inputEv.modifiers = keyEv->state;
                        inputEv.eventType = kEventKeyDown;
                        SkinEvent* event =
                                NativeKeyboardEventHandler::getInstance()
                                        ->handleKeyEvent(inputEv);
                        if (event)
                            EmulatorQtWindow::getInstance()->queueSkinEvent(
                                    event);
                    }
                }
            }
        }
        return false;
    }

private:
    unsigned int mNumLockMask;
};

#else  // windows

#include <windows.h>
#include "android/featurecontrol/FeatureControl.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/native-keyboard-event-handler.h"
#include <QAbstractNativeEventFilter>
#include <QByteArray>

class NativeEventFilter : public QAbstractNativeEventFilter {
public:
    bool nativeEventFilter(const QByteArray& eventType,
                           void* message,
                           long*) override {
        if (eventType.compare("windows_generic_MSG") == 0) {
            MSG* ev = static_cast<MSG*>(message);
            if (ev->message == WM_KEYDOWN || ev->message == WM_SYSKEYDOWN) {
                // Since being upgraded to 5.12.1, the prebuilt QT library
                // cannot handle numpad keys when number lock is on. To
                // workaround, we map numpad keys to its corresponding alpha
                // numerical keys when numlock is on.
                // TODO (wdu@) Find the root cause in prebuilt QT library.
                // Bug: 135141621
                UINT scancode = (ev->lParam >> 16) & 0x1ff;
                if (LOWORD(GetKeyState(VK_NUMLOCK)) != 0 &&
                    skin_keycode_native_is_keypad(scancode)) {
                    scancode = skin_keycode_native_map_keypad(scancode);
                    ev->wParam = MapVirtualKeyA(scancode, MAPVK_VSC_TO_VK);
                }
                if (use_keycode_forwarding) {
                    if (EmulatorQtWindow::getInstance()->isActiveWindow()) {
                        NativeKeyboardEventHandler::KeyEvent keyEv;
                        keyEv.scancode = scancode;
                        keyEv.eventType = kEventKeyDown;
                        SkinEvent* event =
                                NativeKeyboardEventHandler::getInstance()
                                        ->handleKeyEvent(keyEv);
                        if (event) {
                            EmulatorQtWindow::getInstance()->queueSkinEvent(
                                    event);
                        }
                    }
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
