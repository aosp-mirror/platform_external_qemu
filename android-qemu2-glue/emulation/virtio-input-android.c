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

#include "android/multitouch-screen.h"



#include "qemu/osdep.h"
//#include "qemu/iov.h"

//#include "hw/qdev.h"
#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-input.h"


#undef CONFIG_CURSES
#include "ui/console.h"
#include "ui/input.h"

#include "standard-headers/linux/input.h"


#define VIRTIO_ID_NAME_EMU "Emu Virtio Input"

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
    void (*translate_mouse_event)(int dx, int dy, int button_state);
} VirtIOInputMultitouchFuncs;

// Enable multi-touch support in the event device. If this is not called,
// or if |funcs| is NULL, the device will not report multi-touch capabilities
// to the kernel, and will never translate mouse events into multi-touch ones.
extern void virtio_input_enable_multitouch(
        const VirtIOInputMultitouchFuncs* funcs);

static void translate_mouse_event(int x, int y, int buttons_state) {
    int pressure = multitouch_is_touch_down(buttons_state) ? 0x81 : 0;
    int finger = multitouch_is_second_finger(buttons_state);
    multitouch_update_pointer(MTES_DEVICE, finger, x, y, pressure,
                              multitouch_should_skip_sync(buttons_state));
}

static const VirtIOInputMultitouchFuncs s_multitouch_funcs = {
        .get_max_slot = multitouch_get_max_slot,
        .translate_mouse_event = translate_mouse_event,
};

static VirtIOInputEmu* s_virtio_input_emu = NULL;
static void virtio_input_emu_evdev_put_mouse(void* opaque,
                                             int dx,
                                             int dy,
                                             int dz,
                                             int buttons_state) {
    VirtIOInputEmu* vemu = (VirtIOInputEmu*)opaque;
    fprintf(stderr, "################## %s dx %d dy %d dz %d button %d \n",
            __FUNCTION__, dx, dy, dz, buttons_state);
    s_multitouch_funcs.translate_mouse_event(dx, dy, buttons_state);
}

static void virtio_input_emu_realize(DeviceState* dev, Error** errp) {
    VirtIOInputEmu* vemu = VIRTIO_INPUT_EMU(dev);
    s_virtio_input_emu = vemu;
    //vemu->mouse_event = virtio_input_emu_evdev_put_mouse;
    //vemu->multitouch_funcs = s_multitouch_funcs;

    // Register the mouse handler for both absolute and relative position
    // reports. (Relative reports are used in trackball mode.)
    // qemu_add_mouse_event_handler(virtio_input_emu_evdev_put_mouse, vemu, 1,
    //                             TYPE_VIRTIO_INPUT_EMU);
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
                                .vendor = const_le16(
                                        0x0627), /* same we use for usb hid
                                                    devices */
                                .product = const_le16(0x0003),
                                .version = const_le16(0x0002),
                        },
        },
        /*{
                .select = VIRTIO_INPUT_CFG_EV_BITS,
                .subsel = EV_ABS,
                .size = 1,
                .u.bitmap =
                        {
                                (1 << ABS_X) | (1 << ABS_Y) | (1 << ABS_Z),
                        },
        },*/
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
                .u.abs.max = const_le32(9),
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
                .u.abs.max = const_le32(10),
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

const unsigned short absmap[] = {
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
    virtio_input_config keys2;
    int i, bit, byte, bmax = 0;

    //Configure EV_ABS
    memset(&keys2, 0, sizeof(keys2));
    for (i = 0; i < sizeof(absmap) / sizeof(unsigned short); i++) {
        bit = absmap[i];
        byte = bit / 8;
        bit  = bit % 8;
        keys2.u.bitmap[byte] |= (1 << bit);
        if (bmax < byte+1) {
            bmax = byte+1;
        }
    }
    keys2.select = VIRTIO_INPUT_CFG_EV_BITS;
    keys2.subsel = EV_ABS;
    keys2.size   = bmax;
    virtio_input_add_config(vinput, &keys2);

    bit = byte = bmax = 0;
    // Configure EV_KEY
    virtio_input_config keys;

    memset(&keys, 0, sizeof(keys));
    byte = BTN_TOUCH / 8;
    bit = BTN_TOUCH % 8;
    keys.u.bitmap[byte] |= (1 << bit);

    keys.select = VIRTIO_INPUT_CFG_EV_BITS;
    keys.subsel = EV_KEY;
    keys.size = byte + 1;
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
    fprintf(stderr, "#### %s type %d code %d value %d \n", __FUNCTION__, type,
            code, value);
    VirtIOInput* vinput = VIRTIO_INPUT(s_virtio_input_emu);
    virtio_input_event event;
    event.type = cpu_to_le16(type);
    event.code = cpu_to_le16(code);
    event.value = cpu_to_le32(value);
    virtio_input_send(vinput, &event);
}

void android_virtio_kbd_mouse_event(int dx, int dy, int dz, int buttonsState) {
    // fprintf(stderr, "################## %s dx %d dy %d dz %d button %d \n",
    // __FUNCTION__, dx, dy,
    //        dz, buttonsState);
    s_multitouch_funcs.translate_mouse_event(dx, dy, buttonsState);
    VirtIOInput* vinput = VIRTIO_INPUT(s_virtio_input_emu);
    virtio_input_event event = {
            .type = cpu_to_le16(EV_SYN),
            .code = cpu_to_le16(SYN_REPORT),
            .value = 0,
    };

    virtio_input_send(vinput, &event);
}
type_init(virtio_register_types)