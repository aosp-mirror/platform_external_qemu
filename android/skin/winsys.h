/* Copyright (C) 2007-2008 The Android Open Source Project
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
#ifndef ANDROID_SKIN_WINSYS_H
#define ANDROID_SKIN_WINSYS_H

// Window system related definitions.
#include "android/skin/rect.h"

#include <stdbool.h>

// Enable/Disable mouse cursor.
void skin_winsys_show_cursor(bool enabled);

// Enable/Disable input grab.
void skin_winsys_grab_input(bool enabled);

// Return window handle of main UI.
void* skin_winsys_get_window_handle(void);

// Return rectangle of current monitor in pixels.
void skin_winsys_get_monitor_rect(SkinRect* rect);

// Return the monitor's horizontal and vertical resolution in dots per
// inches. Return 0 in case of success, -1 in case of failure.
int skin_winsys_get_monitor_dpi(int* x_dpi, int* y_dpi);

// Set main window position.
void skin_winsys_set_window_pos(int window_x, int window_y);

// Get main window position.
void skin_winsys_get_window_pos(int* window_x, int* window_y);

// Set window title.
void skin_winsys_set_window_title(const char* title);

// Return true iff the main window is fully visible
bool skin_winsys_is_window_fully_visible(void);

#endif  // ANDROID_SKIN_WINSYS_H