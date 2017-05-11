/* Copyright (C) 2017 The Android Open Source Project
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

#include "android/skin/generic-event-buffer.h"

#include "android/utils/debug.h"

#include <stdio.h>

void skin_generic_event_buffer_init(SkinGenericEventBuffer* buffer,
                                    SkinGenericEventFlushFunc flush_func) {
    buffer->generic_event_flush = flush_func;
    buffer->event_count = 0;
}

void skin_generic_event_buffer_add(SkinGenericEventBuffer* buffer,
                                   unsigned type,
                                   unsigned code,
                                   unsigned value) {
    if (buffer->event_count < MAX_GENERIC_EVENTS) {
        int count = (int)buffer->event_count++;
        buffer->events[count].type = type;
        buffer->events[count].code = code;
        buffer->events[count].value = value;
    }
}

void skin_generic_event_buffer_flush(SkinGenericEventBuffer* buffer) {
    if (buffer->event_count > 0) {
        if (VERBOSE_CHECK(events)) {
            int nn;
            printf(">> EVENT");
            for (nn = 0; nn < buffer->event_count; nn++) {
                SkinGenericEventCode* s = &buffer->events[nn];
                printf(" [0x%02x,0x%03x,0x%x]", s->type, s->code, s->value);
            }
            printf("\n");
        }
        if (buffer->generic_event_flush) {
            buffer->generic_event_flush(buffer->events, buffer->event_count);
        }
        buffer->event_count = 0;
    }
}
