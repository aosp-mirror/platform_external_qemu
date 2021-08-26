// Copyright 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android-qemu2-glue/emulation/virtio-input-multi-touch.h"

#include "android/console.h"
#include "android/multitouch-screen.h"
#include "android/skin/event.h"

#include "qemu/osdep.h"

#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-input.h"
#include "hw/virtio/virtio-pci.h"
#include "standard-headers/linux/input.h"
#include "ui/console.h"
#include "ui/input.h"

#if DEBUG
#include <stdio.h>

#include "android/utils/misc.h"
#define D(...) derror(__VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

#define TYPE_VIRTIO_INPUT_MULTI_TOUCH(id) "virtio_input_multi_touch_"#id

#define VIRTIO_INPUT_MULTI_TOUCH(obj,id) \
    OBJECT_CHECK(VirtIOInputMultiTouch##id, (obj), \
    TYPE_VIRTIO_INPUT_MULTI_TOUCH(id))

#define TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI(id) "virtio_input_multi_touch_pci_"#id

#define VIRTIO_INPUT_MULTI_TOUCH_PCI(obj, num) \
    OBJECT_CHECK(VirtIOInputMultiTouchPCI##num, (obj), \
                 TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI(num))

static VirtIOInput* s_virtio_input_multi_touch[VIRTIO_INPUT_MAX_NUM];

static void translate_mouse_event(int x,
                                  int y,
                                  int buttons_state,
                                  int displayId) {
    int pressure = multitouch_is_touch_down(buttons_state) ?
                                            MTS_PRESSURE_RANGE_MAX : 0;
    int finger = multitouch_is_second_finger(buttons_state);
    multitouch_update_displayId(displayId);
    multitouch_update_pointer(MTES_MOUSE, finger, x, y, pressure,
                              multitouch_should_skip_sync(buttons_state));
    multitouch_update_displayId(0);
}

static void translate_pen_event(int x,
                                int y,
                                const SkinEvent* ev,
                                int buttons_state,
                                int displayId) {
    multitouch_update_displayId(displayId);
    multitouch_update_pen_pointer(MTES_PEN, ev, x, y,
                                  multitouch_should_skip_sync(buttons_state));
    multitouch_update_displayId(0);
}

void android_virtio_touch_event(const SkinEvent* const data,
                                int displayId) {
    uint32_t w, h = 0;

    if (displayId < 0 || displayId >= VIRTIO_INPUT_MAX_NUM) {
        displayId = 0;
    }

    if (!getConsoleAgents()->multi_display->getMultiDisplay(
                displayId, NULL, NULL, &w, &h, NULL, NULL, NULL)) {
        getConsoleAgents()->display->getFrameBuffer((int*)&w, (int*)&h, NULL,
                                                    NULL, NULL);
    }

    int dx = qemu_input_scale_axis(data->u.multi_touch_point.x, 0, w, INPUT_EVENT_ABS_MIN,
                                   INPUT_EVENT_ABS_MAX);
    int dy = qemu_input_scale_axis(data->u.multi_touch_point.y, 0, h, INPUT_EVENT_ABS_MIN,
                                   INPUT_EVENT_ABS_MAX);


    multitouch_update_displayId(displayId);
    multitouch_update(MTES_FINGER, data, dx, dy);
    multitouch_update_displayId(0);
}

#define VIRTIO_INPUT_MT_STRUCT(id) \
typedef struct VirtIOInputMultiTouch##id { \
    VirtIOInput parent_obj; \
} VirtIOInputMultiTouch##id;

VIRTIO_INPUT_MT_STRUCT(1)
VIRTIO_INPUT_MT_STRUCT(2)
VIRTIO_INPUT_MT_STRUCT(3)
VIRTIO_INPUT_MT_STRUCT(4)
VIRTIO_INPUT_MT_STRUCT(5)
VIRTIO_INPUT_MT_STRUCT(6)
VIRTIO_INPUT_MT_STRUCT(7)
VIRTIO_INPUT_MT_STRUCT(8)
VIRTIO_INPUT_MT_STRUCT(9)
VIRTIO_INPUT_MT_STRUCT(10)
VIRTIO_INPUT_MT_STRUCT(11)

