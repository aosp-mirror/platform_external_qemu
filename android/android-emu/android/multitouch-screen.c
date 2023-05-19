/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "android/multitouch-screen.h"

#include "android/skin/linux_keycodes.h"
#include "android/skin/generic-event-buffer.h"
#include "android/skin/linux_keycodes.h"
#include "android/console.h"  /* for android_hw */
#include "android/hw-events.h"
#include "android/skin/charmap.h"
#include "android/skin/event.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android-qemu2-glue/emulation/virtio-input-multi-touch.h"
#include "android/skin/generic-event-buffer.h"
#include <assert.h>
#include <stdint.h>

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(mtscreen,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(mtscreen)

#define TRACE_ON    1

#if TRACE_ON
#define  T(...)    VERBOSE_PRINT(mtscreen,__VA_ARGS__)
#else
#define  T(...)
#endif

/* Special tracking ID for the pen pointer, the next one after the fingers.*/
#define MTS_POINTER_PEN         MTS_POINTERS_NUM
/* Default value for TOUCH_MAJOR and TOUCH_MINOR*/
#define MTS_TOUCH_AXIS_DEFAULT  0x500

/* Describes state of a multi-touch pointer  */
typedef struct MTSPointerState {
    /* Tracking ID assigned to the pointer by an app emulating multi-touch. */
    int tracking_id;
    /* X - coordinate of the tracked pointer. */
    int x;
    /* Y - coordinate of the tracked pointer. */
    int y;
    /* Current pressure value. */
    int pressure;
    /* The major axes of the ellipse representing the covered area of the touch point. */
    int touch_major;
    /* The minor axes of the ellipse representing the covered area of the touch point. */
    int touch_minor;
} MTSPointerState;

/* Describes state of a pen pointer  */
typedef struct MTSPenState {
    /* Tracking ID assigned to the pen pointer */
    int64_t tracking_id;
    /* X - coordinate of the tracked pointer. */
    int x;
    /* Y - coordinate of the tracked pointer. */
    int y;
    /* Current pressure value. */
    int pressure;
    /* Current orientation of pen. */
    int orientation;
    /* Current state of pen side button. */
    bool button_pressed;
    /* Current state of pointer type. */
    bool rubber_pointer;
} MTSPenState;

/* Describes state of an emulated multi-touch screen. */
typedef struct MTSState {
    /* Android display interface */
    QAndroidDisplayAgent display_agent;
    /* Multi-touch port connected to the device. */
    AndroidMTSPort* mtsp;
    /* Number of tracked pointers. */
    int             tracked_ptr_num;
    /* Index in the 'tracked_pointers' array of the last pointer for which
     * ABS_MT_SLOT was sent. -1 indicates that no slot selection has been made
     * yet. */
    int             current_slot;
    /* Array of multi-touch pointers. */
    MTSPointerState tracked_pointers[MTS_POINTERS_NUM];
    /* Pen pointer. */
    MTSPenState     tracked_pen;
    /* Header collecting framebuffer information and updates. */
    MTFrameHeader   fb_header;
    /* Boolean value indicating if framebuffer updates are currently being
     * transferred to the application running on the device. */
    int             fb_transfer_in_progress;
    /* Indicates direction in which lines are arranged in the framebuffer. If
     * this value is negative, lines are arranged in bottom-up format (i.e. the
     * bottom line is at the beginning of the buffer). */
    int             ydir;
    /* Current display ID. */
    int             displayId;
    /* Current framebuffer pointer. */
    uint8_t*        current_fb;
} MTSState;

/* Default multi-touch screen descriptor */
static MTSState _MTSState = { };

/* Our very own stash of a pointer to the device that handles user events. */
static const QAndroidUserEventAgent* _UserEventAgent;

/* Pushes event to the event device. */
static void
_push_event(int type, int code, int value)
{
    MTSState* const mts_state = &_MTSState;
    SkinGenericEventCode evt;
    evt.type = type;
    evt.code = code;
    evt.value = value;
    evt.displayId = mts_state->displayId;
    _UserEventAgent->sendGenericEvent(evt);
}

/* Gets an index in the MTS's tracking pointers array MTS for the given
 * tracking id.
 * Return:
 *  Index of a matching entry in the MTS's tracking pointers array, or -1 if
 *  matching entry was not found.
 */
static int
_mtsstate_get_pointer_index(const MTSState* mts_state, int tracking_id)
{
    int index;
    for (index = 0; index < MTS_POINTERS_NUM; index++) {
        if (mts_state->tracked_pointers[index].tracking_id == tracking_id) {
            return index;
        }
    }
    return -1;
}

/* Gets an index of the first untracking pointer in the MTS's tracking pointers
 * array.
 * Return:
 *  An index of the first untracking pointer, or -1 if all pointers are tracked.
 */
static int
_mtsstate_get_available_pointer_index(const MTSState* mts_state)
{
    return _mtsstate_get_pointer_index(mts_state, MTS_POINTER_UP);
}



static void _touch_down(MTSState* mts_state,
                        const SkinEventTouchData* const data,
                        int x,
                        int y) {
    const int slot_index = _mtsstate_get_available_pointer_index(mts_state);
    if (slot_index >= 0) {
        mts_state->tracked_ptr_num++;
        mts_state->tracked_pointers[slot_index].tracking_id = data->id;
        mts_state->tracked_pointers[slot_index].x = x;
        mts_state->tracked_pointers[slot_index].y = y;
        mts_state->tracked_pointers[slot_index].pressure = data->pressure;
        mts_state->tracked_pointers[slot_index].touch_major = data->touch_major;
        mts_state->tracked_pointers[slot_index].touch_minor = data->touch_minor;
        if (slot_index != mts_state->current_slot) {
            _push_event(EV_ABS, LINUX_ABS_MT_SLOT, slot_index);
        }
        _push_event(EV_ABS, LINUX_ABS_MT_TRACKING_ID, slot_index);
        _push_event(EV_ABS, LINUX_ABS_MT_PRESSURE, data->pressure);
        _push_event(EV_ABS, LINUX_ABS_MT_POSITION_X, x);
        _push_event(EV_ABS, LINUX_ABS_MT_POSITION_Y, y);
        _push_event(EV_ABS, LINUX_ABS_MT_TOUCH_MAJOR, data->touch_major);
        _push_event(EV_ABS, LINUX_ABS_MT_TOUCH_MINOR, data->touch_minor);
        _push_event(EV_ABS,LINUX_ABS_MT_TOOL_TYPE, MT_TOOL_FINGER);
        mts_state->current_slot = slot_index;
    } else {
        D("MTS pointer count is exceeded.");
        return;
    }
}

static void _touch_update(MTSState* mts_state,
                          const SkinEventTouchData* const data,
                          int x,
                          int y,
                          int slot_index) {
    MTSPointerState* ptr_state = &mts_state->tracked_pointers[slot_index];
    if (ptr_state->x == x && ptr_state->y == y) {
        return;
    }
    if (slot_index != mts_state->current_slot) {
        _push_event(EV_ABS, LINUX_ABS_MT_SLOT, slot_index);
        mts_state->current_slot = slot_index;
    }
    if (ptr_state->pressure != data->pressure) {
        _push_event(EV_ABS, LINUX_ABS_MT_PRESSURE, data->pressure);
        ptr_state->pressure = data->pressure;
    }
    if (ptr_state->x != x) {
        _push_event(EV_ABS, LINUX_ABS_MT_POSITION_X, x);
        ptr_state->x = x;
    }
    if (ptr_state->y != y) {
        _push_event(EV_ABS, LINUX_ABS_MT_POSITION_Y, y);
        ptr_state->y = y;
    }
    if (ptr_state->touch_major != data->touch_major) {
        _push_event(EV_ABS, LINUX_ABS_MT_TOUCH_MAJOR, data->touch_major);
        ptr_state->touch_major = data->touch_major;
    }
    if (ptr_state->touch_minor != data->touch_minor) {
        _push_event(EV_ABS, LINUX_ABS_MT_TOUCH_MINOR, data->touch_minor);
        ptr_state->touch_minor = data->touch_minor;
    }
}


static void _touch_release(MTSState* mts_state, int slot_index) {
    if (slot_index != mts_state->current_slot) {
        _push_event(EV_ABS, LINUX_ABS_MT_SLOT, slot_index);
    }
    _push_event(EV_ABS, LINUX_ABS_MT_PRESSURE, 0);
    _push_event(EV_ABS, LINUX_ABS_MT_TRACKING_ID, -1);
    mts_state->tracked_pointers[slot_index].tracking_id = MTS_POINTER_UP;
    mts_state->tracked_pointers[slot_index].x = 0;
    mts_state->tracked_pointers[slot_index].y = 0;
    mts_state->tracked_pointers[slot_index].pressure = 0;
    mts_state->tracked_pointers[slot_index].touch_major = 0;
    mts_state->tracked_pointers[slot_index].touch_minor = 0;
    mts_state->current_slot = -1;
    mts_state->tracked_ptr_num--;
    assert(mts_state->tracked_ptr_num >= 0);
}
/* Handles a "pointer down" event
 * Param:
 *  mts_state - MTS state descriptor.
 *  tracking_id - Tracking ID of the "downed" pointer.
 *  x, y - "Downed" pointer coordinates,
 *  pressure - Pressure value for the pointer.
 */
static void
_mts_pointer_down(MTSState* mts_state, int tracking_id, int x, int y, int pressure)
{
    /* Get first available slot for the new pointer. */
    const int slot_index = _mtsstate_get_available_pointer_index(mts_state);

    /* Make sure there is a place for the pointer. */
    if (slot_index >= 0) {
        /* Initialize pointer's entry. */
        mts_state->tracked_ptr_num++;
        mts_state->tracked_pointers[slot_index].tracking_id = tracking_id;
        mts_state->tracked_pointers[slot_index].x = x;
        mts_state->tracked_pointers[slot_index].y = y;
        mts_state->tracked_pointers[slot_index].pressure = pressure;

        /* Send events indicating a "pointer down" to the EventHub */
        /* Make sure that correct slot is selected. */
        if (slot_index != mts_state->current_slot) {
            _push_event(EV_ABS, LINUX_ABS_MT_SLOT, slot_index);
            mts_state->current_slot = slot_index;
        }
        _push_event(EV_ABS, LINUX_ABS_MT_TRACKING_ID, slot_index);
        _push_event(EV_ABS, LINUX_ABS_MT_POSITION_X, x);
        _push_event(EV_ABS, LINUX_ABS_MT_POSITION_Y, y);
        _push_event(EV_ABS, LINUX_ABS_MT_TOOL_TYPE, MT_TOOL_FINGER);
        _push_event(EV_ABS, LINUX_ABS_MT_PRESSURE, pressure);
        _push_event(EV_ABS, LINUX_ABS_MT_ORIENTATION, 0);
        _push_event(EV_ABS, LINUX_ABS_MT_TOUCH_MAJOR, MTS_TOUCH_AXIS_DEFAULT);
        _push_event(EV_ABS, LINUX_ABS_MT_TOUCH_MINOR, MTS_TOUCH_AXIS_DEFAULT);
    } else {
        D("MTS pointer count is exceeded.");
        return;
    }
}

/* Handles a "pointer down" pen event
 * Param:
 *  mts_state - MTS state descriptor.
 *  ev - pointer to struct of the "downed" pen pointer event.
 *  x, y - "Downed" pointer coordinates,
 */
static void
_mts_pointer_pen_down(MTSState* mts_state, const SkinEvent* ev, int x, int y)
{
    /* The tracked pointer for the pen has a fixed tracking ID. */
    const int slot_index = MTS_POINTER_PEN;

    /* Initialize pointer's entry. */
    mts_state->tracked_pen.tracking_id = ev->u.pen.tracking_id;
    mts_state->tracked_pen.x = x;
    mts_state->tracked_pen.y = y;
    mts_state->tracked_pen.pressure = ev->u.pen.pressure;
    mts_state->tracked_pen.orientation = ev->u.pen.orientation;
    mts_state->tracked_pen.button_pressed = ev->u.pen.button_pressed;
    mts_state->tracked_pen.rubber_pointer = ev->u.pen.rubber_pointer;

    /* Send events indicating a "pointer down" to the EventHub */
    /* Make sure that correct slot is selected. */
    if (slot_index != mts_state->current_slot) {
        _push_event(EV_ABS, LINUX_ABS_MT_SLOT, slot_index);
        mts_state->current_slot = slot_index;
    }
    _push_event(EV_ABS, LINUX_ABS_MT_TRACKING_ID, slot_index);
    _push_event(EV_ABS, LINUX_ABS_MT_POSITION_X, x);
    _push_event(EV_ABS, LINUX_ABS_MT_POSITION_Y, y);
    if (!ev->u.pen.rubber_pointer) {
        _push_event(EV_ABS, LINUX_ABS_MT_TOOL_TYPE, MT_TOOL_PEN);
    } else {
        _push_event(EV_ABS, LINUX_ABS_MT_TOOL_TYPE, MT_TOOL_MAX);
        _push_event(EV_KEY, LINUX_BTN_TOOL_RUBBER, MTS_BTN_DOWN);
    }
    _push_event(EV_ABS, LINUX_ABS_MT_PRESSURE, ev->u.pen.pressure);
    _push_event(EV_ABS, LINUX_ABS_MT_ORIENTATION, ev->u.pen.orientation);
    if (ev->u.pen.button_pressed) {
        _push_event(EV_KEY, LINUX_BTN_STYLUS, MTS_BTN_DOWN);
    }
}

/* Handles a "pointer up" event
 * Param:
 *  mts_state - MTS state descriptor.
 *  slot_index - Pointer's index in the MTS's array of tracked pointers.
 */
static void
_mts_pointer_up(MTSState* mts_state, int slot_index)
{
    /* Make sure that correct slot is selected. */
    if (slot_index != mts_state->current_slot) {
        _push_event(EV_ABS, LINUX_ABS_MT_SLOT, slot_index);
    }
    /* Send event indicating "pointer up" to the EventHub. */
    _push_event(EV_ABS, LINUX_ABS_MT_PRESSURE, 0);
    _push_event(EV_ABS, LINUX_ABS_MT_TRACKING_ID, MTS_POINTER_UP);

    /* Update MTS descriptor, removing the tracked pointer. */
    mts_state->tracked_pointers[slot_index].tracking_id = MTS_POINTER_UP;
    mts_state->tracked_pointers[slot_index].x = 0;
    mts_state->tracked_pointers[slot_index].y = 0;
    mts_state->tracked_pointers[slot_index].pressure = 0;

    /* Since current slot is no longer tracked, make sure we will do a "select"
     * next time we send events to the EventHub. */
    mts_state->current_slot = -1;
    mts_state->tracked_ptr_num--;
    assert(mts_state->tracked_ptr_num >= 0);
}

/* Handles a "pointer up" pen event
 * Param:
 *  mts_state - MTS state descriptor.
 */
static void
_mts_pointer_pen_up(MTSState* mts_state)
{
    /* The tracked pointer for the pen has a fixed tracking ID. */
    const int slot_index = MTS_POINTER_PEN;

    /* Make sure that correct slot is selected. */
    if (slot_index != mts_state->current_slot) {
        _push_event(EV_ABS, LINUX_ABS_MT_SLOT, slot_index);
    }
    /* Send event indicating "pointer up" to the EventHub. */
    _push_event(EV_ABS, LINUX_ABS_MT_PRESSURE, 0);
    _push_event(EV_ABS, LINUX_ABS_MT_ORIENTATION, 0);
    if (mts_state->tracked_pen.rubber_pointer) {
        _push_event(EV_KEY, LINUX_BTN_TOOL_RUBBER, MTS_BTN_UP);
    }
    if (mts_state->tracked_pen.button_pressed) {
        _push_event(EV_KEY, LINUX_BTN_STYLUS, MTS_BTN_UP);
    }
    _push_event(EV_ABS, LINUX_ABS_MT_TRACKING_ID, MTS_POINTER_UP);

    /* Update MTS descriptor, removing the tracked pointer. */
    mts_state->tracked_pen.tracking_id = MTS_POINTER_UP;
    mts_state->tracked_pen.x = 0;
    mts_state->tracked_pen.y = 0;
    mts_state->tracked_pen.pressure = 0;
    mts_state->tracked_pen.orientation = 0;
    mts_state->tracked_pen.button_pressed = false;
    mts_state->tracked_pen.rubber_pointer = false;

    /* Since current slot is no longer tracked, make sure we will do a "select"
     * next time we send events to the EventHub. */
    mts_state->current_slot = -1;
}

/* Handles a "pointer move" event
 * Param:
 *  mts_state - MTS state descriptor.
 *  slot_index - Pointer's index in the MTS's array of tracked pointers.
 *  x, y - New pointer coordinates,
 *  pressure - Pressure value for the pointer.
 */
static void
_mts_pointer_move(MTSState* mts_state, int slot_index, int x, int y, int pressure)
{
    MTSPointerState* ptr_state = &mts_state->tracked_pointers[slot_index];

    /* Make sure that coordinates have really changed. */
    if (ptr_state->x == x && ptr_state->y == y) {
        /* Coordinates didn't change. Bail out. */
        return;
    }

    /* Make sure that the right slot is selected. */
    if (slot_index != mts_state->current_slot) {
        _push_event(EV_ABS, LINUX_ABS_MT_SLOT, slot_index);
        mts_state->current_slot = slot_index;
    }
    /* Push the changes down. */
    if (ptr_state->x != x) {
        _push_event(EV_ABS, LINUX_ABS_MT_POSITION_X, x);
        ptr_state->x = x;
    }
    if (ptr_state->y != y) {
        _push_event(EV_ABS, LINUX_ABS_MT_POSITION_Y, y);
        ptr_state->y = y;
    }
    if (ptr_state->pressure != pressure) {
        _push_event(EV_ABS, LINUX_ABS_MT_PRESSURE, pressure);
        ptr_state->pressure = pressure;
    }
}

/* Handles a "pointer move" pen event
 * Param:
 *  mts_state - MTS state descriptor.
 *  ev - pointer to struct of the "downed" pen pointer event.
 *  x, y - New pointer coordinates,
 */
static void
_mts_pointer_pen_move(MTSState* mts_state, const SkinEvent* ev, int x, int y)
{
    MTSPenState* ptr_state = &mts_state->tracked_pen;
    /* The tracked pointer for the pen has a fixed tracking ID. */
    const int slot_index = MTS_POINTER_PEN;

    /* Make sure that coordinates or button state have really changed. */
    if ((ptr_state->x == x) && (ptr_state->y == y) &&
            (ptr_state->button_pressed == ev->u.pen.button_pressed)) {
        /* Coordinates and button state didn't change. Bail out. */
        return;
    }

    /* Make sure that the right slot is selected. */
    if (slot_index != mts_state->current_slot) {
        _push_event(EV_ABS, LINUX_ABS_MT_SLOT, slot_index);
        mts_state->current_slot = slot_index;
    }
    /* Push the changes down. */
    if (ptr_state->x != x) {
        _push_event(EV_ABS, LINUX_ABS_MT_POSITION_X, x);
        ptr_state->x = x;
    }
    if (ptr_state->y != y) {
        _push_event(EV_ABS, LINUX_ABS_MT_POSITION_Y, y);
        ptr_state->y = y;
    }
    if (ptr_state->pressure != ev->u.pen.pressure) {
        _push_event(EV_ABS, LINUX_ABS_MT_PRESSURE, ev->u.pen.pressure);
        ptr_state->pressure = ev->u.pen.pressure;
    }
    if (ptr_state->orientation != ev->u.pen.orientation) {
        _push_event(EV_ABS, LINUX_ABS_MT_ORIENTATION, ev->u.pen.orientation);
        ptr_state->orientation = ev->u.pen.orientation;
    }
    if (ptr_state->button_pressed != ev->u.pen.button_pressed) {
        _push_event(EV_KEY, LINUX_BTN_STYLUS, ev->u.pen.button_pressed);
        ptr_state->button_pressed = ev->u.pen.button_pressed;
    }
}

/********************************************************************************
 *                       Multi-touch API
 *******************************************************************************/

/* Multi-touch service initialization flag. */
static int _is_mt_initialized = 0;

/* Callback that is invoked when framebuffer update has been transmitted to the
 * device. */
static AsyncIOAction
_on_fb_sent(void* opaque, SDKCtlDirectPacket* packet, AsyncIOState status)
{
    MTSState* const mts_state = (MTSState*)opaque;

    if (status == ASIO_STATE_SUCCEEDED) {
        /* Lets see if we have accumulated more changes while transmission has been
         * in progress. */
        if (mts_state->fb_header.w && mts_state->fb_header.h &&
            !mts_state->fb_transfer_in_progress) {
            mts_state->fb_transfer_in_progress = 1;
            /* Send accumulated updates. */
            if (mts_port_send_frame(mts_state->mtsp, &mts_state->fb_header,
                                    mts_state->current_fb, _on_fb_sent, mts_state,
                                    mts_state->ydir)) {
                mts_state->fb_transfer_in_progress = 0;
            }
        }
    }

    return ASIO_ACTION_DONE;
}

/* Common handler for framebuffer updates invoked by both, software, and OpenGLES
 * renderers.
 */
static void
_mt_fb_common_update(MTSState* mts_state, int x, int y, int w, int h)
{
    if (mts_state->fb_header.w == 0 && mts_state->fb_header.h == 0) {
        /* First update after previous one has been transmitted to the device. */
        mts_state->fb_header.x = x;
        mts_state->fb_header.y = y;
        mts_state->fb_header.w = w;
        mts_state->fb_header.h = h;
    } else {
        /*
         * Accumulate framebuffer changes in the header.
         */

        /* "right" and "bottom" coordinates of the current update. */
        int right = mts_state->fb_header.x + mts_state->fb_header.w;
        int bottom = mts_state->fb_header.y + mts_state->fb_header.h;

        /* "right" and "bottom" coordinates of the new update. */
        const int new_right = x + w;
        const int new_bottom = y + h;

        /* Accumulate changed rectangle coordinates in the header. */
        if (mts_state->fb_header.x > x) {
            mts_state->fb_header.x = x;
        }
        if (mts_state->fb_header.y > y) {
            mts_state->fb_header.y = y;
        }
        if (right < new_right) {
            right = new_right;
        }
        if (bottom < new_bottom) {
            bottom = new_bottom;
        }
        mts_state->fb_header.w = right - mts_state->fb_header.x;
        mts_state->fb_header.h = bottom - mts_state->fb_header.y;
    }

    /* We will send updates to the device only after previous transmission is
     * completed. */
    if (!mts_state->fb_transfer_in_progress) {
        mts_state->fb_transfer_in_progress = 1;
        if (mts_port_send_frame(mts_state->mtsp, &mts_state->fb_header,
                                mts_state->current_fb, _on_fb_sent, mts_state,
                                mts_state->ydir)) {
            mts_state->fb_transfer_in_progress = 0;
        }
    }
}

/* A callback invoked on framebuffer updates by software renderer.
 * Param:
 *  opaque - MTSState instance.
 *  x, y, w, h - Defines an updated rectangle inside the framebuffer.
 */
static void
_mt_fb_update(void* opaque, int x, int y, int w, int h)
{
    MTSState* const mts_state = (MTSState*)opaque;

    T("Multi-touch: Software renderer framebuffer update: %d:%d -> %dx%d",
      x, y, w, h);

    /* TODO: For sofware renderer general framebuffer properties can change on
     * the fly. Find a callback that can catch that. For now, just copy FB
     * properties over in every FB update. */
    mts_state->display_agent.getFrameBuffer(
            &mts_state->fb_header.disp_width,
            &mts_state->fb_header.disp_height,
            &mts_state->fb_header.bpl,
            &mts_state->fb_header.bpp,
            &mts_state->current_fb);
    mts_state->ydir = 1;

    _mt_fb_common_update(mts_state, x, y, w, h);
}

void
multitouch_opengles_fb_update(void* context,
                              uint32_t displayId,
                              int w, int h, int ydir,
                              int format, int type,
                              unsigned char* pixels)
{
    MTSState* const mts_state = &_MTSState;

    /* Make sure MT port is initialized. */
    if (!_is_mt_initialized) {
        return;
    }

    T("Multi-touch: openGLES framebuffer update: 0:0 -> %dx%d", w, h);

    /* GLES format is always RGBA8888 */
    mts_state->fb_header.bpp = 4;
    mts_state->fb_header.bpl = 4 * w;
    mts_state->fb_header.disp_width = w;
    mts_state->fb_header.disp_height = h;
    mts_state->current_fb = pixels;
    mts_state->ydir = ydir;

    /* GLES emulator alwas update the entire framebuffer. */
    _mt_fb_common_update(mts_state, 0, 0, w, h);
}

void
multitouch_refresh_screen(void)
{
    MTSState* const mts_state = &_MTSState;

    /* Make sure MT port is initialized. */
    if (!_is_mt_initialized) {
        return;
    }

    /* Lets see if any updates have been received so far. */
    if (NULL != mts_state->current_fb) {
        _mt_fb_common_update(mts_state, 0, 0, mts_state->fb_header.disp_width,
                             mts_state->fb_header.disp_height);
    }
}

void
multitouch_fb_updated(void)
{
    MTSState* const mts_state = &_MTSState;

    /* This concludes framebuffer update. */
    mts_state->fb_transfer_in_progress = 0;

    /* Lets see if we have accumulated more changes while transmission has been
     * in progress. */
    if (mts_state->fb_header.w && mts_state->fb_header.h) {
        mts_state->fb_transfer_in_progress = 1;
        /* Send accumulated updates. */
        if (mts_port_send_frame(mts_state->mtsp, &mts_state->fb_header,
                                mts_state->current_fb, _on_fb_sent, mts_state,
                                mts_state->ydir)) {
            mts_state->fb_transfer_in_progress = 0;
        }
    }
}

void multitouch_init(AndroidMTSPort* mtsp,
                     const QAndroidUserEventAgent* user_event_agent,
                     const QAndroidDisplayAgent* display_agent) {
    if (!_is_mt_initialized) {
        MTSState* const mts_state = &_MTSState;
        int index;

        /* Stash away interface objects. */
        _UserEventAgent = user_event_agent;

        /*
         * Initialize the descriptor.
         */

        memset(mts_state, 0, sizeof(MTSState));
        mts_state->tracked_ptr_num = 0;
        mts_state->current_slot = -1;
        for (index = 0; index < MTS_POINTERS_NUM; index++) {
            mts_state->tracked_pointers[index].tracking_id = MTS_POINTER_UP;
        }
        mts_state->tracked_pen.tracking_id = MTS_POINTER_UP;
        mts_state->mtsp = mtsp;
        mts_state->fb_header.header_size = sizeof(MTFrameHeader);
        mts_state->fb_transfer_in_progress = 0;

        mts_state->display_agent = *display_agent;

        /* Initialize framebuffer information in the screen descriptor. */

        mts_state->fb_header.x = mts_state->fb_header.y = 0;
        mts_state->fb_header.w = mts_state->fb_header.h = 0;
        mts_state->fb_transfer_in_progress = 0;

        mts_state->display_agent.getFrameBuffer(
                &mts_state->fb_header.disp_width,
                &mts_state->fb_header.disp_height,
                &mts_state->fb_header.bpl,
                &mts_state->fb_header.bpp,
                NULL);

        /*
         * Set framebuffer update listener.
         */

        mts_state->display_agent.registerUpdateListener(&_mt_fb_update, mts_state);

        _is_mt_initialized = 1;
    }
}

/* Convenience functions to set and get the button state bit mask */
typedef enum {
    kShiftIsTouchDown = 1,
    kShiftShouldSkipSync = 1 << 1,
    kShiftSecondaryTouch = 1 << 2,
} ButtonStateShiftMask;

int multitouch_create_buttons_state(bool is_down,
                                    bool skip_sync,
                                    bool secondary_finger) {
    int state = 0;
    if (is_down)
        state |= kShiftIsTouchDown;
    if (skip_sync)
        state |= kShiftShouldSkipSync;
    if (secondary_finger)
        state |= kShiftSecondaryTouch;
    return state;
}

bool multitouch_is_touch_down(int buttons_state) {
    return buttons_state & kShiftIsTouchDown;
}

bool multitouch_should_skip_sync(int buttons_state) {
    return buttons_state & kShiftShouldSkipSync;
}

bool multitouch_is_second_finger(int buttons_state) {
    return buttons_state & kShiftSecondaryTouch;
}

void multitouch_update_displayId(int displayId) {
    if (displayId < 0 || displayId >= VIRTIO_INPUT_MAX_NUM) {
        return;
    }
    MTSState* const mts_state = &_MTSState;
    mts_state->displayId = displayId;
}

void multitouch_update(MTESource source,
                       const SkinEvent* const data,
                       int dx,
                       int dy) {
    MTSState* const mts_state = &_MTSState;
    const int slot_index =
            _mtsstate_get_pointer_index(mts_state, data->u.multi_touch_point.id);
    if (data->type == kEventTouchBegin) {
        if (slot_index != -1) {
            assert(1 == 2 && "Event of type kEventTouchBegin must provide new ID for touch point. ID already used");
            return;
        }
        _touch_down(mts_state, &data->u.multi_touch_point, dx, dy);
    } else if (data->type == kEventTouchUpdate) {
        _touch_update(mts_state, &data->u.multi_touch_point, dx, dy, slot_index);
    } else if (data->type == kEventTouchEnd) {
        _touch_release(mts_state, slot_index);
    } else {
        assert(1 == 2 && "Event type not supported");
        return;
    }
    if (!data->u.multi_touch_point.skip_sync) {
        _push_event(EV_SYN, LINUX_SYN_REPORT, 0);
    }
}

void multitouch_update_pointer(MTESource source,
                               int tracking_id,
                               int x,
                               int y,
                               int pressure,
                               bool skip_sync) {
    MTSState* const mts_state = &_MTSState;

    /* Find the tracked pointer for the tracking ID. */
    const int slot_index = _mtsstate_get_pointer_index(mts_state, tracking_id);
    if (slot_index < 0) {
        /* This is the first time the pointer is seen. Must be "pressed",
         * otherwise it's "hoovering", which we don't support yet. */
        if (pressure == 0) {
            D("Unexpected MTS pointer update for tracking id: %d",
                    tracking_id);
            return;
        }

        /* This is a "pointer down" event */
        _mts_pointer_down(mts_state, tracking_id, x, y, pressure);
    } else if (pressure == 0) {
        /* This is a "pointer up" event */
        _mts_pointer_up(mts_state, slot_index);
    } else {
        /* This is a "pointer move" event */
        _mts_pointer_move(mts_state, slot_index, x, y, pressure);
    }

    /* The EV_SYN can be skipped if multitouch events were intended to be
     * delivered at the same time, which is how real devices report events
     * that occur simultaneously. */
    if (!skip_sync) {
        _push_event(EV_SYN, LINUX_SYN_REPORT, 0);
    }
}

/* Handles a MT pen pointer event.
 * Param:
 *  source - Identifies the source of the event (pen).
 *  ev - pointer to struct of the pen pointer event.
 *  x, y - Pointer coordinates,
 *  skip_sync - specifies if sync should be skipped.
 */
void multitouch_update_pen_pointer(MTESource source,
                                   const SkinEvent* ev,
                                   int x,
                                   int y,
                                   bool skip_sync) {
    MTSState* const mts_state = &_MTSState;

    /* The tracked pointer is occupied with a different valid pen tracking ID,
     * it means that currently there is another pen touching the screen
     * so we just ignore the new one, because we only support one pen.  */
    if ( (mts_state->tracked_pen.tracking_id != MTS_POINTER_UP) &&
         (mts_state->tracked_pen.tracking_id != ev->u.pen.tracking_id) ) {
        D("Only one pen at a time is suported in MTS.");
        return;
    }

    switch (ev->type) {
    case kEventPenPress:
        /* This should be a new pointer. */
        if (mts_state->tracked_pen.tracking_id != MTS_POINTER_UP) {
            D("Pen tracking pointer should be free before Press.");
            return;
        }
        /* This is a "pointer down" event */
        _mts_pointer_pen_down(mts_state, ev, x, y);
        break;
    case kEventPenRelease:
        /* This pointer should be occupied. */
        if (mts_state->tracked_pen.tracking_id == MTS_POINTER_UP) {
            D("Pen tracking pointer should be occupied before Release.");
        }
        /* This is a "pointer up" event */
        _mts_pointer_pen_up(mts_state);
        break;
    case kEventPenMove:
        /* This pointer should be occupied. */
        if (mts_state->tracked_pen.tracking_id == MTS_POINTER_UP) {
            D("Pen tracking pointer should be occupied before Move.");
            return;
        }
        /* This is a "pointer move" event */
        _mts_pointer_pen_move(mts_state, ev, x, y);
        break;
    default:
        break;
    }

    /* The EV_SYN can be skipped if multitouch events were intended to be
     * delivered at the same time, which is how real devices report events
     * that occur simultaneously. */
    if (!skip_sync) {
        _push_event(EV_SYN, LINUX_SYN_REPORT, 0);
    }
}

int
multitouch_get_max_slot()
{
    return MTS_POINTERS_NUM;
}
