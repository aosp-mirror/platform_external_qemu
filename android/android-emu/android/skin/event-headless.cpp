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
#include <stdbool.h>

#include "android/skin/event.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-no-qt-no-window.h"
#include "android/utils/utf8_utils.h"

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"
#define  D(...)   VERBOSE_PRINT(surface,__VA_ARGS__)
#else
#define  D(...)   ((void)0)
#endif

extern bool skin_event_poll(SkinEvent* event) {
    EmulatorNoQtNoWindow *window = EmulatorNoQtNoWindow::getInstance();
    if (!window) return false;
    bool retval;
    window->pollEvent(event, &retval);
    return retval;
}


extern void skin_enable_mouse_tracking(bool enable) {
    (void)enable;
}

extern void skin_event_add(SkinEvent* event) { }
