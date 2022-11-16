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

#define AUD_SCRATCH_SIZE                            2048

#define GLUE(A, B) A##B
#define GLUE2(A, B) GLUE(A, B)
#define ABORT(why) abort();
#define FAILURE(X) (X)

union VirtIOSoundCtlRequest {
    struct virtio_snd_hdr hdr;
    struct virtio_snd_query_info r1;
    struct virtio_snd_jack_remap r2;
    struct virtio_snd_pcm_set_params r3;
    struct virtio_snd_pcm_hdr r4;
};

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

static unsigned format16_get_freq_hz(uint16_t format16) {
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

static size_t el_send_pcm_status(VirtQueueElement *e, unsigned status, unsigned latency_bytes) {
    const struct virtio_snd_pcm_status data = {
        .status = status,
        .latency_bytes = latency_bytes,
    };
    return el_send_data(e, &data, sizeof(data));
}

static unsigned el_send_pcm_rx_status(VirtQueueElement *e, unsigned period_bytes,
                                      unsigned status, unsigned latency_bytes) {
    const struct virtio_snd_pcm_status data = {
        .status = status,
        .latency_bytes = latency_bytes,
    };

    iov_from_buf(e->in_sg, e->in_num, period_bytes, &data, sizeof(data));
    return period_bytes + sizeof(data);
}

static void ring_buffer_init(VirtIOSoundRingBuffer *rb) {
    rb->buf = NULL;
    rb->capacity = 0;
    rb->size = RING_BUFFER_BAD_SIZE;
    rb->r = RING_BUFFER_BAD_R;
    rb->w = RING_BUFFER_BAD_W;
};

static bool ring_buffer_alloc(VirtIOSoundRingBuffer *rb, const size_t capacity) {
    if (rb->buf) {
        ABORT("rb->buf");
    }

    if (rb->capacity) {
        ABORT("rb->capacity");
    }

    rb->buf = g_new0(VirtIOSoundRingBufferItem, capacity);
    if (!rb->buf) {
        return FAILURE(false);
    }

    rb->capacity = capacity;
    rb->size = 0;
    rb->r = 0;
    rb->w = 0;

    return true;
};

static void ring_buffer_free(VirtIOSoundRingBuffer *rb) {
    if (rb->size) {
        ABORT("rb->size");
    }

    if (rb->r != rb->w) {
        ABORT("rb->r != rb->w");
    }

    g_free(rb->buf);
    ring_buffer_init(rb);
};

static bool ring_buffer_push(VirtIOSoundRingBuffer *rb,
                             const VirtIOSoundRingBufferItem *item) {
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

static VirtIOSoundRingBufferItem *ring_buffer_top(VirtIOSoundRingBuffer *rb) {
    VirtIOSoundRingBufferItem *item;

    if (!rb->size) {
        return NULL;
    }

    item = &rb->buf[rb->r];

    return item;
}

static void ring_buffer_pop(VirtIOSoundRingBuffer *rb) {
    const int size = rb->size;
    if (size <= 0) {
        ABORT("size <= 0");
    }

    rb->r = (rb->r + 1) % rb->capacity;
    rb->size = size - 1;
}

static uint32_t update_output_latency_bytes(VirtIOSoundPCMStream *stream, int x) {
    const int32_t val = stream->latency_bytes + x;
    stream->latency_bytes = val;
    return val;
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

static bool virtio_snd_voice_reopen(VirtIOSound *snd,
                                    VirtIOSoundPCMStream *stream,
                                    const char *stream_name,
                                    const uint16_t format16) {
    if (format16 == VIRTIO_SND_FORMAT16_BAD) {
        return FAILURE(false);
    }

    struct audsettings as = virtio_snd_unpack_format(format16);

    if (is_output_stream(stream)) {
        if (stream->voice.out) {
            /*
             * format16 is guest_format here, we might wangt to update
             * the only freq field here, keeping nchannels from aud_format.
             */
            as.nchannels = format16_get_nchannels(stream->aud_format);

            stream->voice.out = AUD_open_out(&snd->card,
                                             stream->voice.out,
                                             stream_name,
                                             stream,
                                             &stream_out_cb,
                                             &as);
            if (!stream->voice.out) {
                ABORT("stream->voice.out");
            }

            virtio_snd_stream_set_aud_format(stream, as.nchannels, format16);
            return true;
        } else if (snd->enable_output_prop) {
            for (as.nchannels = VIRTIO_SND_PCM_AUD_NUM_MAX_CHANNELS;
                 as.nchannels >= VIRTIO_SND_PCM_NUM_MIN_CHANNELS;
                 --as.nchannels) {

                stream->voice.out = AUD_open_out(&snd->card,
                                                 NULL,
                                                 stream_name,
                                                 stream,
                                                 &stream_out_cb,
                                                 &as);
                if (stream->voice.out) {
                    virtio_snd_stream_set_aud_format(stream, as.nchannels, format16);
                    return true;
                }
            }
        }
    } else {
        if (stream->voice.in) {
            /*
             * format16 is guest_format here, we might wangt to update
             * the only freq field here, keeping nchannels from aud_format.
             */
            as.nchannels = format16_get_nchannels(stream->aud_format);

            stream->voice.in = AUD_open_in(&snd->card,
                                           stream->voice.in,
                                           stream_name,
                                           stream,
                                           &stream_in_cb,
                                           &as);
            if (!stream->voice.in) {
                ABORT("stream->voice.in");
            }

            virtio_snd_stream_set_aud_format(stream, as.nchannels, format16);
            return true;
        } else if (snd->enable_input_prop) {
            for (as.nchannels = VIRTIO_SND_PCM_AUD_NUM_MAX_CHANNELS;
                 as.nchannels >= VIRTIO_SND_PCM_NUM_MIN_CHANNELS;
                 --as.nchannels) {
                stream->voice.in = AUD_open_in(&snd->card,
                                               NULL,
                                               stream_name,
                                               stream,
                                               &stream_in_cb,
                                               &as);
                if (stream->voice.in) {
                    g_input_stream = stream;

                    virtio_snd_stream_set_aud_format(stream, as.nchannels, format16);
                    return true;
                }
            }
        }
    }

    return FAILURE(false);
}

static void virtio_snd_stream_init(const unsigned stream_id,
                                   VirtIOSoundPCMStream *stream,
                                   VirtIOSound *snd) {
    memset(stream, 0, sizeof(*stream));

    stream->id = stream_id;
    stream->snd = snd;
    stream->voice.raw = NULL;
    stream->aud_format = VIRTIO_SND_FORMAT16_BAD;
    ring_buffer_init(&stream->pcm_buf);
    qemu_mutex_init(&stream->mtx);
    stream->state = VIRTIO_PCM_STREAM_STATE_DISABLED;
}

static bool virtio_snd_stream_start_locked(VirtIOSoundPCMStream *stream) {
    if (!stream->voice.raw) {
        return FAILURE(false);
    }

    if (is_output_stream(stream)) {
        SWVoiceOut *voice = stream->voice.out;
        AUD_set_active_out(voice, 1);
    } else {
        SWVoiceIn *voice = stream->voice.in;
        AUD_set_active_in(voice, 1);
    }

    stream->start_timestamp = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL_RT);
    stream->frames_consumed = 0;
    return true;
}

static void virtio_snd_stream_stop_locked(VirtIOSoundPCMStream *stream) {
    if (stream->voice.raw) {
        if (is_output_stream(stream)) {
            AUD_set_active_out(stream->voice.out, 0);
        } else {
            AUD_set_active_in(stream->voice.in, 0);
        }
    }
}

static void virtio_snd_stream_unprepare_out_locked(VirtIOSoundPCMStream *stream) {
    VirtIOSoundRingBuffer *pcm_buf = &stream->pcm_buf;
    VirtIOSound *snd = stream->snd;
    VirtQueue *tx_vq = snd->tx_vq;

    while (true) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            VirtQueueElement *e = item->el;
            const uint32_t latency_bytes = update_output_latency_bytes(stream,
                                                                       -(item->size - item->pos));

            vq_consume_element(
                tx_vq, e,
                el_send_pcm_status(e, VIRTIO_SND_S_OK, latency_bytes));

            ring_buffer_pop(pcm_buf);
        } else {
            break;
        }
    }

    virtio_notify(&snd->parent, tx_vq);
}

