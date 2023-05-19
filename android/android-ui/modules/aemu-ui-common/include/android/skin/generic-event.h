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

#include "android/skin/event.h"                 // for SkinEvent
#include "android/skin/generic-event-buffer.h"  // for SkinGenericEventFlush...
#include "android/utils/compiler.h"             // for ANDROID_BEGIN_HEADER

ANDROID_BEGIN_HEADER

typedef struct SkinGenericEvent SkinGenericEvent;

extern SkinGenericEvent* skin_generic_event_create(
        SkinGenericEventFlushFunc generic_event_flush);

extern void skin_generic_event_process_event(SkinGenericEvent* generic_event,
                                             SkinEvent* ev);

extern void skin_generic_event_free(SkinGenericEvent* event);

extern void skin_generic_event_add_event(SkinGenericEvent* ge,
                                         unsigned type,
                                         unsigned code,
                                         unsigned value,
                                         unsigned displayId);

extern void skin_generic_event_flush(SkinGenericEvent* ge);

ANDROID_END_HEADER
