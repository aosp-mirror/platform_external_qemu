/*
 * Virtio Sound Device interface
 *
 * Copyright (C) 2019 OpenSynergy GmbH
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * (at your option) any later version.  See the COPYING file in the
 * top-level directory.
 */

#ifndef QEMU_VIRTIO_SND_COMMON_H
#define QEMU_VIRTIO_SND_COMMON_H

#include "qapi/error.h"
#include "qemu/queue.h"
#include "hw/virtio/virtio-snd.h"

#define AUDIO_CAP "virtio-snd"

#define VIOSND_REQUEST_MAX_SIZE     4096
#define VIOSND_RESPONSE_MAX_SIZE    4096

#define VIOSND_PCM_MIN_CHANNELS     2
#define VIOSND_PCM_MAX_CHANNELS     2

#define VIOSND_PCM_FORMATS \
    (VIRTIO_SND_PCM_FMTBIT_S8 | VIRTIO_SND_PCM_FMTBIT_U8 | \
     VIRTIO_SND_PCM_FMTBIT_S16 | VIRTIO_SND_PCM_FMTBIT_U16 | \
     VIRTIO_SND_PCM_FMTBIT_S32 | VIRTIO_SND_PCM_FMTBIT_U32)

#define VIOSND_PCM_RATES \
    (VIRTIO_SND_PCM_RATEBIT_8000 | VIRTIO_SND_PCM_RATEBIT_11025 | \
     VIRTIO_SND_PCM_RATEBIT_16000 | VIRTIO_SND_PCM_RATEBIT_22050 | \
     VIRTIO_SND_PCM_RATEBIT_32000 | VIRTIO_SND_PCM_RATEBIT_44100 | \
     VIRTIO_SND_PCM_RATEBIT_48000 | VIRTIO_SND_PCM_RATEBIT_64000 | \
     VIRTIO_SND_PCM_RATEBIT_88200 | VIRTIO_SND_PCM_RATEBIT_96000 | \
     VIRTIO_SND_PCM_RATEBIT_176400 | VIRTIO_SND_PCM_RATEBIT_192000)

typedef struct VirtIOSoundPCMBlock VirtIOSoundPCMBlock;
typedef struct VirtIOSoundPCMStream VirtIOSoundPCMStream;
typedef struct VirtIOSoundPCM VirtIOSoundPCM;
typedef struct VirtIOSound VirtIOSound;

struct VirtIOSoundPCMBlock {
    QSIMPLEQ_ENTRY(VirtIOSoundPCMBlock) entry;
    int size;
    int offset;
    uint8_t data[];
};

struct VirtIOSoundPCMStream {
    VirtIOSoundPCM *pcm;
    struct audsettings as;
    struct audsettings desired_as;
    union {
        SWVoiceIn *in;
        SWVoiceOut *out;
    } voice;
    QemuMutex queue_mutex;
    QSIMPLEQ_HEAD(, VirtIOSoundPCMBlock) queue;
};

struct VirtIOSoundPCM {
    VirtIOSound *snd;
    VirtIOSoundPCMStream *streams[VIRTIO_SND_F_PCM_INPUT + 1];
};

#endif /* QEMU_VIRTIO_SND_COMMON_H */
