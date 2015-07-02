/* Copyright (C) 2014 The Android Open Source Project
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
#ifndef ANDROID_SKIN_QT_WINSYS_QT_H
#define ANDROID_SKIN_QT_WINSYS_QT_H

// Save the current window position for after the app exit.
// This must be called in the aboutToQuit() signal handle for the
// application window.
extern void skin_winsys_save_window_pos();

#endif  // ANDROID_SKIN_QT_WINSYS_QT_H