static void virtio_snd_stream_unprepare_in_locked(VirtIOSoundPCMStream *stream) {
    VirtIOSoundRingBuffer *pcm_buf = &stream->pcm_buf;
    VirtIOSound *snd = stream->snd;
    VirtQueue *rx_vq = snd->rx_vq;
    const int period_bytes = stream->period_bytes;

    while (true) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            VirtQueueElement *e = item->el;

            const size_t size = el_send_pcm_rx_status(
                e, period_bytes, VIRTIO_SND_S_OK, 0);
            vq_consume_element(rx_vq, e, size);

            ring_buffer_pop(pcm_buf);
        } else {
            break;
        }
    }

    virtio_notify(&snd->parent, rx_vq);
}

static void virtio_snd_stream_unprepare_locked(VirtIOSoundPCMStream *stream) {
    if (is_output_stream(stream)) {
        virtio_snd_stream_unprepare_out_locked(stream);
    } else {
        virtio_snd_stream_unprepare_in_locked(stream);
    }
    ring_buffer_free(&stream->pcm_buf);
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
        if (stream->voice.raw) {
            if (is_output_stream(stream)) {
                AUD_close_out(&stream->snd->card, stream->voice.out);
            } else {
                AUD_close_in(&stream->snd->card, stream->voice.in);
            }

            stream->voice.raw = NULL;
            stream->aud_format = VIRTIO_SND_FORMAT16_BAD;
        }

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
    VirtIOSoundPCMStream *stream;
    const struct virtio_snd_pcm_info *pcm_info;
    struct audsettings as;

    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    pcm_info = &g_pcm_infos[stream_id];

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

    stream = &snd->streams[stream_id];

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

static int convert_channels_1_to_2(int16_t *buffer, int16_t *in_end) {
    const int n = in_end - buffer;
    int16_t *in = in_end - 1;
    int16_t *out_end = buffer + n * 2;
    int16_t *out = out_end - 2;

    FOR_UNROLLED(n, in >= buffer, (in -= 1, out -= 2), ({
            const int in0 = in[0];
            out[0] = in0;
            out[1] = in0;
    }));

    return sizeof(*buffer) * (out_end - buffer);
}

static int convert_channels_1_to_3(int16_t *buffer, int16_t *in_end) {
    const int n = in_end - buffer;
    int16_t *in = in_end - 1;
    int16_t *out_end = buffer + n * 3;
    int16_t *out = out_end - 3;

    FOR_UNROLLED(n, in >= buffer, (in -= 1, out -= 3), ({
            const int in0 = in[0];
            out[0] = in0;
            out[1] = in0;
            out[2] = in0;
    }));

    return sizeof(*buffer) * (out_end - buffer);
}

static int convert_channels_1_to_4(int16_t *buffer, int16_t *in_end) {
    const int n = in_end - buffer;
    int16_t *in = in_end - 1;
    int16_t *out_end = buffer + n * 4;
    int16_t *out = out_end - 4;

    FOR_UNROLLED(n, in >= buffer, (in -= 1, out -= 4), ({
            const int in0 = in[0];
            out[0] = in0;
            out[1] = in0;
            out[2] = in0;
            out[3] = in0;
    }));

    return sizeof(*buffer) * (out_end - buffer);
}

static int convert_channels_1_to_5(int16_t *buffer, int16_t *in_end) {
    const int n = in_end - buffer;
    int16_t *in = in_end - 1;
    int16_t *out_end = buffer + n * 5;
    int16_t *out = out_end - 5;

    FOR_UNROLLED(n, in >= buffer, (in -= 1, out -= 5), ({
            const int in0 = in[0];
            out[0] = in0;
            out[1] = in0;
            out[2] = in0;
            out[3] = in0;
            out[4] = in0;
    }));

    return sizeof(*buffer) * (out_end - buffer);
}

static int convert_channels_2_to_1(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 2;
    int16_t *in = buffer;
    int16_t *out = buffer;

    FOR_UNROLLED(n, in < in_end, (in += 2, out += 1), ({
            const int in0 = in[0];
            const int in1 = in[1];
            out[0] = CLAMP_S16((in0 + in1) / 2);
    }));

    return sizeof(*buffer) * (out - buffer);
}

static int convert_channels_2_to_3(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 2;
    int16_t *in = in_end - 2;
    int16_t *out_end = buffer + n * 3;
    int16_t *out = out_end - 3;

    FOR_UNROLLED(n, in >= buffer, (in -= 2, out -= 3), ({
        const int in0 = in[0];
        const int in1 = in[1];
        out[0] = in0;
        out[1] = in1;
        out[2] = CLAMP_S16((in0 + in1) / 2);
    }));

    return sizeof(*buffer) * (out_end - buffer);
}

static int convert_channels_2_to_4(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 2;
    int16_t *in = in_end - 2;
    int16_t *out_end = buffer + n * 4;
    int16_t *out = out_end - 4;

    FOR_UNROLLED(n, in >= buffer, (in -= 2, out -= 4), ({
        const int in0 = in[0];
        const int in1 = in[1];
        out[0] = in0;
        out[1] = in1;
        out[2] = in0;
        out[3] = in1;
    }));

    return sizeof(*buffer) * (out_end - buffer);
}

static int convert_channels_2_to_5(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 2;
    int16_t *in = in_end - 2;
    int16_t *out_end = buffer + n * 5;
    int16_t *out = out_end - 4;

    FOR_UNROLLED(n, in >= buffer, (in -= 2, out -= 5), ({
        const int in0 = in[0];
        const int in1 = in[1];
        out[0] = in0;
        out[1] = in1;
        out[2] = in0;
        out[3] = in1;
        out[4] = CLAMP_S16((in0 + in1) / 2);
    }));

    return sizeof(*buffer) * (out_end - buffer);
}

static int convert_channels_3_to_1(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 3;
    int16_t *in = buffer;
    int16_t *out = buffer;

    FOR_UNROLLED(n, in < in_end, (in += 3, out += 1), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        out[0] = CLAMP_S16((85 * (in0 + in1 + in2)) / 256);
    }));

    return sizeof(*buffer) * (out - buffer);
}

