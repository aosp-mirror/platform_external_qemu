/* Copyright (C) 2020 The Android Open Source Project
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include "stdint.h"
#include "stdbool.h"
#include "stdlib.h"
#include "stdio.h"
#include "qemu/compiler.h"
#include "qemu/osdep.h"
#include "qemu/timer.h"
#include "qemu/typedefs.h"
#include "qapi/error.h"
#include "standard-headers/linux/virtio_ids.h"
#include "audio/audio_forwarder.h"
#include "hw/audio/soundhw.h"
#include "hw/virtio/virtio-snd.h"
#include "hw/pci/pci.h"
#include "intel-hda-defs.h"

#define VIRTIO_SND_SNAPSHOT_VERSION 1

#define VIRTIO_SND_QUEUE_CTL    0
#define VIRTIO_SND_QUEUE_EVENT  1
#define VIRTIO_SND_QUEUE_TX     2
#define VIRTIO_SND_QUEUE_RX     3

#define VIRTIO_SND_PCM_FORMAT                       S16
#define VIRTIO_SND_PCM_NUM_MIN_CHANNELS             1
#define VIRTIO_SND_PCM_AUD_NUM_MAX_CHANNELS         2
#define VIRTIO_SND_PCM_MIC_NUM_MAX_CHANNELS         2
#define VIRTIO_SND_PCM_SPEAKERS_NUM_MAX_CHANNELS    5

#define VIRTIO_SND_NID_MIC                          0
#define VIRTIO_SND_NID_SPEAKERS                     0

#define RING_BUFFER_BAD_SIZE                        11111
#define RING_BUFFER_BAD_R                           22222
#define RING_BUFFER_BAD_W                           33333

#define AUD_SCRATCH_SIZE                            (800 * VIRTIO_SND_PCM_AUD_NUM_MAX_CHANNELS)

#define GLUE(A, B) A##B
#define GLUE2(A, B) GLUE(A, B)

static void log_failure(const char *tag, const char *func, int line, const char *why) {
    fprintf(stderr, "%s | %s:%d %s\n", tag, func, line, why);
}

#define ABORT(why) do { log_failure("FATAL", __func__, __LINE__, why); abort(); } while (false)

#if 0
#define FAILURE(X) (log_failure("ERROR", __func__, __LINE__, #X), X)
#define ASSERT(X) if (!(X)) { log_failure("FATAL", __func__, __LINE__, #X); abort(); }
#define DPRINTF(FMT, ...) fprintf(stderr, "%s:%d " FMT "\n", __func__, __LINE__, __VA_ARGS__)
#define DEBUG(X) X
#else
#define FAILURE(X) (X)
#define ASSERT(X)
#define DPRINTF(FMT, ...)
#define DEBUG(X)
#endif

union VirtIOSoundCtlRequest {
    struct virtio_snd_hdr hdr;
    struct virtio_snd_query_info r1;
    struct virtio_snd_jack_remap r2;
    struct virtio_snd_pcm_set_params r3;
    struct virtio_snd_pcm_hdr r4;
};

// We don't want to insert zeroes because a predefined pattern (meander: +2, -2)
// will be easier to see in the waveform (for debugging purposes).
static void fill_silence(int16_t* buf, size_t sz) {
    for (sz /= sizeof(*buf); sz > 0; --sz, ++buf) { *buf = ((sz & 8) >> 1) - 2; }
}

/*
 * aaaaaaa bbbbb cccc
 *               frequency (VIRTIO_SND_PCM_RATE_x)
           format (VIRTIO_SND_PCM_FMT_x)
 * nchannels - 1
 */
#define VIRTIO_SND_PACK_FORMAT16(nchannels, fmt, freq) (freq | (fmt << 4) | (nchannels << 9))
#define VIRTIO_SND_FORMAT16_BAD 0

static unsigned format16_get_freq(uint16_t format16) {
    return format16 & 0xF;
}

static unsigned format16_get_format(uint16_t format16) {
    return (format16 >> 4) & 0x1F;
}

static unsigned format16_get_nchannels(uint16_t format16) {
    return format16 >> 9;
}

static int format16_get_freq_hz(uint16_t format16) {
    switch (format16_get_freq(format16)) {
    case VIRTIO_SND_PCM_RATE_8000:  return 8000;
    case VIRTIO_SND_PCM_RATE_11025: return 11025;
    case VIRTIO_SND_PCM_RATE_16000: return 16000;
    case VIRTIO_SND_PCM_RATE_22050: return 22050;
    case VIRTIO_SND_PCM_RATE_32000: return 32000;
    case VIRTIO_SND_PCM_RATE_44100: return 44100;
    case VIRTIO_SND_PCM_RATE_48000: return 48000;
    default:                        return FAILURE(-1);
    }
}

static unsigned format16_get_frame_size(uint16_t format16) {
    return format16_get_nchannels(format16) * sizeof(int16_t);
}

struct audsettings virtio_snd_unpack_format(uint16_t format16) {
    struct audsettings as;

    as.freq = format16_get_freq_hz(format16);

    switch (format16_get_format(format16)) {
    case VIRTIO_SND_PCM_FMT_S16:
        as.fmt = AUD_FMT_S16;
        break;

    default:
        as.fmt = FAILURE(-1);
        break;
    }

    as.nchannels = format16_get_nchannels(format16);
    as.endianness = AUDIO_HOST_ENDIANNESS;

    return as;
}

#define HDA_REG_DEFCONF(CONN, LOC, DEF_DEV, CONN_TYPE, COLOR, DEF_AS, SEQ) \
    (((CONN) << AC_DEFCFG_PORT_CONN_SHIFT) \
    |((LOC) << AC_DEFCFG_LOCATION_SHIFT) \
    |((DEF_DEV) << AC_DEFCFG_DEVICE_SHIFT) \
    |((CONN_TYPE) << AC_DEFCFG_CONN_TYPE_SHIFT) \
    |((COLOR) << AC_DEFCFG_COLOR_SHIFT) \
    |((DEF_AS) << AC_DEFCFG_ASSOC_SHIFT) \
    |(SEQ))

static const struct virtio_snd_jack_info jack_infos[VIRTIO_SND_NUM_JACKS] = {
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_MIC,
        },
        .features = 0,
        .hda_reg_defconf = HDA_REG_DEFCONF(
            AC_JACK_PORT_FIXED,
            AC_JACK_LOC_REAR_PANEL,
            AC_JACK_MIC_IN,
            AC_JACK_CONN_1_8,
            AC_JACK_COLOR_RED,
            1, 0),
        .hda_reg_caps = AC_PINCAP_IN,
        .connected = 1,
    },
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_SPEAKERS,
        },
        .features = 0,
        .hda_reg_defconf = HDA_REG_DEFCONF(
            AC_JACK_PORT_FIXED,
            AC_JACK_LOC_REAR_PANEL,
            AC_JACK_SPEAKER,
            AC_JACK_CONN_1_8,
            AC_JACK_COLOR_GREEN,
            2, 0),
        .hda_reg_caps = AC_PINCAP_OUT,
        .connected = 1,
    },
};

#undef HDA_REG_DEFCONF

const static struct virtio_snd_pcm_info g_pcm_infos[VIRTIO_SND_NUM_PCM_STREAMS] = {
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_MIC,
        },
        .features = 0,
        .formats = (1u << GLUE2(VIRTIO_SND_PCM_FMT_, VIRTIO_SND_PCM_FORMAT)),
        .rates = (1u << VIRTIO_SND_PCM_RATE_8000)
               | (1u << VIRTIO_SND_PCM_RATE_11025)
               | (1u << VIRTIO_SND_PCM_RATE_16000)
               | (1u << VIRTIO_SND_PCM_RATE_22050)
               | (1u << VIRTIO_SND_PCM_RATE_32000)
               | (1u << VIRTIO_SND_PCM_RATE_44100)
               | (1u << VIRTIO_SND_PCM_RATE_48000),
        .direction = VIRTIO_SND_D_INPUT,
        .channels_min = VIRTIO_SND_PCM_NUM_MIN_CHANNELS,
        .channels_max = VIRTIO_SND_PCM_MIC_NUM_MAX_CHANNELS,
    },
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_MIC,
        },
        .features = 0,
        .formats = (1u << GLUE2(VIRTIO_SND_PCM_FMT_, VIRTIO_SND_PCM_FORMAT)),
        .rates = (1u << VIRTIO_SND_PCM_RATE_8000)
               | (1u << VIRTIO_SND_PCM_RATE_11025)
               | (1u << VIRTIO_SND_PCM_RATE_16000)
               | (1u << VIRTIO_SND_PCM_RATE_22050)
               | (1u << VIRTIO_SND_PCM_RATE_32000)
               | (1u << VIRTIO_SND_PCM_RATE_44100)
               | (1u << VIRTIO_SND_PCM_RATE_48000),
        .direction = VIRTIO_SND_D_INPUT,
        .channels_min = VIRTIO_SND_PCM_NUM_MIN_CHANNELS,
        .channels_max = VIRTIO_SND_PCM_MIC_NUM_MAX_CHANNELS,
    },
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_SPEAKERS,
        },
        .features = 0,
        .formats = (1u << GLUE2(VIRTIO_SND_PCM_FMT_, VIRTIO_SND_PCM_FORMAT)),
        .rates = (1u << VIRTIO_SND_PCM_RATE_8000)
               | (1u << VIRTIO_SND_PCM_RATE_11025)
               | (1u << VIRTIO_SND_PCM_RATE_16000)
               | (1u << VIRTIO_SND_PCM_RATE_22050)
               | (1u << VIRTIO_SND_PCM_RATE_32000)
               | (1u << VIRTIO_SND_PCM_RATE_44100)
               | (1u << VIRTIO_SND_PCM_RATE_48000),
        .direction = VIRTIO_SND_D_OUTPUT,
        .channels_min = VIRTIO_SND_PCM_NUM_MIN_CHANNELS,
        .channels_max = VIRTIO_SND_PCM_SPEAKERS_NUM_MAX_CHANNELS,
    },
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_SPEAKERS,
        },
        .features = 0,
        .formats = (1u << GLUE2(VIRTIO_SND_PCM_FMT_, VIRTIO_SND_PCM_FORMAT)),
        .rates = (1u << VIRTIO_SND_PCM_RATE_8000)
               | (1u << VIRTIO_SND_PCM_RATE_11025)
               | (1u << VIRTIO_SND_PCM_RATE_16000)
               | (1u << VIRTIO_SND_PCM_RATE_22050)
               | (1u << VIRTIO_SND_PCM_RATE_32000)
               | (1u << VIRTIO_SND_PCM_RATE_44100)
               | (1u << VIRTIO_SND_PCM_RATE_48000),
        .direction = VIRTIO_SND_D_OUTPUT,
        .channels_min = VIRTIO_SND_PCM_NUM_MIN_CHANNELS,
        .channels_max = VIRTIO_SND_PCM_SPEAKERS_NUM_MAX_CHANNELS,
    },
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_SPEAKERS,
        },
        .features = 0,
        .formats = (1u << GLUE2(VIRTIO_SND_PCM_FMT_, VIRTIO_SND_PCM_FORMAT)),
        .rates = (1u << VIRTIO_SND_PCM_RATE_8000)
               | (1u << VIRTIO_SND_PCM_RATE_11025)
               | (1u << VIRTIO_SND_PCM_RATE_16000)
               | (1u << VIRTIO_SND_PCM_RATE_22050)
               | (1u << VIRTIO_SND_PCM_RATE_32000)
               | (1u << VIRTIO_SND_PCM_RATE_44100)
               | (1u << VIRTIO_SND_PCM_RATE_48000),
        .direction = VIRTIO_SND_D_OUTPUT,
        .channels_min = VIRTIO_SND_PCM_NUM_MIN_CHANNELS,
        .channels_max = VIRTIO_SND_PCM_SPEAKERS_NUM_MAX_CHANNELS,
    },
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_SPEAKERS,
        },
        .features = 0,
        .formats = (1u << GLUE2(VIRTIO_SND_PCM_FMT_, VIRTIO_SND_PCM_FORMAT)),
        .rates = (1u << VIRTIO_SND_PCM_RATE_8000)
               | (1u << VIRTIO_SND_PCM_RATE_11025)
               | (1u << VIRTIO_SND_PCM_RATE_16000)
               | (1u << VIRTIO_SND_PCM_RATE_22050)
               | (1u << VIRTIO_SND_PCM_RATE_32000)
               | (1u << VIRTIO_SND_PCM_RATE_44100)
               | (1u << VIRTIO_SND_PCM_RATE_48000),
        .direction = VIRTIO_SND_D_OUTPUT,
        .channels_min = VIRTIO_SND_PCM_NUM_MIN_CHANNELS,
        .channels_max = VIRTIO_SND_PCM_SPEAKERS_NUM_MAX_CHANNELS,
    },
};

