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
#include "android-qemu2-glue/emulation/virtio-input-android.h"
#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/multitouch-screen.h"

#include "qemu/osdep.h"
#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-input.h"

#include "ui/console.h"
#include "ui/input.h"

#include "standard-headers/linux/input.h"

#define VIRTIO_ID_NAME_EMU "emu_virtio_input_1"

/* Maximum number of pointers, supported by multi-touch emulation. */
#define MTS_POINTERS_NUM 10

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

static VirtIOInputEmu* s_virtio_input_emu = NULL;

static void virtio_input_emu_realize(DeviceState* dev, Error** errp) {
    VirtIOInputEmu* vemu = VIRTIO_INPUT_EMU(dev);
    s_virtio_input_emu = vemu;
}

static void virtio_input_emu_unrealize(DeviceState* dev, Error** errp) {}

static void virtio_input_emu_change_active(VirtIOInput* vinput) {}

static void virtio_input_emu_handle_status(VirtIOInput* vinput,
                                           virtio_input_event* event) {}

static void virtio_input_emu_class_init(ObjectClass* klass, void* data) {
    VirtIOInputClass* vic = VIRTIO_INPUT_CLASS(klass);

    vic->realize = virtio_input_emu_realize;
    vic->unrealize = virtio_input_emu_unrealize;
    vic->change_active = virtio_input_emu_change_active;
    vic->handle_status = virtio_input_emu_handle_status;
}

static struct virtio_input_config virtio_input_emu_config[] = {
        {
                .select = VIRTIO_INPUT_CFG_ID_NAME,
                .size = sizeof(VIRTIO_ID_NAME_EMU),
                .u.string = VIRTIO_ID_NAME_EMU,
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

static void virtio_input_emu_init(Object* obj) {
    VirtIOInput* vinput = VIRTIO_INPUT(obj);
    virtio_input_init_config(vinput, virtio_input_emu_config);
    virtio_input_config keys;
    int i, bit, byte, bmax = 0;

    //Configure EV_ABS
    memset(&keys, 0, sizeof(keys));
    for (i = 0; i < sizeof(ev_abs_keys) / sizeof(unsigned short); i++) {
        bit = ev_abs_keys[i];
        byte = bit / 8;
        bit  = bit % 8;
        keys.u.bitmap[byte] |= (1 << bit);
        if (bmax < byte+1) {
            bmax = byte+1;
        }
    }
    keys.select = VIRTIO_INPUT_CFG_EV_BITS;
    keys.subsel = EV_ABS;
    keys.size = bmax;
    virtio_input_add_config(vinput, &keys);
}

static const TypeInfo virtio_input_emu_info = {
        .name = TYPE_VIRTIO_INPUT_EMU,
        .parent = TYPE_VIRTIO_INPUT,
        .instance_size = sizeof(VirtIOInputEmu),
        .class_init = virtio_input_emu_class_init,
        .instance_init = virtio_input_emu_init,
};

static void virtio_register_types(void) {
    type_register_static(&virtio_input_emu_info);
}

int android_virtio_input_send(int type, int code, int value) {
    D("%s type: %d code: %d value: %d", __func__, type, code, value);
    VirtIOInput* vinput = VIRTIO_INPUT(s_virtio_input_emu);
    virtio_input_event event;
    event.type = cpu_to_le16(type);
    event.code = cpu_to_le16(code);
    event.value = cpu_to_le32(value);
    virtio_input_send(vinput, &event);
}

void android_virtio_kbd_mouse_event(int dx, int dy, int dz, int buttonsState) {
    int w, h = 0;
    gQAndroidDisplayAgent->getFrameBuffer(&w, &h, NULL, NULL, NULL);
    dx = qemu_input_scale_axis(dx, 0, w, INPUT_EVENT_ABS_MIN,
                               INPUT_EVENT_ABS_MAX);
    dy = qemu_input_scale_axis(dy, 0, h, INPUT_EVENT_ABS_MIN,
                               INPUT_EVENT_ABS_MAX);
    translate_mouse_event(dx, dy, buttonsState);
}
type_init(virtio_register_types)