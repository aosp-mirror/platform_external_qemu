/* Copyright (C) 2010 The Android Open Source Project
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

#ifndef _ANDROID_PROTOCOL_UI_COMMANDS_PROXY_H
#define _ANDROID_PROTOCOL_UI_COMMANDS_PROXY_H

/*
 * Contains the Core-side implementation of the "core-ui-control" service that is
 * part of the UI control protocol.
 */

/*
 * Creates and initializes the proxy.
 * Param:
 *  fd - Socket descriptor for the proxy.
 * Return:
 *  0 on success, or < 0 on failure.
 */
extern int uiCmdProxy_create(int fd);

/*
 * Destroys Core->UI UI control service.
 */
extern void uiCmdProxy_destroy();

/* Changes the scale of the emulator window at runtime.
 * Param:
 *  scale, is_dpi - New window scale parameters
 * Return:
 *  0 on success, or < 0 on failure.
 */
extern int uicmd_set_window_scale(double scale, int is_dpi);

#endif /* _ANDROID_PROTOCOL_UI_COMMANDS_PROXY_H */
