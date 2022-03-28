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
#include "qemu/typedefs.h"
#include "qapi/error.h"
#include "standard-headers/linux/virtio_ids.h"
#include "hw/audio/soundhw.h"
#include "hw/virtio/virtio-snd.h"
#include "hw/pci/pci.h"
#include "intel-hda-defs.h"

#define derror(fmt, ...) fprintf(stderr, "rkir555 " fmt "\n", ##__VA_ARGS__)

#define VIRTIO_SND_QUEUE_CTL    0
#define VIRTIO_SND_QUEUE_EVENT  1
#define VIRTIO_SND_QUEUE_TX     2
#define VIRTIO_SND_QUEUE_RX     3

#define VIRTIO_SND_PCM_MIC_NUM_CHANNELS         1
#define VIRTIO_SND_PCM_MIC_FREQ             44100
#define VIRTIO_SND_PCM_MIC_FORMAT             S16
#define VIRTIO_SND_PCM_SPEAKERS_NUM_CHANNELS    2
#define VIRTIO_SND_PCM_SPEAKERS_FREQ        44100
#define VIRTIO_SND_PCM_SPEAKERS_FORMAT        S16

#define VIRTIO_SND_NID_MIC                      0
#define VIRTIO_SND_NID_SPEAKERS                 0

#define GLUE(A, B) A##B
#define GLUE2(A, B) GLUE(A, B)

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
#define VIRTIO_SND_PACK_FORMAT(nchannels, fmt, freq) (freq | (fmt << 4) | ((nchannels - 1) << 9))

static int format16_get_freq_hz(uint16_t format16) {
    switch (format16 & 0xF) {
    case VIRTIO_SND_PCM_RATE_8000:  return 8000;
    case VIRTIO_SND_PCM_RATE_11025: return 11025;
    case VIRTIO_SND_PCM_RATE_16000: return 16000;
    case VIRTIO_SND_PCM_RATE_22050: return 22050;
    case VIRTIO_SND_PCM_RATE_32000: return 32000;
    case VIRTIO_SND_PCM_RATE_44100: return 44100;
    case VIRTIO_SND_PCM_RATE_48000: return 48000;
    default:                        return -1;
    }
}

static int format16_get_nchannels(uint16_t format16) {
    return (format16 >> 9) + 1;
}

static int format16_get_frame_size(uint16_t format16) {
    return format16_get_nchannels(format16) * sizeof(int16_t);
}

struct audsettings virtio_snd_unpack_format(uint16_t format16) {
    struct audsettings as;

    as.freq = format16_get_freq_hz(format16);

    switch ((format16 >> 4) & 0x1F) {
    case VIRTIO_SND_PCM_FMT_S16:
        as.fmt = AUD_FMT_S16;
        break;

    default:
        as.fmt = -1;
        break;
    }

    as.nchannels = format16_get_nchannels(format16);
    as.endianness = AUDIO_HOST_ENDIANNESS;

    return as;
}
static const VMStateDescription virtio_snd_vmstate = {
    .name = TYPE_VIRTIO_SND,
    .minimum_version_id = 0,
    .version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_VIRTIO_DEVICE,
        /* TODO */
        VMSTATE_END_OF_LIST()
    },
};

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

const uint16_t g_stream_formats[VIRTIO_SND_NUM_PCM_STREAMS] = {
    VIRTIO_SND_PACK_FORMAT(VIRTIO_SND_PCM_MIC_NUM_CHANNELS,
                           GLUE2(VIRTIO_SND_PCM_FMT_, VIRTIO_SND_PCM_MIC_FORMAT),
                           GLUE2(VIRTIO_SND_PCM_RATE_, VIRTIO_SND_PCM_MIC_FREQ)),
    VIRTIO_SND_PACK_FORMAT(VIRTIO_SND_PCM_SPEAKERS_NUM_CHANNELS,
                           GLUE2(VIRTIO_SND_PCM_FMT_, VIRTIO_SND_PCM_SPEAKERS_FORMAT),
                           GLUE2(VIRTIO_SND_PCM_RATE_, VIRTIO_SND_PCM_SPEAKERS_FREQ)),
};