static int convert_channels_3_to_2(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 3;
    int16_t *in = buffer;
    int16_t *out = buffer;

    FOR_UNROLLED(n, in < in_end, (in += 3, out += 2), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in22 = in[2] / 2;
        out[0] = CLAMP_S16((170 * (in0 + in22)) / 256);
        out[1] = CLAMP_S16((170 * (in1 + in22)) / 256);
    }));

    return sizeof(*buffer) * (out - buffer);
}

static int convert_channels_3_to_4(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 3;
    int16_t *in = in_end - 3;
    int16_t *out_end = buffer + n * 4;
    int16_t *out = out_end - 4;

    FOR_UNROLLED(n, in >= buffer, (in -= 3, out -= 4), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        out[0] = in0;
        out[1] = in1;
        out[2] = in2;
        out[3] = CLAMP_S16((in1 + in2) / 2);
    }));

    return sizeof(*buffer) * (out_end - buffer);
}

static int convert_channels_3_to_5(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 3;
    int16_t *in = in_end - 3;
    int16_t *out_end = buffer + n * 5;
    int16_t *out = out_end - 5;

    FOR_UNROLLED(n, in >= buffer, (in -= 3, out -= 5), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        out[0] = in0;
        out[1] = in1;
        out[2] = in2;
        out[3] = in2;
        out[4] = in2;
    }));

    return sizeof(*buffer) * (out_end - buffer);
}

