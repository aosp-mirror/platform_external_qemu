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

#include "android-qemu2-glue/qemu-control-impl.h"
#include "android/multitouch-screen.h"

#include "qemu/osdep.h"

#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-input.h"
#include "hw/virtio/virtio-pci.h"
#include "standard-headers/linux/input.h"
#include "ui/console.h"
#include "ui/input.h"

/* Maximum number of pointers, supported by multi-touch emulation. */
#define MTS_POINTERS_NUM 10

#if DEBUG
#include <stdio.h>

#include "android/utils/misc.h"
#define D(...) (fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"))
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
    int pressure = multitouch_is_touch_down(buttons_state) ? 0x81 : 0;
    int finger = multitouch_is_second_finger(buttons_state);
    multitouch_update_displayId(displayId);
    multitouch_update_pointer(MTES_DEVICE, finger, x, y, pressure,
                              multitouch_should_skip_sync(buttons_state));
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
                .u.abs.max = const_le32(MTS_POINTERS_NUM - 1),
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
                .subsel = ABS_MT_TRACKING_ID,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.max = const_le32(MTS_POINTERS_NUM),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_TOUCH_MAJOR,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.max = const_le32(0x7fffffff),
        },
        {
                .select = VIRTIO_INPUT_CFG_ABS_INFO,
                .subsel = ABS_MT_PRESSURE,
                .size = sizeof(virtio_input_absinfo),
                .u.abs.max = const_le32(0x100),
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

const unsigned short ev_abs_keys[] = {
    ABS_X,
    ABS_Y,
    ABS_Z,
    ABS_MT_SLOT,
    ABS_MT_POSITION_X,
    ABS_MT_POSITION_Y,
    ABS_MT_TRACKING_ID,
    ABS_MT_TOUCH_MAJOR,
    ABS_MT_PRESSURE,
};

static void configure_multi_touch_ev_abs(VirtIOInput* vinput) {
    virtio_input_config keys;
    int i, bit, byte, bmax = 0;

    // Configure EV_ABS
    memset(&keys, 0, sizeof(keys));
    for (i = 0; i < sizeof(ev_abs_keys) / sizeof(unsigned short); i++) {
        bit = ev_abs_keys[i];
        byte = bit / 8;
        bit = bit % 8;
        keys.u.bitmap[byte] |= (1 << bit);
        if (bmax < byte + 1) {
            bmax = byte + 1;
        }
    }
    keys.select = VIRTIO_INPUT_CFG_EV_BITS;
    keys.subsel = EV_ABS;
    keys.size = bmax;
    virtio_input_add_config(vinput, &keys);
    i = 0;
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
    configure_multi_touch_ev_abs(vinput); \
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
    if (type != EV_ABS && type != EV_SYN && type != EV_SW) {
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

    if (!gQAndroidEmulatorWindowAgent->getMultiDisplay(displayId, NULL, NULL, &w,
                                                  &h, NULL, NULL, NULL)) {
        gQAndroidDisplayAgent->getFrameBuffer((int*)&w, (int*)&h, NULL, NULL,
                                              NULL);
    }

    dx = qemu_input_scale_axis(dx, 0, w, INPUT_EVENT_ABS_MIN,
                               INPUT_EVENT_ABS_MAX);
    dy = qemu_input_scale_axis(dy, 0, h, INPUT_EVENT_ABS_MIN,
                               INPUT_EVENT_ABS_MAX);

    translate_mouse_event(dx, dy, buttonsState, displayId);
}
