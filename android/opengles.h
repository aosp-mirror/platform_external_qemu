/* Copyright (C) 2011 The Android Open Source Project
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
#ifndef ANDROID_OPENGLES_H
#define ANDROID_OPENGLES_H

#include <stddef.h>

#define ANDROID_OPENGLES_BASE_PORT  22468

/* Call this function to initialize the hardware opengles emulation.
 * This function will abort if we can't find the corresponding host
 * libraries through dlopen() or equivalent.
 */
int android_initOpenglesEmulation(void);

/* Tries to start the renderer process. Returns 0 on success, -1 on error.
 * At the moment, this must be done before the VM starts. The onPost callback
 * may be NULL.
 */
int android_startOpenglesRenderer(int width, int height);

/* See the description in render_api.h. */
typedef void (*OnPostFunc)(void* context, int width, int height, int ydir,
                           int format, int type, unsigned char* pixels);
void android_setPostCallback(OnPostFunc onPost, void* onPostContext);

/* Retrieve the Vendor/Renderer/Version strings describing the underlying GL
 * implementation. The call only works while the renderer is started.
 *
 * Each string is copied into the corresponding buffer. If the original string
 * (including NUL terminator) is more than xxBufSize bytes, it will be
 * truncated. In all cases, including failure, the buffer will be NUL-
 * terminated when this function returns.
 */
void android_getOpenglesHardwareStrings(char* vendor, size_t vendorBufSize,
                                        char* renderer, size_t rendererBufSize,
                                        char* version, size_t versionBufSize);

int android_showOpenglesWindow(void* window, int x, int y, int width, int height, float rotation);

int android_hideOpenglesWindow(void);

void android_redrawOpenglesWindow(void);

/* Stop the renderer process */
void android_stopOpenglesRenderer(void);

/* set to TRUE if you want to use fast GLES pipes, 0 if you want to
 * fallback to local TCP ones
 */
extern int  android_gles_fast_pipes;

/* Write the path of the Unix socket we're going to use to access GLES on a given <port> */
/* The result is only valid on Unix systems */
void android_gles_unix_path(char* buff, size_t buffsize, int port);

#endif /* ANDROID_OPENGLES_H */
