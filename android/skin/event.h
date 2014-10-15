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
#ifndef ANDROID_SKIN_EVENT_H
#define ANDROID_SKIN_EVENT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    kEventKeyDown,
    kEventKeyUp,
    kEventTextInput,
    kEventMouseButtonDown,
    kEventMouseButtonUp,
    kEventMouseMotion,
    kEventVideoExpose,
    kEventQuit,
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
    SkinEventType type;
    union {
        SkinEventKeyData key;
        SkinEventMouseData mouse;
        SkinEventTextInputData text;
    } u;
} SkinEvent;

// Poll for incoming input events. On success, return true and sets |*event|.
// On failure, i.e. if there are no events, return false.
extern bool skin_event_poll(SkinEvent* event);

// Set/unset unicode translation for key events.
extern void skin_event_enable_unicode(bool enabled);

#endif  // ANDROID_SKIN_EVENT_H