static int convert_channels_4_to_1(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 4;
    int16_t *in = buffer;
    int16_t *out = buffer;

    FOR_UNROLLED(n, in < in_end, (in += 4, out += 1), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        out[0] = CLAMP_S16((in0 + in1 + in2 + in3) / 4);
    }));

    return sizeof(*buffer) * (out - buffer);
}

static int convert_channels_4_to_2(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 4;
    int16_t *in = buffer;
    int16_t *out = buffer;

    FOR_UNROLLED(n, in < in_end, (in += 4, out += 2), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        out[0] = CLAMP_S16((in0 + in2) / 2);
        out[1] = CLAMP_S16((in1 + in3) / 2);
    }));

    return sizeof(*buffer) * (out - buffer);
}

static int convert_channels_4_to_3(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 4;
    int16_t *in = buffer;
    int16_t *out = buffer;

    FOR_UNROLLED(n, in < in_end, (in += 4, out += 3), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        out[0] = in0;
        out[1] = in1;
        out[2] = CLAMP_S16((in2 + in3) / 2);
    }));

    return sizeof(*buffer) * (out - buffer);
}

static int convert_channels_4_to_5(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 4;
    int16_t *in = in_end - 4;
    int16_t *out_end = buffer + n * 5;
    int16_t *out = out_end - 5;

    FOR_UNROLLED(n, in >= buffer, (in -= 4, out -= 5), ({
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

    return sizeof(*buffer) * (out_end - buffer);
}

static int convert_channels_5_to_1(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 5;
    int16_t *in = buffer;
    int16_t *out = buffer;

    FOR_UNROLLED(n, in < in_end, (in += 5, out += 1), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        const int in4 = in[4];
        out[0] = CLAMP_S16((51 * (in0 + in1 + in2 + in3 + in4)) / 256);
    }));

    return sizeof(*buffer) * (out - buffer);
}

static int convert_channels_5_to_2(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 5;
    int16_t *in = buffer;
    int16_t *out = buffer;

    FOR_UNROLLED(n, in < in_end, (in += 5, out += 2), ({
        const int in0 = in[0];
        const int in1 = in[1];
        const int in2 = in[2];
        const int in3 = in[3];
        const int in44 = in[4] / 4;
        out[0] = CLAMP_S16(in0 / 2 + in2 / 4 + in44);
        out[1] = CLAMP_S16(in1 / 2 + in3 / 4 + in44);
    }));

    return sizeof(*buffer) * (out - buffer);
}

static int convert_channels_5_to_3(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 5;
    int16_t *in = buffer;
    int16_t *out = buffer;

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

    return sizeof(*buffer) * (out - buffer);
}

static int convert_channels_5_to_4(int16_t *buffer, int16_t *in_end) {
    const int n = (in_end - buffer) / 5;
    int16_t *in = buffer;
    int16_t *out = buffer;

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

    return sizeof(*buffer) * (out - buffer);
}

static int convert_channels(int16_t *buffer, const int in_sz,
                            const int in_fs, const int out_fs) {
    if (out_fs == in_fs) {
        return in_sz;
    } else {
        int16_t *buffer_end = buffer + in_sz / sizeof(*buffer);

        switch (((in_fs << 4) + out_fs) / sizeof(*buffer)) {
        case 0x12: return convert_channels_1_to_2(buffer, buffer_end);
        case 0x13: return convert_channels_1_to_3(buffer, buffer_end);
        case 0x14: return convert_channels_1_to_4(buffer, buffer_end);
        case 0x15: return convert_channels_1_to_5(buffer, buffer_end);

        case 0x21: return convert_channels_2_to_1(buffer, buffer_end);
        case 0x23: return convert_channels_2_to_3(buffer, buffer_end);
        case 0x24: return convert_channels_2_to_4(buffer, buffer_end);
        case 0x25: return convert_channels_2_to_5(buffer, buffer_end);

        case 0x31: return convert_channels_3_to_1(buffer, buffer_end);
        case 0x32: return convert_channels_3_to_2(buffer, buffer_end);
        case 0x34: return convert_channels_3_to_4(buffer, buffer_end);
        case 0x35: return convert_channels_3_to_5(buffer, buffer_end);

        case 0x41: return convert_channels_4_to_1(buffer, buffer_end);
        case 0x42: return convert_channels_4_to_2(buffer, buffer_end);
        case 0x43: return convert_channels_4_to_3(buffer, buffer_end);
        case 0x45: return convert_channels_4_to_5(buffer, buffer_end);

        case 0x51: return convert_channels_5_to_1(buffer, buffer_end);
        case 0x52: return convert_channels_5_to_2(buffer, buffer_end);
        case 0x53: return convert_channels_5_to_3(buffer, buffer_end);
        case 0x54: return convert_channels_5_to_4(buffer, buffer_end);

        default:
            ABORT("bad");
        }
    }
}

static int calc_stream_adjustment_bytes(const uint64_t elapsed_nf,
                                        const uint64_t consumed_nf,
                                        const int driver_frame_size) {
    return (consumed_nf < elapsed_nf) ? (driver_frame_size * (elapsed_nf - consumed_nf)) : 0;
}

static int calc_drop_bytes_out(VirtIOSoundPCMStream *stream, const int n_periods) {
    const uint64_t elapsed_usec = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL_RT) - stream->start_timestamp;
    const uint64_t elapsed_nf = elapsed_usec * (uint64_t)stream->freq_hz / 1000000ul;
    // We have to feed the soundcard ahead of the real time.
    const uint64_t to_be_consumed_nf = elapsed_nf + stream->period_frames * n_periods;

    return calc_stream_adjustment_bytes(to_be_consumed_nf, stream->frames_consumed,
                                        stream->driver_frame_size);
}

