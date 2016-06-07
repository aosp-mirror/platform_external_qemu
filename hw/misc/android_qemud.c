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
** Description
**
** qemud framing wrappers for android pipe.
**
** qemud is a simple legacy framing protocol for the android emulator.
** These wrappers simply allow for the handling of the framing
** when using the newer android pipe service to transport data.
**
*/

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "android/emulation/android_pipe_host.h"

#include "qemu-common.h"
#include "qemu/timer.h"
#include "hw/misc/android_qemud.h"

/* #define DEBUG_QEMUD */

#ifdef DEBUG_QEMUD
#define D(fmt, ...) \
    do { fprintf(stderr, "%s: " fmt , __func__, ## __VA_ARGS__); } while (0)
#else
#define D(fmt, ...) do {} while (0)
#endif

int qemud_pipe_read_buffers(const AndroidPipeBuffer *buf, int cnt,
                            GPtrArray *msgs, GString *remaining)
{
    int i, consumed = 0;
    gchar *p;
    for (i = 0; i < cnt; i++) {
        g_string_append_len(remaining,
                            (gchar *) buf[i].data,
                            (gssize) buf[i].size);
        consumed += buf[i].size;
    }
    D("consumed %d bytes (%s)\n", consumed, remaining->str);

    /* Now walk through the string extracting frames */
    p = remaining->str;
    do {
        int frame_length = 0;
        if (sscanf(p, "%4x", &frame_length) == 1) {
            p += 4;
            if (frame_length+4 <= remaining->len) {
                gchar *frame = g_strndup(p, frame_length);
                g_ptr_array_add(msgs, frame);
                g_string_erase(remaining, 0, frame_length+4);
                p = remaining->str;
            } else {
                break;
            }
        } else {
            /* we need more (unlikely) */
            break;
        }
    } while (remaining->len > 0);

    return consumed;
}

int qemud_pipe_write_buffers(GPtrArray *msgs, GString *left,
                             AndroidPipeBuffer *buf, int cnt)
{
    int b = 0, total = 0;
    D("have %d buffers, %d/%d msgs\n", cnt, (int) left->len, msgs->len);
    do {
        if (left->len > 0) {
            /* Handle any remaining partially sent message */
            int buf_space = buf[b].size;
            void *buf_data = buf[b].data;
            int to_copy = left->len <= buf_space ? left->len : buf_space;
            memcpy(buf_data, left->str, to_copy);
            g_string_erase(left, 0, to_copy);
            total += to_copy;
            b++;
        } else {
            /* Format next message to send if we have one */
            if (msgs->len > 0) {
                gchar *msg = g_ptr_array_index(msgs, 0);
                g_string_printf(left, "%04x%s", (int) strlen(msg), msg);
                g_ptr_array_remove(msgs, msg);
            } else {
                break;
            }
        }
    } while ((left->len > 0) && (b < cnt));

    D("sent %d bytes, %d/%d msgs\n", total, (int) left->len, msgs->len);

    return total;
}
