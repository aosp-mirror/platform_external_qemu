/* Copyright (C) 2015 Linaro Limited
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** Simple QEMUD framing wrappers
*/

#ifndef ANDROID_QEMUD_H
#define ANDROID_QEMUD_H

#include <glib.h>

/* Read Android buffers into an array of terminated strings */
int qemud_pipe_read_buffers(const AndroidPipeBuffer *buf, int cnt,
                            GPtrArray *msgs, GString *remaining);

/* Write an array of strings into a series on Android pipe buffers */
int qemud_pipe_write_buffers(GPtrArray *msgs, GString *remaining,
                             AndroidPipeBuffer *buf, int cnt);

#endif /* ANDROID_QEMUD_H */
