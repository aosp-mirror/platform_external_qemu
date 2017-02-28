/* Copyright (c) 2017 The Android Open Source Project
**
** Authors:
**    Huihong Luo huisinro@google.com
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/virtio/virtio.h"
#include "hw/virtio/virtio-bus.h"
#include "hw/virtio/virtio-pci.h"
#include "hw/virtio/virtio-goldfish-pipe.h"
#include "qapi/error.h"

/* Set to > 0 for debug output */
#define PIPE_DEBUG 2

/* Set to 1 to debug i/o register reads/writes */
#define PIPE_DEBUG_REGS 0

#if PIPE_DEBUG >= 1
#define D(fmt, ...) \
    do { fprintf(stdout, "virtio_goldfish_pipe: " fmt "\n", ## __VA_ARGS__); } while (0)
#else
#define D(fmt, ...)  do { /* nothing */ } while (0)
#endif

#if PIPE_DEBUG >= 2
#define DD(fmt, ...) \
    do { fprintf(stdout, "virtio_goldfish_pipe: " fmt "\n", ## __VA_ARGS__); } while (0)
#else
#define DD(fmt, ...)  do { /* nothing */ } while (0)
#endif

#if PIPE_DEBUG_REGS >= 1
#  define DR(...)   D(__VA_ARGS__)
#else
#  define DR(...)   do { /* nothing */ } while (0)
#endif

#define E(fmt, ...)  \
    do { fprintf(stdout, "ERROR:" fmt "\n", ## __VA_ARGS__); } while (0)

#define APANIC(...)                     \
    do {                                \
        error_report(__VA_ARGS__);      \
        exit(1);                        \
    } while (0)

#define VPIPE_CMD_WRITE    20
#define VPIPE_CMD_READ	    21
#define VPIPE_CMD_OPEN	    22
#define VPIPE_CMD_CLOSE    23

// data sent from host to vm
#define READ_BUFSIZE 		(2 * 1024 * 1024)
// data from vm to host
#define WRITE_BUFSIZE 		(4 * 1024 * 1024)

#ifdef _MSC_VER
#define __attribute__(a)
#pragma pack(push,1)
#endif

struct VPIPE_HDR
{	
    // id to identify the fiber session
    int id;

    // command or return value
    union {
        int cmd;
        int rc;
    };

    // data length to write or read
    int length;
} __attribute__((packed));

#ifdef _MSC_VER
#pragma pack(pop)
#endif

typedef struct VPIPE_HDR VPIPE_HDR;

// Data comes from the vm, this is from guest perspective
static void virtio_goldfish_pipe_handle_transmit(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOGoldfishPipe *vgp = VIRTIO_GOLDFISH_PIPE(vdev);
    VirtQueueElement *elem;
    VPIPE_HDR hdr = {0};
    int rc = 0;

    DD("%s():", __func__);

    for (;;) {
        elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (!elem) {
            break;
        }

        g_free(elem);
    }  /* end for loop */
}

// data is sent to the vm, this from guest perspective
static void virtio_goldfish_pipe_handle_receive(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOGoldfishPipe *vgp = VIRTIO_GOLDFISH_PIPE(vdev);
    DD("%s():", __func__);
}

static void virtio_goldfish_pipe_handle_ctrl(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtIOGoldfishPipe *vgp = VIRTIO_GOLDFISH_PIPE(vdev);
    VirtQueueElement *elem;
    size_t in_len;
    struct iovec *in_iov;
    struct iovec *out_iov;
    unsigned in_num;
    unsigned out_num;
    uint32_t cmd;
    VPIPE_HDR CtlHdr;
    VPIPE_HDR *input;

    DD("%s():", __func__);

    for (;;) {
        elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (!elem) {
            break;
        }

        out_num = elem->out_num;
        out_iov = elem->out_sg;
        in_num = elem->in_num;
        in_iov = elem->in_sg;
        if (unlikely(iov_to_buf(out_iov, out_num, 0, &CtlHdr, sizeof(CtlHdr))
                    != sizeof(CtlHdr))) {
            //virtio_error(vdev, "virtio-goldfish-pipe request CtlHdr too small");
            g_free(elem);
            break;
        }
        iov_discard_front(&out_iov, &out_num, sizeof(CtlHdr));

        cmd = CtlHdr.cmd;
        DD("%s(): cmd=%d", __func__, cmd);

        in_len = iov_size(in_iov, in_num);
        input = (void *)in_iov[in_num - 1].iov_base
                      + in_iov[in_num - 1].iov_len
                      - sizeof(*input);
        if (cmd == VPIPE_CMD_OPEN) {
            iov_discard_back(in_iov, &in_num, sizeof(*input));
            input->rc = 0;
	    input->length = 0;
        }
        else if (cmd == VPIPE_CMD_CLOSE) {
            iov_discard_back(in_iov, &in_num, sizeof(*input));
            input->rc = 0;
	    input->length = 0;
        }
        else {
            iov_discard_back(in_iov, &in_num, sizeof(*input));
            input->rc = -1;
	    input->length = 0;
        }

        in_len = iov_size(in_iov, in_num);
        virtqueue_push(vq, elem, in_len);
        virtio_notify(vdev, vq);

        g_free(elem);
    }  /* end for loop */
}

static uint64_t get_features(VirtIODevice *vdev, uint64_t f, Error **errp)
{
    return f;
}

static void virtio_goldfish_pipe_device_realize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOGoldfishPipe *vgp = VIRTIO_GOLDFISH_PIPE(dev);
    Error *local_err = NULL;

    DD("%s(): enter, VIRTIO_ID_GOLDFISH_PIPE=%d", __func__, VIRTIO_ID_GOLDFISH_PIPE);

    virtio_init(vdev, "virtio-goldfish-pipe", VIRTIO_ID_GOLDFISH_PIPE, 0);

    // make sure the order of adding queues must be consistent with the kernel
    // driver. Note that we don't specify name here even though there is a unique
    // name for each queue inside kernel driver
    vgp->tx_vq = virtio_add_queue(vdev, 1024, virtio_goldfish_pipe_handle_transmit);
    vgp->rx_vq = virtio_add_queue(vdev, 1024, virtio_goldfish_pipe_handle_receive);
    vgp->ctrl_vq = virtio_add_queue(vdev, 16, virtio_goldfish_pipe_handle_ctrl);

    DD("%s(): exit, tx_vq=%p, rx_vq=%p, ctrl_vq=%p", __func__, vgp->tx_vq, vgp->rx_vq, vgp->ctrl_vq);
}

static void virtio_goldfish_pipe_device_unrealize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOGoldfishPipe *vgp = VIRTIO_GOLDFISH_PIPE(dev);

    DD("%s():", __func__);
    virtio_cleanup(vdev);
}

static void virtio_goldfish_pipe_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

    DD("%s():", __func__);

    //dc->props = virtio_rng_properties;
    //dc->vmsd = &vmstate_virtio_goldfish_pipe;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    vdc->realize = virtio_goldfish_pipe_device_realize;
    vdc->unrealize = virtio_goldfish_pipe_device_unrealize;
    vdc->get_features = get_features;
}

static void virtio_goldfish_pipe_initfn(Object *obj)
{
    VirtIOGoldfishPipe *dev = VIRTIO_GOLDFISH_PIPE(obj);
}

static const TypeInfo virtio_goldfish_pipe_info = {
    .name          = TYPE_VIRTIO_GOLDFISH_PIPE,
    .parent        = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOGoldfishPipe),
    .instance_init = virtio_goldfish_pipe_initfn,
    .class_init    = virtio_goldfish_pipe_class_init,
};

static void virtio_goldfish_pipe_register_types(void)
{
    type_register_static(&virtio_goldfish_pipe_info);
}
type_init(virtio_goldfish_pipe_register_types)