static int calc_insert_bytes_in(VirtIOSoundPCMStream *stream) {
    const uint64_t elapsed_usec = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL_RT) - stream->start_timestamp;
    const uint64_t elapsed_nf = elapsed_usec * (uint64_t)stream->freq_hz / 1000000ul;
    // The consumed position could be delayed up to a period.
    const uint64_t consumed_nf = stream->frames_consumed + stream->period_frames;

    return calc_stream_adjustment_bytes(elapsed_nf, consumed_nf,
                                        stream->driver_frame_size);
}

static VirtQueue *stream_out_cb_locked(VirtIOSoundPCMStream *stream, int avail) {
    SWVoiceOut *const voice = stream->voice.out;
    VirtIOSoundRingBuffer *const pcm_buf = &stream->pcm_buf;
    VirtIOSound *const snd = stream->snd;
    VirtQueue *const tx_vq = snd->tx_vq;
    const int aud_fs = stream->aud_frame_size;
    const int driver_fs = stream->driver_frame_size;
    bool notify_vq = false;
    int16_t scratch[AUD_SCRATCH_SIZE];
    const int max_scratch_frames = sizeof(scratch) / MAX(aud_fs, driver_fs);
    int drop_bytes;

    while (avail >= aud_fs) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            int item_size = item->size - item->pos;
            VirtQueueElement *const e = item->el;
            uint32_t latency_bytes = 0;

            while (true) {
                const int nf = MIN(avail / aud_fs,
                                   MIN(item_size / driver_fs,
                                       max_scratch_frames));
                if (!nf) {
                    break;
                }

                const int driver_nb = nf * driver_fs;
                iov_to_buf(e->out_sg, e->out_num, item->pos, scratch, driver_nb);
                const int aud_nb = convert_channels(scratch, driver_nb, driver_fs, aud_fs);
                const int aud_sent_nb = AUD_write(voice, scratch, aud_nb);
                if (aud_sent_nb <= 0) {
                    goto aud_write_full;
                }
                avail -= aud_sent_nb;

                const int sent_fn = aud_sent_nb / aud_fs;
                const int driver_sent_nb = sent_fn * driver_fs;

                item->pos += driver_sent_nb;
                item_size -= driver_sent_nb;
                latency_bytes = update_output_latency_bytes(stream, -driver_sent_nb);
                stream->frames_consumed += sent_fn;
            }

            if (item_size < driver_fs) {
                vq_consume_element(
                    tx_vq, e,
                    el_send_pcm_status(e, VIRTIO_SND_S_OK, latency_bytes));
                ring_buffer_pop(pcm_buf);
                notify_vq = true;
            }
        } else {
            memset(scratch, 0, MIN(avail, sizeof(scratch)));
            while (avail >= aud_fs) {
                const int32_t nf = MIN(avail, sizeof(scratch)) / aud_fs;
                const int32_t nb = nf * aud_fs;
                const int32_t sent_bytes = AUD_write(voice, scratch, nb);
                if (sent_bytes <= 0) {
                    break;
                }

                avail -= sent_bytes;
            }
            goto done;
        }
    }

