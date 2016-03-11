/* Copyright (C) 2007-2013 The Android Open Source Project
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
#ifndef _HW_GOLDFISH_EVENT_H
#define _HW_GOLDFISH_EVENT_H


// Multi-touch support is implemented by an external component that interfaces
// with the goldfish events device through a simple interface. Please read the
// following to understand it:
//
//   https://www.kernel.org/doc/Documentation/input/multi-touch-protocol.txt
//
// When multi-touch is enabled, goldfish_event_enable_multitouch() must be
// called, passing the address of a GoldfishEventMultitouchFuncs structure.
//
// This will have the following effects:
//
// 1/ When the events device is being setup, it will call get_max_slots(), to
//    retrieve the maximum number of ABS_MTS_SLOTS to report to the kernel,
//
// 2/ Mouse / trackball events are sent from the UI directly to the events
//    device, but must be converted into multi-touch events by the component.
//    To do so, the device will call translate_mouse_event(), which may
//    trigger the component to perform one or more calls to
//    goldfish_event_send().
//

typedef struct {
    // Retrieve the maximum ABS_MTS_SLOT value to report to the
    // kernel. This is called once when the device is setup.
    int (*get_max_slot)(void);

    // Called by the device whenever a mouse or trackball event occurs.
    // The implementation will translate this into appropriate MT events
    // and call |event_func| registered above to do that.
    // |x| and |y| are the absolute mouse position, and |button_state|
    // contains two flags: bit0 indicates whether this is a press (1) or
    // release (0), and bit1 indicates if this is the primary (0) or
    // secondary (1) button.
    void (*translate_mouse_event)(int dx,
                                  int dy,
                                  int button_state);
} GoldfishEventMultitouchFuncs;

// Enable multi-touch support in the event device. If this is not called,
// or if |funcs| is NULL, the device will not report multi-touch capabilities
// to the kernel, and will never translate mouse events into multi-touch ones.
extern void goldfish_events_enable_multitouch(
        const GoldfishEventMultitouchFuncs* funcs);

extern int goldfish_get_event_type_count(void);
extern int goldfish_get_event_type_name(int type, char *buf);
extern int goldfish_get_event_type_value(char *codename);
extern int goldfish_get_event_code_count(const char *typename);
extern int goldfish_get_event_code_name(const char *typename, unsigned int code,
                                  char *buf);
extern int goldfish_get_event_code_value(int typeval, char *codename);
extern int goldfish_event_send(int type, int code, int value);

#endif
