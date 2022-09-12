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
#define TYPE_VIRTIO_SND_PCI "virtio-snd-pci"
#define VIRTIO_SND(obj) \
        OBJECT_CHECK(VirtIOSound, (obj), TYPE_VIRTIO_SND)

typedef struct VirtIOSoundVqRingBufferItem VirtIOSoundVqRingBufferItem;
typedef struct VirtIOSoundVqRingBuffer VirtIOSoundVqRingBuffer;
typedef struct VirtIOSoundPCMBlock VirtIOSoundPCMBlock;
typedef struct VirtIOSoundPCMStream VirtIOSoundPCMStream;
typedef struct VirtIOSoundPCM VirtIOSoundPCM;
typedef struct VirtIOSound VirtIOSound;

enum {
    VIRTIO_PCM_STREAM_STATE_DISABLED = 0,
    VIRTIO_PCM_STREAM_STATE_ENABLED,
    VIRTIO_PCM_STREAM_STATE_PARAMS_SET,
    VIRTIO_PCM_STREAM_STATE_PREPARED,
    VIRTIO_PCM_STREAM_STATE_RUNNING
};

struct VirtIOSoundVqRingBufferItem {
    VirtQueueElement *el;
    int pos;
    int size;
};

struct VirtIOSoundVqRingBuffer {
    VirtIOSoundVqRingBufferItem *buf;
    uint16_t capacity;
    uint16_t size;
    uint16_t r;
    uint16_t w;
};

struct VirtIOSoundPCMStream {
    uint32_t buffer_bytes;
    uint32_t period_bytes;
    uint16_t guest_format;
    uint8_t state;

    /* not saved to snapshots */
    VirtIOSound *snd;
    union {
        SWVoiceIn *in;
        SWVoiceOut *out;
        void *raw;
    } voice;
    int64_t start_timestamp;
    VirtIOSoundVqRingBuffer pcm_buf;
    QemuMutex mtx;

    uint64_t frames_consumed;
    uint32_t period_frames;
    int32_t latency_bytes;
    uint32_t freq_hz;
    uint16_t aud_format;
    uint8_t id;
    uint8_t driver_frame_size;
    uint8_t aud_frame_size;
};

typedef struct VirtIOSound {
    VirtIODevice parent;
    VirtIOSoundPCMStream streams[VIRTIO_SND_NUM_PCM_STREAMS];

    /* not saved to snapshots */
    VirtQueue *ctl_vq;
    VirtQueue *event_vq;
    VirtQueue *tx_vq;
    VirtQueue *rx_vq;
    QEMUSoundCard card;
    bool enable_input_prop;
    bool enable_output_prop;
} VirtIOSound;

#endif /* QEMU_VIRTIO_SND_H */