aud_write_full:
    drop_bytes = calc_drop_bytes_out(stream, 4);
    while (drop_bytes >= driver_fs) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            int item_size = item->size - item->pos;
            const int nf = MIN(drop_bytes, item_size) / driver_fs;
            const int nb = nf * driver_fs;
            uint32_t latency_bytes;
            drop_bytes -= nb;
            item->pos += nb;
            item_size -= nb;
            latency_bytes = update_output_latency_bytes(stream, -nb);
            stream->frames_consumed += nf;

            if (item_size < driver_fs) {
                VirtQueueElement *const e = item->el;
                vq_consume_element(
                    tx_vq, e,
                    el_send_pcm_status(e, VIRTIO_SND_S_OK, latency_bytes));
                ring_buffer_pop(pcm_buf);
                notify_vq = true;
            }
        } else {
            break;
        }
    }

done:
    return notify_vq ? tx_vq : NULL;
}

static void stream_out_cb(void *opaque, int avail) {
    VirtIOSoundPCMStream *const stream = (VirtIOSoundPCMStream *)opaque;

    qemu_mutex_lock(&stream->mtx);
    VirtQueue *vq_to_notify = stream_out_cb_locked(stream, avail);
    qemu_mutex_unlock(&stream->mtx);

    if (vq_to_notify) {
        virtio_notify(&stream->snd->parent, vq_to_notify);
    }
}

static bool virtio_snd_process_tx(VirtQueue *vq, VirtQueueElement *e, VirtIOSound *snd) {
    VirtIOSoundRingBufferItem item;
    struct virtio_snd_pcm_xfer xfer;
    VirtIOSoundPCMStream *stream;
    const size_t req_size = iov_size(e->out_sg, e->out_num);

    if (req_size < sizeof(xfer)) {
        vq_consume_element(vq, e, 0);
        return FAILURE(false);
    }

    iov_to_buf(e->out_sg, e->out_num, 0, &xfer, sizeof(xfer));
    if (xfer.stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        vq_consume_element(vq, e, el_send_pcm_status(e, VIRTIO_SND_S_BAD_MSG, 0));
        return FAILURE(false);
    }

    stream = &snd->streams[xfer.stream_id];

    item.el = e;
    item.pos = sizeof(xfer);
    item.size = req_size;

    qemu_mutex_lock(&stream->mtx);

    if (ring_buffer_push(&stream->pcm_buf, &item)) {
        update_output_latency_bytes(stream, item.size - item.pos);
    } else {
        ABORT("ring_buffer_push");
    }

    qemu_mutex_unlock(&stream->mtx);
    return true;
}

static VirtQueue *stream_in_cb_locked(VirtIOSoundPCMStream *stream, int avail) {
    SWVoiceIn *const voice = stream->voice.in;
    const int aud_fs = stream->aud_frame_size;
    const int driver_fs = stream->driver_frame_size;
    const int period_bytes = stream->period_bytes;
    VirtIOSoundRingBuffer *const pcm_buf = &stream->pcm_buf;
    VirtIOSound *const snd = stream->snd;
    VirtQueue *const rx_vq = snd->rx_vq;
    bool notify_vq = false;
    int16_t scratch[AUD_SCRATCH_SIZE];
    const int max_scratch_frames = sizeof(scratch) / MAX(aud_fs, driver_fs);
    int insert_bytes;

    while (avail >= aud_fs) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            VirtQueueElement *const e = item->el;

            while (true) {
                const int nf = MIN(avail / aud_fs,
                                   MIN((period_bytes - item->pos) / driver_fs,
                                       max_scratch_frames));
                if (!nf) {
                    break;
                }

                const int aud_to_read = nf * aud_fs;
                const int aud_read_nb = AUD_read(voice, scratch, aud_to_read);
                if (aud_read_nb <= 0) {
                    goto aud_read_empty;
                }
                avail -= aud_read_nb;

                const int driver_read_nb = convert_channels(scratch, aud_read_nb, aud_fs, driver_fs);

                iov_from_buf(e->in_sg, e->in_num, item->pos, scratch, driver_read_nb);
                item->pos += driver_read_nb;
                stream->frames_consumed += (driver_read_nb / driver_fs);
            }

            if ((period_bytes - item->pos) < driver_fs) {
                const size_t size = el_send_pcm_rx_status(
                    e, period_bytes, VIRTIO_SND_S_OK, avail);
                vq_consume_element(rx_vq, e, size);
                ring_buffer_pop(pcm_buf);
                notify_vq = true;
            }
        } else {
            while (avail >= aud_fs) {
                const int nf = MIN(avail, sizeof(scratch)) / aud_fs;
                const int nb = nf * aud_fs;
                const int read_bytes = AUD_read(voice, scratch, nb);
                if (read_bytes <= 0) {
                    break;
                }
                avail -= read_bytes;
            }
            goto done;
        }
    }