const struct virtio_snd_chmap_info g_chmap_infos[VIRTIO_SND_NUM_CHMAPS] = {
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_MIC,
        },
        .direction = VIRTIO_SND_D_INPUT,
        .channels = VIRTIO_SND_PCM_MIC_NUM_MAX_CHANNELS,
        .positions = {
            VIRTIO_SND_CHMAP_FL,
            VIRTIO_SND_CHMAP_FR,
         },
    },
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_SPEAKERS,
        },
        .direction = VIRTIO_SND_D_OUTPUT,
        .channels = VIRTIO_SND_PCM_SPEAKERS_NUM_MAX_CHANNELS,
        .positions = {
            VIRTIO_SND_CHMAP_FL,
            VIRTIO_SND_CHMAP_FR,
            VIRTIO_SND_CHMAP_RL,
            VIRTIO_SND_CHMAP_RR,
            VIRTIO_SND_CHMAP_LFE,
        },
    },
};

static const char *const g_stream_name[VIRTIO_SND_NUM_PCM_STREAMS] = {
    "virtio-snd-mic0",
    "virtio-snd-mic1",
    "virtio-snd-speakers0",
    "virtio-snd-speakers1",
    "virtio-snd-speakers2",
    "virtio-snd-speakers3",
};

static const char *stream_state_str(const int state) {
    switch (state) {
    case VIRTIO_PCM_STREAM_STATE_DISABLED:  return "disabled";
    case VIRTIO_PCM_STREAM_STATE_ENABLED:   return "enabled";
    case VIRTIO_PCM_STREAM_STATE_PARAMS_SET:return "params_set";
    case VIRTIO_PCM_STREAM_STATE_PREPARED:  return "prepared";
    case VIRTIO_PCM_STREAM_STATE_RUNNING:   return "running";
    default:                                return "unknown";
    }
}

static bool is_output_stream(const VirtIOSoundPCMStream *stream) {
    ASSERT(stream->id < VIRTIO_SND_NUM_PCM_STREAMS);
    return g_pcm_infos[stream->id].direction == VIRTIO_SND_D_OUTPUT;
}

static void vq_consume_element(VirtQueue *vq, VirtQueueElement *e, size_t size) {
    virtqueue_push(vq, e, size);
    g_free(e);
}

static size_t el_send_data(VirtQueueElement *e, const void *data, size_t size) {
    return iov_from_buf(e->in_sg, e->in_num, 0, data, size);
}

static size_t el_send_status_data(VirtQueueElement *e,
                                  unsigned code,
                                  const void *data, size_t size) {
    struct virtio_snd_hdr status = { .code = code };

    size_t n = iov_from_buf(e->in_sg, e->in_num, 0, &status, sizeof(status));
    n += iov_from_buf(e->in_sg, e->in_num, sizeof(status), data, size);

    return n;
}

static size_t el_send_status(VirtQueueElement *e, unsigned status) {
    struct virtio_snd_hdr data = { .code = status };
    return el_send_data(e, &data, sizeof(data));
}

static size_t el_send_pcm_status(VirtQueueElement *e, size_t offset,
                                 unsigned status, unsigned latency_bytes) {
    const struct virtio_snd_pcm_status data = {
        .status = status,
        .latency_bytes = latency_bytes,
    };

    iov_from_buf(e->in_sg, e->in_num, offset, &data, sizeof(data));
    return offset + sizeof(data);
}

static void vq_ring_buffer_init(VirtIOSoundVqRingBuffer *rb) {
    ASSERT(rb);

    rb->buf = NULL;
    rb->capacity = 0;
    rb->size = RING_BUFFER_BAD_SIZE;
    rb->r = RING_BUFFER_BAD_R;
    rb->w = RING_BUFFER_BAD_W;
};

static bool vq_ring_buffer_alloc(VirtIOSoundVqRingBuffer *rb, const size_t capacity) {
    ASSERT(capacity > 0);
    ASSERT(rb);
    ASSERT(!rb->buf);
    ASSERT(!rb->capacity);
    DPRINTF("rb=%p capacity=%zu", rb, capacity);

    rb->buf = g_new0(VirtIOSoundVqRingBufferItem, capacity);
    if (!rb->buf) {
        return FAILURE(false);
    }

    rb->capacity = capacity;
    rb->size = 0;
    rb->r = 0;
    rb->w = 0;

    return true;
};

static void vq_ring_buffer_free(VirtIOSoundVqRingBuffer *rb) {
    ASSERT(rb);
    ASSERT(!rb->size);
    ASSERT(rb->r == rb->w);
    ASSERT(rb->buf);
    DPRINTF("rb=%p", rb);

    g_free(rb->buf);
    vq_ring_buffer_init(rb);
};

static bool vq_ring_buffer_push(VirtIOSoundVqRingBuffer *rb,
                                const VirtIOSoundVqRingBufferItem *item) {
    ASSERT(rb);
    ASSERT(rb->capacity > 0);
    ASSERT(rb->size <= rb->capacity);
    ASSERT(rb->w < rb->capacity);

    const int capacity = rb->capacity;
    const int size = rb->size;
    int w;

    if (size >= capacity) {
        return FAILURE(false);
    }

    w = rb->w;
    rb->buf[w] = *item;
    rb->w = (w + 1) % capacity;
    rb->size = size + 1;

    return true;
}

static VirtIOSoundVqRingBufferItem *vq_ring_buffer_top(VirtIOSoundVqRingBuffer *rb) {
    ASSERT(rb);
    ASSERT(rb->capacity > 0);
    ASSERT(rb->size <= rb->capacity);
    ASSERT(rb->r < rb->capacity);

    if (rb->size > 0) {
        ASSERT(rb->buf[rb->r].el != NULL);
        return &rb->buf[rb->r];
    } else {
        return NULL;
    }
}

static void vq_ring_buffer_pop(VirtIOSoundVqRingBuffer *rb) {
    ASSERT(rb);
    ASSERT(rb->capacity > 0);
    ASSERT(rb->size > 0);
    ASSERT(rb->size <= rb->capacity);
    ASSERT(rb->r < rb->capacity);

    DEBUG(rb->buf[rb->r].el = NULL);
    rb->r = (rb->r + 1) % rb->capacity;
    rb->size = rb->size - 1;
}

static void vq_ring_buffer_save(const VirtIOSoundVqRingBuffer *rb, QEMUFile *f) {
    ASSERT(rb);
    DPRINTF("rb=%p size=%u", rb, rb->size);

    const unsigned capacity = rb->capacity;
    unsigned size = rb->size;
    ASSERT(size <= capacity);
    qemu_put_be16(f, size);

    unsigned r = rb->r;
    for (; size > 0; --size, r = ((r + 1) % capacity)) {
        ASSERT(r < capacity);
        ASSERT(rb->buf[r].el != NULL);
        qemu_put_virtqueue_element(f, rb->buf[r].el);
    }
}

static int vq_ring_buffer_load(VirtIOSoundVqRingBuffer *rb, VirtIODevice *vdev,
                               QEMUFile *f, const size_t capacity) {
    ASSERT(rb);
    ASSERT(!rb->buf);
    ASSERT(!rb->capacity);

    if (!capacity) {
        return 0;
    }

    unsigned size = qemu_get_be16(f);
    DPRINTF("rb=%p capacity=%zu size=%u", rb, capacity, size);
    if (size > capacity) {
        return FAILURE(-EINVAL);
    }

    if (!vq_ring_buffer_alloc(rb, capacity)) {
        return -ENOMEM;
    }

    for (; size > 0; --size) {
        VirtIOSoundVqRingBufferItem item;
        item.el = qemu_get_virtqueue_element(vdev, f, sizeof(VirtQueueElement));
        if (!item.el) {
            ABORT("qemu_get_virtqueue_element");
        }

        if (!vq_ring_buffer_push(rb, &item)) {
            ABORT("vq_ring_buffer_push");
        }
    }

    return 0;
}

static void pcm_ring_buffer_init(VirtIOSoundPcmRingBuffer *rb) {
    ASSERT(rb);
    rb->buf = NULL;
    rb->capacity = 0;
    rb->size = RING_BUFFER_BAD_SIZE;
    rb->r = RING_BUFFER_BAD_R;
    rb->w = RING_BUFFER_BAD_W;
}

static bool pcm_ring_buffer_alloc(VirtIOSoundPcmRingBuffer *rb, const size_t capacity) {
    ASSERT(capacity > 0);
    ASSERT(rb);
    ASSERT(!rb->buf);
    ASSERT(!rb->capacity);
    DPRINTF("rb=%p capacity=%zu", rb, capacity);

    rb->buf = g_new0(uint8_t, capacity);
    if (!rb->buf) {
        return FAILURE(false);
    }

    rb->capacity = capacity;
    rb->size = 0;
    rb->r = 0;
    rb->w = 0;

    return true;
}

