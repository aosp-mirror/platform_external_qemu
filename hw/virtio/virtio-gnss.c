/* Copyright 2021 The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at

** http://www.apache.org/licenses/LICENSE-2.0

** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** A virtio device implementing a hardware gnss device.
**
*/

#include "hw/virtio/virtio-gnss.h"
#include "chardev/char-fe.h"
#include "hw/qdev.h"
#include "hw/virtio/virtio-serial.h"
#include "hw/virtio/virtio.h"
#include "qapi/error.h"
#include "qapi/qapi-events-char.h"
#include "qemu/error-report.h"
#include "qemu/iov.h"
#include "qemu/osdep.h"
#include "qom/object_interfaces.h"
#include "trace.h"

static bool is_guest_ready_to_recv(VirtIOGNSS *vgnss) {
    if (!vgnss->ivq) return false;

    VirtIODevice *vdev = VIRTIO_DEVICE(vgnss);
    if (virtio_queue_ready(vgnss->ivq) &&
        (vdev->status & VIRTIO_CONFIG_S_DRIVER_OK)) {
        return true;
    }
    return false;
}

static size_t get_request_size_to_recv(VirtQueue *vq) {
    unsigned int in, out;

    virtqueue_get_avail_bytes(vq, &in, &out, 1024, 0);
    return in;
}

static void virtio_gnss_process_output(VirtIOGNSS *vgnss) {
    fprintf(stderr, "%s %d calling\n", __func__, __LINE__);

    size_t size;
    int offset, len;
    VirtIODevice *vdev = VIRTIO_DEVICE(vgnss);
    VirtQueueElement *elem;

    char buf[1024];
    int bufsize = sizeof(buf);

    offset = 0;
    while (offset < bufsize) {
        elem = virtqueue_pop(vgnss->ovq, sizeof(VirtQueueElement));
        if (!elem) {
            break;
        }
        len = iov_to_buf(elem->out_sg, elem->out_num, 0, buf + offset,
                         bufsize - offset);
        offset += len;

        virtqueue_push(vgnss->ovq, elem, len);
        g_free(elem);
    }
    buf[offset] = '\0';
    fprintf(stderr, "%s %d got message of size %d from guest:'%s'\n",
            __func__, __LINE__, offset, buf);
    virtio_notify(vdev, vgnss->ovq);
}

static void virtio_gnss_process_input(VirtIOGNSS *vgnss) {
    size_t size;
    int offset, len;
    VirtIODevice *vdev = VIRTIO_DEVICE(vgnss);
    VirtQueueElement *elem;

    fprintf(stderr, "%s %d calling\n", __func__, __LINE__);
    char buf[1024] = "hello from host";
    int bufsize = strlen(buf);

    if (!is_guest_ready_to_recv(vgnss)) {
        return;
    }

    size = get_request_size_to_recv(vgnss->ivq);

    if (!size) {
        return;
    }

    offset = 0;
    while (offset < size && offset < bufsize) {
        elem = virtqueue_pop(vgnss->ivq, sizeof(VirtQueueElement));
        if (!elem) {
            break;
        }
        len = iov_from_buf(elem->in_sg, elem->in_num, 0, buf + offset,
                           bufsize - offset);
        offset += len;

        virtqueue_push(vgnss->ivq, elem, len);
        g_free(elem);
    }
    fprintf(stderr, "%s %d send message to guest:'%s'\n",
            __func__, __LINE__, buf);
    virtio_notify(vdev, vgnss->ovq);
    virtio_notify(vdev, vgnss->ivq);
}

static void handle_input(VirtIODevice *vdev, VirtQueue *vq) {
    VirtIOGNSS *vgnss = VIRTIO_GNSS(vdev);
    virtio_gnss_process_input(vgnss);
}

static void handle_output(VirtIODevice *vdev, VirtQueue *vq) {
    VirtIOGNSS *vgnss = VIRTIO_GNSS(vdev);
    virtio_gnss_process_output(vgnss);
}

static uint64_t get_features(VirtIODevice *vdev, uint64_t f, Error **errp) {
    return f;
}

static void virtio_gnss_vm_state_change(void *opaque, int running,
                                        RunState state) {
    VirtIOGNSS *vgnss = opaque;

    if (running && is_guest_ready_to_recv(vgnss)) {
        virtio_gnss_process_input(vgnss);
    }
}

static void virtio_gnss_device_realize(DeviceState *dev, Error **errp) {
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOGNSS *vgnss = VIRTIO_GNSS(dev);
    Error *local_err = NULL;

    // Hack: need to define this id in kernel
    int VIRTIO_ID_GNSS = 30;
    virtio_init(vdev, "virtio-gnss", VIRTIO_ID_GNSS, 0);

    vgnss->ivq = virtio_add_queue(vdev, 8, handle_input);
    vgnss->ovq = virtio_add_queue(vdev, 8, handle_output);

    vgnss->vmstate =
        qemu_add_vm_change_state_handler(virtio_gnss_vm_state_change, vgnss);
}

static void virtio_gnss_device_unrealize(DeviceState *dev, Error **errp) {
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOGNSS *vgnss = VIRTIO_GNSS(dev);

    virtio_cleanup(vdev);
}

static const VMStateDescription vmstate_virtio_gnss = {
    .name = "virtio-gnss",
    .minimum_version_id = 1,
    .version_id = 1,
    .fields = (VMStateField[]){VMSTATE_VIRTIO_DEVICE, VMSTATE_END_OF_LIST()},
};

static Property virtio_gnss_properties[] = {
    DEFINE_PROP_UINT32("xtype", VirtIOGNSS, xtype, 1),
    DEFINE_PROP_END_OF_LIST(),
};

static void virtio_gnss_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

    dc->props = virtio_gnss_properties;
    dc->vmsd = &vmstate_virtio_gnss;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    vdc->realize = virtio_gnss_device_realize;
    vdc->unrealize = virtio_gnss_device_unrealize;
    vdc->get_features = get_features;
}

static const TypeInfo virtio_gnss_info = {
    .name = TYPE_VIRTIO_GNSS,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOGNSS),
    .class_init = virtio_gnss_class_init,
};

static void virtio_register_types(void) {
    type_register_static(&virtio_gnss_info);
}

type_init(virtio_register_types)