aud_read_empty:
    insert_bytes = calc_insert_bytes_in(stream);
    memset(scratch, 0, MIN(insert_bytes, sizeof(scratch)));
    while (insert_bytes >= driver_fs) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            VirtQueueElement *const e = item->el;
            const int nf = MIN(insert_bytes,
                               MIN(sizeof(scratch),
                                   period_bytes - item->pos)) / driver_fs;
            const int nb = nf * driver_fs;
            iov_from_buf(e->in_sg, e->in_num, item->pos, scratch, nb);
            insert_bytes -= nb;
            item->pos += nb;
            stream->frames_consumed += nf;

            if (item->pos >= period_bytes) {
                const size_t size = el_send_pcm_rx_status(
                    e, period_bytes, VIRTIO_SND_S_OK, 0);
                vq_consume_element(rx_vq, e, size);
                ring_buffer_pop(pcm_buf);
                notify_vq = true;
            }
        } else {
            break;
        }
    }

done:
    return notify_vq ? rx_vq : NULL;
}

static void stream_in_cb(void *opaque, int avail) {
    VirtIOSoundPCMStream *const stream = (VirtIOSoundPCMStream *)opaque;

    qemu_mutex_lock(&stream->mtx);
    VirtQueue *vq_to_notify = stream_in_cb_locked(stream, avail);
    qemu_mutex_unlock(&stream->mtx);

    if (vq_to_notify) {
        virtio_notify(&stream->snd->parent, vq_to_notify);
    }
}

static bool virtio_snd_process_rx(VirtQueue *vq, VirtQueueElement *e, VirtIOSound *snd) {
    VirtIOSoundRingBufferItem item;
    struct virtio_snd_pcm_xfer xfer;
    VirtIOSoundPCMStream *stream;
    const size_t req_size = iov_size(e->out_sg, e->out_num);

    if (req_size < sizeof(xfer)) {
        vq_consume_element(vq, e, 0);
        return FAILURE(false);
    }

    iov_to_buf(e->out_sg, e->out_num, 0, &xfer, sizeof(xfer));
    if (xfer.stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        vq_consume_element(vq, e, el_send_pcm_rx_status(e, 0, VIRTIO_SND_S_BAD_MSG, 0));
        return FAILURE(false);
    }

    stream = &snd->streams[xfer.stream_id];

    item.el = e;
    item.pos = 0;
    item.size = req_size;  /* not used in RX but convinient for snapshots */

    qemu_mutex_lock(&stream->mtx);

    if (!ring_buffer_push(&stream->pcm_buf, &item)) {
        ABORT("ring_buffer_push");
    }

    qemu_mutex_unlock(&stream->mtx);
    return true;
}

static bool virtio_snd_stream_prepare_vars(VirtIOSoundPCMStream *stream) {
    if (!ring_buffer_alloc(&stream->pcm_buf,
                           2 * stream->buffer_bytes / stream->period_bytes)) {
        return FAILURE(false);
    }

    stream->freq_hz = format16_get_freq_hz(stream->guest_format);
    stream->driver_frame_size = format16_get_frame_size(stream->guest_format);
    stream->period_frames = stream->period_bytes / stream->driver_frame_size;
    stream->latency_bytes = 0;

    return true;
}

static uint32_t
virtio_snd_process_ctl_pcm_prepare_impl(unsigned stream_id, VirtIOSound* snd) {
    VirtIOSoundPCMStream *stream;
    uint32_t r;

    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);

    if (!stream->buffer_bytes) {
        r = FAILURE(VIRTIO_SND_S_BAD_MSG);
        goto done;
    }

    switch (stream->state) {
    case VIRTIO_PCM_STREAM_STATE_PARAMS_SET:
        if (!virtio_snd_stream_prepare_vars(stream)) {
            r = FAILURE(VIRTIO_SND_S_BAD_MSG);
            goto done;
        }

        if (!virtio_snd_voice_reopen(snd, stream, g_stream_name[stream_id],
                                     stream->guest_format)) {
            r = FAILURE(VIRTIO_SND_S_BAD_MSG);
            ring_buffer_free(&stream->pcm_buf);
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
    VirtIOSoundPCMStream *stream;

    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    stream = &snd->streams[stream_id];

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
    VirtIOSoundPCMStream *stream;
    uint32_t r;
    uint32_t period_bytes = 0;
    bool is_output = false;

    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);

    if (stream->state != VIRTIO_PCM_STREAM_STATE_PREPARED) {
        r = FAILURE(VIRTIO_SND_S_BAD_MSG);
        goto done;
    }

    if (!virtio_snd_stream_start_locked(stream)) {
        r = FAILURE(VIRTIO_SND_S_IO_ERR);
        goto done;
    }

    is_output = is_output_stream(stream);
    period_bytes = stream->period_bytes;
    stream->state = VIRTIO_PCM_STREAM_STATE_RUNNING;
    r = VIRTIO_SND_S_OK;

done:
    qemu_mutex_unlock(&stream->mtx);

    if (is_output && (period_bytes > 0)) {
        stream_out_cb(stream, period_bytes * 2);
    }

    return r;
}