const static struct virtio_snd_pcm_info g_pcm_infos[VIRTIO_SND_NUM_PCM_STREAMS] = {
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_MIC,
        },
        .features = 0,
        .formats = (1u << GLUE2(VIRTIO_SND_PCM_FMT_, VIRTIO_SND_PCM_MIC_FORMAT)),
        .rates = (1u << GLUE2(VIRTIO_SND_PCM_RATE_, VIRTIO_SND_PCM_MIC_FREQ)),
        .direction = VIRTIO_SND_D_INPUT,
        .channels_min = VIRTIO_SND_PCM_MIC_NUM_CHANNELS,
        .channels_max = VIRTIO_SND_PCM_MIC_NUM_CHANNELS,
    },
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_SPEAKERS,
        },
        .features = 0,
        .formats = (1u << GLUE2(VIRTIO_SND_PCM_FMT_, VIRTIO_SND_PCM_SPEAKERS_FORMAT)),
        .rates = (1u << GLUE2(VIRTIO_SND_PCM_RATE_, VIRTIO_SND_PCM_SPEAKERS_FREQ)),
        .direction = VIRTIO_SND_D_OUTPUT,
        .channels_min = VIRTIO_SND_PCM_SPEAKERS_NUM_CHANNELS,
        .channels_max = VIRTIO_SND_PCM_SPEAKERS_NUM_CHANNELS,
    },
};

