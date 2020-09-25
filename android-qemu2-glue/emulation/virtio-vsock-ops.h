/*
 */

#ifndef _QEMU_VIRTIO_VSOCK_OPS_H
#define _QEMU_VIRTIO_VSOCK_OPS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

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

typedef int(*virtio_vsock_read_streams_sink_t)(VSockStream *,
                                               const void *, int);

typedef VSockStream *(*virtio_vsock_open_stream_t)(uint32_t, uint32_t);
typedef VSockStream *(*virtio_vsock_find_stream_t)(uint32_t, uint32_t);
typedef void (*virtio_vsock_unlock_stream_t)(VSockStream *);
typedef void (*virtio_vsock_close_stream_t)(VSockStream *);
typedef void (*virtio_vsock_write_stream_t)(VSockStream *, const void *, size_t);
typedef void (*virtio_vsock_read_streams_t)(virtio_vsock_read_streams_sink_t);

typedef struct VirtIOVSockOpsTag {
    virtio_vsock_open_stream_t      open;   // locks
    virtio_vsock_find_stream_t      find;   // locks
    virtio_vsock_unlock_stream_t    unlock; // unlocks
    virtio_vsock_close_stream_t     close;  // unlocks
    virtio_vsock_write_stream_t     write_to_stream;
    virtio_vsock_read_streams_t     read_from_streams;
} VirtIOVSockOps;

const VirtIOVSockOps *virtio_vsock_set_ops(const VirtIOVSockOps *new_ops);
void virtio_vsock_host_to_guest_notify();

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* _QEMU_VIRTIO_VSOCK_OPS_H */
