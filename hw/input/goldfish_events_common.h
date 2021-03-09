/* Copyright (C) 2007-2016 The Android Open Source Project
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
#ifndef GOLDFISH_EVENTS_UTIL_H
#define GOLDFISH_EVENTS_UTIL_H

/*
 * This header file contains code common to goldfish_events and goldfish_rotary.
 * It should only be included in files implementing the goldfish_events-like
 * devices.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "ui/input.h"
#include "ui/console.h"
#include "hw/input/android_keycodes.h"
#include "hw/input/linux_keycodes.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif


#define MAX_EVENTS (4096 * 4)

/* Event types (as per Linux input event layer) */
#define EV_SYN                  0x00
#define EV_KEY                  0x01
#define EV_REL                  0x02
#define EV_ABS                  0x03
#define EV_MSC                  0x04
#define EV_SW                   0x05
#define EV_LED                  0x11
#define EV_SND                  0x12
#define EV_REP                  0x14
#define EV_FF                   0x15
#define EV_PWR                  0x16
#define EV_FF_STATUS            0x17
#define EV_MAX                  0x1f

/* Absolute axes */
#define ABS_X                   0x00
#define ABS_Y                   0x01
#define ABS_Z                   0x02
#define ABS_MT_SLOT             0x2f    /* MT slot being modified */
#define ABS_MT_TOUCH_MAJOR      0x30    /* Major axis of touching ellipse */
#define ABS_MT_TOUCH_MINOR      0x31    /* Minor axis (omit if circular) */
#define ABS_MT_WIDTH_MAJOR      0x32    /* Major axis of approaching ellipse */
#define ABS_MT_WIDTH_MINOR      0x33    /* Minor axis (omit if circular) */
#define ABS_MT_ORIENTATION      0x34    /* Ellipse orientation */
#define ABS_MT_POSITION_X       0x35    /* Center X ellipse position */
#define ABS_MT_POSITION_Y       0x36    /* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE        0x37    /* Type of touching device */
#define ABS_MT_BLOB_ID          0x38    /* Group a set of packets as a blob */
#define ABS_MT_TRACKING_ID      0x39    /* Unique ID of initiated contact */
#define ABS_MT_PRESSURE         0x3a    /* Pressure on contact area */
#define ABS_MT_DISTANCE         0x3b    /* Contact hover distance */
#define ABS_MAX                 0x3f

/* Relative axes */
#define REL_X                   0x00
#define REL_Y                   0x01
#define REL_WHEEL               0x08

#define BTN_MISC 0x100
#define BTN_0 0x100
#define BTN_1 0x101
#define BTN_2 0x102
#define BTN_3 0x103
#define BTN_4 0x104
#define BTN_5 0x105
#define BTN_6 0x106
#define BTN_7 0x107
#define BTN_8 0x108
#define BTN_9 0x109
#define BTN_MOUSE 0x110
#define BTN_LEFT 0x110
#define BTN_RIGHT 0x111
#define BTN_MIDDLE 0x112
#define BTN_SIDE 0x113
#define BTN_EXTRA 0x114
#define BTN_FORWARD 0x115
#define BTN_BACK 0x116
#define BTN_TASK 0x117
#define BTN_JOYSTICK 0x120
#define BTN_TRIGGER 0x120
#define BTN_THUMB 0x121
#define BTN_THUMB2 0x122
#define BTN_TOP 0x123
#define BTN_TOP2 0x124
#define BTN_PINKIE 0x125
#define BTN_BASE 0x126
#define BTN_BASE2 0x127
#define BTN_BASE3 0x128
#define BTN_BASE4 0x129
#define BTN_BASE5 0x12a
#define BTN_BASE6 0x12b
#define BTN_DEAD 0x12f
#define BTN_GAMEPAD 0x130
#define BTN_A 0x130
#define BTN_B 0x131
#define BTN_C 0x132
#define BTN_X 0x133
#define BTN_Y 0x134
#define BTN_Z 0x135
#define BTN_TL 0x136
#define BTN_TR 0x137
#define BTN_TL2 0x138
#define BTN_TR2 0x139
#define BTN_SELECT 0x13a
#define BTN_START 0x13b
#define BTN_MODE 0x13c
#define BTN_THUMBL 0x13d
#define BTN_THUMBR 0x13e
#define BTN_DIGI 0x140
#define BTN_TOOL_PEN 0x140
#define BTN_TOOL_RUBBER 0x141
#define BTN_TOOL_BRUSH 0x142
#define BTN_TOOL_PENCIL 0x143
#define BTN_TOOL_AIRBRUSH 0x144
#define BTN_TOOL_FINGER 0x145
#define BTN_TOOL_MOUSE 0x146
#define BTN_TOOL_LENS 0x147
#define BTN_TOUCH 0x14a
#define BTN_STYLUS 0x14b
#define BTN_STYLUS2 0x14c
#define BTN_TOOL_DOUBLETAP 0x14d
#define BTN_TOOL_TRIPLETAP 0x14e
#define BTN_WHEEL 0x150
#define BTN_GEAR_DOWN 0x150
#define BTN_GEAR_UP  0x151