const struct virtio_snd_chmap_info g_chmap_infos[VIRTIO_SND_NUM_CHMAPS] = {
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_MIC,
        },
        .direction = VIRTIO_SND_D_INPUT,
        .channels = VIRTIO_SND_PCM_MIC_NUM_CHANNELS,
        .positions = {
#if VIRTIO_SND_PCM_MIC_NUM_CHANNELS == 1
            VIRTIO_SND_CHMAP_MONO
#else
            VIRTIO_SND_CHMAP_FL,
            VIRTIO_SND_CHMAP_FR,
            VIRTIO_SND_CHMAP_RL,
            VIRTIO_SND_CHMAP_RR,
#endif
            },
    },
    {
        .hdr = {
            .hda_fn_nid = VIRTIO_SND_NID_SPEAKERS,
        },
        .direction = VIRTIO_SND_D_OUTPUT,
        .channels = VIRTIO_SND_PCM_SPEAKERS_NUM_CHANNELS,
        .positions = {
#if VIRTIO_SND_PCM_SPEAKERS_NUM_CHANNELS == 1
            VIRTIO_SND_CHMAP_MONO
#else
            VIRTIO_SND_CHMAP_FL,
            VIRTIO_SND_CHMAP_FR,
            VIRTIO_SND_CHMAP_RL,
            VIRTIO_SND_CHMAP_RR,
            VIRTIO_SND_CHMAP_LFE,
#endif
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

static void ring_buffer_init(VirtIOSoundRingBuffer *rb) {
    rb->buf = NULL;
    rb->capacity = 0;
    rb->size = 111111111;
    rb->r = 222222222;
    rb->w = 333333333;
};

static bool ring_buffer_alloc(VirtIOSoundRingBuffer *rb, size_t capacity) {
    if (rb->buf) {
        abort();
    }

    if (rb->capacity) {
        abort();
    }

    rb->buf = g_new0(VirtIOSoundRingBufferItem, capacity);;
    if (!rb->buf) {
        return false;
    }

    rb->capacity = capacity;
    rb->size = 0;
    rb->r = 0;
    rb->w = 0;

    return true;
};

static void ring_buffer_free(VirtIOSoundRingBuffer *rb) {
    if (rb->size) {
        abort();
    }

    g_free(rb->buf);
    rb->buf = NULL;
    rb->capacity = 0;
    rb->size = 111111111;
    rb->r = 222222222;
    rb->w = 333333333;
};

static bool ring_buffer_push(VirtIOSoundRingBuffer *rb,
                             const VirtIOSoundRingBufferItem *item) {
    const int capacity = rb->capacity;
    const int size = rb->size;
    int w;

    if (size >= capacity) {
        return false;
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

static bool ring_buffer_pop(VirtIOSoundRingBuffer *rb) {
    const int size = rb->size;
    if (!size) {
        return false;
    }

    rb->r = (rb->r + 1) % rb->capacity;
    rb->size = size - 1;


    return true;
}

static uint32_t update_output_latency_bytes(VirtIOSoundPCMStream *stream, int x) {
    uint32_t *pval = &stream->latency_bytes;
    const uint32_t val = *pval + x;
    *pval = val;
    return val;
}

static void stream_out_cb(void *opaque, int avail);
static void stream_in_cb(void *opaque, int avail);

static void virtio_snd_voice_open(VirtIOSound *snd, VirtIOSoundPCMStream *stream) {
    struct audsettings as = virtio_snd_unpack_format(stream->format);

    if (is_output_stream(stream)) {
        stream->voice.out = AUD_open_out(&snd->card,
                                         NULL,
                                         g_stream_name[stream->id],
                                         stream,
                                         &stream_out_cb,
                                         &as);
    } else {
        stream->voice.in = AUD_open_in(&snd->card,
                                       NULL,
                                       g_stream_name[stream->id],
                                       stream,
                                       &stream_in_cb,
                                       &as);
    }
}

static void virtio_snd_voice_close(VirtIOSound *snd, VirtIOSoundPCMStream *stream) {
    if (stream->voice.raw) {
        if (is_output_stream(stream)) {
            AUD_close_out(&snd->card, stream->voice.out);
        } else {
            AUD_close_in(&snd->card, stream->voice.in);
        }
    }
}

static void virtio_snd_stream_init(const unsigned stream_id,
                                   VirtIOSoundPCMStream *stream,
                                   VirtIOSound *snd) {
    memset(stream, 0, sizeof(*stream));

    stream->id = stream_id;
    stream->snd = snd;
    stream->format = g_stream_formats[stream_id];
    stream->voice.raw = NULL;
    virtio_snd_voice_open(snd, stream);
    ring_buffer_init(&stream->pcm_buf);
    qemu_mutex_init(&stream->mtx);
    stream->state = VIRTIO_PCM_STREAM_STATE_DISABLED;
}

static void virtio_snd_process_ctl_stream_stop_locked(VirtIOSoundPCMStream *stream) {
    if (stream->voice.raw) {
        if (is_output_stream(stream)) {
            AUD_set_active_out(stream->voice.out, 0);
        } else {
            AUD_set_active_in(stream->voice.in, 0);
        }
    }
}

static void virtio_snd_process_ctl_stream_unprepare_out_locked(VirtIOSoundPCMStream *stream) {
    VirtIOSoundRingBuffer *pcm_buf = &stream->pcm_buf;
    VirtIOSound *snd = stream->snd;
    VirtQueue *tx_vq = snd->tx_vq;

    while (true) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            VirtQueueElement *e = item->el;
            const uint32_t latency_bytes = update_output_latency_bytes(stream, -item->size);

            vq_consume_element(
                tx_vq, e,
                el_send_pcm_status(e, VIRTIO_SND_S_OK, latency_bytes));

            ring_buffer_pop(pcm_buf);
        } else {
            break;
        }
    }

    ring_buffer_free(pcm_buf);
    virtio_notify(&snd->parent, tx_vq);
}

static void virtio_snd_process_ctl_stream_unprepare_in_locked(VirtIOSoundPCMStream *stream) {
    VirtIOSoundRingBuffer *pcm_buf = &stream->pcm_buf;
    VirtIOSound *snd = stream->snd;
    VirtQueue *rx_vq = snd->rx_vq;

    while (true) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            VirtQueueElement *e = item->el;
            el_send_pcm_status(e, VIRTIO_SND_S_OK, 0);
            vq_consume_element(rx_vq, e, item->pos);
            ring_buffer_pop(pcm_buf);
        } else {
            break;
        }
    }

    ring_buffer_free(pcm_buf);
    virtio_notify(&snd->parent, rx_vq);
}

static void virtio_snd_process_ctl_stream_unprepare_locked(VirtIOSoundPCMStream *stream) {
    if (is_output_stream(stream)) {
        virtio_snd_process_ctl_stream_unprepare_out_locked(stream);
    } else {
        virtio_snd_process_ctl_stream_unprepare_in_locked(stream);
    }
}