#define VIRTIO_INPUT_MT_PCI_STRUCT(id) \
typedef struct VirtIOInputMultiTouchPCI##id { \
    VirtIOPCIProxy parent_obj; \
    VirtIOInputMultiTouch##id vdev; \
} VirtIOInputMultiTouchPCI##id;

VIRTIO_INPUT_MT_PCI_STRUCT(1)
VIRTIO_INPUT_MT_PCI_STRUCT(2)
VIRTIO_INPUT_MT_PCI_STRUCT(3)
VIRTIO_INPUT_MT_PCI_STRUCT(4)
VIRTIO_INPUT_MT_PCI_STRUCT(5)
VIRTIO_INPUT_MT_PCI_STRUCT(6)
VIRTIO_INPUT_MT_PCI_STRUCT(7)
VIRTIO_INPUT_MT_PCI_STRUCT(8)
VIRTIO_INPUT_MT_PCI_STRUCT(9)
VIRTIO_INPUT_MT_PCI_STRUCT(10)
VIRTIO_INPUT_MT_PCI_STRUCT(11)

// Vendor and product values must be 0 to
// indicate an internal device and prevent a
// similar lookup that could conflict with a
// physical device.
#define VIRTIO_INPUT_MT_CONFIG(id) \
static struct virtio_input_config virtio_input_multi_touch_config##id[] = { \
        {   .select = VIRTIO_INPUT_CFG_ID_NAME, \
            .size = sizeof(TYPE_VIRTIO_INPUT_MULTI_TOUCH(id)), \
            .u.string = TYPE_VIRTIO_INPUT_MULTI_TOUCH(id), \
        },\
        {    .select = VIRTIO_INPUT_CFG_ID_DEVIDS, \
                .size = sizeof(struct virtio_input_devids), \
                .u.ids = { .bustype = const_le16(BUS_VIRTUAL), \
                           .vendor = const_le16(0), \
                           .product = const_le16(0), \
                           .version = const_le16(0), \
                } \
        }, \
        {/* end of list */}, \
};

VIRTIO_INPUT_MT_CONFIG(1)
VIRTIO_INPUT_MT_CONFIG(2)
VIRTIO_INPUT_MT_CONFIG(3)
VIRTIO_INPUT_MT_CONFIG(4)
VIRTIO_INPUT_MT_CONFIG(5)
VIRTIO_INPUT_MT_CONFIG(6)
VIRTIO_INPUT_MT_CONFIG(7)
VIRTIO_INPUT_MT_CONFIG(8)
VIRTIO_INPUT_MT_CONFIG(9)
VIRTIO_INPUT_MT_CONFIG(10)
VIRTIO_INPUT_MT_CONFIG(11)

static struct virtio_input_config multi_touch_config[] = {
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_X,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.min = const_le32(INPUT_EVENT_ABS_MIN),
                .u.abs.max = const_le32(INPUT_EVENT_ABS_MAX),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_Y,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.min = const_le32(INPUT_EVENT_ABS_MIN),
                .u.abs.max = const_le32(INPUT_EVENT_ABS_MAX),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_Z,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.min = const_le32(INPUT_EVENT_ABS_MIN),
                .u.abs.max = const_le32(1),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_SLOT,
                .size = sizeof(virtio_input_absinfo),
                // SLOT max value seems to be TRACKING_ID-1
                .u.abs.max = const_le32(MTS_POINTERS_NUM),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_TOUCH_MAJOR,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.max = const_le32(MTS_TOUCH_AXIS_RANGE_MAX),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_TOUCH_MINOR,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.max = const_le32(MTS_TOUCH_AXIS_RANGE_MAX),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_ORIENTATION,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.max = const_le32(MTS_ORIENTATION_RANGE_MAX),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_POSITION_X,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.max = const_le32(INPUT_EVENT_ABS_MAX),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_POSITION_Y,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.max = const_le32(INPUT_EVENT_ABS_MAX),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_TOOL_TYPE,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.max = const_le32(MT_TOOL_MAX),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_TRACKING_ID,
                .size = sizeof(virtio_input_absinfo),
                // TRACKING_ID max value seems to be 0xFFFF
                .u.abs.max = const_le32(MTS_POINTERS_NUM + 1),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_PRESSURE,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.max = const_le32(MTS_PRESSURE_RANGE_MAX),
        },
        {       // Needed for fold/unfold (EV_SW)
                .select    = VIRTIO_INPUT_CFG_EV_BITS,
                .subsel    = EV_SW,
                .size      = 1,
                .u.bitmap  = {
                    1,
                },
        },
        {/* end of list */},
};

