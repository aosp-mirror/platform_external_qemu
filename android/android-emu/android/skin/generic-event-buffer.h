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

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

/* Contains declarations for routines that manage generic events that need
 * to be transferred to the emulator core for further processing.
 */

/* Maximum number of generic events kept in the array. */
#define MAX_GENERIC_EVENTS (256 * 2)

/* The attributes of any event we pass to the emulator. */
typedef struct SkinGenericEventCode {
    int type;
    int code;
    int value;
} SkinGenericEventCode;

// Type of a function used to flush buffered events
typedef void (*SkinGenericEventFlushFunc)(struct SkinGenericEventCode* events,
                                          int count);

/* Describes array of generic events collected for transferring to the core. */
typedef struct SkinGenericEventBuffer {
    SkinGenericEventFlushFunc generic_event_flush;
    int event_count;
    SkinGenericEventCode events[MAX_GENERIC_EVENTS];
} SkinGenericEventBuffer;

void skin_generic_event_buffer_init(SkinGenericEventBuffer* buffer,
                                    SkinGenericEventFlushFunc flush_func);

/* Adds an event to the array of generic events. */
void skin_generic_event_buffer_add(SkinGenericEventBuffer* buffer,
                                   unsigned type,
                                   unsigned code,
                                   unsigned value);

/* Flushes (transfers) collected events to the core. */
void skin_generic_event_buffer_flush(SkinGenericEventBuffer* buffer);

ANDROID_END_HEADER
