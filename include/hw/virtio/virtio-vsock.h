/*
 * Virt IO vsock virtio device
 *
 */

#ifndef _QEMU_VIRTIO_VSOCK_H
#define _QEMU_VIRTIO_VSOCK_H

#include <stdint.h>
#include <hw/virtio/virtio.h>

typedef struct VSockBufTag {
    uint8_t *buf;
    uint32_t size;
} VSockBuf;

typedef struct VirtIOVSockTag {
    VirtIODevice parent;
    uint64_t guest_cid;
    uint32_t guest_to_host_buf_i;
    VSockBuf guest_to_host_buf;

    /* do not save/load below to snapshot */
    VirtQueue *host_to_guest_vq;
    VirtQueue *guest_to_host_vq;
    VirtQueue *host_to_guest_event_vq;
} VirtIOVSock;

#define TYPE_VIRTIO_VSOCK "virtio-vsock"
#define VIRTIO_VSOCK(obj) OBJECT_CHECK(VirtIOVSock, (obj), TYPE_VIRTIO_VSOCK)

#endif /* _QEMU_VIRTIO_VSOCK_H */
