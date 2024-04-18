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
#include "android/skin/generic-event.h"

#include <memory>                               // for unique_ptr

#include "android/skin/generic-event-buffer.h"  // for skin_generic_event_bu...
#include "android/utils/debug.h"                // for VERBOSE_PRINT

#define DEBUG 1

#if DEBUG
#define D(...) VERBOSE_PRINT(keys, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

struct SkinGenericEvent {
    SkinGenericEventBuffer event_buf[1];
};

SkinGenericEvent* skin_generic_event_create(
        SkinGenericEventFlushFunc generic_event_flush) {
    auto gb = std::unique_ptr<SkinGenericEvent>(new SkinGenericEvent());

    skin_generic_event_buffer_init(gb->event_buf, generic_event_flush);
    return gb.release();
}

void skin_generic_event_process_event(SkinGenericEvent* ge, SkinEvent* ev) {
    if (ev->type == kEventGeneric) {
        int type = ev->u.generic_event.type;
        int code = ev->u.generic_event.code;
        int value = ev->u.generic_event.value;
        int displayId = 0;
        skin_generic_event_add_event(ge, type, code, value, displayId);
        skin_generic_event_flush(ge);
    }
}

void skin_generic_event_add_event(SkinGenericEvent* ge,
                                  unsigned type,
                                  unsigned code,
                                  unsigned value,
                                  unsigned displayId) {
    skin_generic_event_buffer_add(ge->event_buf, type, code, value, displayId);
}

void skin_generic_event_flush(SkinGenericEvent* ge) {
    skin_generic_event_buffer_flush(ge->event_buf);
}

void skin_generic_event_free(SkinGenericEvent* generic_event) {
    if (generic_event) {
        delete generic_event;
    }
}

SkinEvent createSkinEvent(SkinEventType t) {
    SkinEvent e;
    e.type = t;
    switch (t) {
        case kEventKeyDown:
        case kEventKeyUp:
            e.u.key = {};
            break;
        case kEventGeneric:
            e.u.generic_event = {};
            break;
        case kEventTextInput:
            e.u.text = {};
            break;
        case kEventMouseButtonDown:
        case kEventMouseButtonUp:
        case kEventMouseMotion:
        case kEventMouseStartTracking:
        case kEventMouseStopTracking:
            e.u.mouse = {};
            break;
        case kEventMouseWheel:
            e.u.wheel = {};
            break;
        case kEventScrollBarChanged:
        case kEventZoomedWindowResized:
            e.u.scroll = {};
            break;
        case kEventRotaryInput:
            e.u.rotary_input = {};
            break;
        case kEventSetScale:
        case kEventSetZoom:
        case kEventWindowMoved:
        case kEventWindowChanged:
            e.u.window = {};
            break;
        case kEventLayoutRotate:
            e.u.layout_rotation = {};
            break;
        case kEventScreenChanged:
            e.u.screen = {};
            break;
        case kEventSetDisplayRegion:
        case kEventSetDisplayRegionAndUpdate:
            e.u.display_region = {};
            break;
        case kEventPenPress:
        case kEventPenRelease:
        case kEventPenMove:
            e.u.pen = {};
            break;
        case kEventTouchBegin:
        case kEventTouchEnd:
        case kEventTouchUpdate:
            e.u.multi_touch_point = {};
            break;
        case kEventSetDisplayActiveConfig:
            e.u.display_active_config = {};
            break;
        case kEventAddDisplay:
            e.u.add_display = {};
            break;
        case kEventRemoveDisplay:
            e.u.remove_display = {};
            break;
        case kEventQuit:
        case kEventForceRedraw:
        case kEventToggleTrackball:
        case kEventSetNoSkin:
        case kEventRestoreSkin:
        case kEventSetFoldedSkin:
            // No additional parameters
            break;
        default:
            // Crash if there's an unhandled SkinEventType, as we want an explicit initialization
            // for each type.
            dfatal("Unknown SkinEventType=%u", t);
            break;
    }
    return e;
}
