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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "android/skin/rect.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    kEventKeyDown,
    kEventKeyUp,
    kEventGeneric,
    kEventTextInput,
    kEventMouseButtonDown,
    kEventMouseButtonUp,
    kEventMouseMotion,
    kEventMouseWheel,
    kEventQuit,
    kEventScrollBarChanged,
    kEventRotaryInput,
    kEventSetScale,
    kEventSetZoom,
    kEventForceRedraw,
    kEventWindowMoved,
    kEventLayoutRotate,
    kEventScreenChanged,
    kEventZoomedWindowResized,
    kEventToggleTrackball,
    kEventWindowChanged,
    kEventSetDisplayRegion,
    kEventSetDisplayRegionAndUpdate,
    kEventSetNoSkin,
    kEventRestoreSkin,
    kEventMouseStartTracking,
    kEventMouseStopTracking,
    kEventPenPress,
    kEventPenRelease,
    kEventPenMove,
    kEventTouchBegin,
    kEventTouchEnd,
    kEventTouchUpdate,
    kEventSetDisplayConfigs,
    kEventSetDisplayActiveConfig,
} SkinEventType;

// The numeric values represent the bit positions in the button state
typedef enum {
    kMouseButtonLeft = 0,
    kMouseButtonRight = 1,
    kMouseButtonMiddle = 2,
    kMouseButtonSecondaryTouch
} SkinMouseButtonType;

typedef struct {
    uint32_t keycode;
    uint32_t mod;
} SkinEventKeyData;

typedef struct {
    uint32_t type;
    uint32_t code;
    uint32_t value;
} SkinEventGenericData;

typedef struct {
    uint8_t text[32];
    bool down;
    int32_t keycode;
    int32_t mod;
} SkinEventTextInputData;

typedef struct {
    int x;
    int y;
    int xrel;
    int yrel;
    int x_global;
    int y_global;
    int button;
    bool skip_sync;
} SkinEventMouseData;

typedef struct {
    int x;
    int y;
    int x_global;
    int y_global;
    int64_t tracking_id;
    int pressure;
    int orientation;
    bool button_pressed;
    bool rubber_pointer;
    bool skip_sync;
} SkinEventPenData;

typedef struct {
    int id;
    int pressure;
    int orientation;
    int x;
    int y;
    int x_global;
    int y_global;
    int touch_major;
    int touch_minor;
    bool skip_sync;
} SkinEventTouchData;

typedef struct {
    int x_delta;
    int y_delta;
} SkinEventWheelData;

typedef struct {
    SkinRotation rotation;
} SkinEventLayoutRotateData;

typedef struct {
    int xOffset;
    int yOffset;
    int width;
    int height;
} SkinEventDisplayRegion;

typedef struct {
    int id;
    int xOffset;
    int yOffset;
    int width;
    int height;
    bool add;
} SkinEventMultiDisplay;

typedef struct {
    int x; // Send current window coordinates to maintain window location
    int y;
    int scroll_h; // Height of the horizontal scrollbar, needed for OSX
    double scale;
} SkinEventWindowData;

typedef struct {
    int x;
    int y;
    int xmax;
    int ymax;
    int scroll_h; // Height of the horizontal scrollbar, needed for OSX
} SkinEventScrollData;

typedef struct {
    int delta; // Positive or negative relative rotation
} SkinEventRotaryInputData;

typedef struct {
    int configId;
    int width;
    int height;
    int dpiX;
    int dpiY;
} SkinEventSetDisplayConfigs;

typedef struct {
    SkinEventType type;
    union {
        SkinEventKeyData key;
        SkinEventGenericData generic_event;
        SkinEventMouseData mouse;
        SkinEventPenData pen;
        SkinEventWheelData wheel;
        SkinEventScrollData scroll;
        SkinEventRotaryInputData rotary_input;
        SkinEventTextInputData text;
        SkinEventWindowData window;
        SkinEventLayoutRotateData layout_rotation;
        SkinEventDisplayRegion display_region;
        SkinEventMultiDisplay multi_display;
        SkinEventTouchData multi_touch_point;
        SkinEventSetDisplayConfigs display_configs;
        int display_active_config;
    } u;
} SkinEvent;

// Poll for incoming input events. On success, return true and sets |*event|.
// On failure, i.e. if there are no events, return false.
extern bool skin_event_poll(SkinEvent* event);

// Turns mouse tracking on/off. If tracking is on, we receive the mouse
// position even if there is no click.
extern void skin_enable_mouse_tracking(bool enable);

extern void skin_event_add(SkinEvent *ev);

#ifdef __cplusplus
}
#endif