static const unsigned short ev_key_codes[] = {
    BTN_TOOL_RUBBER,
    BTN_STYLUS
};

static const unsigned short ev_abs_codes[] = {
    ABS_X,
    ABS_Y,
    ABS_Z,
    ABS_MT_SLOT,
    ABS_MT_TOUCH_MAJOR,
    ABS_MT_TOUCH_MINOR,
    ABS_MT_ORIENTATION,
    ABS_MT_POSITION_X,
    ABS_MT_POSITION_Y,
    ABS_MT_TOOL_TYPE,
    ABS_MT_TRACKING_ID,
    ABS_MT_PRESSURE
};

// Configure an EV_* class of codes
static void virtio_multi_touch_ev_config(VirtIOInput* vinput,
                                         const unsigned short* ev_codes,
                                         size_t ev_size,
                                         uint8_t ev_type) {
    virtio_input_config keys;
    int i, bit, byte, bmax = 0;

    memset(&keys, 0, sizeof(keys));
    for (i = 0; i < ev_size; i++) {
        bit = ev_codes[i];
        byte = bit / 8;
        bit = bit % 8;
        keys.u.bitmap[byte] |= (1 << bit);
        if (bmax < byte + 1) {
            bmax = byte + 1;
        }
    }
    keys.select = VIRTIO_INPUT_CFG_EV_BITS;
    keys.subsel = ev_type;
    keys.size = bmax;
    virtio_input_add_config(vinput, &keys);
}

static void configure_multi_touch_ev(VirtIOInput* vinput) {
    // Configure EV_KEY
    virtio_multi_touch_ev_config(vinput, ev_key_codes,
                                        ARRAY_SIZE(ev_key_codes), EV_KEY);
    // Configure EV_ABS
    virtio_multi_touch_ev_config(vinput, ev_abs_codes,
                                        ARRAY_SIZE(ev_abs_codes), EV_ABS);

    int i = 0;
    while (multi_touch_config[i].select) {
        virtio_input_add_config(vinput, multi_touch_config + i);
        i++;
    }
}

#define VIRTIO_INPUT_MT_INSTANCE_INIT(id) \
static void virtio_input_multi_touch_init##id(Object* obj) { \
    VirtIOInput* vinput = VIRTIO_INPUT(obj); \
    s_virtio_input_multi_touch[(id)-1] = vinput; \
    virtio_input_init_config(vinput, virtio_input_multi_touch_config##id); \
    configure_multi_touch_ev(vinput); \
};

VIRTIO_INPUT_MT_INSTANCE_INIT(1)
VIRTIO_INPUT_MT_INSTANCE_INIT(2)
VIRTIO_INPUT_MT_INSTANCE_INIT(3)
VIRTIO_INPUT_MT_INSTANCE_INIT(4)
VIRTIO_INPUT_MT_INSTANCE_INIT(5)
VIRTIO_INPUT_MT_INSTANCE_INIT(6)
VIRTIO_INPUT_MT_INSTANCE_INIT(7)
VIRTIO_INPUT_MT_INSTANCE_INIT(8)
VIRTIO_INPUT_MT_INSTANCE_INIT(9)
VIRTIO_INPUT_MT_INSTANCE_INIT(10)
VIRTIO_INPUT_MT_INSTANCE_INIT(11)

#define VIRTIO_INPUT_MT_TYPE_INFO(id) \
    { \
        .name = TYPE_VIRTIO_INPUT_MULTI_TOUCH(id), \
        .parent = TYPE_VIRTIO_INPUT, \
        .instance_size = sizeof(VirtIOInputMultiTouch##id), \
        .instance_init = virtio_input_multi_touch_init##id, \
    }

#define VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(id) \
static void virtio_input_multi_touch_pci_init##id(Object* obj) { \
    VirtIOInputMultiTouchPCI##id* dev = VIRTIO_INPUT_MULTI_TOUCH_PCI(obj, id); \
    virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev), \
                                TYPE_VIRTIO_INPUT_MULTI_TOUCH(id));} \

VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(1)
VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(2)
VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(3)
VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(4)
VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(5)
VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(6)
VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(7)
VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(8)
VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(9)
VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(10)
VIRTIO_INPUT_MT_PCI_INSTANCE_INIT(11)

#define VIRTIO_INPUT_MT_PCI_TYPE_INFO(id) \
    { \
        .name = TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI(id), \
        .parent = TYPE_VIRTIO_INPUT_PCI, \
        .instance_size = sizeof(VirtIOInputMultiTouchPCI##id), \
        .instance_init = virtio_input_multi_touch_pci_init##id, \
    }

static const TypeInfo types[] = {
    VIRTIO_INPUT_MT_TYPE_INFO(1),
    VIRTIO_INPUT_MT_TYPE_INFO(2),
    VIRTIO_INPUT_MT_TYPE_INFO(3),
    VIRTIO_INPUT_MT_TYPE_INFO(4),
    VIRTIO_INPUT_MT_TYPE_INFO(5),
    VIRTIO_INPUT_MT_TYPE_INFO(6),
    VIRTIO_INPUT_MT_TYPE_INFO(7),
    VIRTIO_INPUT_MT_TYPE_INFO(8),
    VIRTIO_INPUT_MT_TYPE_INFO(9),
    VIRTIO_INPUT_MT_TYPE_INFO(10),
    VIRTIO_INPUT_MT_TYPE_INFO(11),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(1),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(2),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(3),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(4),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(5),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(6),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(7),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(8),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(9),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(10),
    VIRTIO_INPUT_MT_PCI_TYPE_INFO(11),
};

DEFINE_TYPES(types)

int android_virtio_input_send(int type, int code, int value, int displayId) {
    if (type != EV_ABS && type != EV_KEY && type != EV_SYN && type != EV_SW) {
        return 0;
    }
    if (displayId < 0 || displayId >= VIRTIO_INPUT_MAX_NUM) {
        displayId = 0;
    }
    VirtIOInput* vinput = VIRTIO_INPUT(s_virtio_input_multi_touch[displayId]);
    if (vinput) {
        virtio_input_event event;
        event.type = cpu_to_le16(type);
        event.code = cpu_to_le16(code);
        event.value = cpu_to_le32(value);
        virtio_input_send(vinput, &event);
    }
    return 1;
}

void android_virtio_kbd_mouse_event(int dx,
                                    int dy,
                                    int dz,
                                    int buttonsState,
                                    int displayId) {
    uint32_t w, h = 0;

    if (displayId < 0 || displayId >= VIRTIO_INPUT_MAX_NUM) {
        displayId = 0;
    }

    if (!getConsoleAgents()->multi_display->getMultiDisplay(displayId, NULL, NULL, &w,
                                                  &h, NULL, NULL, NULL)) {
        getConsoleAgents()->display->getFrameBuffer((int*)&w, (int*)&h, NULL, NULL,
                                              NULL);
    }

    dx = qemu_input_scale_axis(dx, 0, w, INPUT_EVENT_ABS_MIN,
                               INPUT_EVENT_ABS_MAX);
    dy = qemu_input_scale_axis(dy, 0, h, INPUT_EVENT_ABS_MIN,
                               INPUT_EVENT_ABS_MAX);

    translate_mouse_event(dx, dy, buttonsState, displayId);
}

void android_virtio_pen_event(int dx,
                              int dy,
                              const SkinEvent* ev,
                              int buttonsState,
                              int displayId) {
    uint32_t w, h = 0;

    if (displayId < 0 || displayId >= VIRTIO_INPUT_MAX_NUM) {
        displayId = 0;
    }

    if (!getConsoleAgents()->multi_display->getMultiDisplay(displayId, NULL, NULL, &w,
                                                  &h, NULL, NULL, NULL)) {
        getConsoleAgents()->display->getFrameBuffer((int*)&w, (int*)&h, NULL, NULL,
                                              NULL);
    }

    dx = qemu_input_scale_axis(dx, 0, w, INPUT_EVENT_ABS_MIN,
                               INPUT_EVENT_ABS_MAX);
    dy = qemu_input_scale_axis(dy, 0, h, INPUT_EVENT_ABS_MIN,
                               INPUT_EVENT_ABS_MAX);

    translate_pen_event(dx, dy, ev, buttonsState, displayId);
}
