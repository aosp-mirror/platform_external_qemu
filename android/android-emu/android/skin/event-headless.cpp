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
<<<<<<< HEAD   (464e37 Merge "Merge empty history for sparse-5409122-L7540000028739)
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
    (void)event;
    return false;
}

extern void skin_enable_mouse_tracking(bool enable) {
    (void)enable;
}
=======
#include "android/skin/event.h"                        // for SkinEvent, ski...
#include "android/skin/qt/emulator-no-qt-no-window.h"  // for EmulatorNoQtNo...

#define  DEBUG  1

#if DEBUG
#include "android/utils/debug.h"                       // for VERBOSE_PRINT

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
>>>>>>> BRANCH (510a80 Merge "Merge cherrypicks of [1623139] into sparse-7187391-L1)
