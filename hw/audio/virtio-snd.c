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
    "virtio-snd-mic",
    "virtio-snd-speakers",
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

    room_size = MIN(room_size, rb->capacity);

    const int capacity = rb->capacity;
    const int to_drop = room_size - (capacity - rb->size);
    if (to_drop > 0) {
        ASSERT(to_drop <= rb->size);
        rb->r = (rb->r + to_drop) % rb->capacity;
        rb->size -= to_drop;
    }

    const int left = capacity - rb->size;
    const int r = rb->r;
    const int w = rb->w;

    if (w >= r) {
        *chunk_sz = MIN(room_size, MIN(rb->capacity - w, left));
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

static void stream_out_cb(void *opaque, int avail);
static void stream_in_cb(void *opaque, int avail);

static VirtIOSoundPCMStream *g_input_stream = NULL;

static SWVoiceIn *virtio_snd_set_voice_in(SWVoiceIn *voice) {
    if (g_input_stream) {
        struct audsettings as = virtio_snd_unpack_format(g_input_stream->aud_format);

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
    const uint16_t aud_format = VIRTIO_SND_PACK_FORMAT16(nchannels, fmt, freq);

    stream->aud_format = aud_format;
    stream->aud_frame_size = format16_get_frame_size(aud_format);
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
    stream->aud_format = VIRTIO_SND_FORMAT16_BAD;
}

static void virtio_snd_stream_init(const unsigned stream_id,
                                   VirtIOSoundPCMStream *stream,
                                   VirtIOSound *snd) {
    memset(stream, 0, sizeof(*stream));

    stream->id = stream_id;
    stream->snd = snd;
    stream->voice.raw = NULL;
    stream->aud_format = VIRTIO_SND_FORMAT16_BAD;
    vq_ring_buffer_init(&stream->kpcm_buf);
    pcm_ring_buffer_init(&stream->qpcm_buf);
    qemu_mutex_init(&stream->mtx);
    stream->state = VIRTIO_PCM_STREAM_STATE_DISABLED;
}

static bool virtio_snd_stream_kpcm_to_qpcm_locked(VirtIOSoundPCMStream *stream);
static void virtio_snd_stream_output_irq(void* opaque);
static void virtio_snd_stream_input_irq(void* opaque);

static void virtio_snd_stream_start_locked(VirtIOSoundPCMStream *stream) {
    if (is_output_stream(stream)) {
        timer_init_us(&stream->vq_irq_timer, QEMU_CLOCK_VIRTUAL_RT,
                      &virtio_snd_stream_output_irq, stream);
        virtio_snd_stream_kpcm_to_qpcm_locked(stream);  // prefill
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

static bool virtio_snd_stream_prepare_locked(VirtIOSoundPCMStream *stream) {
    ASSERT(stream->buffer_bytes > 0);
    ASSERT(stream->period_bytes > 0);

    const uint16_t guest_format = stream->guest_format;
    ASSERT(guest_format != VIRTIO_SND_FORMAT16_BAD);

    const ldiv_t n_periods = ldiv(stream->buffer_bytes, stream->period_bytes);
    if ((n_periods.rem != 0) || (n_periods.quot <= 0)) {
        return FAILURE(false);
    }

    if (!virtio_snd_voice_open(stream, guest_format)) {
        return FAILURE(false);
    }

    const int freq_hz = format16_get_freq_hz(guest_format);
    ASSERT(freq_hz > 0);
    stream->driver_frame_size = format16_get_frame_size(guest_format);
    ASSERT(stream->driver_frame_size > 0);
    stream->n_periods = n_periods.quot;
    const int period_frames = stream->period_bytes / stream->driver_frame_size;
    ASSERT(period_frames > 0);
    stream->period_frames = period_frames;
    stream->period_us = 1000000LL * period_frames / freq_hz;

    if (!vq_ring_buffer_alloc(&stream->kpcm_buf, n_periods.quot)) {
        virtio_snd_voice_close(stream);
        return FAILURE(false);
    }

    ASSERT(stream->n_periods > 0);
    ASSERT(stream->aud_frame_size > 0);

    if (!pcm_ring_buffer_alloc(&stream->qpcm_buf,
                               (stream->n_periods + 1) * period_frames
                                   * stream->aud_frame_size)) {
        virtio_snd_voice_close(stream);
        vq_ring_buffer_free(&stream->kpcm_buf);
        return FAILURE(false);
    }

    return true;
}

static void virtio_snd_stream_flush_vq_locked(VirtIOSoundPCMStream *stream,
                                              VirtQueue *vq) {
    const int kernel_latency_bytes =
        stream->qpcm_buf.size / stream->aud_frame_size * stream->driver_frame_size;
    VirtIOSoundVqRingBuffer *kpcm_buf = &stream->kpcm_buf;

    while (true) {
        VirtIOSoundVqRingBufferItem *item = vq_ring_buffer_top(kpcm_buf);
        if (item) {
            VirtQueueElement *e = item->el;

            vq_consume_element(
                vq, e,
                el_send_pcm_status(e, 0, VIRTIO_SND_S_OK, kernel_latency_bytes));
            vq_ring_buffer_pop(kpcm_buf);
        } else {
            break;
        }
    }

    virtio_notify(&stream->snd->parent, vq);
}

static void virtio_snd_stream_unprepare_locked(VirtIOSoundPCMStream *stream) {
    virtio_snd_stream_flush_vq_locked(stream, is_output_stream(stream) ?
                                      stream->snd->tx_vq : stream->snd->rx_vq);
    virtio_snd_voice_close(stream);
    vq_ring_buffer_free(&stream->kpcm_buf);
    pcm_ring_buffer_free(&stream->qpcm_buf);
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

    if (!req->buffer_bytes) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    if (!req->period_bytes) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    VirtIOSoundPCMStream *stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);
    switch (stream->state) {
    case VIRTIO_PCM_STREAM_STATE_PREPARED:
        virtio_snd_stream_unprepare_locked(stream);
        /* fallthrough */

    case VIRTIO_PCM_STREAM_STATE_PARAMS_SET:
    case VIRTIO_PCM_STREAM_STATE_ENABLED:
        stream->buffer_bytes = req->buffer_bytes;
        stream->period_bytes = req->period_bytes;
        stream->guest_format = VIRTIO_SND_PACK_FORMAT16(req->channels, req->format, req->rate);
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

static bool virtio_snd_stream_kpcm_to_qpcm_locked(VirtIOSoundPCMStream *stream) {
    const int aud_fs = stream->aud_frame_size;
    ASSERT(aud_fs > 0);
    const int period_frames = stream->period_frames;
    ASSERT(period_frames > 0);

    if (stream->qpcm_buf.size >= (stream->n_periods * period_frames * aud_fs)) {
        return false;
    }

    VirtIOSoundVqRingBufferItem *item = vq_ring_buffer_top(&stream->kpcm_buf);
    if (!item) {
        return FAILURE(false);
    }

    const int driver_fs = stream->driver_frame_size;
    ASSERT(driver_fs > 0);
    int16_t scratch[AUD_SCRATCH_SIZE];
    const int max_dst_chunk_size = sizeof(scratch) / driver_fs * aud_fs;

    VirtQueueElement *const e = item->el;
    ASSERT(e != NULL);

    int pos = sizeof(struct virtio_snd_pcm_xfer);
    int qsize = period_frames * aud_fs;
    ASSERT(qsize > 0);
    while (qsize > 0) {
        int dst_sz;
        void *dst = pcm_ring_buffer_get_produce_chunk(&stream->qpcm_buf, qsize, &dst_sz);
        ASSERT(dst != NULL);
        dst_sz = MIN(dst_sz, max_dst_chunk_size);
        ASSERT(dst_sz > 0);
        const int src_sz = dst_sz / aud_fs * driver_fs;
        ASSERT(src_sz > 0);

        iov_to_buf(e->out_sg, e->out_num, pos, scratch, src_sz);
        convert_channels(scratch, src_sz, driver_fs, aud_fs, (int16_t *)dst);
        pcm_ring_buffer_produce(&stream->qpcm_buf, dst_sz);
        pos += src_sz;
        qsize -= dst_sz;
    }

    vq_consume_element(
        stream->snd->tx_vq, e,
        el_send_pcm_status(e, 0, VIRTIO_SND_S_OK,
                           stream->qpcm_buf.size / aud_fs * driver_fs));
    vq_ring_buffer_pop(&stream->kpcm_buf);
    return true;
}

static void stream_out_cb_locked(VirtIOSoundPCMStream *stream, int avail) {
    ASSERT(avail > 0);
    SWVoiceOut *const voice = stream->voice.out;
    ASSERT(voice != NULL);
    const int aud_fs = stream->aud_frame_size;
    ASSERT(aud_fs > 0);
    ASSERT((avail % aud_fs) == 0);
    ASSERT(stream->period_frames > 0);
    int min_write_sz = MIN(avail, MAX(stream->period_frames / 16, 4) * aud_fs);

    while (avail > 0) {
        int size;
        void *data = pcm_ring_buffer_get_consume_chunk(&stream->qpcm_buf, &size);
        if (size > 0) {
            ASSERT((size % aud_fs) == 0);
            const int to_write_sz = MIN(size, avail);
            const int written_sz = AUD_write(voice, data, to_write_sz);
            if (written_sz > 0) {
                ASSERT((written_sz % aud_fs) == 0);
                pcm_ring_buffer_consume(&stream->qpcm_buf, written_sz);
                avail -= written_sz;
                min_write_sz -= written_sz;
            } else {
                return;
            }
        } else {
            ASSERT(stream->qpcm_buf.size == 0);
            if (min_write_sz > 0) {
                int16_t scratch[AUD_SCRATCH_SIZE];

                // Insert `min_write_sz` bytes of silence.
                fill_silence(scratch, MIN(sizeof(scratch), min_write_sz));
                do {
                    const int to_write_sz =
                        MIN(sizeof(scratch), min_write_sz) / aud_fs * aud_fs;
                    const int written_sz = AUD_write(voice, scratch, to_write_sz);
                    if (written_sz > 0) {
                        ASSERT((written_sz % aud_fs) == 0);
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
        stream->next_vq_irq_us +=
            virtio_snd_stream_kpcm_to_qpcm_locked(stream) ?
                stream->period_us : (stream->period_us / 4);
        timer_mod(&stream->vq_irq_timer, stream->next_vq_irq_us);
        virtio_notify(&stream->snd->parent, stream->snd->tx_vq);
    }
    qemu_mutex_unlock(&stream->mtx);
}

static void stream_out_cb(void *opaque, int avail) {
    VirtIOSoundPCMStream *const stream = (VirtIOSoundPCMStream *)opaque;

    qemu_mutex_lock(&stream->mtx);
    stream_out_cb_locked(stream, avail);
    qemu_mutex_unlock(&stream->mtx);
}

static void virtio_snd_process_tx(VirtQueue *vq, VirtQueueElement *e, VirtIOSound *snd) {
    const size_t req_size = iov_size(e->out_sg, e->out_num);
    struct virtio_snd_pcm_xfer xfer;

    if (req_size < sizeof(xfer)) {
        vq_consume_element(vq, e, 0);
        return;
    }

    iov_to_buf(e->out_sg, e->out_num, 0, &xfer, sizeof(xfer));
    if (xfer.stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        vq_consume_element(vq, e, el_send_pcm_status(e, 0, VIRTIO_SND_S_BAD_MSG, 0));
        return;
    }

    VirtIOSoundPCMStream *stream = &snd->streams[xfer.stream_id];
    VirtIOSoundVqRingBufferItem item;
    item.el = e;

    qemu_mutex_lock(&stream->mtx);
    if (!vq_ring_buffer_push(&stream->kpcm_buf, &item)) {
        ABORT("vq_ring_buffer_push");
    }
    qemu_mutex_unlock(&stream->mtx);
}

static bool virtio_snd_stream_qpcm_to_kpcm_locked(VirtIOSoundPCMStream *stream) {
    VirtIOSoundVqRingBufferItem *item = vq_ring_buffer_top(&stream->kpcm_buf);
    if (!item) {
        return FAILURE(false);
    }

    const int aud_fs = stream->aud_frame_size;
    ASSERT(aud_fs > 0);
    const int driver_fs = stream->driver_frame_size;
    ASSERT(driver_fs > 0);
    int16_t scratch[AUD_SCRATCH_SIZE];
    const int max_src_chunk_size =
        MIN(stream->period_frames, sizeof(scratch) / driver_fs) * aud_fs;
    ASSERT(max_src_chunk_size >= aud_fs);

    VirtQueueElement *e = item->el;
    ASSERT(e != NULL);

    int pos = 0;
    int ksize = stream->period_bytes;
    ASSERT(ksize > 0);
    while (ksize > 0) {
        int src_size;
        const void *src = pcm_ring_buffer_get_consume_chunk(&stream->qpcm_buf, &src_size);
        if (src_size > 0) {
            ASSERT((src_size % aud_fs) == 0);
            src_size = MIN(MIN(src_size, max_src_chunk_size),
                           ksize / driver_fs * aud_fs);
            ASSERT(src_size > 0);
            const int dst_size = convert_channels(src, src_size,
                                                  aud_fs, driver_fs, scratch);
            ASSERT(dst_size > 0);
            ASSERT((dst_size % driver_fs) == 0);
            iov_from_buf(e->in_sg, e->in_num, pos, scratch, dst_size);
            pcm_ring_buffer_consume(&stream->qpcm_buf, src_size);
            pos += dst_size;
            ksize -= dst_size;
        } else {
            ASSERT(stream->qpcm_buf.size == 0);
            ASSERT(ksize > 0);

            // Insert `ksize` bytes of silence.
            fill_silence(scratch, MIN(sizeof(scratch), ksize));
            do {
                const int dst_size = MIN(sizeof(scratch), ksize);
                iov_from_buf(e->in_sg, e->in_num, pos, scratch, dst_size);
                pos += dst_size;
                ksize -= dst_size;
            } while (ksize > 0);

            break;
        }
    }

    ASSERT(pos == stream->period_bytes);

    vq_consume_element(
        stream->snd->rx_vq, e,
        el_send_pcm_status(e, pos, VIRTIO_SND_S_OK,
                           stream->qpcm_buf.size / aud_fs * driver_fs));
    vq_ring_buffer_pop(&stream->kpcm_buf);
    return true;
}

static void stream_in_cb_locked(VirtIOSoundPCMStream *stream, int avail) {
    ASSERT(avail > 0);
    const int aud_fs = stream->aud_frame_size;
    ASSERT(aud_fs > 0);
    ASSERT((avail % aud_fs) == 0);
    ASSERT(stream->qpcm_buf.capacity > 0);
    int16_t scratch[AUD_SCRATCH_SIZE];
    const int max_read_size = MIN(sizeof(scratch) / aud_fs * aud_fs,
                                  stream->qpcm_buf.capacity);

    SWVoiceIn *const voice = stream->voice.in;
    ASSERT(voice != NULL);

    while (avail > 0) {
        int src_sz = AUD_read(voice, scratch, MIN(avail, max_read_size));
        if (src_sz > 0) {
            avail -= src_sz;
            ASSERT((src_sz % aud_fs) == 0);
            const uint8_t* src = (const uint8_t*)(&scratch[0]);

            while (src_sz > 0) {
                int dst_sz;
                void *dst = pcm_ring_buffer_get_produce_chunk(&stream->qpcm_buf,
                                                              src_sz, &dst_sz);
                dst_sz = MIN(dst_sz, src_sz);
                memcpy(dst, src, dst_sz);
                pcm_ring_buffer_produce(&stream->qpcm_buf, dst_sz);
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
        stream->next_vq_irq_us +=
            virtio_snd_stream_qpcm_to_kpcm_locked(stream) ?
                stream->period_us : (stream->period_us / 4);
        timer_mod(&stream->vq_irq_timer, stream->next_vq_irq_us);
        virtio_notify(&stream->snd->parent, stream->snd->rx_vq);
    }
    qemu_mutex_unlock(&stream->mtx);
}

static void stream_in_cb(void *opaque, int avail) {
    VirtIOSoundPCMStream *const stream = (VirtIOSoundPCMStream *)opaque;

    qemu_mutex_lock(&stream->mtx);
    stream_in_cb_locked(stream, avail);
    qemu_mutex_unlock(&stream->mtx);
}

static void virtio_snd_process_rx(VirtQueue *vq, VirtQueueElement *e, VirtIOSound *snd) {
    const size_t req_size = iov_size(e->out_sg, e->out_num);
    struct virtio_snd_pcm_xfer xfer;

    if (req_size < sizeof(xfer)) {
        vq_consume_element(vq, e, 0);
        return;
    }

    iov_to_buf(e->out_sg, e->out_num, 0, &xfer, sizeof(xfer));
    if (xfer.stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        vq_consume_element(vq, e, el_send_pcm_status(e, 0, VIRTIO_SND_S_BAD_MSG, 0));
        return;
    }

    VirtIOSoundPCMStream *stream = &snd->streams[xfer.stream_id];
    VirtIOSoundVqRingBufferItem item;
    item.el = e;

    qemu_mutex_lock(&stream->mtx);
    if (!vq_ring_buffer_push(&stream->kpcm_buf, &item)) {
        ABORT("ring_buffer_push");
    }
    qemu_mutex_unlock(&stream->mtx);
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
            r = FAILURE(VIRTIO_SND_S_BAD_MSG);
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

    while (true) {
        VirtQueueElement *e = (VirtQueueElement *)virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (e) {
            vq_consume_element(vq, e, virtio_snd_process_ctl(e, snd));
        } else {
            break;
        }
    }

    virtio_notify(vdev, vq);
}

/* device -> driver */
static void virtio_snd_handle_event(VirtIODevice *vdev, VirtQueue *vq) {
    /* nothing so far */
}

/* device <- driver */
static void virtio_snd_handle_tx(VirtIODevice *vdev, VirtQueue *vq) {
    VirtIOSound *snd = VIRTIO_SND(vdev);

    while (true) {
        VirtQueueElement *e = (VirtQueueElement *)virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (e) {
            virtio_snd_process_tx(vq, e, snd);
        } else {
            break;
        }
    }

    virtio_notify(vdev, vq);
}

/* device -> driver */
static void virtio_snd_handle_rx(VirtIODevice *vdev, VirtQueue *vq) {
    VirtIOSound *snd = VIRTIO_SND(vdev);

    while (true) {
        VirtQueueElement *e = (VirtQueueElement *)virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (e) {
            virtio_snd_process_rx(vq, e, snd);
        } else {
            break;
        }
    }

    virtio_notify(vdev, vq);
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
    snd->ctl_vq = virtio_add_queue(vdev, VIRTIO_SND_NUM_PCM_STREAMS * 4,
                                   virtio_snd_handle_ctl);
    snd->event_vq = virtio_add_queue(vdev, VIRTIO_SND_NUM_PCM_STREAMS * 2,
                                   virtio_snd_handle_event);
    snd->tx_vq = virtio_add_queue(vdev, VIRTIO_SND_NUM_PCM_TX_STREAMS * 8,
                                  virtio_snd_handle_tx);
    snd->rx_vq = virtio_add_queue(vdev, VIRTIO_SND_NUM_PCM_RX_STREAMS * 8,
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
    return features;
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

static int vmstate_VirtIOSound_pre_xyz(void *opaque) {
    VirtIOSound *snd = (VirtIOSound *)opaque;
    unsigned i;

    for (i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        VirtIOSoundPCMStream *stream = &snd->streams[i];

        switch (stream->state) {
        case VIRTIO_PCM_STREAM_STATE_RUNNING:
            virtio_snd_stream_stop_locked(stream);
            /* fallthrough */

        case VIRTIO_PCM_STREAM_STATE_PREPARED:
            virtio_snd_stream_unprepare_locked(stream);
            /* fallthrough */

        case VIRTIO_PCM_STREAM_STATE_PARAMS_SET:
        case VIRTIO_PCM_STREAM_STATE_ENABLED:
        case VIRTIO_PCM_STREAM_STATE_DISABLED:
            break;

        default:
            ABORT("stream->state");
        }
    }

    return 0;
}

static int vmstate_VirtIOSound_post_xyz(void *opaque) {
    VirtIOSound *snd = (VirtIOSound *)opaque;
    unsigned i;

    for (i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        VirtIOSoundPCMStream *stream = &snd->streams[i];

        if (stream->state >= VIRTIO_PCM_STREAM_STATE_PREPARED) {
            if (!virtio_snd_stream_prepare_locked(stream)) {
                ABORT("virtio_snd_stream_prepare_locked");
            }
        }
    }

    virtio_snd_handle_tx(&snd->parent, snd->tx_vq);
    virtio_snd_handle_rx(&snd->parent, snd->rx_vq);

    for (i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        VirtIOSoundPCMStream *stream = &snd->streams[i];

        if (stream->state > VIRTIO_PCM_STREAM_STATE_RUNNING) {
            ABORT("stream->state");
        } else if (stream->state == VIRTIO_PCM_STREAM_STATE_RUNNING) {
            virtio_snd_stream_start_locked(stream);
        }
    }

    return 0;
}

static int vmstate_VirtIOSound_post_xyz_ver(void *opaque, int version) {
    return vmstate_VirtIOSound_post_xyz(opaque);
}

static const VMStateDescription vmstate_VirtIOSoundPCMStream = {
    .name = "VirtIOSoundPCMStream",
    .minimum_version_id = 0,
    .version_id = 0,
    .fields = (VMStateField[]) {
        /* The `pcm_buf` field in not saved. The `buf` elements must be
         * placed back (in `pre_save`) to the vq before saving
         * `VirtIOSoundVqRingBuffer` and restored after loading (in `post_load`).
         */
        VMSTATE_UINT32(buffer_bytes, VirtIOSoundPCMStream),
        VMSTATE_UINT32(period_bytes, VirtIOSoundPCMStream),
        VMSTATE_UINT16(guest_format, VirtIOSoundPCMStream),
        VMSTATE_UINT8(state, VirtIOSoundPCMStream),
        VMSTATE_END_OF_LIST()
    },
};

static const VMStateDescription vmstate_VirtIOSound = {
    .name = TYPE_VIRTIO_SND,
    .minimum_version_id = 0,
    .version_id = 0,
    .pre_load = vmstate_VirtIOSound_pre_xyz,
    .post_load = vmstate_VirtIOSound_post_xyz_ver,
    .pre_save = vmstate_VirtIOSound_pre_xyz,
    .post_save = vmstate_VirtIOSound_post_xyz,
    .fields = (VMStateField[]) {
        VMSTATE_VIRTIO_DEVICE,
        VMSTATE_STRUCT_ARRAY(streams, VirtIOSound, VIRTIO_SND_NUM_PCM_STREAMS, 0,
                             vmstate_VirtIOSoundPCMStream, VirtIOSoundPCMStream),
        VMSTATE_END_OF_LIST()
    },
};

static void virtio_snd_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->props = (Property*)virtio_snd_properties;
    set_bit(DEVICE_CATEGORY_SOUND, dc->categories);
    dc->vmsd = &vmstate_VirtIOSound;

    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    vdc->realize = &virtio_snd_device_realize;
    vdc->unrealize = &virtio_snd_device_unrealize;
    vdc->get_features = &virtio_snd_device_get_features;
    vdc->get_config = &virtio_snd_get_config;
    vdc->set_status = &virtio_snd_set_status;
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
