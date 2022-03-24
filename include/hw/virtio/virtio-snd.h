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

#define VIRTIO_SND_NUM_JACKS           2    /* speaker, mic */
#define VIRTIO_SND_NUM_PCM_TX_STREAMS  1    /* driver -> device */
#define VIRTIO_SND_NUM_PCM_RX_STREAMS  1    /* device -> driver */
#define VIRTIO_SND_NUM_PCM_STREAMS     (VIRTIO_SND_NUM_PCM_TX_STREAMS + VIRTIO_SND_NUM_PCM_RX_STREAMS)
#define VIRTIO_SND_NUM_CHMAPS          2    /* output, input */

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
    uint32_t buffer_bytes;
    uint16_t format;
    VirtIOSound *snd;
    union {
        SWVoiceIn *in;
        SWVoiceOut *out;
        void *raw;
    } voice;

    VirtIOSoundRingBuffer pcm_buf;
    QemuMutex mtx;

    QEMUAudioTimeStamp start_timestamp;
    uint64_t frames_sent;
    uint64_t frames_skipped;

    uint32_t latency_bytes;
    uint32_t features;
    int period_bytes;

    uint8_t state;
    uint32_t buffer_frames;
    uint32_t freq_hz;
    uint8_t id;
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
} VirtIOSound;

#endif /* QEMU_VIRTIO_SND_H */
