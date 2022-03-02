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

#define VIRTIO_SND_NUM_JACKS        2   /* speaker, mic */
#define VIRTIO_SND_NUM_PCM_STREAMS  2
#define VIRTIO_SND_NUM_CHMAPS       2   /* output, input */

#define TYPE_VIRTIO_SND "virtio-snd"
#define VIRTIO_SND(obj) \
        OBJECT_CHECK(VirtIOSound, (obj), TYPE_VIRTIO_SND)

typedef struct VirtIOSoundRingBufferItem VirtIOSoundRingBufferItem;
typedef struct VirtIOSoundRingBuffer VirtIOSoundRingBuffer;
typedef struct VirtIOSoundPCMBlock VirtIOSoundPCMBlock;
typedef struct VirtIOSoundPCMStream VirtIOSoundPCMStream;
typedef struct VirtIOSoundPCM VirtIOSoundPCM;
typedef struct VirtIOSound VirtIOSound;

enum {
    VIRTIO_PCM_STREAM_STATE_DISABLED = 0,
    VIRTIO_PCM_STREAM_STATE_ENABLED,
    VIRTIO_PCM_STREAM_STATE_PREPARED,
    VIRTIO_PCM_STREAM_STATE_RUNNING
};

struct VirtIOSoundRingBufferItem {
    VirtQueueElement *el;
    int pos;
    int size;
};

struct VirtIOSoundRingBuffer {
    VirtIOSoundRingBufferItem *buf;
    int capacity;
    int size;
    int r;
    int w;
};

struct VirtIOSoundPCMStream {
    VirtIOSound *snd;
    union {
        SWVoiceIn *in;
        SWVoiceOut *out;
        void *raw;
    } voice;

    VirtIOSoundRingBuffer pcm_buf;
    struct audsettings as;
    QemuMutex mtx;

    QEMUAudioTimeStamp start_timestamp;
    uint64_t frames_sent;
    uint64_t frames_skipped;

    uint32_t latency_bytes;
    uint32_t features;
    int buffer_bytes;
    int period_bytes;
    int buffer_frames;

    uint8_t direction;
    uint8_t state;
    uint8_t frame_size;
};

typedef struct VirtIOSound {
    VirtIODevice parent;
    VirtQueue *ctl_vq;
    VirtQueue *event_vq;
    VirtQueue *tx_vq;
    VirtQueue *rx_vq;
    VirtIOSoundPCMStream streams[VIRTIO_SND_NUM_PCM_STREAMS];
    QEMUSoundCard card;

    union {
        struct virtio_snd_hdr hdr;
        struct virtio_snd_query_info r1;
        struct virtio_snd_jack_remap r2;
        struct virtio_snd_pcm_set_params r3;
        struct virtio_snd_pcm_hdr r4;
    } ctl_req_buf;
} VirtIOSound;

#endif /* QEMU_VIRTIO_SND_H */
