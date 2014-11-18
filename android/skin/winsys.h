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
#include <stddef.h>

// Enable/Disable relative mouse mode. Used for trackball emulation.
// This hides the cursor and grabs the input.
void skin_winsys_set_relative_mouse_mode(bool enabled);

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

// Start main window support. |no_window| must be true to indicate that
// no window needs to be shown, but that the SDL backend still needs to be
// initialized.
void skin_winsys_start(bool no_window, bool raw_keys);

// Set the main window's icon.
// |icon_data| is the start of the icon in PNG format.
// |icon_data_size| is its size in bytes.
//
// Note: On Windows, the icon data is ignored, and the icon is directly
//       extracted from the executable.
void skin_winsys_set_window_icon(const unsigned char* icon_data,
                                 size_t icon_data_size);

// Stop main window and quit program. Must be called from inside event loop.
void skin_winsys_quit(void);

#endif  // ANDROID_SKIN_WINSYS_H
