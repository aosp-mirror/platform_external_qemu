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

#ifndef ANDROID_MULTITOUCH_SCREEN_H_
#define ANDROID_MULTITOUCH_SCREEN_H_

#include "android/multitouch-port.h"

/*
 * Encapsulates functionality of multi-touch screen. Main task of this component
 * is to report touch events to the emulated system via event device (see
 * hw/goldfish_events_device.c) The source of touch events can be a mouse, or an
 * actual android device that is used for multi-touch emulation. Note that since
 * we need to simultaneousely support a mouse and a device as event source, we
 * need to know which one has sent us a touch event. This is important for proper
 * tracking of pointer IDs when multitouch is in play.
 */

/* Defines a source of multi-touch event. This is used to properly track
 * pointer IDs.
 */
typedef enum MTESource {
    /* The event is associated with a mouse. */
    MTES_MOUSE,
    /* The event is associated with an actual android device. */
    MTES_DEVICE,
} MTESource;

/* Initializes MTSState instance.
 * Param:
 *  mtsp - Instance of the multi-touch port connected to the device.
 */
extern void multitouch_init(AndroidMTSPort* mtsp);

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
                                      int pressure);

/* Gets maximum slot index available for the multi-touch emulation. */
extern int multitouch_get_max_slot();

/* Saves screen size reported by the device that emulates multi-touch. */
extern void multitouch_set_device_screen_size(int width, int height);

#endif  /* ANDROID_MULTITOUCH_SCREEN_H_ */

