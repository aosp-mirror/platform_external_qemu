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

#define VIRTIO_INPUT_MULTI_TOUCH_ID_NAME1 "virtio_input_multi_touch_1"

#define VIRTIO_INPUT_MULTI_TOUCH_ID_NAME2 "virtio_input_multi_touch_2"

/* Maximum number of pointers, supported by multi-touch emulation. */
#define MTS_POINTERS_NUM 10

/* Maximum number of virtio input devices*/
#define VIRTIO_INPUT_MAX_NUM 2

#define TYPE_VIRTIO_INPUT_MULTI_TOUCH(id) "virtio_input_multi_touch_"#id

#define VIRTIO_INPUT_MULTI_TOUCH(obj,id) \
    OBJECT_CHECK(VirtIOInputMultiTouch##id, (obj), \ 
                 TYPE_VIRTIO_INPUT_MULTI_TOUCH(id))

#define TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI(id) "virtio_input_multi_touch_pci_"#id

#define VIRTIO_INPUT_MULTI_TOUCH_PCI(obj, num) \
    OBJECT_CHECK(VirtIOInputMultiTouchPCI##num, (obj), \
                 TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI(num))

#define VIRTIO_INPUT_MT(id) typedef struct VirtIOInputMultiTouch##id VirtIOInputMultiTouch##id;
#define VIRTIO_INPUT_MT_PCI(id) typedef struct VirtIOInputMultiTouchPCI##id VirtIOInputMultiTouchPCI##id;

VIRTIO_INPUT_MT(1)
VIRTIO_INPUT_MT(2)

VIRTIO_INPUT_MT_PCI(1)
VIRTIO_INPUT_MT_PCI(2)

#if DEBUG
#include <stdio.h>

#include "android/utils/misc.h"
#define D(...) (fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"))
#else
#define D(...) ((void)0)
#endif

static void translate_mouse_event(int x, int y, int buttons_state) {
    int pressure = multitouch_is_touch_down(buttons_state) ? 0x81 : 0;
    int finger = multitouch_is_second_finger(buttons_state);
    multitouch_update_pointer(MTES_DEVICE, finger, x, y, pressure,
                              multitouch_should_skip_sync(buttons_state));
}

struct VirtIOInputMultiTouch1 {
    VirtIOInput parent_obj;
};

struct VirtIOInputMultiTouch2 {
    VirtIOInput parent_obj;
};

static VirtIOInput* s_current_virtio_input = NULL;

static VirtIOInput* s_virtio_input_multi_touch[VIRTIO_INPUT_MAX_NUM];

static void virtio_input_multi_touch_realize1(DeviceState* dev, Error** errp) {
    VirtIOInput* vinput = VIRTIO_INPUT(dev);
    s_virtio_input_multi_touch[0] = vinput;
}

static void virtio_input_multi_touch_realize2(DeviceState* dev, Error** errp) {
    VirtIOInput* vinput = VIRTIO_INPUT(dev);
    s_virtio_input_multi_touch[1] = vinput;
}

static void virtio_input_multi_touch_unrealize(DeviceState* dev, Error** errp) {
}

static void virtio_input_multi_touch_change_active(VirtIOInput* vinput) {}

static void virtio_input_multi_touch_handle_status(VirtIOInput* vinput,
                                                   virtio_input_event* event) {}

static void virtio_input_multi_touch_class_init1(ObjectClass* klass,
                                                 void* data) {
    VirtIOInputClass* vic = VIRTIO_INPUT_CLASS(klass);

    vic->realize = virtio_input_multi_touch_realize1;
    vic->unrealize = virtio_input_multi_touch_unrealize;
    vic->change_active = virtio_input_multi_touch_change_active;
    vic->handle_status = virtio_input_multi_touch_handle_status;
}

static void virtio_input_multi_touch_class_init2(ObjectClass* klass,
                                                 void* data) {
    VirtIOInputClass* vic = VIRTIO_INPUT_CLASS(klass);

    vic->realize = virtio_input_multi_touch_realize2;
    vic->unrealize = virtio_input_multi_touch_unrealize;
    vic->change_active = virtio_input_multi_touch_change_active;
    vic->handle_status = virtio_input_multi_touch_handle_status;
}

static struct virtio_input_config virtio_input_multi_touch_config1[] = {
        {
                .select = VIRTIO_INPUT_CFG_ID_NAME,
                .size = sizeof(VIRTIO_INPUT_MULTI_TOUCH_ID_NAME1),
                .u.string = VIRTIO_INPUT_MULTI_TOUCH_ID_NAME1,
        },
        {
                .select = VIRTIO_INPUT_CFG_ID_DEVIDS,
                .size = sizeof(struct virtio_input_devids),
                .u.ids =
                        {
                                .bustype = const_le16(BUS_VIRTUAL),
                                // Vendor and product values must be 0 to
                                // indicate an internal device and prevent a
                                // similar lookup that could conflict with a
                                // physical device.
                                .vendor = const_le16(0),
                                .product = const_le16(0),
                                .version = const_le16(0x0001),
                        },
        },

        {/* end of list */},
};