static void pcm_ring_buffer_free(VirtIOSoundPcmRingBuffer *rb) {
    ASSERT(rb);
    ASSERT(rb->buf);
    DPRINTF("rb=%p", rb);

    g_free(rb->buf);
    pcm_ring_buffer_init(rb);
}

static void* pcm_ring_buffer_get_consume_chunk(VirtIOSoundPcmRingBuffer *rb,
                                               int *chunk_sz) {
    ASSERT(rb);
    ASSERT(rb->capacity > 0);
    ASSERT(rb->size >= 0);
    ASSERT(rb->size <= rb->capacity);
    ASSERT(rb->r < rb->capacity);
    ASSERT(rb->w < rb->capacity);

    const int r = rb->r;
    const int w = rb->w;
    const int size = rb->size;

    if (w > r) {
        *chunk_sz = MIN(w - r, size);
    } else {
        *chunk_sz = MIN(rb->capacity - r, size);
    }

    return &rb->buf[rb->r];
}

static int pcm_ring_buffer_consume(VirtIOSoundPcmRingBuffer *rb, const int sz) {
    ASSERT(rb);
    ASSERT(rb->capacity > 0);
    ASSERT(rb->size >= 0);
    ASSERT(rb->size <= rb->capacity);
    ASSERT(rb->r < rb->capacity);
    ASSERT(rb->w < rb->capacity);
    ASSERT(sz >= 0);
    ASSERT(sz <= rb->size);
    ASSERT((rb->r + sz) <= rb->capacity);

    rb->r = (rb->r + sz) % rb->capacity;
    const int new_size = rb->size - sz;
    rb->size = new_size;
    return new_size;
}

static void* pcm_ring_buffer_get_produce_chunk(VirtIOSoundPcmRingBuffer *rb,
                                               int room_size,
                                               int *chunk_sz) {
    ASSERT(rb);
    ASSERT(rb->capacity > 0);
    ASSERT(rb->size >= 0);
    ASSERT(rb->size <= rb->capacity);
    ASSERT(rb->r < rb->capacity);
    ASSERT(rb->w < rb->capacity);
    ASSERT(room_size >= 0);

    const int capacity = rb->capacity;
    room_size = MIN(room_size, capacity);

    const int to_drop = room_size - (capacity - rb->size);
    if (to_drop > 0) {
        ASSERT(to_drop <= rb->size);
        DPRINTF("rb=%p room_size=%d to_drop=%d", rb, room_size, to_drop);
        rb->r = (rb->r + to_drop) % capacity;
        rb->size -= to_drop;
    }

    const int left = capacity - rb->size;
    const int r = rb->r;
    const int w = rb->w;

    if (w >= r) {
        *chunk_sz = MIN(room_size, MIN(capacity - w, left));
    } else {
        *chunk_sz = MIN(room_size, MIN(r - w, left));
    }

    return &rb->buf[rb->w];
}

static int pcm_ring_buffer_produce(VirtIOSoundPcmRingBuffer *rb, const int sz) {
    ASSERT(sz >= 0);
    ASSERT(rb);
    ASSERT(rb->capacity > 0);
    ASSERT(sz <= rb->capacity);
    ASSERT(rb->size >= 0);
    ASSERT(rb->size <= rb->capacity);
    ASSERT((rb->size + sz) <= rb->capacity);
    ASSERT(rb->r >= 0);
    ASSERT(rb->r < rb->capacity);
    ASSERT(rb->w >= 0);
    ASSERT(rb->w < rb->capacity);
    ASSERT((rb->w + sz) <= rb->capacity);

    rb->w = (rb->w + sz) % rb->capacity;
    const int new_size = rb->size + sz;
    rb->size = new_size;
    return new_size;
}

/*
 * The contents will be replaced with silence, otherwise we will have to handle
 * channel conversion just to play extra ~25ms. See pcm_ring_buffer_post_load.
 */
static void pcm_ring_buffer_save(const VirtIOSoundPcmRingBuffer *rb, QEMUFile *f,
                                 const size_t frame_size) {
    ASSERT(rb);
    ASSERT(frame_size > 0);
    DPRINTF("rb=%p size=%u frame_size=%zu", rb, rb->size, frame_size);
    qemu_put_be32(f, rb->size / frame_size);
}

static int pcm_ring_buffer_load(VirtIOSoundPcmRingBuffer *rb, QEMUFile *f) {
    ASSERT(rb);
    ASSERT(!rb->buf);
    ASSERT(!rb->capacity);
    rb->size = qemu_get_be32(f);  // this needs to be properly handled in pcm_ring_buffer_post_load
    DPRINTF("rb=%p size=%u", rb, rb->size);
    return 0;
}

static int pcm_ring_buffer_post_load(VirtIOSoundPcmRingBuffer *rb,
                                     const size_t capacity,
                                     const size_t frame_size) {
    ASSERT(rb);
    ASSERT(!rb->buf);
    ASSERT(!rb->capacity);
    ASSERT(capacity > 0);
    ASSERT(frame_size > 0);
    DPRINTF("rb=%p size=%u capacity=%zu frame_size=%zu",
            rb, rb->size, capacity, frame_size);

    const size_t silence_bytes = rb->size * frame_size;
    if (silence_bytes > capacity) {
        return FAILURE(-EINVAL);
    }

    if (!pcm_ring_buffer_alloc(rb, capacity)) {
        return -ENOMEM;
    }

    memset(rb->buf, 0, silence_bytes);
    rb->w = silence_bytes;
    rb->size = silence_bytes;

    return 0;
}

static void stream_out_cb(void *opaque, int avail);
static void stream_in_cb(void *opaque, int avail);

static VirtIOSoundPCMStream *g_input_stream = NULL;

static SWVoiceIn *virtio_snd_set_voice_in(SWVoiceIn *voice) {
    if (g_input_stream) {
        struct audsettings as = virtio_snd_unpack_format(g_input_stream->host_format);

        g_input_stream->voice.in = AUD_open_in(&g_input_stream->snd->card,
                                               voice,
                                               g_stream_name[g_input_stream->id],
                                               g_input_stream,
                                               &stream_in_cb,
                                               &as);

        return g_input_stream->voice.in;
    } else {
        return voice;
    }
}

static SWVoiceIn *virtio_snd_get_voice_in() {
    return g_input_stream ? g_input_stream->voice.in : NULL;
}

static void virtio_snd_stream_set_aud_format(VirtIOSoundPCMStream *stream,
                                             const unsigned nchannels,
                                             const uint16_t format16_fmt_freq) {
    const unsigned fmt = format16_get_format(format16_fmt_freq);
    const unsigned freq = format16_get_freq(format16_fmt_freq);
    const uint16_t host_format = VIRTIO_SND_PACK_FORMAT16(nchannels, fmt, freq);

    stream->host_format = host_format;
    stream->host_frame_size = format16_get_frame_size(host_format);
}

static bool virtio_snd_voice_open(VirtIOSoundPCMStream *stream,
                                  const uint16_t kernel_format16) {
    ASSERT(stream->voice.raw == NULL);

    if (kernel_format16 == VIRTIO_SND_FORMAT16_BAD) {
        return FAILURE(false);
    }

    struct audsettings as = virtio_snd_unpack_format(kernel_format16);

    if (is_output_stream(stream)) {
        if (stream->snd->enable_output_prop) {
            for (as.nchannels = MIN(as.nchannels, VIRTIO_SND_PCM_AUD_NUM_MAX_CHANNELS);
                 as.nchannels > 0; --as.nchannels) {

                stream->voice.out = AUD_open_out(&stream->snd->card,
                                                 NULL,
                                                 g_stream_name[stream->id],
                                                 stream,
                                                 &stream_out_cb,
                                                 &as);
                if (stream->voice.out) {
                    AUD_set_active_out(stream->voice.out, 1);
                    virtio_snd_stream_set_aud_format(stream, as.nchannels, kernel_format16);
                    return true;
                }
            }
        }
    } else {
        if (stream->snd->enable_input_prop) {
            for (as.nchannels = MIN(as.nchannels, VIRTIO_SND_PCM_AUD_NUM_MAX_CHANNELS);
                 as.nchannels > 0; --as.nchannels) {
                stream->voice.in = AUD_open_in(&stream->snd->card,
                                               NULL,
                                               g_stream_name[stream->id],
                                               stream,
                                               &stream_in_cb,
                                               &as);
                if (stream->voice.in) {
                    AUD_set_active_in(stream->voice.in, 1);
                    g_input_stream = stream;
                    virtio_snd_stream_set_aud_format(stream, as.nchannels, kernel_format16);
                    return true;
                }
            }
        }
    }

    return FAILURE(false);
}

static void virtio_snd_voice_close(VirtIOSoundPCMStream *stream) {
    ASSERT(stream->voice.raw != NULL);

    if (is_output_stream(stream)) {
        AUD_set_active_out(stream->voice.out, 0);
        AUD_close_out(&stream->snd->card, stream->voice.out);
    } else {
        g_input_stream = NULL;
        AUD_set_active_in(stream->voice.in, 0);
        AUD_close_in(&stream->snd->card, stream->voice.in);
    }

    stream->voice.raw = NULL;
    stream->host_format = VIRTIO_SND_FORMAT16_BAD;
}

static void virtio_snd_stream_init(const unsigned stream_id,
                                   VirtIOSoundPCMStream *stream,
                                   VirtIOSound *snd) {
    memset(stream, 0, sizeof(*stream));

    stream->id = stream_id;
    stream->snd = snd;
    stream->voice.raw = NULL;
    stream->host_format = VIRTIO_SND_FORMAT16_BAD;
    vq_ring_buffer_init(&stream->gpcm_buf);
    pcm_ring_buffer_init(&stream->hpcm_buf);
    qemu_mutex_init(&stream->mtx);
    stream->state = VIRTIO_PCM_STREAM_STATE_DISABLED;
}

static void virtio_snd_stream_output_irq(void* opaque);
static void virtio_snd_stream_input_irq(void* opaque);

static void virtio_snd_stream_start_locked(VirtIOSoundPCMStream *stream) {
    if (is_output_stream(stream)) {
        timer_init_us(&stream->vq_irq_timer, QEMU_CLOCK_VIRTUAL_RT,
                      &virtio_snd_stream_output_irq, stream);
    } else {
        timer_init_us(&stream->vq_irq_timer, QEMU_CLOCK_VIRTUAL_RT,
                      &virtio_snd_stream_input_irq, stream);
    }

    stream->next_vq_irq_us = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL_RT) + stream->period_us;
    timer_mod(&stream->vq_irq_timer, stream->next_vq_irq_us);
}

static void virtio_snd_stream_stop_locked(VirtIOSoundPCMStream *stream) {
    timer_del(&stream->vq_irq_timer);
}

