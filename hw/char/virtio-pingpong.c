/* virtio-pingpong: virtio-serial backend that just sends all
 * data it receives back to the guest.
 *
 * Copyright (c) 2013 Linaro Limited
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 */

#include "sysemu/char.h"
#include "qemu/error-report.h"
#include "trace.h"
#include "hw/virtio/virtio-serial.h"

#define TYPE_VIRTIO_PINGPONG "virtpingpong"
#define VIRTIO_PINGPONG(obj) \
    OBJECT_CHECK(VirtPingpong, (obj), TYPE_VIRTIO_PINGPONG)

typedef struct VirtPingpong {
    VirtIOSerialPort parent_obj;
    /* Any per-backend data goes here */
} VirtPingpong;


/* Callback function that's called when the guest sends us data */
static ssize_t virtpingpong_have_data(VirtIOSerialPort *port,
                                      const uint8_t *buf, ssize_t len)
{
    //VirtPingpong *vpp = VIRTIO_PINGPONG(port);
    ssize_t ret;

    printf("virtpingpong_have_data: %ld bytes\n", len);

    ret = virtio_serial_write(port, buf, len);

    printf("virtpingpong_have_data: passed %ld bytes back\n", ret);

    if (ret < len) {
        /* We didn't manage to send all the data: throttle the
         * port so we don't get any more for now.
         */
        virtio_serial_throttle_port(port, true);
    }

    return ret;
}

static void virtpingpong_guest_writable(VirtIOSerialPort *port)
{
    printf("virtpingpong_guest_writable\n");
    /* Just unthrottle the other end of the port so it can send us more data */
    virtio_serial_throttle_port(port, false);
}

/* Callback function that's called when the guest opens/closes the port */
static void virtpingpong_set_guest_connected(VirtIOSerialPort *port,
                                             int guest_connected)
{
    //VirtPingpong *vpp = VIRTIO_PINGPONG(port);

    printf("virtpingpong: guest %sconnected\n", guest_connected ? "" : "dis");
}

static void virtpingpong_realize(DeviceState *dev, Error **errp)
{
    VirtIOSerialPort *port = VIRTIO_SERIAL_PORT(dev);
    //VirtPingpong *vpp = VIRTIO_PINGPONG(dev);
    VirtIOSerialPortClass *k = VIRTIO_SERIAL_PORT_GET_CLASS(dev);

    if (port->id == 0 && !k->is_console) {
        error_setg(errp, "Port number 0 on virtio-serial devices reserved "
                   "for virtconsole devices for backward compatibility.");
        return;
    }

    /* The host end of this connection is always open. */
    virtio_serial_open(port);
}

static void virtpingpong_unrealize(DeviceState *dev, Error **errp)
{
    // VirtPingpong *vpp = VIRTIO_PINGPONG(dev);

    /* Nothing for us to do here */
}

static void virtpingpong_class_init(ObjectClass *klass, void *data)
{
    VirtIOSerialPortClass *k = VIRTIO_SERIAL_PORT_CLASS(klass);

    k->realize = virtpingpong_realize;
    k->unrealize = virtpingpong_unrealize;
    k->have_data = virtpingpong_have_data;
    k->guest_writable = virtpingpong_guest_writable;
    k->set_guest_connected = virtpingpong_set_guest_connected;
    /* If we have any device state for save/load we should
     * register a vmstate here.
     */
}

static const TypeInfo virtpingpong_info = {
    .name          = TYPE_VIRTIO_PINGPONG,
    .parent        = TYPE_VIRTIO_SERIAL_PORT,
    .instance_size = sizeof(VirtPingpong),
    .class_init    = virtpingpong_class_init,
};

static void virtpingpong_register_types(void)
{
    type_register_static(&virtpingpong_info);
}

type_init(virtpingpong_register_types)
