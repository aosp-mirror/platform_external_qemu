/*
 * Virt IO vsock virtio device
 *
 */

#ifndef _QEMU_VIRTIO_VSOCK_H
#define _QEMU_VIRTIO_VSOCK_H

#include <stdint.h>
#include <hw/virtio/virtio.h>

// WIP
typedef struct {
    uint32_t src_port;
    uint32_t dst_port;
    uint32_t buf_alloc;
    uint32_t fwd_cnt;
    uint32_t sent_cnt;
    uint32_t remote_buf_alloc;
    uint32_t remote_fwd_cnt;
    const char *msg;
} VSockStream;

typedef struct {
    char *buf;
    uint32_t size;
} VSockBuf;

typedef struct {
    VirtIODevice parent;
    uint64_t guest_cid;

    VSockBuf guest_to_host_buf;
    uint32_t guest_to_host_buf_i;

    /* do not save/load below to snapshot */
    VirtQueue *host_to_guest_vq;
    VirtQueue *guest_to_host_vq;
    VirtQueue *host_to_guest_event_vq;
} VirtIOVSock;

#define TYPE_VIRTIO_VSOCK "virtio-vsock"
#define VIRTIO_VSOCK(obj) OBJECT_CHECK(VirtIOVSock, (obj), TYPE_VIRTIO_VSOCK)

#endif /* _QEMU_VIRTIO_VSOCK_H */