static size_t virtio_snd_stream_get_hpcm_capacity(const VirtIOSoundPCMStream *s) {
    ASSERT(s->n_periods > 0);
    ASSERT(s->period_frames > 0);
    ASSERT(s->host_frame_size > 0);
    return (s->n_periods + 1) * s->period_frames * s->host_frame_size;
}

static bool virtio_snd_stream_prepare0_locked(VirtIOSoundPCMStream *stream) {
    const uint16_t guest_format = stream->guest_format;
    ASSERT(guest_format != VIRTIO_SND_FORMAT16_BAD);

    if (!virtio_snd_voice_open(stream, guest_format)) {
        return FAILURE(false);
    }

    stream->guest_frame_size = format16_get_frame_size(guest_format);
    ASSERT(stream->guest_frame_size > 0);
    const int freq_hz = format16_get_freq_hz(guest_format);
    ASSERT(freq_hz > 0);
    ASSERT(stream->period_frames > 0);
    stream->period_us = 1000000LL * stream->period_frames / freq_hz;

    return true;
}

static bool virtio_snd_stream_prepare_locked(VirtIOSoundPCMStream *stream) {
    if (!virtio_snd_stream_prepare0_locked(stream)) {
        return false;
    }

    ASSERT(stream->n_periods > 0);
    if (!vq_ring_buffer_alloc(&stream->gpcm_buf, stream->n_periods)) {
        virtio_snd_voice_close(stream);
        return FAILURE(false);
    }

    ASSERT(stream->host_frame_size > 0);
    if (!pcm_ring_buffer_alloc(&stream->hpcm_buf,
                               virtio_snd_stream_get_hpcm_capacity(stream))) {
        virtio_snd_voice_close(stream);
        vq_ring_buffer_free(&stream->gpcm_buf);
        return FAILURE(false);
    }

    return true;
}

static void virtio_snd_stream_flush_vq_locked(VirtIOSoundPCMStream *stream,
                                              VirtQueue *vq) {
    const int guest_latency_bytes =
        stream->hpcm_buf.size / stream->host_frame_size * stream->guest_frame_size;
    VirtIOSoundVqRingBuffer *gpcm_buf = &stream->gpcm_buf;
    bool need_notify = false;

    while (true) {
        VirtIOSoundVqRingBufferItem *item = vq_ring_buffer_top(gpcm_buf);
        if (item) {
            VirtQueueElement *e = item->el;

            vq_consume_element(
                vq, e,
                el_send_pcm_status(e, 0, VIRTIO_SND_S_OK, guest_latency_bytes));
            vq_ring_buffer_pop(gpcm_buf);
            need_notify = true;
        } else {
            break;
        }
    }

    if (need_notify) {
        virtio_notify(&stream->snd->parent, vq);
    }
}

static void virtio_snd_stream_unprepare_locked(VirtIOSoundPCMStream *stream) {
    virtio_snd_stream_flush_vq_locked(stream, is_output_stream(stream) ?
                                      stream->snd->tx_vq : stream->snd->rx_vq);
    virtio_snd_voice_close(stream);
    vq_ring_buffer_free(&stream->gpcm_buf);
    pcm_ring_buffer_free(&stream->hpcm_buf);
}

static void virtio_snd_stream_enable(VirtIOSoundPCMStream *stream) {
    qemu_mutex_lock(&stream->mtx);
    switch (stream->state) {
    case VIRTIO_PCM_STREAM_STATE_DISABLED:
        stream->state = VIRTIO_PCM_STREAM_STATE_ENABLED;
        /* fallthrough */

    case VIRTIO_PCM_STREAM_STATE_ENABLED:
    case VIRTIO_PCM_STREAM_STATE_PARAMS_SET:
    case VIRTIO_PCM_STREAM_STATE_PREPARED:
    case VIRTIO_PCM_STREAM_STATE_RUNNING:
        break;

    default:
        ABORT("stream->state");
    }
    qemu_mutex_unlock(&stream->mtx);
}

static void virtio_snd_stream_disable(VirtIOSoundPCMStream *stream) {
    qemu_mutex_lock(&stream->mtx);
    switch (stream->state) {
    case VIRTIO_PCM_STREAM_STATE_RUNNING:
        virtio_snd_stream_stop_locked(stream);
        /* fallthrough */

    case VIRTIO_PCM_STREAM_STATE_PREPARED:
        virtio_snd_stream_unprepare_locked(stream);
        /* fallthrough */

    case VIRTIO_PCM_STREAM_STATE_PARAMS_SET:
    case VIRTIO_PCM_STREAM_STATE_ENABLED:
        stream->state = VIRTIO_PCM_STREAM_STATE_DISABLED;
        /* fallthrough */

    case VIRTIO_PCM_STREAM_STATE_DISABLED:
        break;

    default:
        ABORT("stream->state");
    }
    qemu_mutex_unlock(&stream->mtx);
}

static size_t
virtio_snd_process_ctl_jack_info(VirtQueueElement *e,
                                 const struct virtio_snd_query_info* req,
                                 VirtIOSound* snd) {
    if (req->size == sizeof(struct virtio_snd_jack_info)) {
        DPRINTF("req={start_id=%u, count=%u}", req->start_id, req->count);
        if ((req->count <= VIRTIO_SND_NUM_JACKS)
                && ((req->start_id + req->count) <= VIRTIO_SND_NUM_JACKS)
                && ((req->start_id + req->count) > req->start_id)) {
            return el_send_status_data(e, VIRTIO_SND_S_OK,
                &jack_infos[req->start_id],
                sizeof(jack_infos[0]) * req->count);
        }
    }

    return el_send_status(e, FAILURE(VIRTIO_SND_S_BAD_MSG));
}

static size_t
virtio_snd_process_ctl_jack_remap(VirtQueueElement *e,
                                  const struct virtio_snd_jack_remap* req,
                                  VirtIOSound* snd) {
    return el_send_status(e, FAILURE(VIRTIO_SND_S_NOT_SUPP));
}

static size_t
virtio_snd_process_ctl_pcm_info(VirtQueueElement *e,
                                const struct virtio_snd_query_info* req,
                                VirtIOSound* snd) {
    if (req->size == sizeof(struct virtio_snd_pcm_info)) {
        DPRINTF("req={start_id=%u, count=%u}", req->start_id, req->count);
        if ((req->count <= VIRTIO_SND_NUM_PCM_STREAMS)
                && ((req->start_id + req->count) <= VIRTIO_SND_NUM_PCM_STREAMS)
                && ((req->start_id + req->count) > req->start_id)) {
            return el_send_status_data(e, VIRTIO_SND_S_OK,
                &g_pcm_infos[req->start_id],
                sizeof(g_pcm_infos[0]) * req->count);
        }
    }

    return el_send_status(e, FAILURE(VIRTIO_SND_S_BAD_MSG));
}

static uint32_t
virtio_snd_process_ctl_pcm_set_params_impl(const struct virtio_snd_pcm_set_params* req,
                                           VirtIOSound* snd) {
    const unsigned stream_id = req->hdr.stream_id;
    DPRINTF("req={stream_id=%u, buffer_bytes=%u, period_bytes=%u, features=0x%08X, "
            "channels=%u, format=%u, rate=%u}", stream_id, req->buffer_bytes,
            req->period_bytes, req->features, req->channels, req->format, req->rate);
    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    const struct virtio_snd_pcm_info *pcm_info = &g_pcm_infos[stream_id];

    if (!(pcm_info->rates & (1u << req->rate))) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    if (!(pcm_info->formats & (1u << req->format))) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    if ((req->channels < pcm_info->channels_min) || (req->channels > pcm_info->channels_max)) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    const uint32_t buffer_bytes = req->buffer_bytes;
    if (!buffer_bytes) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    const uint32_t period_bytes = req->period_bytes;
    if (!period_bytes) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    ldiv_t r = ldiv(buffer_bytes, period_bytes);
    if ((r.quot <= 0) || (r.quot > 255) || r.rem) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    const uint8_t n_periods = r.quot;
    const uint16_t guest_format = VIRTIO_SND_PACK_FORMAT16(req->channels, req->format, req->rate);
    if (guest_format == VIRTIO_SND_FORMAT16_BAD) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }
    const uint8_t frame_size = format16_get_frame_size(guest_format);
    if (!frame_size) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }
    r = ldiv(req->period_bytes, frame_size);
    if ((r.quot <= 0) || (r.quot > UINT16_MAX) || r.rem) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }
    const uint16_t period_frames = r.quot;

    VirtIOSoundPCMStream *stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);
    switch (stream->state) {
    case VIRTIO_PCM_STREAM_STATE_PREPARED:
        virtio_snd_stream_unprepare_locked(stream);
        /* fallthrough */

    case VIRTIO_PCM_STREAM_STATE_PARAMS_SET:
    case VIRTIO_PCM_STREAM_STATE_ENABLED:
        DEBUG(stream->period_us = 0);
        DEBUG(stream->host_format = 0);
        DEBUG(stream->guest_frame_size = 0);
        DEBUG(stream->host_frame_size = 0);
        stream->guest_format = guest_format;
        stream->period_frames = period_frames;
        stream->n_periods = n_periods;
        stream->state = VIRTIO_PCM_STREAM_STATE_PARAMS_SET;
        qemu_mutex_unlock(&stream->mtx);
        return VIRTIO_SND_S_OK;

    case VIRTIO_PCM_STREAM_STATE_RUNNING:
    case VIRTIO_PCM_STREAM_STATE_DISABLED:
        qemu_mutex_unlock(&stream->mtx);
        return FAILURE(VIRTIO_SND_S_BAD_MSG);

    default:
        ABORT("stream->state");
    }
}

#define CLAMP_S16(S) ({ int s = (S); \
    s -= (s > 32767) * (s - 32767); \
    s += (s < -32768) * (-32768 - s); \
    s; \
})

#define FOR_UNROLLED(N, COND, NEXT, OP) \
    do { \
        for (int rem = ((N) % 8); rem > 0; --rem) { \
            OP; NEXT; \
        } \
        while ((COND)) { \
            OP; NEXT; OP; NEXT; OP; NEXT; OP; NEXT; \
            OP; NEXT; OP; NEXT; OP; NEXT; OP; NEXT; \
        } \
    } while (0)

static void convert_channels_1_to_2(int n,
                                    const int16_t *in_begin, const int16_t *in_end,
                                    int16_t *out_begin, int16_t *out_end) {
    const int16_t *in = in_end - 1;
    int16_t *out = out_end - 2;

    FOR_UNROLLED(n, in >= in_begin, (in -= 1, out -= 2), ({
            const int in0 = in[0];
            out[0] = in0;
            out[1] = in0;
    }));
}

static void convert_channels_1_to_3(int n,
                                    const int16_t *in_begin, const int16_t *in_end,
                                    int16_t *out_begin, int16_t *out_end) {
    const int16_t *in = in_end - 1;
    int16_t *out = out_end - 3;

    FOR_UNROLLED(n, in >= in_begin, (in -= 1, out -= 3), ({
            const int in0 = in[0];
            out[0] = in0;
            out[1] = in0;
            out[2] = in0;
    }));
}

