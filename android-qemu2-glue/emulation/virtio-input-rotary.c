// Copyright 2020 The Android Open Source Project
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

#include "android-qemu2-glue/emulation/virtio-input-rotary.h"

#include "qemu/osdep.h"

#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-input.h"
#include "hw/virtio/virtio-pci.h"
#include "standard-headers/linux/input.h"

#define TYPE_VIRTIO_INPUT_ROTARY "virtio_input_rotary"
#define TYPE_VIRTIO_INPUT_ROTARY_PCI "virtio_input_rotary_pci"

static VirtIOInput* s_virtio_rotary_device;

typedef struct VirtIOInputRotary {
    VirtIOInput parent_obj;
} VirtIOInputRotary;

typedef struct VirtIOInputRotaryPCI {
      VirtIOPCIProxy parent_obj;
      VirtIOInputRotary vdev;
} VirtIOInputRotaryPCI;

#define VIRTIO_INPUT_ROTARY_PCI(obj) \
    OBJECT_CHECK(VirtIOInputRotaryPCI, (obj), TYPE_VIRTIO_INPUT_ROTARY_PCI)

static struct virtio_input_config virtio_input_rotary_config[] = {
    {
        .select = VIRTIO_INPUT_CFG_ID_NAME,
        .size = sizeof(TYPE_VIRTIO_INPUT_ROTARY),
        .u.string = TYPE_VIRTIO_INPUT_ROTARY
    },
    {
        .select = VIRTIO_INPUT_CFG_ID_DEVIDS,
        .size = sizeof(struct virtio_input_devids),
        .u.ids = {
            .bustype = const_le16(BUS_VIRTUAL),
            .vendor = const_le16(0),
            .product = const_le16(0),
            .version = const_le16(0),
        }
    },
    {},
};

static void virtio_input_rotary_add_wheel(VirtIOInput* vinput) {
    virtio_input_config cfg = {};

    memset(&cfg, 0, sizeof(virtio_input_config));

    uint8_t input_code = REL_WHEEL;

    int byte = input_code / 8;
    int bit = input_code % 8;

    cfg.select = VIRTIO_INPUT_CFG_EV_BITS;
    cfg.subsel = EV_REL;
    cfg.size = byte + 1;
    cfg.u.bitmap[byte] = 1 << bit;

    virtio_input_add_config(vinput, &cfg);
}

static void virtio_input_rotary_init(Object* obj) {
    VirtIOInput* vinput = VIRTIO_INPUT(obj);

    s_virtio_rotary_device = vinput;

    virtio_input_init_config(vinput, virtio_input_rotary_config);
    virtio_input_rotary_add_wheel(vinput);
}

static void virtio_input_rotary_pci_init(Object* obj) {
    VirtIOInputRotaryPCI* dev = VIRTIO_INPUT_ROTARY_PCI(obj);
    virtio_instance_init_common(obj,
                                &dev->vdev,
                                sizeof(dev->vdev),
                                TYPE_VIRTIO_INPUT_ROTARY);
}

static const TypeInfo types[] = {
    {
        .name = TYPE_VIRTIO_INPUT_ROTARY_PCI,
        .parent = TYPE_VIRTIO_INPUT_PCI,
        .instance_size = sizeof(VirtIOInputRotaryPCI),
        .instance_init = virtio_input_rotary_pci_init
    },
    {
        .name = TYPE_VIRTIO_INPUT_ROTARY,
        .parent = TYPE_VIRTIO_INPUT,
        .instance_size = sizeof(VirtIOInputRotary),
        .instance_init = virtio_input_rotary_init,
    },
};

DEFINE_TYPES(types)

void virtio_send_rotary_event(int delta) {
    VirtIOInput* vinput = VIRTIO_INPUT(s_virtio_rotary_device);

    if (vinput) {
        virtio_input_event motion_event = {
            .type = cpu_to_le16(EV_REL),
            .code = cpu_to_le16(REL_WHEEL),
            .value = cpu_to_le32(delta),
        };
        virtio_input_send(vinput, &motion_event);

        virtio_input_event flush_event = {
            .type = cpu_to_le16(EV_SYN),
            .code = cpu_to_le16(SYN_REPORT),
            .value = 0
        };
        virtio_input_send(vinput, &flush_event);
    }
}