/* Switches */
#define SW_LID               0
#define SW_TABLET_MODE       1
#define SW_HEADPHONE_INSERT  2
#define SW_MICROPHONE_INSERT 4

#define KEY_CODE(_name, _val) {#_name, _val}
#define BTN_CODE(_code) {#_code, (_code)}

#define EV_TYPE(_type, _codes)    {#_type, (_type), _codes}
#define EV_TYPE_END     {NULL, 0}

#define EV_CODE(_code)    {#_code, (_code)}
#define EV_CODE_END     {NULL, 0}

enum {
    REG_READ        = 0x00,
    REG_SET_PAGE    = 0x00,
    REG_LEN         = 0x04,
    REG_DATA        = 0x08,

    PAGE_NAME       = 0x00000,
    PAGE_EVBITS     = 0x10000,
    PAGE_ABSDATA    = 0x20000 | EV_ABS,
};

/* These corresponds to the state of the driver.
 * Unfortunately, we have to buffer events coming
 * from the UI, since the kernel driver is not
 * capable of receiving them until XXXXXX
 */
enum {
    STATE_INIT = 0,  /* The device is initialized */
    STATE_BUFFERED,  /* Events have been buffered, but no IRQ raised yet */
    STATE_LIVE       /* Events can be sent directly to the kernel */
};

typedef struct GoldfishEvDevState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    MemoryRegion iomem;
    qemu_irq irq;

    /* Device properties */
    bool have_dpad;
    bool have_trackball;
    bool have_camera;
    bool have_keyboard;
    bool have_keyboard_lid;
    bool have_tablet_mode;
    bool have_touch;
    bool have_multitouch;

    /* Actual device state */
    int32_t page;
    uint32_t events[MAX_EVENTS];
    uint32_t first;
    uint32_t last;
    uint32_t state;

    /* Latency measurement */
    bool measure_latency;
    uint32_t enqueue_times_us[MAX_EVENTS];
    uint32_t dequeue_times_us[MAX_EVENTS];

    uint32_t modifier_state;

    /* All data below here is set up at realize and not modified thereafter */

    const char *name;

    struct {
        size_t   len;
        uint8_t *bits;
    } ev_bits[EV_MAX + 1];

    int32_t *abs_info;
    size_t abs_info_count;
#ifdef _WIN32
    SRWLOCK lock;
#else
    pthread_mutex_t lock;
#endif
} GoldfishEvDevState;

#define GOLDFISHEVDEV_VM_STATE_DESCRIPTION(device_name) \
{ \
    .name = (device_name),\
    .version_id = 1,\
    .minimum_version_id = 1,\
    .fields = (VMStateField[]) {\
        VMSTATE_INT32(page, GoldfishEvDevState),\
        VMSTATE_UINT32_ARRAY(events, GoldfishEvDevState, MAX_EVENTS),\
        VMSTATE_UINT32(first, GoldfishEvDevState),\
        VMSTATE_UINT32(last, GoldfishEvDevState),\
        VMSTATE_UINT32(state, GoldfishEvDevState),\
        VMSTATE_UINT32(modifier_state, GoldfishEvDevState),\
        VMSTATE_END_OF_LIST()\
    }\
}

#define GOLDFISHEVDEV(obj, type_name) \
    OBJECT_CHECK(GoldfishEvDevState, (obj), (type_name))
void goldfish_enqueue_event(GoldfishEvDevState *s,
                   unsigned int type, unsigned int code, int value);
uint64_t goldfish_events_read(void *opaque, hwaddr offset, unsigned size);
void goldfish_events_write(void *opaque, hwaddr offset, uint64_t val, unsigned size);
void goldfish_events_set_bits(GoldfishEvDevState *s, int type, int bitl, int bith);
void goldfish_events_set_bit(GoldfishEvDevState *s, int  type, int  bit);
void goldfish_events_clr_bit(GoldfishEvDevState *s, int type, int bit);
int goldfish_event_drop_count();

#endif // GOLDFISH_EVENTS_UTIL_H