static void convert_channels_1_to_4(int n,
                                    const int16_t *in_begin, const int16_t *in_end,
                                    int16_t *out_begin, int16_t *out_end) {
    const int16_t *in = in_end - 1;
    int16_t *out = out_end - 4;

    FOR_UNROLLED(n, in >= in_begin, (in -= 1, out -= 4), ({
            const int in0 = in[0];
            out[0] = in0;
            out[1] = in0;
            out[2] = in0;
            out[3] = in0;
    }));
}

static void convert_channels_1_to_5(int n,
                                    const int16_t *in_begin, const int16_t *in_end,
                                    int16_t *out_begin, int16_t *out_end) {
    const int16_t *in = in_end - 1;
    int16_t *out = out_end - 5;

    FOR_UNROLLED(n, in >= in_begin, (in -= 1, out -= 5), ({
            const int in0 = in[0];
            out[0] = in0;
            out[1] = in0;
            out[2] = in0;
            out[3] = in0;
            out[4] = in0;
    }));
}

static void convert_channels_2_to_1(int n,
                                    const int16_t *in, const int16_t *in_end,
                                    int16_t *out, int16_t *out_end) {
    FOR_UNROLLED(n, in < in_end, (in += 2, out += 1), ({
            const int in0 = in[0];
            const int in1 = in[1];
            out[0] = CLAMP_S16((in0 + in1) / 2);
    }));
}

static void convert_channels_2_to_3(int n,
                                    const int16_t *in_begin, const int16_t *in_end,
                                    int16_t *out_begin, int16_t *out_end) {
    const int16_t *in = in_end - 2;
    int16_t *out = out_end - 3;

    FOR_UNROLLED(n, in >= in_begin, (in -= 2, out -= 3), ({
        const int in0 = in[0];
        const int in1 = in[1];
        out[0] = in0;
        out[1] = in1;
        out[2] = CLAMP_S16((in0 + in1) / 2);
    }));
}

static void convert_channels_2_to_4(int n,
                                    const int16_t *in_begin, const int16_t *in_end,
                                    int16_t *out_begin, int16_t *out_end) {
    const int16_t *in = in_end - 2;
    int16_t *out = out_end - 4;

    FOR_UNROLLED(n, in >= in_begin, (in -= 2, out -= 4), ({
        const int in0 = in[0];
        const int in1 = in[1];
        out[0] = in0;
        out[1] = in1;
        out[2] = in0;
        out[3] = in1;
    }));
}

static void convert_channels_2_to_5(int n,
                                    const int16_t *in_begin, const int16_t *in_end,
                                    int16_t *out_begin, int16_t *out_end) {
    const int16_t *in = in_end - 2;
    int16_t *out = out_end - 5;

    FOR_UNROLLED(n, in >= in_begin, (in -= 2, out -= 5), ({
        const int in0 = in[0];
        const int in1 = in[1];
        out[0] = in0;
        out[1] = in1;
        out[2] = in0;
        out[3] = in1;
        out[4] = CLAMP_S16((in0 + in1) / 2);
    }));
}

static void convert_channels_3_to_1(int n,
                                    const int16_t *in, const int16_t *in_end,
                                    int16_t *out, int16_t *out_end) {
    FOR_UNROLLED(n, in < in_end, (in += 3, out += 1), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        out[0] = CLAMP_S16((85 * (in0 + in1 + in2)) / 256);
    }));
}

static void convert_channels_3_to_2(int n,
                                    const int16_t *in, const int16_t *in_end,
                                    int16_t *out, int16_t *out_end) {
    FOR_UNROLLED(n, in < in_end, (in += 3, out += 2), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in22 = in[2] / 2;
        out[0] = CLAMP_S16((170 * (in0 + in22)) / 256);
        out[1] = CLAMP_S16((170 * (in1 + in22)) / 256);
    }));
}

static void convert_channels_3_to_4(int n,
                                    const int16_t *in_begin, const int16_t *in_end,
                                    int16_t *out_begin, int16_t *out_end) {
    const int16_t *in = in_end - 3;
    int16_t *out = out_end - 4;

    FOR_UNROLLED(n, in >= in_begin, (in -= 3, out -= 4), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        out[0] = in0;
        out[1] = in1;
        out[2] = in2;
        out[3] = CLAMP_S16((in1 + in2) / 2);
    }));
}

static void convert_channels_3_to_5(int n,
                                    const int16_t *in_begin, const int16_t *in_end,
                                    int16_t *out_begin, int16_t *out_end) {
    const int16_t *in = in_end - 3;
    int16_t *out = out_end - 5;

    FOR_UNROLLED(n, in >= in_begin, (in -= 3, out -= 5), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        out[0] = in0;
        out[1] = in1;
        out[2] = in2;
        out[3] = in2;
        out[4] = in2;
    }));
}

static void convert_channels_4_to_1(int n,
                                    const int16_t *in, const int16_t *in_end,
                                    int16_t *out, int16_t *out_end) {
    FOR_UNROLLED(n, in < in_end, (in += 4, out += 1), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        out[0] = CLAMP_S16((in0 + in1 + in2 + in3) / 4);
    }));
}

static void convert_channels_4_to_2(int n,
                                    const int16_t *in, const int16_t *in_end,
                                    int16_t *out, int16_t *out_end) {
    FOR_UNROLLED(n, in < in_end, (in += 4, out += 2), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        out[0] = CLAMP_S16((in0 + in2) / 2);
        out[1] = CLAMP_S16((in1 + in3) / 2);
    }));
}

static void convert_channels_4_to_3(int n,
                                    const int16_t *in, const int16_t *in_end,
                                    int16_t *out, int16_t *out_end) {
    FOR_UNROLLED(n, in < in_end, (in += 4, out += 3), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        out[0] = in0;
        out[1] = in1;
        out[2] = CLAMP_S16((in2 + in3) / 2);
    }));
}

static void convert_channels_4_to_5(int n,
                                    const int16_t *in_begin, const int16_t *in_end,
                                    int16_t *out_begin, int16_t *out_end) {
    const int16_t *in = in_end - 4;
    int16_t *out = out_end - 5;

    FOR_UNROLLED(n, in >= in_begin, (in -= 4, out -= 5), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        out[0] = in0;
        out[1] = in1;
        out[2] = in2;
        out[3] = in3;
        out[4] = CLAMP_S16((in2 + in3) / 2);
    }));
}

static void convert_channels_5_to_1(int n,
                                    const int16_t *in, const int16_t *in_end,
                                    int16_t *out, int16_t *out_end) {
    FOR_UNROLLED(n, in < in_end, (in += 5, out += 1), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        const int in4 = in[4];
        out[0] = CLAMP_S16((51 * (in0 + in1 + in2 + in3 + in4)) / 256);
    }));
}

static void convert_channels_5_to_2(int n,
                                    const int16_t *in, const int16_t *in_end,
                                    int16_t *out, int16_t *out_end) {
    FOR_UNROLLED(n, in < in_end, (in += 5, out += 2), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        const int in44 = in[4] / 4;
        out[0] = CLAMP_S16(in0 / 2 + in2 / 4 + in44);
        out[1] = CLAMP_S16(in1 / 2 + in3 / 4 + in44);
    }));
}

static void convert_channels_5_to_3(int n,
                                    const int16_t *in, const int16_t *in_end,
                                    int16_t *out, int16_t *out_end) {
    FOR_UNROLLED(n, in < in_end, (in += 5, out += 3), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        const int in4 = in[4];
        out[0] = in0;
        out[1] = in1;
        out[2] = CLAMP_S16((85 * (in2 + in3 + in4)) / 256);
    }));
}

static void convert_channels_5_to_4(int n,
                                    const int16_t *in, const int16_t *in_end,
                                    int16_t *out, int16_t *out_end) {
    FOR_UNROLLED(n, in < in_end, (in += 5, out += 4), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        const int in42 = in[4] / 2;
        out[0] = in0;
        out[1] = in1;
        out[2] = CLAMP_S16((170 * (in2 + in42)) / 256);
        out[3] = CLAMP_S16((170 * (in3 + in42)) / 256);
    }));
}

