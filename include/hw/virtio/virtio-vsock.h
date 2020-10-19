/*
 * Virt IO vsock virtio device
 *
 */

#ifndef _QEMU_VIRTIO_VSOCK_H
#define _QEMU_VIRTIO_VSOCK_H

#include <stdint.h>
#include <qemu/compiler.h>
#include <qemu/osdep.h>
#include <qemu/typedefs.h>
#include <hw/virtio/virtio.h>
#include <qapi/error.h>

typedef struct VirtIOVSockTag {
    VirtIODevice parent;
    uint64_t guest_cid;
    void *impl;  // see virtio_vsock_ctor/virtio_vsock_dctor

    /* do not save/load below to the snapshot */
    VirtQueue *host_to_guest_vq;
    VirtQueue *guest_to_host_vq;
    VirtQueue *host_to_guest_event_vq;
} VirtIOVSock;

void virtio_vsock_ctor(VirtIOVSock *s, Error **errp);
void virtio_vsock_dctor(VirtIOVSock *s);

void virtio_vsock_set_status(VirtIODevice *dev, uint8_t status);

void virtio_vsock_handle_host_to_guest(VirtIODevice *dev, VirtQueue *vq);
void virtio_vsock_handle_guest_to_host(VirtIODevice *dev, VirtQueue *vq);
void virtio_vsock_handle_event_to_guest(VirtIODevice *dev, VirtQueue *vq);

int virtio_vsock_impl_load(QEMUFile *f, VirtIOVSock *s);
void virtio_vsock_impl_save(QEMUFile *f, const void *impl);

#define TYPE_VIRTIO_VSOCK "virtio-vsock"
#define VIRTIO_VSOCK(obj) OBJECT_CHECK(VirtIOVSock, (obj), TYPE_VIRTIO_VSOCK)

#endif /* _QEMU_VIRTIO_VSOCK_H */