static void virtio_snd_stream_enable(VirtIOSoundPCMStream *stream) {
    qemu_mutex_lock(&stream->mtx);
    switch (stream->state) {
    case VIRTIO_PCM_STREAM_STATE_RUNNING:
    case VIRTIO_PCM_STREAM_STATE_PREPARED:
    case VIRTIO_PCM_STREAM_STATE_ENABLED:
        break;

    case VIRTIO_PCM_STREAM_STATE_DISABLED:
        stream->state = VIRTIO_PCM_STREAM_STATE_ENABLED;
        break;

    default:
        derror("%s:%d unexpected state, %d", __func__, __LINE__, stream->state);
        break;
    }
    qemu_mutex_unlock(&stream->mtx);
}

static void virtio_snd_stream_disable(VirtIOSoundPCMStream *stream) {
    qemu_mutex_lock(&stream->mtx);
    switch (stream->state) {
    case VIRTIO_PCM_STREAM_STATE_RUNNING:
        virtio_snd_process_ctl_stream_stop_locked(stream);
        /* fallthrough */

    case VIRTIO_PCM_STREAM_STATE_PREPARED:
        virtio_snd_process_ctl_stream_unprepare_locked(stream);
        /* fallthrough */

    case VIRTIO_PCM_STREAM_STATE_ENABLED:
        stream->state = VIRTIO_PCM_STREAM_STATE_DISABLED;
        /* fallthrough */

    case VIRTIO_PCM_STREAM_STATE_DISABLED:
        stream->buffer_bytes = 0;
        stream->period_bytes = 0;
        stream->buffer_frames = 0;
        break;

    default:
        derror("%s:%d unexpected state, %d", __func__, __LINE__, stream->state);
        break;
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

    return el_send_status(e, VIRTIO_SND_S_BAD_MSG);
}

static size_t
virtio_snd_process_ctl_jack_remap(VirtQueueElement *e,
                                  const struct virtio_snd_jack_remap* req,
                                  VirtIOSound* snd) {
    return el_send_status(e, VIRTIO_SND_S_NOT_SUPP);
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

    return el_send_status(e, VIRTIO_SND_S_BAD_MSG);
}

static uint32_t
virtio_snd_process_ctl_pcm_set_params_impl(const struct virtio_snd_pcm_set_params* req,
                                           VirtIOSound* snd) {
    const unsigned stream_id = req->hdr.stream_id;
    VirtIOSoundPCMStream *stream;
    const struct virtio_snd_pcm_info *pcm_info;
    struct audsettings as;

    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return VIRTIO_SND_S_BAD_MSG;
    }

    pcm_info = &g_pcm_infos[stream_id];

    if (!(pcm_info->rates & (1u << req->rate))) {
        return VIRTIO_SND_S_BAD_MSG;
    }

    if (!(pcm_info->formats & (1u << req->format))) {
        return VIRTIO_SND_S_BAD_MSG;
    }

    if ((req->channels < pcm_info->channels_min) || (req->channels > pcm_info->channels_max)) {
        return VIRTIO_SND_S_BAD_MSG;
    }

    if (!req->buffer_bytes) {
        return VIRTIO_SND_S_BAD_MSG;
    }

    if (!req->period_bytes) {
        return VIRTIO_SND_S_BAD_MSG;
    }

    stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);
    switch (stream->state) {
    case VIRTIO_PCM_STREAM_STATE_PREPARED:
        virtio_snd_process_ctl_stream_unprepare_locked(stream);
        break;

    case VIRTIO_PCM_STREAM_STATE_ENABLED:
        break;

    default:
        qemu_mutex_unlock(&stream->mtx);
        return VIRTIO_SND_S_BAD_MSG;
    }

    stream->buffer_bytes = req->buffer_bytes;
    stream->period_bytes = req->period_bytes;
    stream->format = VIRTIO_SND_PACK_FORMAT(req->channels, req->format, req->rate);

    qemu_mutex_unlock(&stream->mtx);
    return VIRTIO_SND_S_OK;
}

static int calc_stream_adjustment_bytes(const VirtIOSoundPCMStream *stream,
                                        uint64_t elapsed_usec) {
    const uint64_t elapsed_nf = elapsed_usec * (uint64_t)stream->freq_hz / 1000000ul;
    const uint64_t consumed_nf = stream->frames_sent + stream->frames_skipped;

    return (consumed_nf < elapsed_nf) ? (stream->frame_size * (elapsed_nf - consumed_nf)) : 0;
}

