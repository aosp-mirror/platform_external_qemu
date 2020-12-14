/* Copyright (C) 2020 The Android Open Source Project
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <qemu/compiler.h>
#include <qemu/osdep.h>
#include <qemu/typedefs.h>
#include <qapi/error.h>
#include <standard-headers/linux/virtio_vsock.h>
#include <hw/virtio/virtio-vsock.h>

#define VIRTIO_VSOCK_HOST_CID 2
#define VIRTIO_VSOCK_QUEUE_SIZE 128

// virtio spec 5.10.2
enum virtio_vsock_queue_id {
    VIRTIO_VSOCK_QUEUE_ID_RX = 0,    // host to guest
    VIRTIO_VSOCK_QUEUE_ID_TX = 1,    // guest to host
    VIRTIO_VSOCK_QUEUE_ID_EVENT = 2  // host to guest
};

static const Property virtio_vsock_properties[] = {
    DEFINE_PROP_UINT64("guest-cid", VirtIOVSock, guest_cid, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static int vmstate_info_virtio_vsock_impl_load(QEMUFile *f,
                                               void *opaque,
                                               size_t size,
                                               VMStateField *field) {
    return virtio_vsock_impl_load(f, opaque);
}

static int vmstate_info_virtio_vsock_impl_save(QEMUFile *f,
                                               void *opaque,
                                               size_t size,
                                               VMStateField *field,
                                               QJSON *vmdesc) {
    virtio_vsock_impl_save(f, opaque);
    return 0;
}

const VMStateInfo vmstate_info_virtio_vsock_impl = {
    .name = "vmstate_info_virtio_vsock_impl",
    .get = &vmstate_info_virtio_vsock_impl_load,
    .put = &vmstate_info_virtio_vsock_impl_save,
};

/*
static const VMStateDescription vmstate_virtio_vsock = {
    .name = TYPE_VIRTIO_VSOCK,
    .minimum_version_id = 0,
    .version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_VIRTIO_DEVICE,
        VMSTATE_UINT64(guest_cid, VirtIOVSock),
        VMSTATE_POINTER(impl, VirtIOVSock, 0, vmstate_info_virtio_vsock_impl, void*),
        VMSTATE_END_OF_LIST()
    },
};
*/

static void virtio_vsock_device_realize(DeviceState *dev, Error **errp) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);

    if (s->guest_cid <= 2 || s->guest_cid >= UINT32_MAX) {
        error_setg(errp, "guest-cid property is incorrect");
        return;
    }

    VirtIODevice *v = &s->parent;
    virtio_init(v, TYPE_VIRTIO_VSOCK, VIRTIO_ID_VSOCK, sizeof(struct virtio_vsock_config));
    s->host_to_guest_vq =
        virtio_add_queue(v, VIRTIO_VSOCK_QUEUE_SIZE, &virtio_vsock_handle_host_to_guest);
    s->guest_to_host_vq =
        virtio_add_queue(v, VIRTIO_VSOCK_QUEUE_SIZE, &virtio_vsock_handle_guest_to_host);
    s->host_to_guest_event_vq =
        virtio_add_queue(v, 16, &virtio_vsock_handle_event_to_guest);

    virtio_vsock_ctor(s, errp);
}

static void virtio_vsock_device_unrealize(DeviceState *dev, Error **errp) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    VirtIODevice *v = &s->parent;

    virtio_vsock_dctor(s);

    virtio_del_queue(v, VIRTIO_VSOCK_QUEUE_ID_EVENT);
    virtio_del_queue(v, VIRTIO_VSOCK_QUEUE_ID_TX);
    virtio_del_queue(v, VIRTIO_VSOCK_QUEUE_ID_RX);
    virtio_cleanup(v);
}

static uint64_t virtio_vsock_device_get_features(VirtIODevice *dev,
                                                 uint64_t requested_features,
                                                 Error **errp) {
    return requested_features;
}

static void virtio_vsock_get_config(VirtIODevice *dev, uint8_t *raw) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);

    struct virtio_vsock_config cfg = { .guest_cid = s->guest_cid };
    memcpy(raw, &cfg, sizeof(cfg));
}

static void virtio_vsock_set_config(VirtIODevice *dev, const uint8_t *raw) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);

    struct virtio_vsock_config cfg;
    memcpy(&cfg, raw, sizeof(cfg));
    s->guest_cid = cfg.guest_cid;
}

static void virtio_vsock_guest_notifier_mask(VirtIODevice *dev, int idx, bool mask) {
}

static bool virtio_vsock_guest_notifier_pending(VirtIODevice *dev, int idx) {
    return 0;
}

static void virtio_vsock_instance_init(Object* obj) {
    VirtIOVSock *s = VIRTIO_VSOCK(obj);
}

static void virtio_vsock_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->props = (Property*)virtio_vsock_properties;
    /*dc->vmsd = &vmstate_virtio_vsock;*/
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);

    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    vdc->realize = &virtio_vsock_device_realize;
    vdc->unrealize = &virtio_vsock_device_unrealize;
    vdc->get_features = &virtio_vsock_device_get_features;
    vdc->get_config = &virtio_vsock_get_config;
    vdc->set_config = &virtio_vsock_set_config;
    vdc->set_status = &virtio_vsock_set_status;
    vdc->guest_notifier_mask = &virtio_vsock_guest_notifier_mask;
    vdc->guest_notifier_pending = &virtio_vsock_guest_notifier_pending;
}

static const TypeInfo virtio_vsock_typeinfo = {
    .name = TYPE_VIRTIO_VSOCK,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOVSock),
    .instance_init = &virtio_vsock_instance_init,
    .class_init = &virtio_vsock_class_init,
};

int virtio_vsock_is_enabled();

static void virtio_vsock_register_types(void) {
    if (virtio_vsock_is_enabled()) {
        type_register_static(&virtio_vsock_typeinfo);
    }
}

type_init(virtio_vsock_register_types);
