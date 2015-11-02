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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    kEventKeyDown,
    kEventKeyUp,
    kEventTextInput,
    kEventMouseButtonDown,
    kEventMouseButtonUp,
    kEventMouseMotion,
    kEventQuit,
    kEventScrollBarChanged,
    kEventSetScale,
    kEventSetZoom,
    kEventVideoExpose,
    kEventWindowMoved,
    kEventScreenChanged,
} SkinEventType;

typedef enum {
    kMouseButtonLeft = 1,
    kMouseButtonRight,
    kMouseButtonCenter,
    kMouseButtonScrollUp,
    kMouseButtonScrollDown,
} SkinMouseButtonType;

typedef struct {
    uint32_t keycode;
    uint32_t mod;
    //uint32_t unicode;
} SkinEventKeyData;

typedef struct {
    bool down;
    uint8_t text[32];
} SkinEventTextInputData;

typedef struct {
    int x;
    int y;
    int xrel;
    int yrel;
    int button;
} SkinEventMouseData;

typedef struct {
    int x; // Send current window coordinates to maintain window location
    int y;
    double scale;
} SkinEventWindowData;

typedef struct {
    int x;
    int y;
    int xmax;
    int ymax;
} SkinEventScrollData;

typedef struct {
    SkinEventType type;
    union {
        SkinEventKeyData key;
        SkinEventMouseData mouse;
        SkinEventScrollData scroll;
        SkinEventTextInputData text;
        SkinEventWindowData window;
    } u;
} SkinEvent;

// Poll for incoming input events. On success, return true and sets |*event|.
// On failure, i.e. if there are no events, return false.
extern bool skin_event_poll(SkinEvent* event);

// Set/unset unicode translation for key events.
extern void skin_event_enable_unicode(bool enabled);

#ifdef __cplusplus
}
#endif
