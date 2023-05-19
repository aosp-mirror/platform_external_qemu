/* Copyright (C) 2015 The Android Open Source Project
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
#include "android/skin/event.h"                  // for SkinEvent, skin_enab...
#include "android/skin/qt/emulator-qt-window.h"  // for EmulatorQtWindow

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"                 // for VERBOSE_PRINT

#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

extern bool skin_event_poll(SkinEvent* event) {
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (!window) return false;
    bool retval;
    window->pollEvent(event, &retval);
    return retval;
}

extern void skin_enable_mouse_tracking(bool enable) {
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (window) {
        window->setMouseTracking(enable);
    }
}

extern void skin_event_add(SkinEvent* event) {
    EmulatorQtWindow *window = EmulatorQtWindow::getInstance();
    if (!window) return;
    window->queueSkinEvent(event);
}