static int convert_channels(const int16_t *buffer_in, const int in_sz,
                            const int in_fs, const int out_fs,
                            int16_t *buffer_out) {
    ASSERT(in_sz >= 0);

    if (!in_sz) {
        return 0;
    } else if (out_fs == in_fs) {
        if (buffer_in != buffer_out) {
            memcpy(buffer_out, buffer_in, in_sz);
        }
        return in_sz;
    } else {
        const int n = in_sz / in_fs;
        const int out_sz = n * out_fs;
        const int16_t *buffer_in_end = buffer_in + in_sz / sizeof(*buffer_in);
        int16_t *buffer_out_end = buffer_out + out_sz / sizeof(*buffer_out);

        switch (((in_fs << 4) + out_fs) / sizeof(*buffer_in)) {
        case 0x12:
            convert_channels_1_to_2(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x13:
            convert_channels_1_to_3(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x14:
            convert_channels_1_to_4(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x15:
            convert_channels_1_to_5(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x21:
            convert_channels_2_to_1(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x23:
            convert_channels_2_to_3(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x24:
            convert_channels_2_to_4(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x25:
            convert_channels_2_to_5(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x31:
            convert_channels_3_to_1(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x32:
            convert_channels_3_to_2(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x34:
            convert_channels_3_to_4(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x35:
            convert_channels_3_to_5(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x41:
            convert_channels_4_to_1(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x42:
            convert_channels_4_to_2(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x43:
            convert_channels_4_to_3(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x45:
            convert_channels_4_to_5(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x51:
            convert_channels_5_to_1(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x52:
            convert_channels_5_to_2(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x53:
            convert_channels_5_to_3(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;
        case 0x54:
            convert_channels_5_to_4(n, buffer_in, buffer_in_end, buffer_out, buffer_out_end);
            break;

        default:
            ABORT("bad");
        }

        return out_sz;
    }
}

static bool virtio_snd_stream_gpcm_to_hpcm_locked(VirtIOSoundPCMStream *stream) {
    const int host_fs = stream->host_frame_size;
    ASSERT(host_fs > 0);
    const int period_frames = stream->period_frames;
    ASSERT(period_frames > 0);

    if (stream->hpcm_buf.size >= (stream->n_periods * period_frames * host_fs)) {
        return false;
    }

    VirtIOSoundVqRingBufferItem *item = vq_ring_buffer_top(&stream->gpcm_buf);
    if (!item) {
        return FAILURE(false);
    }

    const int guest_fs = stream->guest_frame_size;
    ASSERT(guest_fs > 0);
    int16_t scratch[AUD_SCRATCH_SIZE];
    const int max_dst_chunk_size = sizeof(scratch) / guest_fs * host_fs;

    VirtQueueElement *const e = item->el;
    ASSERT(e != NULL);

    int pos = sizeof(struct virtio_snd_pcm_xfer);
    int hsize = period_frames * host_fs;
    ASSERT(hsize > 0);
    while (hsize > 0) {
        int dst_sz;
        void *dst = pcm_ring_buffer_get_produce_chunk(&stream->hpcm_buf, hsize, &dst_sz);
        ASSERT(dst != NULL);
        dst_sz = MIN(dst_sz, max_dst_chunk_size);
        ASSERT(dst_sz > 0);
        const int src_sz = dst_sz / host_fs * guest_fs;
        ASSERT(src_sz > 0);

        iov_to_buf(e->out_sg, e->out_num, pos, scratch, src_sz);
        convert_channels(scratch, src_sz, guest_fs, host_fs, (int16_t *)dst);
        pcm_ring_buffer_produce(&stream->hpcm_buf, dst_sz);
        pos += src_sz;
        hsize -= dst_sz;
    }

    vq_consume_element(
        stream->snd->tx_vq, e,
        el_send_pcm_status(e, 0, VIRTIO_SND_S_OK,
                           stream->hpcm_buf.size / host_fs * guest_fs));
    vq_ring_buffer_pop(&stream->gpcm_buf);
    return true;
}

static void stream_out_cb_locked(VirtIOSoundPCMStream *stream, int avail) {
    ASSERT(avail > 0);
    SWVoiceOut *const voice = stream->voice.out;
    ASSERT(voice != NULL);
    const int host_fs = stream->host_frame_size;
    ASSERT(host_fs > 0);
    ASSERT((avail % host_fs) == 0);
    ASSERT(stream->period_frames > 0);
    int min_write_sz = MIN(avail, MAX(stream->period_frames / 16, 4) * host_fs);

    while (avail > 0) {
        int size;
        void *data = pcm_ring_buffer_get_consume_chunk(&stream->hpcm_buf, &size);
        if (size > 0) {
            ASSERT((size % host_fs) == 0);
            const int to_write_sz = MIN(size, avail);
            const int written_sz = AUD_write(voice, data, to_write_sz);
            if (written_sz > 0) {
                ASSERT((written_sz % host_fs) == 0);
                pcm_ring_buffer_consume(&stream->hpcm_buf, written_sz);
                avail -= written_sz;
                min_write_sz -= written_sz;
            } else {
                return;
            }
        } else {
            ASSERT(stream->hpcm_buf.size == 0);
            if (min_write_sz > 0) {
                int16_t scratch[AUD_SCRATCH_SIZE];
                DPRINTF("inserting %d frames of silence, state=%s",
                        min_write_sz / host_fs, stream_state_str(stream->state));

                // Insert `min_write_sz` bytes of silence.
                fill_silence(scratch, MIN(sizeof(scratch), min_write_sz));
                do {
                    const int to_write_sz =
                        MIN(sizeof(scratch), min_write_sz) / host_fs * host_fs;
                    const int written_sz = AUD_write(voice, scratch, to_write_sz);
                    if (written_sz > 0) {
                        ASSERT((written_sz % host_fs) == 0);
                        min_write_sz -= written_sz;
                    } else {
                        return;
                    }
                } while (min_write_sz > 0);
            }
            break;
        }
    }
}

static void virtio_snd_stream_output_irq(void *opaque) {
    VirtIOSoundPCMStream *stream = (VirtIOSoundPCMStream *)opaque;

    qemu_mutex_lock(&stream->mtx);
    if (stream->state == VIRTIO_PCM_STREAM_STATE_RUNNING) {
        if (virtio_snd_stream_gpcm_to_hpcm_locked(stream)) {
            stream->next_vq_irq_us += stream->period_us;
            virtio_notify(&stream->snd->parent, stream->snd->tx_vq);
        } else {
            stream->next_vq_irq_us += (stream->period_us / 4);
        }

        timer_mod(&stream->vq_irq_timer, stream->next_vq_irq_us);
    }
    qemu_mutex_unlock(&stream->mtx);
}

static void stream_out_cb(void *opaque, int avail) {
    VirtIOSoundPCMStream *const stream = (VirtIOSoundPCMStream *)opaque;

    qemu_mutex_lock(&stream->mtx);
    stream_out_cb_locked(stream, avail);
    qemu_mutex_unlock(&stream->mtx);
}

static bool virtio_snd_process_tx(VirtQueue *vq, VirtQueueElement *e, VirtIOSound *snd) {
    const size_t req_size = iov_size(e->out_sg, e->out_num);
    struct virtio_snd_pcm_xfer xfer;

    if (req_size < sizeof(xfer)) {
        vq_consume_element(vq, e, 0);
        return FAILURE(true);
    }

    iov_to_buf(e->out_sg, e->out_num, 0, &xfer, sizeof(xfer));
    if (xfer.stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        vq_consume_element(vq, e, el_send_pcm_status(e, 0, VIRTIO_SND_S_BAD_MSG, 0));
        return FAILURE(true);
    }

    VirtIOSoundPCMStream *stream = &snd->streams[xfer.stream_id];
    VirtIOSoundVqRingBufferItem item;
    item.el = e;

    qemu_mutex_lock(&stream->mtx);
    if (!vq_ring_buffer_push(&stream->gpcm_buf, &item)) {
        ABORT("vq_ring_buffer_push");
    }
    qemu_mutex_unlock(&stream->mtx);
    return false;
}

static bool virtio_snd_stream_hpcm_to_gpcm_locked(VirtIOSoundPCMStream *stream) {
    VirtIOSoundVqRingBufferItem *item = vq_ring_buffer_top(&stream->gpcm_buf);
    if (!item) {
        return FAILURE(false);
    }

    const int host_fs = stream->host_frame_size;
    ASSERT(host_fs > 0);
    const int guest_fs = stream->guest_frame_size;
    ASSERT(guest_fs > 0);
    int16_t scratch[AUD_SCRATCH_SIZE];
    const int max_src_chunk_size =
        MIN(stream->period_frames, sizeof(scratch) / guest_fs) * host_fs;
    ASSERT(max_src_chunk_size >= host_fs);

    VirtQueueElement *e = item->el;
    ASSERT(e != NULL);

    const int period_bytes = stream->period_frames * guest_fs;
    int pos = 0;
    int gsize = period_bytes;
    ASSERT(gsize > 0);
    while (gsize > 0) {
        int src_size;
        const void *src = pcm_ring_buffer_get_consume_chunk(&stream->hpcm_buf, &src_size);
        if (src_size > 0) {
            ASSERT((src_size % host_fs) == 0);
            src_size = MIN(MIN(src_size, max_src_chunk_size),
                           gsize / guest_fs * host_fs);
            ASSERT(src_size > 0);
            const int dst_size = convert_channels(src, src_size,
                                                  host_fs, guest_fs, scratch);
            ASSERT(dst_size > 0);
            ASSERT((dst_size % guest_fs) == 0);
            iov_from_buf(e->in_sg, e->in_num, pos, scratch, dst_size);
            pcm_ring_buffer_consume(&stream->hpcm_buf, src_size);
            pos += dst_size;
            gsize -= dst_size;
        } else {
            ASSERT(stream->hpcm_buf.size == 0);
            ASSERT(gsize > 0);
            DPRINTF("inserting %d frames of silence", gsize / guest_fs);

            // Insert `gsize` bytes of silence.
            fill_silence(scratch, MIN(sizeof(scratch), gsize));
            do {
                const int dst_size = MIN(sizeof(scratch), gsize);
                iov_from_buf(e->in_sg, e->in_num, pos, scratch, dst_size);
                pos += dst_size;
                gsize -= dst_size;
            } while (gsize > 0);

            break;
        }
    }

    ASSERT(pos == period_bytes);

    vq_consume_element(
        stream->snd->rx_vq, e,
        el_send_pcm_status(e, pos, VIRTIO_SND_S_OK,
                           stream->hpcm_buf.size / host_fs * guest_fs));
    vq_ring_buffer_pop(&stream->gpcm_buf);
    return true;
}

static void stream_in_cb_locked(VirtIOSoundPCMStream *stream, int avail) {
    ASSERT(avail > 0);
    const int host_fs = stream->host_frame_size;
    ASSERT(host_fs > 0);
    ASSERT((avail % host_fs) == 0);
    ASSERT(stream->hpcm_buf.capacity > 0);
    int16_t scratch[AUD_SCRATCH_SIZE];
    const int max_read_size = MIN(sizeof(scratch) / host_fs * host_fs,
                                  stream->hpcm_buf.capacity);

    SWVoiceIn *const voice = stream->voice.in;
    ASSERT(voice != NULL);

    while (avail > 0) {
        int src_sz = AUD_read(voice, scratch, MIN(avail, max_read_size));
        if (src_sz > 0) {
            avail -= src_sz;
            ASSERT((src_sz % host_fs) == 0);
            const uint8_t* src = (const uint8_t*)(&scratch[0]);

            while (src_sz > 0) {
                int dst_sz;
                void *dst = pcm_ring_buffer_get_produce_chunk(&stream->hpcm_buf,
                                                              src_sz, &dst_sz);
                dst_sz = MIN(dst_sz, src_sz);
                memcpy(dst, src, dst_sz);
                pcm_ring_buffer_produce(&stream->hpcm_buf, dst_sz);
                src += dst_sz;
                src_sz -= dst_sz;
            }
        } else {
            break;
        }
    }
}

static void virtio_snd_stream_input_irq(void* opaque) {
    VirtIOSoundPCMStream *stream = (VirtIOSoundPCMStream *)opaque;

    qemu_mutex_lock(&stream->mtx);
    if (stream->state == VIRTIO_PCM_STREAM_STATE_RUNNING) {
        if (virtio_snd_stream_hpcm_to_gpcm_locked(stream)) {
            stream->next_vq_irq_us += stream->period_us;
            virtio_notify(&stream->snd->parent, stream->snd->rx_vq);
        } else {
            stream->next_vq_irq_us += (stream->period_us / 4);
        }

        timer_mod(&stream->vq_irq_timer, stream->next_vq_irq_us);
    }
    qemu_mutex_unlock(&stream->mtx);
}

static void stream_in_cb(void *opaque, int avail) {
    VirtIOSoundPCMStream *const stream = (VirtIOSoundPCMStream *)opaque;

    qemu_mutex_lock(&stream->mtx);
    stream_in_cb_locked(stream, avail);
    qemu_mutex_unlock(&stream->mtx);
}

static bool virtio_snd_process_rx(VirtQueue *vq, VirtQueueElement *e, VirtIOSound *snd) {
    const size_t req_size = iov_size(e->out_sg, e->out_num);
    struct virtio_snd_pcm_xfer xfer;

    if (req_size < sizeof(xfer)) {
        vq_consume_element(vq, e, 0);
        return FAILURE(true);
    }

    iov_to_buf(e->out_sg, e->out_num, 0, &xfer, sizeof(xfer));
    if (xfer.stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        vq_consume_element(vq, e, el_send_pcm_status(e, 0, VIRTIO_SND_S_BAD_MSG, 0));
        return FAILURE(true);
    }

    VirtIOSoundPCMStream *stream = &snd->streams[xfer.stream_id];
    VirtIOSoundVqRingBufferItem item;
    item.el = e;

    qemu_mutex_lock(&stream->mtx);
    if (!vq_ring_buffer_push(&stream->gpcm_buf, &item)) {
        ABORT("ring_buffer_push");
    }
    qemu_mutex_unlock(&stream->mtx);
    return false;
}

static uint32_t
virtio_snd_process_ctl_pcm_prepare_impl(unsigned stream_id, VirtIOSound* snd) {
    DPRINTF("stream_id=%u", stream_id);
    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    VirtIOSoundPCMStream *stream = &snd->streams[stream_id];
    uint32_t r;

    qemu_mutex_lock(&stream->mtx);
    switch (stream->state) {
    case VIRTIO_PCM_STREAM_STATE_PARAMS_SET:
        if (!virtio_snd_stream_prepare_locked(stream)) {
            r = FAILURE(VIRTIO_SND_S_IO_ERR);
            goto done;
        }
        stream->state = VIRTIO_PCM_STREAM_STATE_PREPARED;
        /* fallthrough */

    case VIRTIO_PCM_STREAM_STATE_PREPARED:
        r = VIRTIO_SND_S_OK;
        goto done;

    default:
        r = FAILURE(VIRTIO_SND_S_BAD_MSG);
        goto done;
    }

done:
    qemu_mutex_unlock(&stream->mtx);
    return r;
}

static uint32_t
virtio_snd_process_ctl_pcm_release_impl(unsigned stream_id, VirtIOSound* snd) {
    DPRINTF("stream_id=%u", stream_id);
    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    VirtIOSoundPCMStream *stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);
    if (stream->state != VIRTIO_PCM_STREAM_STATE_PREPARED) {
        qemu_mutex_unlock(&stream->mtx);
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }
    virtio_snd_stream_unprepare_locked(stream);
    stream->state = VIRTIO_PCM_STREAM_STATE_PARAMS_SET;
    qemu_mutex_unlock(&stream->mtx);

    return VIRTIO_SND_S_OK;
}

static uint32_t
virtio_snd_process_ctl_pcm_start_impl(unsigned stream_id, VirtIOSound* snd) {
    DPRINTF("stream_id=%u", stream_id);
    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    VirtIOSoundPCMStream *stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);

    if (stream->state != VIRTIO_PCM_STREAM_STATE_PREPARED) {
        qemu_mutex_unlock(&stream->mtx);
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    virtio_snd_stream_start_locked(stream);
    stream->state = VIRTIO_PCM_STREAM_STATE_RUNNING;

    qemu_mutex_unlock(&stream->mtx);
    return VIRTIO_SND_S_OK;
}

static uint32_t
virtio_snd_process_ctl_pcm_stop_impl(unsigned stream_id, VirtIOSound* snd) {
    DPRINTF("stream_id=%u", stream_id);
    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    VirtIOSoundPCMStream *stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);

    if (stream->state != VIRTIO_PCM_STREAM_STATE_RUNNING) {
        qemu_mutex_unlock(&stream->mtx);
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    virtio_snd_stream_stop_locked(stream);
    stream->state = VIRTIO_PCM_STREAM_STATE_PREPARED;

    qemu_mutex_unlock(&stream->mtx);
    return VIRTIO_SND_S_OK;
}

static size_t
virtio_snd_process_ctl_pcm_set_params(VirtQueueElement *e,
                                      const struct virtio_snd_pcm_set_params* req,
                                      VirtIOSound* snd) {
    return el_send_status(e, virtio_snd_process_ctl_pcm_set_params_impl(req, snd));
}

static size_t
virtio_snd_process_ctl_pcm_prepare(VirtQueueElement *e,
                                   const struct virtio_snd_pcm_hdr* req,
                                   VirtIOSound* snd) {
    return el_send_status(e, virtio_snd_process_ctl_pcm_prepare_impl(req->stream_id, snd));
}

static size_t
virtio_snd_process_ctl_pcm_release(VirtQueueElement *e,
                                   const struct virtio_snd_pcm_hdr* req,
                                   VirtIOSound* snd) {
    return el_send_status(e, virtio_snd_process_ctl_pcm_release_impl(req->stream_id, snd));
}

static size_t
virtio_snd_process_ctl_pcm_start(VirtQueueElement *e,
                                 const struct virtio_snd_pcm_hdr* req,
                                 VirtIOSound* snd) {
    return el_send_status(e, virtio_snd_process_ctl_pcm_start_impl(req->stream_id, snd));
}

static size_t
virtio_snd_process_ctl_pcm_stop(VirtQueueElement *e,
                                const struct virtio_snd_pcm_hdr* req,
                                VirtIOSound* snd) {
    return el_send_status(e, virtio_snd_process_ctl_pcm_stop_impl(req->stream_id, snd));
}

static size_t
virtio_snd_process_ctl_chmap_info(VirtQueueElement *e,
                                  const struct virtio_snd_query_info* req,
                                  VirtIOSound* snd) {
    if (req->size == sizeof(struct virtio_snd_chmap_info)) {
        DPRINTF("req={start_id=%u, count=%u}", req->start_id, req->count);
        if ((req->count <= VIRTIO_SND_NUM_CHMAPS)
                && ((req->start_id + req->count) <= VIRTIO_SND_NUM_CHMAPS)
                && ((req->start_id + req->count) > req->start_id)) {
            return el_send_status_data(e, VIRTIO_SND_S_OK,
                &g_chmap_infos[req->start_id],
                sizeof(g_chmap_infos[0]) * req->count);
        }
    }

    return el_send_status(e, FAILURE(VIRTIO_SND_S_BAD_MSG));
}

static size_t virtio_snd_process_ctl(VirtQueueElement *e, VirtIOSound *snd) {
    union VirtIOSoundCtlRequest ctl_req_buf;
    const size_t req_size = MIN(iov_size(e->out_sg, e->out_num), sizeof(ctl_req_buf));
    struct virtio_snd_hdr *hdr = &ctl_req_buf.hdr;

    if (req_size < sizeof(*hdr)) {
        return 0;
    }

    iov_to_buf(e->out_sg, e->out_num, 0, hdr, req_size);

    switch (hdr->code) {
#define HANDLE_CTL(CODE, TYPE, HANDLER) \
    case VIRTIO_SND_R_##CODE: \
        if (req_size >= sizeof(struct TYPE)) { \
            return HANDLER(e, (struct TYPE*)hdr, snd); \
        } else { \
            goto incomplete_request; \
        }
    HANDLE_CTL(JACK_INFO, virtio_snd_query_info, virtio_snd_process_ctl_jack_info)
    HANDLE_CTL(JACK_REMAP, virtio_snd_jack_remap, virtio_snd_process_ctl_jack_remap)
    HANDLE_CTL(PCM_INFO, virtio_snd_query_info, virtio_snd_process_ctl_pcm_info)
    HANDLE_CTL(PCM_SET_PARAMS, virtio_snd_pcm_set_params, virtio_snd_process_ctl_pcm_set_params)
    HANDLE_CTL(PCM_PREPARE, virtio_snd_pcm_hdr, virtio_snd_process_ctl_pcm_prepare)
    HANDLE_CTL(PCM_RELEASE, virtio_snd_pcm_hdr, virtio_snd_process_ctl_pcm_release)
    HANDLE_CTL(PCM_START, virtio_snd_pcm_hdr, virtio_snd_process_ctl_pcm_start)
    HANDLE_CTL(PCM_STOP, virtio_snd_pcm_hdr, virtio_snd_process_ctl_pcm_stop)
    HANDLE_CTL(CHMAP_INFO, virtio_snd_query_info, virtio_snd_process_ctl_chmap_info)
#undef HANDLE_CTL
    default:
        return el_send_status(e, FAILURE(VIRTIO_SND_S_NOT_SUPP));
    }

incomplete_request:
    return el_send_status(e, FAILURE(VIRTIO_SND_S_BAD_MSG));
}

/* device <- driver */
static void virtio_snd_handle_ctl(VirtIODevice *vdev, VirtQueue *vq) {
    VirtIOSound *snd = VIRTIO_SND(vdev);
    bool need_notify = false;

    while (true) {
        VirtQueueElement *e = (VirtQueueElement *)virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (e) {
            vq_consume_element(vq, e, virtio_snd_process_ctl(e, snd));
            need_notify = true;
        } else {
            break;
        }
    }

    if (need_notify) {
        virtio_notify(vdev, vq);
    }
}

/* device -> driver */
static void virtio_snd_handle_event(VirtIODevice *vdev, VirtQueue *vq) {
    /* nothing so far */
}

/* device <- driver */
static void virtio_snd_handle_tx(VirtIODevice *vdev, VirtQueue *vq) {
    VirtIOSound *snd = VIRTIO_SND(vdev);
    bool need_notify = false;

    while (true) {
        VirtQueueElement *e = (VirtQueueElement *)virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (e) {
            need_notify = virtio_snd_process_tx(vq, e, snd) || need_notify;
        } else {
            break;
        }
    }

    if (need_notify) {
        virtio_notify(vdev, vq);
    }
}

/* device -> driver */
static void virtio_snd_handle_rx(VirtIODevice *vdev, VirtQueue *vq) {
    VirtIOSound *snd = VIRTIO_SND(vdev);
    bool need_notify = false;

    while (true) {
        VirtQueueElement *e = (VirtQueueElement *)virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (e) {
            need_notify = virtio_snd_process_rx(vq, e, snd) || need_notify;
        } else {
            break;
        }
    }

    if (need_notify) {
        virtio_notify(vdev, vq);
    }
}

#ifdef __linux__
static void linux_mic_workaround_in_cb(void *unused1, int unused2) {}
#endif  // __linux__

static void virtio_snd_device_realize(DeviceState *dev, Error **errp) {
    VirtIOSound *snd = VIRTIO_SND(dev);
    DPRINTF("snd=%p", snd);
    VirtIODevice *vdev = &snd->parent;
    int i;

    AUD_register_card(TYPE_VIRTIO_SND, &snd->card);
#ifdef __linux__
    if (snd->enable_input_prop) {
        struct audsettings as = {
            .freq = 8000,
            .nchannels = 1,
            .fmt = AUD_FMT_S16,
            .endianness = AUDIO_HOST_ENDIANNESS,
        };

        // It does not make sense, but on linux we have to open the microphone
        // very early to make it working. On other platform we open it only when
        // the guest asks for it, see b/292115117.
        // Probably will not be required once we upgrade QEMU.
        snd->linux_mic_workaround =
            AUD_open_in(&snd->card, NULL, "linux_mic_workaround", NULL,
                        &linux_mic_workaround_in_cb, &as);
    }
#endif  // __linux__

    virtio_init(vdev, TYPE_VIRTIO_SND, VIRTIO_ID_SOUND,
                sizeof(struct virtio_snd_config));
    snd->ctl_vq = virtio_add_queue(vdev, 4 * MAX(VIRTIO_SND_NUM_PCM_TX_STREAMS,
                                                 VIRTIO_SND_NUM_PCM_RX_STREAMS),
                                   virtio_snd_handle_ctl);
    snd->event_vq = virtio_add_queue(vdev, 2, virtio_snd_handle_event);  // not implemented
    snd->tx_vq = virtio_add_queue(vdev, 4 * VIRTIO_SND_NUM_PCM_TX_STREAMS,
                                  virtio_snd_handle_tx);
    snd->rx_vq = virtio_add_queue(vdev, 4 * VIRTIO_SND_NUM_PCM_RX_STREAMS,
                                  virtio_snd_handle_rx);

    for (i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        virtio_snd_stream_init(i, &snd->streams[i], snd);
    }

    audio_forwarder_register_card(&virtio_snd_set_voice_in, &virtio_snd_get_voice_in);
}

static void virtio_snd_device_unrealize(DeviceState *dev, Error **errp) {
    VirtIOSound *snd = VIRTIO_SND(dev);
    DPRINTF("snd=%p", snd);
    VirtIODevice *vdev = &snd->parent;
    unsigned i;

    audio_forwarder_register_card(NULL, NULL);

    for (i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        virtio_snd_stream_disable(&snd->streams[i]);
    }

    virtio_del_queue(vdev, VIRTIO_SND_QUEUE_RX);
    virtio_del_queue(vdev, VIRTIO_SND_QUEUE_TX);
    virtio_del_queue(vdev, VIRTIO_SND_QUEUE_EVENT);
    virtio_del_queue(vdev, VIRTIO_SND_QUEUE_CTL);
    virtio_cleanup(vdev);

#ifdef __linux__
    if (snd->linux_mic_workaround) {
        AUD_close_in(&snd->card, snd->linux_mic_workaround);
    }
#endif  // __linux__
    AUD_remove_card(&snd->card);
}

static uint64_t virtio_snd_device_get_features(VirtIODevice *vdev,
                                               uint64_t features,
                                               Error **errp) {
    return features | (1ULL << VIRTIO_F_VERSION_1);
}

static void virtio_snd_get_config(VirtIODevice *vdev, uint8_t *raw) {
    struct virtio_snd_config *config = (struct virtio_snd_config *)raw;

    config->jacks = VIRTIO_SND_NUM_JACKS;
    config->streams = VIRTIO_SND_NUM_PCM_STREAMS;
    config->chmaps = VIRTIO_SND_NUM_CHMAPS;
}

static void virtio_snd_set_status(VirtIODevice *vdev, const uint8_t status) {
    VirtIOSound *snd = VIRTIO_SND(vdev);

    if (status & VIRTIO_CONFIG_S_FAILED) {
        DPRINTF("%s", "VIRTIO_CONFIG_S_FAILED");
    }

    for (unsigned i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        VirtIOSoundPCMStream *stream = &snd->streams[i];

        if (status & VIRTIO_CONFIG_S_DRIVER_OK) {
            virtio_snd_stream_enable(stream);
        } else {
            virtio_snd_stream_disable(stream);
        }
    }
}

static const Property virtio_snd_properties[] = {
    DEFINE_PROP_BOOL("input", VirtIOSound, enable_input_prop, true),
    DEFINE_PROP_BOOL("output", VirtIOSound, enable_output_prop, true),
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription virtio_snd_vmstate = {
    .name = "virtio-snd",
    .minimum_version_id = 0,
    .version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_VIRTIO_DEVICE,
        VMSTATE_BOOL(enable_input_prop, VirtIOSound),
        VMSTATE_BOOL(enable_output_prop, VirtIOSound),
        VMSTATE_END_OF_LIST()
    },
};

static void virtio_snd_save_stream(QEMUFile *f, const VirtIOSound *snd,
                                   const VirtIOSoundPCMStream *stream) {
    DPRINTF("stream(%p)={state=%u, guest_format=0x%04X, period_frames=%u "
            "n_periods=%u, gpcm_buf={capacity=%u, size=%u}, "
            "hpcm_buf={capacity=%u, size=%u}}",
            stream, stream->state, stream->guest_format, stream->period_frames,
            stream->n_periods, stream->gpcm_buf.capacity, stream->gpcm_buf.size,
            stream->hpcm_buf.capacity, stream->hpcm_buf.size);

    qemu_put_byte(f, stream->state);
    if (stream->state >= VIRTIO_PCM_STREAM_STATE_PARAMS_SET) {
        qemu_put_be16(f, stream->guest_format);
        qemu_put_be16(f, stream->period_frames);
        qemu_put_byte(f, stream->n_periods);
        if (stream->state >= VIRTIO_PCM_STREAM_STATE_PREPARED) {
            vq_ring_buffer_save(&stream->gpcm_buf, f);
            pcm_ring_buffer_save(&stream->hpcm_buf, f, stream->host_frame_size);
        }
    }
}

static int virtio_snd_load_stream(QEMUFile *f, VirtIOSound *snd,
                                  VirtIOSoundPCMStream *stream) {
    stream->state = qemu_get_byte(f);
    DPRINTF("stream=%p state=%u", stream, stream->state);

    if (stream->state > VIRTIO_PCM_STREAM_STATE_RUNNING) {
        return FAILURE(-EINVAL);
    }

    if (stream->state >= VIRTIO_PCM_STREAM_STATE_PARAMS_SET) {
        stream->guest_format = qemu_get_be16(f);
        if (stream->guest_format == VIRTIO_SND_FORMAT16_BAD) {
            return FAILURE(-EINVAL);
        }
        stream->period_frames = qemu_get_be16(f);
        if (!stream->period_frames) {
            return FAILURE(-EINVAL);
        }
        stream->n_periods = qemu_get_byte(f);
        if (!stream->n_periods) {
            return FAILURE(-EINVAL);
        }

        DPRINTF("stream=%p guest_format=%04X period_frames=%u n_periods=%u",
                stream, stream->guest_format, stream->period_frames, stream->n_periods);

        if (stream->state >= VIRTIO_PCM_STREAM_STATE_PREPARED) {
            int r;
            r = vq_ring_buffer_load(&stream->gpcm_buf, &snd->parent, f, stream->n_periods);
            if (r) { return r; }
            r = pcm_ring_buffer_load(&stream->hpcm_buf, f);
            if (r) { return r; }
        }
    }

    return 0;
}

static int virtio_snd_post_load_stream(VirtIOSound *snd,
                                       VirtIOSoundPCMStream *stream) {
    if (stream->state < VIRTIO_PCM_STREAM_STATE_PREPARED) {
        return 0;
    }

    if (!virtio_snd_stream_prepare0_locked(stream)) {
        return -EINVAL;
    }

    int r = pcm_ring_buffer_post_load(&stream->hpcm_buf,
                                      virtio_snd_stream_get_hpcm_capacity(stream),
                                      stream->host_frame_size);
    if (r) { return r; }

    if (stream->state < VIRTIO_PCM_STREAM_STATE_RUNNING) {
        return 0;
    }

    virtio_snd_stream_start_locked(stream);

    return 0;
}

static void virtio_snd_pre_load(VirtIOSound *snd) {
    DPRINTF("snd=%p", snd);

    for (unsigned i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        virtio_snd_stream_disable(&snd->streams[i]);
    }
}

static int virtio_snd_load_impl_v1(VirtIOSound *snd, QEMUFile *f) {
    for (unsigned i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        int r = virtio_snd_load_stream(f, snd, &snd->streams[i]);
        if (r) { return r; }
    }

    return 0;
}

static int virtio_snd_load_impl(VirtIOSound *snd, QEMUFile *f) {
    const unsigned snapshot_version = qemu_get_be16(f);
    DPRINTF("snd=%p snapshot_version=%u", snd, snapshot_version);
    switch (snapshot_version) {
    case 1:
        return virtio_snd_load_impl_v1(snd, f);

    default:
        return FAILURE(-EINVAL);
    }
}

static int virtio_snd_post_load(VirtIOSound *snd) {
    for (unsigned i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        int r = virtio_snd_post_load_stream(snd, &snd->streams[i]);
        if (r) { return r; }
    }

    return 0;
}

static void virtio_snd_save(VirtIODevice *vdev, QEMUFile *f) {
    VirtIOSound *snd = VIRTIO_SND(vdev);
    DPRINTF("snd=%p", snd);

    qemu_put_be16(f, VIRTIO_SND_SNAPSHOT_VERSION);
    for (unsigned i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        virtio_snd_save_stream(f, snd, &snd->streams[i]);
    }
}

static int virtio_snd_load(VirtIODevice *vdev, QEMUFile *f, int version_id) {
    VirtIOSound *snd = VIRTIO_SND(vdev);
    DPRINTF("snd=%p version_id=%d", snd, version_id);

    virtio_snd_pre_load(snd);
    int r = virtio_snd_load_impl(snd, f);
    return r ? r : virtio_snd_post_load(snd);
}

static void virtio_snd_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->props = (Property*)virtio_snd_properties;
    dc->vmsd = &virtio_snd_vmstate;
    set_bit(DEVICE_CATEGORY_SOUND, dc->categories);

    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    vdc->realize = &virtio_snd_device_realize;
    vdc->unrealize = &virtio_snd_device_unrealize;
    vdc->get_features = &virtio_snd_device_get_features;
    vdc->get_config = &virtio_snd_get_config;
    vdc->set_status = &virtio_snd_set_status;
    vdc->save = &virtio_snd_save;
    vdc->load = &virtio_snd_load;
}

static int virtio_snd_and_codec_init(PCIBus *bus, const char *props) {
    DeviceState *dev = DEVICE(pci_create(bus, -1, TYPE_VIRTIO_SND_PCI));
    Error *errp = NULL;
    object_properties_parse(OBJECT(dev), props, &errp);
    if (errp) {
        error_free(errp);
        abort();
    }
    qdev_init_nofail(dev);
    return 0;
}

static const TypeInfo virtio_snd_typeinfo = {
    .name = TYPE_VIRTIO_SND,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOSound),
    .class_init = &virtio_snd_class_init,
};

static void virtio_snd_register_types(void) {
    type_register_static(&virtio_snd_typeinfo);
    pci_register_soundhw(TYPE_VIRTIO_SND_PCI,
                         "Virtio sound",
                         &virtio_snd_and_codec_init);
}

type_init(virtio_snd_register_types);
