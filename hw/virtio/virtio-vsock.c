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

#define VIRTIO_VSOCK_QUEUE_SIZE 128

// virtio spec 5.10.2
enum virtio_vsock_queue_id {
    VIRTIO_VSOCK_QUEUE_ID_RX = 0,
    VIRTIO_VSOCK_QUEUE_ID_TX = 1,
    VIRTIO_VSOCK_QUEUE_ID_EVENT = 2
};

static void virtio_vsock_handle_rx(VirtIODevice *dev, VirtQueue *vq) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);
}

static void virtio_vsock_handle_tx(VirtIODevice *dev, VirtQueue *vq) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);
}

static void virtio_vsock_handle_event(VirtIODevice *dev, VirtQueue *vq) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);
}

static const Property virtio_vsock_properties[] = {
    DEFINE_PROP_UINT64("guest-cid", VirtIOVSock, guest_cid, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_virtio_vsock = {
    .name = TYPE_VIRTIO_VSOCK,
    .minimum_version_id = 0,
    .version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_VIRTIO_DEVICE,
        VMSTATE_UINT64(guest_cid, VirtIOVSock),
        VMSTATE_END_OF_LIST()
    },
};

static void virtio_vsock_device_realize(DeviceState *dev, Error **errp) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    VirtIODevice *v = &s->parent;

    fprintf(stderr, "rkir555 %s:%d guest-cid=%u\n", __func__, __LINE__, (uint32_t)s->guest_cid);

    if (s->guest_cid <= 2 || s->guest_cid >= UINT32_MAX) {
        error_setg(errp, "guest-cid property is incorrect");
        return;
    }

    virtio_init(v, TYPE_VIRTIO_VSOCK, VIRTIO_ID_VSOCK, sizeof(struct virtio_vsock_config));
    s->rx_vq = virtio_add_queue(v, VIRTIO_VSOCK_QUEUE_SIZE, &virtio_vsock_handle_rx);
    s->tx_vq = virtio_add_queue(v, VIRTIO_VSOCK_QUEUE_SIZE, &virtio_vsock_handle_tx);
    s->event_vq = virtio_add_queue(v, VIRTIO_VSOCK_QUEUE_SIZE, &virtio_vsock_handle_event);


    // TODO
}

static void virtio_vsock_device_unrealize(DeviceState *dev, Error **errp) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    VirtIODevice *v = &s->parent;
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);

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
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);
    memcpy(raw, &cfg, sizeof(cfg));
}

static void virtio_vsock_set_config(VirtIODevice *dev, const uint8_t *raw) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);

    struct virtio_vsock_config cfg;
    memcpy(&cfg, raw, sizeof(cfg));
    s->guest_cid = cfg.guest_cid;
}

static void virtio_vsock_set_status(VirtIODevice *dev, uint8_t status) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    fprintf(stderr, "rkir555 %s:%d status=0x%x\n", __func__, __LINE__, status);
    const bool should_start = status & VIRTIO_CONFIG_S_DRIVER_OK;

    // TODO
}

static void virtio_vsock_guest_notifier_mask(VirtIODevice *dev, int idx, bool mask) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    // TODO
    fprintf(stderr, "rkir555 %s:%d mask=%d\n", __func__, __LINE__, mask);
}

static bool virtio_vsock_guest_notifier_pending(VirtIODevice *dev, int idx) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    fprintf(stderr, "rkir555 %s:%d idx=%d\n", __func__, __LINE__, idx);
    // TODO
    return 0;
}

static void virtio_vsock_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

    dc->props = (Property*)virtio_vsock_properties;
    dc->vmsd = &vmstate_virtio_vsock;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);

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
    .class_init = &virtio_vsock_class_init,
};

static void virtio_vsock_register_types(void) {
    type_register_static(&virtio_vsock_typeinfo);
}

type_init(virtio_vsock_register_types);

