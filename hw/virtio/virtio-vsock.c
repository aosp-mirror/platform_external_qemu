#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <qemu/compiler.h>
#include <qemu/osdep.h>
#include <qemu/typedefs.h>
#include <qemu/iov.h>
#include <qapi/error.h>
#include <standard-headers/linux/virtio_vsock.h>
#include <hw/virtio/virtio-vsock.h>
#include <android-qemu2-glue/emulation/virtio-vsock-ops.h>

#define VIRTIO_VSOCK_HOST_CID 2
#define VIRTIO_VSOCK_QUEUE_SIZE 128

////////////////////////////////////////////////////////////////////////////////

static VSockStream *virtio_vsock_stream_open_default(uint32_t src_port,
                                                     uint32_t dst_port) {
    return NULL;
}

static VSockStream *virtio_vsock_stream_find_default(uint32_t src_port,
                                                     uint32_t dst_port) {
    return NULL;
}

static void virtio_vsock_stream_unlock_default(VSockStream *stream) {}

static void virtio_vsock_stream_close_default(VSockStream *stream) {}

static void virtio_vsock_stream_write_default(VSockStream *stream,
                                              const void *buf,
                                              size_t size) {}

static void virtio_vsock_streams_read_default(virtio_vsock_read_streams_sink_t sink) {}

const VirtIOVSockOps default_ops = {
    .open = &virtio_vsock_stream_open_default,
    .find = &virtio_vsock_stream_find_default,
    .unlock = &virtio_vsock_stream_unlock_default,
    .close = &virtio_vsock_stream_close_default,
    .write_to_stream = &virtio_vsock_stream_write_default,
    .read_from_streams = &virtio_vsock_streams_read_default,
};

static const VirtIOVSockOps *g_ops = &default_ops;
static VirtIOVSock *g_sock = NULL;

////////////////////////////////////////////////////////////////////////////////

const VirtIOVSockOps* virtio_vsock_set_ops(const VirtIOVSockOps *new_ops) {
    const VirtIOVSockOps *old_ops = g_ops;
    g_ops = new_ops;
    return old_ops;
}

void virtio_vsock_host_to_guest_notify() {
    virtio_notify(&g_sock->parent, g_sock->host_to_guest_vq);
}

////////////////////////////////////////////////////////////////////////////////

static VSockStream *virtio_vsock_stream_open(uint32_t src_port,
                                             uint32_t dst_port,
                                             uint32_t remote_buf_alloc,
                                             uint32_t remote_fwd_cnt,
                                             uint32_t *buf_alloc) {
    VSockStream *stream = (*g_ops->open)(src_port, dst_port);
    if (stream) {
        stream->remote_buf_alloc = remote_buf_alloc;
        stream->remote_fwd_cnt = remote_fwd_cnt;
        *buf_alloc = stream->buf_alloc;
        return stream;
    } else {
        return NULL;
    }
}

static VSockStream *virtio_vsock_stream_find(uint32_t src_port,
                                             uint32_t dst_port) {
    return (*g_ops->find)(src_port, dst_port);
}

static void virtio_vsock_stream_unlock(VSockStream *stream) {
    (*g_ops->unlock)(stream);
}

static void virtio_vsock_stream_close(VSockStream *stream) {
    (*g_ops->close)(stream);
}

static void virtio_vsock_stream_get_pos(VSockStream *stream,
                                        uint32_t *buf_alloc,
                                        uint32_t *fwd_cnt) {
    *buf_alloc = stream->buf_alloc;
    *fwd_cnt = stream->fwd_cnt;
}

static void virtio_vsock_stream_set_remote_pos(VSockStream *stream,
                                               uint32_t remote_buf_alloc,
                                               uint32_t remote_fwd_cnt) {
    stream->remote_buf_alloc = remote_buf_alloc;
    stream->remote_fwd_cnt = remote_fwd_cnt;
}

static void virtio_vsock_stream_write(VSockStream *stream,
                                      const void *buf, size_t size,
                                      uint32_t *buf_alloc,
                                      uint32_t *fwd_cnt) {
    (*g_ops->write_to_stream)(stream, buf, size);

    stream->fwd_cnt += size;
    *buf_alloc = stream->buf_alloc;
    *fwd_cnt = stream->fwd_cnt;
}