static int calc_drop_bytes_out(VirtIOSoundPCMStream *stream) {
    return calc_stream_adjustment_bytes(stream,
                                        AUD_get_elapsed_usec_out(stream->voice.out,
                                                                 &stream->start_timestamp));
}

static int calc_insert_bytes_in(VirtIOSoundPCMStream *stream) {
    return calc_stream_adjustment_bytes(stream,
                                        AUD_get_elapsed_usec_in(stream->voice.in,
                                                                &stream->start_timestamp));
}

static void stream_out_cb_locked(VirtIOSoundPCMStream *stream, int avail) {
    SWVoiceOut *const voice = stream->voice.out;
    VirtIOSoundRingBuffer *const pcm_buf = &stream->pcm_buf;
    VirtIOSound *const snd = stream->snd;
    VirtQueue *const tx_vq = snd->tx_vq;
    const int frame_size = stream->frame_size;
    bool notify_vq = false;
    int drop_bytes;
    uint8_t scratch[2048];

    while (avail >= frame_size) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            VirtQueueElement *const e = item->el;
            uint32_t latency_bytes = 0;
            int sz = MIN(avail, item->size);

            while (sz >= frame_size) {
                const int nf = MIN(sz, sizeof(scratch)) / frame_size;
                const int nb = nf * frame_size;
                int sent_bytes;

                iov_to_buf(e->out_sg, e->out_num, item->pos, scratch, nb);
                sent_bytes = AUD_write(voice, scratch, nb);
                if (sent_bytes <= 0) {
                    goto done;
                }

                item->pos += sent_bytes;
                item->size -= sent_bytes;
                avail -= sent_bytes;
                sz -= sent_bytes;
                latency_bytes = update_output_latency_bytes(stream, -sent_bytes);
                stream->frames_sent += (sent_bytes / frame_size);
            }

            if (item->size < frame_size) {
                vq_consume_element(
                    tx_vq, e,
                    el_send_pcm_status(e, VIRTIO_SND_S_OK, latency_bytes));
                ring_buffer_pop(pcm_buf);
                notify_vq = true;
            }
        } else {
            memset(scratch, 0, MIN(avail, sizeof(scratch)));
            while (avail > frame_size) {
                const int32_t nf = MIN(avail, sizeof(scratch)) / frame_size;
                const int32_t nb = nf * frame_size;
                const int32_t sent_bytes = AUD_write(voice, scratch, nb);

                avail -= sent_bytes;
                stream->frames_wasted += (sent_bytes / frame_size);
            }
            goto done;
        }
    }

    drop_bytes = calc_drop_bytes_out(stream);
    while (drop_bytes >= frame_size) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            const int nf = MIN(drop_bytes, item->size) / frame_size;
            const int nb = nf * frame_size;
            uint32_t latency_bytes;
            drop_bytes -= nb;
            item->pos += nb;
            item->size -= nb;
            latency_bytes = update_output_latency_bytes(stream, -nb);
            stream->frames_skipped += nf;

            if (item->size < frame_size) {
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
    if (notify_vq) {
        virtio_notify(&snd->parent, tx_vq);
    }
}

static void stream_out_cb(void *opaque, int avail) {
    VirtIOSoundPCMStream *const stream = (VirtIOSoundPCMStream *)opaque;

    qemu_mutex_lock(&stream->mtx);
    stream_out_cb_locked(stream, avail);
    qemu_mutex_unlock(&stream->mtx);
}

static void virtio_snd_in_cb(void *opaque, int avail) {
    /* TODO */
}

