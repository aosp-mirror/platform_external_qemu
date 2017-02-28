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

#ifndef _QEMU_VIRTIO_GOLDFISH_PIPE_H
#define _QEMU_VIRTIO_GOLDFISH_PIPE_H

#include "standard-headers/linux/virtio_goldfish_pipe.h"
#include "hw/virtio/virtio.h"
#include "sysemu/iothread.h"

#define DEBUG_VIRTIO_GOLDFISH_PIPE 0

#define DPRINTF(fmt, ...) \
do { \
    if (DEBUG_VIRTIO_GOLDFISH_PIPE) { \
        fprintf(stderr, "virtio_goldfish_pipe: " fmt, ##__VA_ARGS__); \
    } \
} while (0)


#define TYPE_VIRTIO_GOLDFISH_PIPE "virtio-goldfish-pipe-device"
#define VIRTIO_GOLDFISH_PIPE(obj) \
        OBJECT_CHECK(VirtIOGoldfishPipe, (obj), TYPE_VIRTIO_GOLDFISH_PIPE)
#define VIRTIO_GOLDFISH_PIPE_GET_PARENT_CLASS(obj) \
        OBJECT_GET_PARENT_CLASS(obj, TYPE_VIRTIO_GOLDFISH_PIPE)


typedef struct VirtIOGoldfishPipe {
    VirtIODevice parent_obj;

    VirtQueue *tx_vq;
    VirtQueue *rx_vq;
    VirtQueue *ctrl_vq;
} VirtIOGoldfishPipe;

#endif /* _QEMU_VIRTIO_GOLDFISH_PIPE_H */