static size_t virtio_vsock_write(VirtIOVSock *s, VirtQueue *vq,
                                 const void *buf, const size_t size);

static int
virtio_vsock_read_streams_sink(VSockStream *stream,
                               const void *buf, const int buf_size) {
    if (buf_size <= 0) {
        return 0;
    }

    const unsigned remote_space_available =
        stream->remote_buf_alloc -
            (stream->sent_cnt - MIN(stream->remote_fwd_cnt, stream->sent_cnt));

    unsigned size_to_send = MIN(buf_size, remote_space_available);
    if (size_to_send == 0) {
        return 0;
    }

    VirtIOVSock *s = g_sock;
    VirtQueue *vq = s->host_to_guest_vq;

    unsigned vq_space_available;
    virtqueue_get_avail_bytes(
        vq,
        &vq_space_available,
        NULL,
        sizeof(struct virtio_vsock_hdr) + size_to_send,
        0);

    if (vq_space_available < MIN(sizeof(struct virtio_vsock_hdr) + size_to_send,
                                 sizeof(struct virtio_vsock_hdr) * 4)) {
        return -1;  // vq is full
    }

    size_to_send = MIN(size_to_send,
                       vq_space_available - sizeof(struct virtio_vsock_hdr)); 

    const struct virtio_vsock_hdr hdr = {
        .src_cid = VIRTIO_VSOCK_HOST_CID,
        .dst_cid = s->guest_cid,
        .src_port = stream->dst_port,
        .dst_port = stream->src_port,
        .len = size_to_send,
        .type = VIRTIO_VSOCK_TYPE_STREAM,
        .op = VIRTIO_VSOCK_OP_RW,
        .flags = 0,
        .buf_alloc = stream->buf_alloc,
        .fwd_cnt = stream->fwd_cnt,
    };

    virtio_vsock_write(s, vq, &hdr, sizeof(hdr));
    virtio_vsock_write(s, vq, buf, size_to_send);
    stream->sent_cnt += size_to_send;

    return size_to_send;
}

static void virtio_vsock_handle_host_to_guest(VirtIODevice *dev, VirtQueue *vq) {
    (*g_ops->read_from_streams)(&virtio_vsock_read_streams_sink);
}

////////////////////////////////////////////////////////////////////////////////

// virtio spec 5.10.2
enum virtio_vsock_queue_id {
    VIRTIO_VSOCK_QUEUE_ID_RX = 0,    // host to guest
    VIRTIO_VSOCK_QUEUE_ID_TX = 1,    // guest to host
    VIRTIO_VSOCK_QUEUE_ID_EVENT = 2  // host to guest
};

static void *vsock_buf_init(VSockBuf *buf, size_t size) {
    buf->size = size;
    buf->buf = (size > 0) ? g_malloc(size) : NULL;
    return buf->buf;
}

static void vsock_buf_free(VSockBuf *buf) {
    g_free(buf->buf);
    buf->buf = NULL;
    buf->size = 0;
}

static size_t vsock_buf_resize(VSockBuf *buf, size_t new_size) {
    uint8_t *new_buf = g_malloc(new_size);
    if (new_buf) {
        memcpy(new_buf, buf->buf, MIN(new_size, buf->size));
        g_free(buf->buf);
        buf->buf = new_buf;
        buf->size = new_size;
    }
    return buf->size;
}

static size_t virtio_vsock_write(VirtIOVSock *s, VirtQueue *vq,
                                 const void *buf, const size_t size) {
    const char *buf8 = buf;
    size_t rem = size;

    while (rem > 0) {
        VirtQueueElement *e = virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (!e) {
            break;
        }

        size_t chunk_size = iov_from_buf(e->in_sg, e->in_num, 0, buf8, rem);
        virtqueue_push(vq, e, chunk_size);
        g_free(e);

        buf8 += chunk_size;
        rem -= chunk_size;
    }

    virtio_notify(&s->parent, vq);
    return size - rem;
}

