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
typedef struct VirtIOSoundPcmRingBuffer VirtIOSoundPcmRingBuffer;
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

// These buffers are in the kernel format
struct VirtIOSoundVqRingBufferItem {
    VirtQueueElement *el;
    int size;
};

struct VirtIOSoundVqRingBuffer {
    VirtIOSoundVqRingBufferItem *buf;
    uint16_t capacity;
    uint16_t size;
    uint16_t r;
    uint16_t w;
};

// This buffer is in the QEMU audio format (to use directly with AUD_xyz calls).
struct VirtIOSoundPcmRingBuffer {
    uint8_t *buf;
    uint32_t capacity;
    uint32_t size;
    uint32_t r;
    uint32_t w;
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
    QEMUTimer vq_irq_timer;             // lives between start-stop
    int64_t next_vq_irq_us;
    VirtIOSoundVqRingBuffer kpcm_buf;   // kernel
    VirtIOSoundPcmRingBuffer qpcm_buf;  // qemu
    QemuMutex mtx;

    uint32_t period_us;
    uint16_t period_frames;
    uint16_t aud_format;
    uint8_t id;
    uint8_t driver_frame_size;
    uint8_t aud_frame_size;
    uint8_t n_periods;
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
#ifdef __linux__
    SWVoiceIn *linux_mic_workaround;  // b/292115117
#endif  // __linux__
    bool enable_input_prop;
    bool enable_output_prop;
} VirtIOSound;

#endif /* QEMU_VIRTIO_SND_H */
