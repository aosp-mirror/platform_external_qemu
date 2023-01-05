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

#pragma once

#include "android/emulation/control/user_event_agent.h"
#include "host-common/display_agent.h"
#include "android/sdk-controller-socket.h"
#include "android/skin/event.h"
#include "android/multitouch-port.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/*
 * Encapsulates functionality of multi-touch screen. Main task of this component
 * is to report touch events to the emulated system via event device (see
 * hw/android/goldfish/events_device.c) The source of touch events can be a
 * mouse, or an actual android device that is used for multi-touch emulation.
 * Note that since we need to simultaneousely support a mouse and a device as
 * event source, we need to know which one has sent us a touch event. This is
 * important for proper tracking of pointer IDs when multitouch is in play.
 */

/* Maximum number of pointers, supported by multi-touch emulation. */
#define MTS_POINTERS_NUM    10
/* Signals that pointer is not tracked (or is "up"). */
#define MTS_POINTER_UP      -1

/* Range maximum value of the major and minor touch axis*/
#define MTS_TOUCH_AXIS_RANGE_MAX    0x7fffffff
/* Range maximum value of orientation, corresponds to 90 degrees*/
#define MTS_ORIENTATION_RANGE_MAX   90
/* Range maximum value of pressure, corresponds to 1024 levels of pressure */
#define MTS_PRESSURE_RANGE_MAX      0x400

/*
 * MT_TOOL types, duplicated from include\standard-headers\linux\input.h
 */
#define MT_TOOL_FINGER      0
#define MT_TOOL_PEN         1
#define MT_TOOL_MAX         0x0F

/* Defines a source of multi-touch event. This is used to properly track
 * pointer IDs.
 */
typedef enum MTESource {
    /* The event is associated with a mouse. */
    MTES_MOUSE,
    /* The event is associated with a pen. */
    MTES_PEN,
    /* The event is associated with an actual android device. */
    MTES_DEVICE,
    /* The event is associated with a touch event. */
    MTES_FINGER
} MTESource;

/* Defines the states of a BTN_* event */
typedef enum MTSButtonState {
    MTS_BTN_UP = 0,
    MTS_BTN_DOWN = 1
} MTSButtonState;


/* Initializes MTSState instance.
 * Param:
 *  mtsp - Instance of the multi-touch port connected to the device.
 *  user_event_agent - Instance of QAndroidUserEventAgent to pipe events to VM.
 *  display_agent - Instance of QAndroidDisplayAgent to interact with the display.
 */
extern void multitouch_init(AndroidMTSPort* mtsp,
                            const QAndroidUserEventAgent* user_event_agent,
                            const QAndroidDisplayAgent* display_agent);

/* Convenience functions to set and get the button state bit mask */
extern int multitouch_create_buttons_state(bool is_down,
                                           bool skip_sync,
                                           bool secondary_finger);

extern bool multitouch_is_touch_down(int buttons_state);
extern bool multitouch_should_skip_sync(int buttons_state);
extern bool multitouch_is_second_finger(int buttons_state);

extern void multitouch_update_displayId(int displayId);

extern void multitouch_update(MTESource source,
                              const SkinEvent* const data,
                              int dx,
                              int dy);

/* Handles a MT pointer event.
 * Param:
 *  source - Identifies the source of the event (mouse or a device).
 *  tracking_id - Tracking ID of the pointer.
 *  x, y - Pointer coordinates,
 *  pressure - Pressure value for the pointer.
 */
extern void multitouch_update_pointer(MTESource source,
                                      int tracking_id,
                                      int x,
                                      int y,
                                      int pressure,
                                      bool skip_sync);

/* Handles a MT pen pointer event.
 * Param:
 *  source - Identifies the source of the event (pen).
 *  ev - pointer to struct of the pen pointer event.
 *  x, y - Pointer coordinates,
 *  skip_sync - specifies if sync should be skipped.
 */
extern void multitouch_update_pen_pointer(MTESource source,
                                          const SkinEvent* ev,
                                          int x,
                                          int y,
                                          bool skip_sync);

/* Gets maximum slot index available for the multi-touch emulation. */
extern int multitouch_get_max_slot();

/* A callback set to monitor OpenGLES framebuffer updates.
 * This callback is called by the renderer just before each new frame is
 * displayed, providing a copy of the framebuffer contents.
 * The callback will be called from one of the renderer's threads, so it may
 * require synchronization on any data structures it modifies. The pixels buffer
 * may be overwritten as soon as the callback returns.
 * The pixels buffer is intentionally not const: the callback may modify the data
 * without copying to another buffer if it wants, e.g. in-place RGBA to RGB
 * conversion, or in-place y-inversion.
 * Param:
 *   context        The pointer optionally provided when the callback was
 *                  registered. The client can use this to pass whatever
 *                  information it wants to the callback.
 *   displayId      Default is 0, can also be 1 to 10 if multi display is configured.
 *   width, height  Dimensions of the image, in pixels. Rows are tightly packed;
 *                  there is no inter-row padding.
 *   ydir           Indicates row order: 1 means top-to-bottom order, -1 means
 *                  bottom-to-top order.
 *   format, type   Format and type GL enums, as used in glTexImage2D() or
 *                  glReadPixels(), describing the pixel format.
 *   pixels         The framebuffer image.
 *
 * In the first implementation, ydir is always -1 (bottom to top), format and
 * type are always GL_RGBA and GL_UNSIGNED_BYTE, and the width and height will
 * always be the same as the ones passed to initOpenGLRenderer().
 */
extern void multitouch_opengles_fb_update(void* context,
                                          uint32_t displayId,
                                          int width,
                                          int height,
                                          int ydir,
                                          int format,
                                          int type,
                                          unsigned char* pixels);

/* Pushes the entire framebuffer to the device. This will force the device to
 * refresh the entire screen.
 */
extern void multitouch_refresh_screen(void);

/* Framebuffer update has been handled by the device. */
extern void multitouch_fb_updated(void);

ANDROID_END_HEADER