static int virtio_vsock_write_op(VirtIOVSock *s, VirtQueue *vq,
                                 const struct virtio_vsock_hdr *request,
                                 enum virtio_vsock_op op,
                                 uint32_t buf_alloc,
                                 uint32_t fwd_cnt) {
    const uint32_t flags = (op == VIRTIO_VSOCK_OP_SHUTDOWN) ?
        (VIRTIO_VSOCK_SHUTDOWN_RCV | VIRTIO_VSOCK_SHUTDOWN_SEND) : 0;

    const struct virtio_vsock_hdr response = {
        .src_cid = request->dst_cid,
        .dst_cid = request->src_cid,
        .src_port = request->dst_port,
        .dst_port = request->src_port,
        .len = 0,
        .type = VIRTIO_VSOCK_TYPE_STREAM,
        .op = op,
        .flags = flags,
        .buf_alloc = buf_alloc,
        .fwd_cnt = fwd_cnt,
    };

    fprintf(stderr, "rkir555 %s:%d "
            "src_cid=%u dst_cid=%u src_port=%u dst_port=%u op=%d buf_alloc=%u fwd_cnt=%u\n",
            __func__, __LINE__,
            (uint32_t)response.src_cid, (uint32_t)response.dst_cid,
            response.src_port, response.dst_port,
            response.op,
            response.buf_alloc, response.fwd_cnt);

    if (virtio_vsock_write(s, vq, &response, sizeof(response)) == sizeof(response)) {
        return 1;
    } else {
        fprintf(stderr, "rkir555 %s:%d virtio_vsock_write did not send all bytes, op=%d\n",
                __func__, __LINE__, op);
        return 0;
    }
}