static struct virtio_input_config virtio_input_multi_touch_config2[] = {
        {
                .select = VIRTIO_INPUT_CFG_ID_NAME,
                .size = sizeof(VIRTIO_INPUT_MULTI_TOUCH_ID_NAME2),
                .u.string = VIRTIO_INPUT_MULTI_TOUCH_ID_NAME2,
        },
        {
                .select = VIRTIO_INPUT_CFG_ID_DEVIDS,
                .size = sizeof(struct virtio_input_devids),
                .u.ids =
                        {
                                .bustype = const_le16(BUS_VIRTUAL),
                                // Vendor and product values must be 0 to
                                // indicate an internal device and prevent a
                                // similar lookup that could conflict with a
                                // physical device.
                                .vendor = const_le16(0),
                                .product = const_le16(0),
                                .version = const_le16(0x0002),
                        },
        },
        {/* end of list */},
};

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

static void configure_multi_touch(VirtIOInput* vinput) {
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
static void virtio_input_multi_touch_init1(Object* obj) {
    VirtIOInput* vinput = VIRTIO_INPUT(obj);
    virtio_input_init_config(vinput, virtio_input_multi_touch_config1);
    configure_multi_touch(vinput);
}

static void virtio_input_multi_touch_init2(Object* obj) {
    VirtIOInput* vinput = VIRTIO_INPUT(obj);
    virtio_input_init_config(vinput, virtio_input_multi_touch_config2);
    configure_multi_touch(vinput);
}

static const TypeInfo virtio_input_multi_touch_info1 = {
        .name = TYPE_VIRTIO_INPUT_MULTI_TOUCH(1),
        .parent = TYPE_VIRTIO_INPUT,
        .instance_size = sizeof(VirtIOInputMultiTouch1),
        .class_init = virtio_input_multi_touch_class_init1,
        .instance_init = virtio_input_multi_touch_init1,
};

static const TypeInfo virtio_input_multi_touch_info2 = {
        .name = TYPE_VIRTIO_INPUT_MULTI_TOUCH(2),
        .parent = TYPE_VIRTIO_INPUT,
        .instance_size = sizeof(VirtIOInputMultiTouch2),
        .class_init = virtio_input_multi_touch_class_init2,
        .instance_init = virtio_input_multi_touch_init2,
};

struct VirtIOInputMultiTouchPCI1 {
    VirtIOPCIProxy parent_obj;
    VirtIOInputMultiTouch1 vdev;
};

struct VirtIOInputMultiTouchPCI2 {
    VirtIOPCIProxy parent_obj;
    VirtIOInputMultiTouch2 vdev;
};

static void virtio_input_multi_touch_initfn1(Object* obj) {
    VirtIOInputMultiTouchPCI1* dev = VIRTIO_INPUT_MULTI_TOUCH_PCI(obj, 1);
    virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                TYPE_VIRTIO_INPUT_MULTI_TOUCH(1));
}

static void virtio_input_multi_touch_initfn2(Object* obj) {
    VirtIOInputMultiTouchPCI2* dev = VIRTIO_INPUT_MULTI_TOUCH_PCI(obj, 2);
    virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                TYPE_VIRTIO_INPUT_MULTI_TOUCH(2));
}

static const TypeInfo virtio_input_multi_touch_pci_info1 = {
        .name = TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI(1),
        .parent = TYPE_VIRTIO_INPUT_PCI,
        .instance_size = sizeof(VirtIOInputMultiTouchPCI1),
        .instance_init = virtio_input_multi_touch_initfn1,
};

static const TypeInfo virtio_input_multi_touch_pci_info2 = {
        .name = TYPE_VIRTIO_INPUT_MULTI_TOUCH_PCI(2),
        .parent = TYPE_VIRTIO_INPUT_PCI,
        .instance_size = sizeof(VirtIOInputMultiTouchPCI2),
        .instance_init = virtio_input_multi_touch_initfn2,
};

static void virtio_register_types(void) {
    type_register_static(&virtio_input_multi_touch_info1);
    type_register_static(&virtio_input_multi_touch_pci_info1);
    type_register_static(&virtio_input_multi_touch_info2);
    type_register_static(&virtio_input_multi_touch_pci_info2);
}

int android_virtio_input_send(int type, int code, int value) {
    if (type != EV_ABS && type != EV_SYN) {
        return 0;
    }
    VirtIOInput* vinput = VIRTIO_INPUT(s_current_virtio_input);
    virtio_input_event event;
    event.type = cpu_to_le16(type);
    event.code = cpu_to_le16(code);
    event.value = cpu_to_le32(value);
    virtio_input_send(vinput, &event);
    return 1;
}

void android_virtio_kbd_mouse_event(int dx,
                                    int dy,
                                    int dz,
                                    int buttonsState,
                                    int displayId) {
    int w, h = 0;
    gQAndroidDisplayAgent->getFrameBuffer(&w, &h, NULL, NULL, NULL);
    dx = qemu_input_scale_axis(dx, 0, w, INPUT_EVENT_ABS_MIN,
                               INPUT_EVENT_ABS_MAX);
    dy = qemu_input_scale_axis(dy, 0, h, INPUT_EVENT_ABS_MIN,
                               INPUT_EVENT_ABS_MAX);
    if (displayId < VIRTIO_INPUT_MAX_NUM && displayId >= 0)
        s_current_virtio_input = s_virtio_input_multi_touch[displayId];
    else
        s_current_virtio_input = s_virtio_input_multi_touch[0];
    translate_mouse_event(dx, dy, buttonsState);
}

type_init(virtio_register_types)