static void virtio_snd_process_tx(VirtQueue *vq, VirtQueueElement *e, VirtIOSound *snd) {
    VirtIOSoundRingBufferItem item;
    struct virtio_snd_pcm_xfer xfer;
    VirtIOSoundPCMStream *stream;
    const size_t req_size = iov_size(e->out_sg, e->out_num);

    if (req_size < sizeof(xfer)) {
        vq_consume_element(vq, e, 0);
        return;
    }

    iov_to_buf(e->out_sg, e->out_num, 0, &xfer, sizeof(xfer));
    if (xfer.stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        vq_consume_element(vq, e, el_send_pcm_status(e, VIRTIO_SND_S_IO_ERR, 0));
        return;
    }

    stream = &snd->streams[xfer.stream_id];

    item.el = e;
    item.pos = sizeof(xfer);
    item.size = req_size - sizeof(xfer);

    qemu_mutex_lock(&stream->mtx);

    if (ring_buffer_push(&stream->pcm_buf, &item)) {
        update_output_latency_bytes(stream, +item.size);
    } else {
        abort();
    }

    qemu_mutex_unlock(&stream->mtx);
}

static void stream_in_cb_locked(VirtIOSoundPCMStream *stream, int avail) {
    SWVoiceIn *const voice = stream->voice.in;
    const int frame_size = stream->frame_size;
    const int period_bytes = stream->period_bytes;
    const int item_size_bytes = period_bytes + sizeof(struct virtio_snd_pcm_status);
    VirtIOSoundRingBuffer *const pcm_buf = &stream->pcm_buf;
    VirtIOSound *const snd = stream->snd;
    VirtQueue *const rx_vq = snd->rx_vq;
    bool notify_vq = false;
    int insert_bytes;
    char scratch[2048];

    while (avail >= frame_size) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            VirtQueueElement *const e = item->el;
            int sz = MIN(avail, item_size_bytes - item->pos);

            while (sz >= frame_size) {
                const int nf = MIN(sz, sizeof(scratch)) / frame_size;
                const int nb = nf * frame_size;
                const int read_bytes = AUD_read(voice, scratch, nb);
                if (read_bytes <= 0) {
                    goto done;
                }

                iov_from_buf(e->in_sg, e->in_num, item->pos, scratch, read_bytes);
                sz -= read_bytes;
                avail -= read_bytes;
                item->pos += read_bytes;
                stream->frames_sent += (read_bytes / frame_size);

                if (item->pos == item_size_bytes) {
                    el_send_pcm_status(e, VIRTIO_SND_S_OK, avail);
                    vq_consume_element(rx_vq, e, item_size_bytes);
                    ring_buffer_pop(pcm_buf);
                    notify_vq = true;
                }
            }
        } else {
            while (avail >= frame_size) {
                const int nf = MIN(avail, sizeof(scratch)) / frame_size;
                const int nb = nf * frame_size;
                const int read_bytes = AUD_read(voice, scratch, nb);
                if (read_bytes <= 0) {
                    goto done;
                }
                avail -= read_bytes;
                stream->frames_wasted += (read_bytes / frame_size);
            }
            goto done;
        }
    }

    insert_bytes = calc_insert_bytes_in(stream);
    memset(scratch, 0, MIN(insert_bytes, sizeof(scratch)));
    while (insert_bytes >= frame_size) {
        VirtIOSoundRingBufferItem *item = ring_buffer_top(pcm_buf);
        if (item) {
            VirtQueueElement *const e = item->el;
            const int nf = MIN(insert_bytes, item_size_bytes - item->pos) / frame_size;
            const int nb = nf * frame_size;
            iov_from_buf(e->in_sg, e->in_num, item->pos, scratch, nb);
            insert_bytes -= nb;
            item->pos += nb;
            stream->frames_skipped += nf;

            if (item->pos == item_size_bytes) {
                el_send_pcm_status(e, VIRTIO_SND_S_OK, 0);
                vq_consume_element(rx_vq, e, item_size_bytes);
                ring_buffer_pop(pcm_buf);
                notify_vq = true;
            }
        } else {
            break;
        }
    }

done:
    if (notify_vq) {
        virtio_notify(&snd->parent, rx_vq);
    }
}

static void stream_in_cb(void *opaque, int avail) {
    VirtIOSoundPCMStream *const stream = (VirtIOSoundPCMStream *)opaque;

    qemu_mutex_lock(&stream->mtx);
    stream_in_cb_locked(stream, avail);
    qemu_mutex_unlock(&stream->mtx);
}