static void
virtio_vsock_handle_guest_to_host_parse_request(VirtIOVSock *s,
                                                const struct virtio_vsock_hdr *request) {
    VSockStream *stream;
    uint32_t buf_alloc;

    fprintf(stderr, "rkir555 %s:%d "
            "src_cid=%u dst_cid=%u "
            "src_port=%u dst_port=%u "
            "len=%u type=%u op=%u flags=0x%x "
            "buf_alloc=%u fwd_cnt=%u\n",
            __func__, __LINE__,
            (uint32_t)request->src_cid, (uint32_t)request->dst_cid,
            request->src_port, request->dst_port,
            request->len, request->type, request->op, request->flags,
            request->buf_alloc, request->fwd_cnt);

    if (request->type != VIRTIO_VSOCK_TYPE_STREAM) {
        virtio_vsock_write_op(s, s->host_to_guest_vq, request,
                              VIRTIO_VSOCK_OP_RST, 0, 0);
        return;
    }

    if (request->op == VIRTIO_VSOCK_OP_REQUEST) {
        stream = virtio_vsock_stream_open(request->src_port, request->dst_port,
                                          request->buf_alloc, request->fwd_cnt,
                                          &buf_alloc);
        if (stream) {
            virtio_vsock_stream_unlock(stream);
            virtio_vsock_write_op(s, s->host_to_guest_vq, request,
                                  VIRTIO_VSOCK_OP_RESPONSE, buf_alloc, 0);
        } else {
            virtio_vsock_write_op(s, s->host_to_guest_vq, request,
                                  VIRTIO_VSOCK_OP_RST, 0, 0);
        }
    } else {
        uint32_t fwd_cnt;

        stream = virtio_vsock_stream_find(request->src_port, request->dst_port);
        if (stream) {
            switch (request->op) {
            case VIRTIO_VSOCK_OP_SHUTDOWN:
                virtio_vsock_stream_get_pos(stream, &buf_alloc, &fwd_cnt);
                virtio_vsock_stream_close(stream);
                virtio_vsock_write_op(s, s->host_to_guest_vq, request,
                                      VIRTIO_VSOCK_OP_SHUTDOWN, buf_alloc, fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_RST:
                virtio_vsock_stream_get_pos(stream, &buf_alloc, &fwd_cnt);
                virtio_vsock_stream_close(stream);
                virtio_vsock_write_op(s, s->host_to_guest_vq, request,
                                      VIRTIO_VSOCK_OP_RST, buf_alloc, fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_RESPONSE:
                virtio_vsock_stream_get_pos(stream, &buf_alloc, &fwd_cnt);
                virtio_vsock_stream_close(stream);
                virtio_vsock_write_op(s, s->host_to_guest_vq, request,
                                      VIRTIO_VSOCK_OP_RST, buf_alloc, fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_RW:
                virtio_vsock_stream_set_remote_pos(stream,
                                                   request->buf_alloc,
                                                   request->fwd_cnt);
                virtio_vsock_stream_write(stream, request + 1, request->len,
                                          &buf_alloc, &fwd_cnt);
                virtio_vsock_stream_unlock(stream);
                virtio_vsock_write_op(s, s->host_to_guest_vq, request,
                                      VIRTIO_VSOCK_OP_CREDIT_UPDATE,
                                      buf_alloc, fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_CREDIT_UPDATE:
                virtio_vsock_stream_set_remote_pos(stream,
                                                   request->buf_alloc,
                                                   request->fwd_cnt);
                virtio_vsock_stream_unlock(stream);
                break;

            case VIRTIO_VSOCK_OP_CREDIT_REQUEST:
                virtio_vsock_stream_set_remote_pos(stream,
                                                   request->buf_alloc,
                                                   request->fwd_cnt);
                virtio_vsock_stream_get_pos(stream, &buf_alloc, &fwd_cnt);
                virtio_vsock_stream_unlock(stream);
                virtio_vsock_write_op(s, s->host_to_guest_vq, request,
                                      VIRTIO_VSOCK_OP_CREDIT_UPDATE,
                                      buf_alloc, fwd_cnt);
                break;

            default:
                virtio_vsock_stream_close(stream);
                virtio_vsock_write_op(s, s->host_to_guest_vq, request,
                                      VIRTIO_VSOCK_OP_RST, 0, 0);
                break;
            }
        } else {
            virtio_vsock_write_op(s, s->host_to_guest_vq, request,
                                  VIRTIO_VSOCK_OP_RST, 0, 0);
        }
    }
}

static size_t virtio_vsock_handle_guest_to_host_impl(VirtIOVSock *s, size_t i) {
    uint8_t *buf = s->guest_to_host_buf.buf;
    const struct virtio_vsock_hdr *request = (const struct virtio_vsock_hdr *)buf;

    fprintf(stderr, "rkir555 %s:%d parsing %zu bytes\n", __func__, __LINE__, i);

    while (true) {
        if (i < sizeof(*request)) {
            fprintf(stderr, "rkir555 %s:%d i=%zu\n", __func__, __LINE__, i);
            break;
        }

        const size_t request_size = sizeof(*request) + request->len;
        if (i < request_size) {
            fprintf(stderr, "rkir555 %s:%d i=%zu\n", __func__, __LINE__, i);
            break;
        }

        virtio_vsock_handle_guest_to_host_parse_request(s, request);

        i -= request_size;
        memmove(buf, buf + request_size, i);
    }

    return i;
}

static void virtio_vsock_handle_guest_to_host(VirtIODevice *dev, VirtQueue *vq) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);

    while (true) {
        VirtQueueElement *e = virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (!e) {
            break;
        }
        const size_t sz = iov_size(e->out_sg, e->out_num);
        const size_t new_guest_to_host_buf_i = s->guest_to_host_buf_i + sz;

        fprintf(stderr, "rkir555 %s:%d received %zu bytes from the guest\n",
                __func__, __LINE__, sz);

        if (new_guest_to_host_buf_i > s->guest_to_host_buf.size) {
            const size_t new_guest_to_host_buf_sz =
                MAX(new_guest_to_host_buf_i, 2 * s->guest_to_host_buf.size);

            if (!vsock_buf_resize(&s->guest_to_host_buf, new_guest_to_host_buf_sz)) {
                fprintf(stderr, "%s:%d resize_vq_buf failed for size=%zu\n",
                        __func__, __LINE__, new_guest_to_host_buf_sz);
                goto resize_failed;
            }
        }

        iov_to_buf(e->out_sg, e->out_num, 0,
                   s->guest_to_host_buf.buf + s->guest_to_host_buf_i,
                   sz);

        s->guest_to_host_buf_i =
            virtio_vsock_handle_guest_to_host_impl(s, new_guest_to_host_buf_i);

resize_failed:
        virtqueue_push(vq, e, 0);
        g_free(e);
    }

    virtio_notify(dev, vq);
}

static void virtio_vsock_handle_event_to_guest(VirtIODevice *dev, VirtQueue *vq) {
    // nothing so far
}

static const Property virtio_vsock_properties[] = {
    DEFINE_PROP_UINT64("guest-cid", VirtIOVSock, guest_cid, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_vsock_buf = {
    .name = "VSockBuf",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(size, VSockBuf),
        VMSTATE_VBUFFER_ALLOC_UINT32(buf, VSockBuf, 0, NULL, size),
        VMSTATE_END_OF_LIST()
    },
};

static const VMStateDescription vmstate_virtio_vsock = {
    .name = TYPE_VIRTIO_VSOCK,
    .minimum_version_id = 0,
    .version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_VIRTIO_DEVICE,
        VMSTATE_UINT64(guest_cid, VirtIOVSock),
        VMSTATE_UINT32(guest_to_host_buf_i, VirtIOVSock),
        VMSTATE_STRUCT(guest_to_host_buf, VirtIOVSock, 0,
                       vmstate_vsock_buf, VSockBuf),
        VMSTATE_END_OF_LIST()
    },
};

static void virtio_vsock_device_realize(DeviceState *dev, Error **errp) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);

    if (s->guest_cid <= 2 || s->guest_cid >= UINT32_MAX) {
        error_setg(errp, "guest-cid property is incorrect");
        return;
    }

    if (!vsock_buf_init(&s->guest_to_host_buf, 2048)) {
        error_setg(errp, "Could not allocate guest_to_host_buf");
        return;
    }

    s->guest_to_host_buf_i = 0;

    VirtIODevice *v = &s->parent;
    virtio_init(v, TYPE_VIRTIO_VSOCK, VIRTIO_ID_VSOCK, sizeof(struct virtio_vsock_config));
    s->host_to_guest_vq =
        virtio_add_queue(v, VIRTIO_VSOCK_QUEUE_SIZE, &virtio_vsock_handle_host_to_guest);
    s->guest_to_host_vq =
        virtio_add_queue(v, VIRTIO_VSOCK_QUEUE_SIZE, &virtio_vsock_handle_guest_to_host);
    s->host_to_guest_event_vq =
        virtio_add_queue(v, 16, &virtio_vsock_handle_event_to_guest);

    g_sock = s;
}

