/*
 * Virt IO vsock virtio device
 *
 */

#ifndef _QEMU_VIRTIO_VSOCK_H
#define _QEMU_VIRTIO_VSOCK_H

#include <stdint.h>
#include <hw/virtio/virtio.h>

typedef struct {
    VirtIODevice parent;
    uint64_t guest_cid;

    /* do not save/load to snapshot */
    VirtQueue *rx_vq;
    VirtQueue *tx_vq;
    VirtQueue *event_vq;
} VirtIOVSock;

#define TYPE_VIRTIO_VSOCK "virtio-vsock"
#define VIRTIO_VSOCK(obj) OBJECT_CHECK(VirtIOVSock, (obj), TYPE_VIRTIO_VSOCK)

#endif /* _QEMU_VIRTIO_VSOCK_H */