static uint32_t
virtio_snd_process_ctl_pcm_stop_impl(unsigned stream_id, VirtIOSound* snd) {
    VirtIOSoundPCMStream *stream;

    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return FAILURE(VIRTIO_SND_S_BAD_MSG);
    }

    stream = &snd->streams[stream_id];

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

    while (virtio_queue_ready(vq)) {
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
static bool virtio_snd_handle_tx_impl(VirtIOSound *snd, VirtQueue *vq) {
    bool need_notify = false;

    while (virtio_queue_ready(vq)) {
        VirtQueueElement *e = (VirtQueueElement *)virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (e) {
            need_notify = !virtio_snd_process_tx(vq, e, snd) || need_notify;
        } else {
            break;
        }
    }

    return need_notify;
}

static void virtio_snd_handle_tx(VirtIODevice *vdev, VirtQueue *vq) {
    if (virtio_snd_handle_tx_impl(VIRTIO_SND(vdev), vq)) {
        virtio_notify(vdev, vq);
    }
}

/* device -> driver */
static bool virtio_snd_handle_rx_impl(VirtIOSound *snd, VirtQueue *vq) {
    bool need_notify = false;

    while (virtio_queue_ready(vq)) {
        VirtQueueElement *e = (VirtQueueElement *)virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (e) {
            need_notify = !virtio_snd_process_rx(vq, e, snd) || need_notify;
        } else {
            break;
        }
    }

    return need_notify;
}

static void virtio_snd_handle_rx(VirtIODevice *vdev, VirtQueue *vq) {
    if (virtio_snd_handle_rx_impl(VIRTIO_SND(vdev), vq)) {
        virtio_notify(vdev, vq);
    }
}

static void virtio_snd_device_realize(DeviceState *dev, Error **errp) {
    VirtIOSound *snd = VIRTIO_SND(dev);
    VirtIODevice *vdev = &snd->parent;
    int i;

    AUD_register_card(TYPE_VIRTIO_SND, &snd->card);

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
    unsigned i;

    for (i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
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

static void virtio_snd_unpop_pcm_buf_back_to_vq(VirtIOSound *snd,
                                                VirtIOSoundPCMStream *stream) {
    const bool is_output = is_output_stream(stream);
    VirtQueue *vq = is_output ? snd->tx_vq : snd->rx_vq;
    VirtIOSoundRingBuffer *pcm_buf = &stream->pcm_buf;
    const unsigned capacity = pcm_buf->capacity;
    const unsigned capacity1 = capacity - 1;

    while (pcm_buf->size > 0) {
        const unsigned i = (pcm_buf->w + capacity1) % capacity;
        VirtIOSoundRingBufferItem *const item = &pcm_buf->buf[i];

        virtqueue_unpop(vq, item->el, item->size);
        pcm_buf->w = i;
        --pcm_buf->size;
    }

    stream->latency_bytes = 0;
}

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
            virtio_snd_unpop_pcm_buf_back_to_vq(snd, stream);
            ring_buffer_free(&stream->pcm_buf);
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
            if (!virtio_snd_stream_prepare_vars(stream)) {
                ABORT("virtio_snd_stream_prepare_vars");
            }

            virtio_snd_voice_reopen(snd, stream, g_stream_name[i], stream->guest_format);
        }
    }

    virtio_snd_handle_tx_impl(snd, snd->tx_vq);
    virtio_snd_handle_rx_impl(snd, snd->tx_vq);

    for (i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        VirtIOSoundPCMStream *stream = &snd->streams[i];

        if (stream->state > VIRTIO_PCM_STREAM_STATE_RUNNING) {
            ABORT("stream->state");
        } else if (stream->state == VIRTIO_PCM_STREAM_STATE_RUNNING) {
            if (!virtio_snd_stream_start_locked(stream)) {
                ABORT("virtio_snd_stream_start_locked");
            }
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
         * `VirtIOSoundRingBuffer` and restored after loading (in `post_load`).
         */
        VMSTATE_UINT32(buffer_bytes, VirtIOSoundPCMStream),
        VMSTATE_UINT32(period_bytes, VirtIOSoundPCMStream),
        VMSTATE_UINT32(buffer_frames, VirtIOSoundPCMStream),
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