static void virtio_vsock_device_unrealize(DeviceState *dev, Error **errp) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    VirtIODevice *v = &s->parent;

    g_sock = NULL;

    virtio_del_queue(v, VIRTIO_VSOCK_QUEUE_ID_EVENT);
    virtio_del_queue(v, VIRTIO_VSOCK_QUEUE_ID_TX);
    virtio_del_queue(v, VIRTIO_VSOCK_QUEUE_ID_RX);
    virtio_cleanup(v);

    vsock_buf_free(&s->guest_to_host_buf);
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

static void virtio_vsock_set_status(VirtIODevice *dev, uint8_t status) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    fprintf(stderr, "rkir555 %s:%d status=0x%x\n", __func__, __LINE__, status);
}

static void virtio_vsock_guest_notifier_mask(VirtIODevice *dev, int idx, bool mask) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    fprintf(stderr, "rkir555 %s:%d mask=%d\n", __func__, __LINE__, mask);
}

static bool virtio_vsock_guest_notifier_pending(VirtIODevice *dev, int idx) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    fprintf(stderr, "rkir555 %s:%d idx=%d\n", __func__, __LINE__, idx);
    return 0;
}

static void virtio_vsock_instance_init(Object* obj) {
    VirtIOVSock *s = VIRTIO_VSOCK(obj);
    vsock_buf_init(&s->guest_to_host_buf, 0);
}

static void virtio_vsock_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->props = (Property*)virtio_vsock_properties;
    dc->vmsd = &vmstate_virtio_vsock;
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

static void virtio_vsock_register_types(void) {
    type_register_static(&virtio_vsock_typeinfo);
}

type_init(virtio_vsock_register_types);