static void virtio_snd_process_rx(VirtQueue *vq, VirtQueueElement *e, VirtIOSound *snd) {
    VirtIOSoundRingBufferItem item;
    struct virtio_snd_pcm_xfer xfer;
    VirtIOSoundPCMStream *stream;
    const size_t req_size = iov_size(e->out_sg, e->out_num);

    if (req_size < sizeof(xfer)) {
        vq_consume_element(vq, e, 0);
        return;
    }

    iov_to_buf(e->out_sg, e->out_num, 0, &xfer, sizeof(xfer));
    if (xfer.stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        vq_consume_element(vq, e, el_send_pcm_status(e, VIRTIO_SND_S_IO_ERR, 0));
        return;
    }

    stream = &snd->streams[xfer.stream_id];

    item.el = e;
    item.pos = sizeof(xfer);
    item.size = 0;  /* not used in RX */

    qemu_mutex_lock(&stream->mtx);

    if (!ring_buffer_push(&stream->pcm_buf, &item)) {
        abort();
    }

    qemu_mutex_unlock(&stream->mtx);
}

static uint32_t
virtio_snd_process_ctl_pcm_prepare_impl(unsigned stream_id, VirtIOSound* snd) {
    VirtIOSoundPCMStream *stream;
    uint32_t r;

    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return VIRTIO_SND_S_BAD_MSG;
    }

    stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);

    if (!stream->voice.raw) {
        r = VIRTIO_SND_S_IO_ERR;
        goto done;
    }

    if (!stream->buffer_bytes) {
        r = VIRTIO_SND_S_BAD_MSG;
        goto done;
    }

    switch (stream->state) {
    case VIRTIO_PCM_STREAM_STATE_ENABLED:
        if (!ring_buffer_alloc(&stream->pcm_buf,
                               2 * stream->buffer_bytes / stream->period_bytes)) {
            r = VIRTIO_SND_S_BAD_MSG;
            goto done;
        }

        stream->freq_hz = format16_get_freq_hz(stream->format);
        stream->frame_size = format16_get_frame_size(stream->format);
        stream->buffer_frames = stream->buffer_bytes / stream->frame_size;
        stream->latency_bytes = 0;
        stream->state = VIRTIO_PCM_STREAM_STATE_PREPARED;
        r = VIRTIO_SND_S_OK;
        goto done;

    case VIRTIO_PCM_STREAM_STATE_PREPARED:
        r = VIRTIO_SND_S_OK;
        goto done;

    default:
        derror("%s:%d stream_id=%u state=%s(%d)", __func__, __LINE__,
               stream_id, stream_state_str(stream->state), stream->state);
        r = VIRTIO_SND_S_BAD_MSG;
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
        return VIRTIO_SND_S_BAD_MSG;
    }

    stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);
    if (stream->state != VIRTIO_PCM_STREAM_STATE_PREPARED) {
        qemu_mutex_unlock(&stream->mtx);
        return VIRTIO_SND_S_BAD_MSG;
    }
    virtio_snd_process_ctl_stream_unprepare_locked(stream);
    stream->state = VIRTIO_PCM_STREAM_STATE_ENABLED;
    qemu_mutex_unlock(&stream->mtx);

    return VIRTIO_SND_S_OK;
}

static uint32_t
virtio_snd_process_ctl_pcm_start_impl(unsigned stream_id, VirtIOSound* snd) {
    VirtIOSoundPCMStream *stream;
    uint32_t r;

    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return VIRTIO_SND_S_BAD_MSG;
    }


    stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);

    if (stream->state != VIRTIO_PCM_STREAM_STATE_PREPARED) {
        r = VIRTIO_SND_S_BAD_MSG;
        goto done;
    }

    if (!stream->voice.raw) {
        r = VIRTIO_SND_S_IO_ERR;
        goto done;
    } else if (is_output_stream(stream)) {
        SWVoiceOut *voice = stream->voice.out;
        AUD_set_active_out(voice, 1);
        AUD_init_time_stamp_out(voice, &stream->start_timestamp);
    } else {
        SWVoiceIn *voice = stream->voice.in;
        AUD_set_active_in(voice, 1);
        AUD_init_time_stamp_in(voice, &stream->start_timestamp);
    }

    stream->frames_sent = 0;
    stream->frames_skipped = 0;
    stream->frames_wasted = 0;
    stream->state = VIRTIO_PCM_STREAM_STATE_RUNNING;
    r = VIRTIO_SND_S_OK;

