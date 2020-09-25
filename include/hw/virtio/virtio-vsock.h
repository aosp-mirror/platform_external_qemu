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

typedef struct VSockStreamTag {
    void *cookie;
    uint32_t src_port;
    uint32_t dst_port;
    uint32_t buf_alloc;
    uint32_t fwd_cnt;
    uint32_t sent_cnt;
    uint32_t remote_buf_alloc;
    uint32_t remote_fwd_cnt;
} VSockStream;

typedef size_t(*virtio_vsock_read_streams_sink_t)(VSockStream *,
                                                  const void *, size_t);

typedef VSockStream *(*virtio_vsock_create_stream_t)(uint32_t, uint32_t);
typedef void (*virtio_vsock_close_stream_t)(VSockStream *);
typedef VSockStream *(*virtio_vsock_find_stream_t)(uint32_t, uint32_t);
typedef int (*virtio_vsock_write_stream_t)(VSockStream *, const void *, size_t);
typedef void (*virtio_vsock_read_streams_t)(virtio_vsock_read_streams_sink_t);

typedef struct VirtIOVSockOpsTag {
    virtio_vsock_create_stream_t create;
    virtio_vsock_close_stream_t  close;
    virtio_vsock_find_stream_t   find;
    virtio_vsock_write_stream_t  write_to_stream;
    virtio_vsock_read_streams_t  read_from_streams;
} VirtIOVSockOps;

const VirtIOVSockOps* virtio_vsock_set_ops(const VirtIOVSockOps *new_ops);
int virtio_vsock_write_to_guest(VSockStream *stream, const void *buf, size_t size);

#define TYPE_VIRTIO_VSOCK "virtio-vsock"
#define VIRTIO_VSOCK(obj) OBJECT_CHECK(VirtIOVSock, (obj), TYPE_VIRTIO_VSOCK)

#endif /* _QEMU_VIRTIO_VSOCK_H */
