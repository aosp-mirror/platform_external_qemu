/*
 * Virtio Sound Device
 *
 * Copyright (C) 2019 OpenSynergy GmbH
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.  See the COPYING file in the
 * top-level directory.
 */

#ifndef QEMU_VIRTIO_SND_H
#define QEMU_VIRTIO_SND_H

#include "standard-headers/linux/virtio_snd.h"
#include "hw/virtio/virtio.h"
#include "audio/audio.h"

#define VIOSND_QUEUE_CTL 0
#define VIOSND_QUEUE_PCM 1
#define VIOSND_QUEUE_COUNT 2

#define TYPE_VIRTIO_SND "virtio-snd-device"
#define VIRTIO_SND(obj) \
        OBJECT_CHECK(VirtIOSound, (obj), TYPE_VIRTIO_SND)

typedef struct VirtIOSound {
    VirtIODevice parent_obj;
    VirtQueue *queues[VIOSND_QUEUE_COUNT];
    struct VirtIOSoundPCM *pcm;
    QEMUSoundCard card;
} VirtIOSound;

#endif /* QEMU_VIRTIO_SND_H */