done:
    qemu_mutex_unlock(&stream->mtx);
    return r;
}

static uint32_t
virtio_snd_process_ctl_pcm_stop_impl(unsigned stream_id, VirtIOSound* snd) {
    VirtIOSoundPCMStream *stream;

    if (stream_id >= VIRTIO_SND_NUM_PCM_STREAMS) {
        return VIRTIO_SND_S_BAD_MSG;
    }

    stream = &snd->streams[stream_id];

    qemu_mutex_lock(&stream->mtx);

    if (stream->state != VIRTIO_PCM_STREAM_STATE_RUNNING) {
        qemu_mutex_unlock(&stream->mtx);
        return VIRTIO_SND_S_BAD_MSG;
    }

    virtio_snd_process_ctl_stream_stop_locked(stream);

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

    return el_send_status(e, VIRTIO_SND_S_BAD_MSG);
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
        derror("%s:%d unexpected ctl=%u", __func__, __LINE__, hdr->code);
        return el_send_status(e, VIRTIO_SND_S_NOT_SUPP);
    }

incomplete_request:
    return el_send_status(e, VIRTIO_SND_S_BAD_MSG);
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
}

/* device <- driver */
static void virtio_snd_handle_tx(VirtIODevice *vdev, VirtQueue *vq) {
    VirtIOSound *snd = VIRTIO_SND(vdev);

    while (virtio_queue_ready(vq)) {
        VirtQueueElement *e = (VirtQueueElement *)virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (e) {
            virtio_snd_process_tx(vq, e, snd);
        } else {
            break;
        }
    }
}

/* device -> driver */
static void virtio_snd_handle_rx(VirtIODevice *vdev, VirtQueue *vq) {
    VirtIOSound *snd = VIRTIO_SND(vdev);

    while (virtio_queue_ready(vq)) {
        VirtQueueElement *e = (VirtQueueElement *)virtqueue_pop(vq, sizeof(VirtQueueElement));
        if (e) {
            virtio_snd_process_rx(vq, e, snd);
        } else {
            break;
        }
    }
}

static void virtio_snd_device_realize(DeviceState *dev, Error **errp) {
    VirtIOSound *snd = VIRTIO_SND(dev);
    VirtIODevice *vdev = &snd->parent;
    int i;

    AUD_register_card(TYPE_VIRTIO_SND, &snd->card);

    for (i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        virtio_snd_stream_init(i, &snd->streams[i], snd);
    }

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
}

static void virtio_snd_device_unrealize(DeviceState *dev, Error **errp) {
    VirtIOSound *snd = VIRTIO_SND(dev);
    VirtIODevice *vdev = &snd->parent;
    unsigned i;

    for (i = 0; i < VIRTIO_SND_NUM_PCM_STREAMS; ++i) {
        VirtIOSoundPCMStream *stream = &snd->streams[i];

        virtio_snd_stream_disable(stream);
        virtio_snd_voice_close(snd, stream);
    }

    AUD_remove_card(&snd->card);

    virtio_del_queue(vdev, VIRTIO_SND_QUEUE_RX);
    virtio_del_queue(vdev, VIRTIO_SND_QUEUE_TX);
    virtio_del_queue(vdev, VIRTIO_SND_QUEUE_EVENT);
    virtio_del_queue(vdev, VIRTIO_SND_QUEUE_CTL);
    virtio_cleanup(vdev);
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

static void virtio_snd_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    set_bit(DEVICE_CATEGORY_SOUND, dc->categories);
    dc->vmsd = &virtio_snd_vmstate;

    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);
    vdc->realize = &virtio_snd_device_realize;
    vdc->unrealize = &virtio_snd_device_unrealize;
    vdc->get_features = &virtio_snd_device_get_features;
    vdc->get_config = &virtio_snd_get_config;
    vdc->set_status = &virtio_snd_set_status;
}

static int virtio_snd_and_codec_init(PCIBus *bus) {
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
    pci_register_soundhw(TYPE_VIRTIO_SND, "Virtio sound", &virtio_snd_and_codec_init);
}

type_init(virtio_snd_register_types);
