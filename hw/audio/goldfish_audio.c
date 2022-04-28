/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "qemu/osdep.h"
#include "hw/hw.h"
#include "audio/audio.h"
#include "hw/sysbus.h"
#include "qemu/error-report.h"
#include "trace.h"

#define TYPE_GOLDFISH_AUDIO "goldfish_audio"
#define GOLDFISH_AUDIO(obj) OBJECT_CHECK(struct goldfish_audio_state, (obj), TYPE_GOLDFISH_AUDIO)

enum {
	/* audio status register */
	AUDIO_INT_STATUS	= 0x00,
	/* set this to enable IRQ */
	AUDIO_INT_ENABLE	= 0x04,
	/* set these to specify buffer addresses */
	AUDIO_SET_WRITE_BUFFER_1 = 0x08,
	AUDIO_SET_WRITE_BUFFER_2 = 0x0C,
	/* set number of bytes in buffer to write */
	AUDIO_WRITE_BUFFER_1  = 0x10,
	AUDIO_WRITE_BUFFER_2  = 0x14,

	/* true if audio input is supported */
	AUDIO_READ_SUPPORTED = 0x18,
	/* buffer to use for audio input */
	AUDIO_SET_READ_BUFFER = 0x1C,

	/* driver writes number of bytes to read */
	AUDIO_START_READ  = 0x20,

	/* number of bytes available in read buffer */
	AUDIO_READ_BUFFER_AVAILABLE  = 0x24,

	/* for 64-bit guest CPUs only */
	AUDIO_SET_WRITE_BUFFER_1_HIGH = 0x28,
	AUDIO_SET_WRITE_BUFFER_2_HIGH = 0x30,
	AUDIO_SET_READ_BUFFER_HIGH = 0x34,

	/* AUDIO_INT_STATUS bits */

	/* this bit set when it is safe to write more bytes to the buffer */
	AUDIO_INT_WRITE_BUFFER_1_EMPTY	= 1U << 0,
	AUDIO_INT_WRITE_BUFFER_2_EMPTY	= 1U << 1,
	AUDIO_INT_READ_BUFFER_FULL      = 1U << 2,
};

struct goldfish_audio_buff {
    uint64_t  address;
    uint32_t  length;
    uint8_t*  data;
    uint32_t  capacity;
    uint32_t  offset;
};


struct goldfish_audio_state {
    SysBusDevice parent;

    bool input;
    bool output;

    MemoryRegion iomem;
    qemu_irq irq;

    // buffer flags
    uint32_t int_status;
    // irq enable mask for int_status
    uint32_t int_enable;

    // number of bytes available in the read buffer
    uint32_t read_buffer_available;

    // set to 0 or 1 to indicate which buffer we are writing from, or -1 if both buffers are empty
    int8_t current_buffer;

    // current data to write
    struct goldfish_audio_buff  out_buffs[2];
    struct goldfish_audio_buff  in_buff;

    // for QEMU sound output
    QEMUSoundCard card;
    SWVoiceOut *voice;
    SWVoiceIn*  voicein;
};

static void
goldfish_audio_buff_init( struct goldfish_audio_buff*  b )
{
    b->address  = 0;
    b->length   = 0;
    b->data     = NULL;
    b->capacity = 0;
    b->offset   = 0;
}

static void
goldfish_audio_buff_reset( struct goldfish_audio_buff*  b )
{
    b->offset = 0;
    b->length = 0;
}

static uint32_t
goldfish_audio_buff_length( struct goldfish_audio_buff*  b )
{
    return b->length;
}

static void
goldfish_audio_buff_ensure( struct goldfish_audio_buff*  b, uint32_t  size )
{
    if (b->capacity < size) {
        b->data     = g_realloc(b->data, size);
        b->capacity = size;
    }
}

static void
goldfish_audio_buff_set_address( struct goldfish_audio_buff*  b, uint32_t  addr )
{
    b->address = deposit64(b->address, 0, 32, addr);
}

static void
goldfish_audio_buff_set_address_high( struct goldfish_audio_buff*  b,
                                      uint32_t  addr )
{
    b->address = deposit64(b->address, 32, 32, addr);
}

static void
goldfish_audio_buff_set_length( struct goldfish_audio_buff*  b, uint32_t  len )
{
    b->length = len;
    b->offset = 0;
    goldfish_audio_buff_ensure(b, len);
}

static void
goldfish_audio_buff_read( struct goldfish_audio_buff*  b )
{
    cpu_physical_memory_read(b->address, b->data, b->length);
}

static void
goldfish_audio_buff_write( struct goldfish_audio_buff*  b )
{
    cpu_physical_memory_write(b->address, b->data, b->length);
}

static int
goldfish_audio_buff_available( struct goldfish_audio_buff*  b )
{
    return b->length - b->offset;
}

static void goldfish_audio_in_callback(void *opaque, int avail);

SWVoiceIn* goldfish_audio_get_voicein(struct goldfish_audio_state* s)
{
    SWVoiceIn* voice = s->voicein;
    if (!voice && s->input) {
        struct audsettings as;

        as.freq       = 8000;
        as.nchannels  = 1;
        as.fmt        = AUD_FMT_S16;
        as.endianness = AUDIO_HOST_ENDIANNESS;

        voice = AUD_open_in(
            &s->card,
            NULL,
            "goldfish_audio_in",
            s,
            goldfish_audio_in_callback,
            &as);

        if (voice) {
            goldfish_audio_buff_init(&s->in_buff);
        } else {
            error_report("warning: opening audio input failed");
        }

        s->voicein = voice;
    }

    return voice;
}

static int
goldfish_audio_buff_recv( struct goldfish_audio_buff*  b, int  avail, struct goldfish_audio_state*  s )
{
    int     missing = b->length - b->offset;
    int     avail2 = (avail > missing) ? missing : avail;
    int     read;

    read = AUD_read(goldfish_audio_get_voicein(s), b->data + b->offset, avail2);
    if (read == 0)
        return 0;

    if (avail2 > 0)
        trace_goldfish_audio_buff_recv(avail2, read);

    cpu_physical_memory_write(b->address + b->offset, b->data + b->offset,
            read);
    b->offset += read;

    return read;
}

/* update this whenever you change the goldfish_audio_state structure */
#define  AUDIO_STATE_SAVE_VERSION  3

static void enable_audio(struct goldfish_audio_state *s, int enable)
{
    SWVoiceIn* voicein;

    if (s->voice != NULL) {
        goldfish_audio_buff_reset( &s->out_buffs[0] );
        goldfish_audio_buff_reset( &s->out_buffs[1] );
    }

    voicein = goldfish_audio_get_voicein(s);

    if (voicein) {
        AUD_set_active_in(voicein, (enable & AUDIO_INT_READ_BUFFER_FULL) != 0);
        goldfish_audio_buff_reset( &s->in_buff );
    }
    s->current_buffer = -1;
}

static void start_read(struct goldfish_audio_state *s, uint32_t count)
{
    //printf( "... goldfish audio start_read, count=%d\n", count );
    goldfish_audio_buff_set_length( &s->in_buff, count );
    s->read_buffer_available = count;
}

static uint64_t goldfish_audio_read(void *opaque, hwaddr offset, unsigned size)
{
    uint64_t ret;
    struct goldfish_audio_state *s = opaque;
    switch(offset) {
        case AUDIO_INT_STATUS:
            // return current buffer status flags
            ret = s->int_status & s->int_enable;
            if(ret) {
                qemu_irq_lower(s->irq);
            }
            return ret;

	case AUDIO_READ_SUPPORTED:
            trace_goldfish_audio_memory_read("AUDIO_READ_SUPPORTED", (s->input != 0));
            return (s->input != 0);

	case AUDIO_READ_BUFFER_AVAILABLE:
            trace_goldfish_audio_memory_read("AUDIO_READ_BUFFER_AVAILABLE",
               s->read_buffer_available);
            goldfish_audio_buff_write( &s->in_buff );
	    return s->read_buffer_available;

        default:
            error_report ("goldfish_audio_read: Bad offset 0x" TARGET_FMT_plx,
                    offset);
            return 0;
    }
}

static void goldfish_audio_write_buffer(struct goldfish_audio_state *s,
        unsigned int buf, uint32_t length)
{
    if (s->current_buffer == -1)
        s->current_buffer = buf;
    goldfish_audio_buff_set_length(&s->out_buffs[buf], length);
    goldfish_audio_buff_read(&s->out_buffs[buf]);
    AUD_set_active_out(s->voice, 1);
}

static void goldfish_audio_write(void *opaque, hwaddr offset, uint64_t val,
        unsigned size)
{
    struct goldfish_audio_state *s = opaque;

    switch(offset) {
        case AUDIO_INT_ENABLE:
            /* enable buffer empty interrupts */
            trace_goldfish_audio_memory_write("AUDIO_INT_ENABLE", val);
            enable_audio(s, val);
            s->int_enable = val;
            s->int_status = (AUDIO_INT_WRITE_BUFFER_1_EMPTY | AUDIO_INT_WRITE_BUFFER_2_EMPTY);
            qemu_set_irq(s->irq, s->int_status & s->int_enable);
            break;
        case AUDIO_SET_WRITE_BUFFER_1:
            /* save pointer to buffer 1 */
            trace_goldfish_audio_memory_write("AUDIO_SET_WRITE_BUFFER_1", val);
            goldfish_audio_buff_set_address( &s->out_buffs[0], val );
            break;
        case AUDIO_SET_WRITE_BUFFER_1_HIGH:
            /* save pointer to buffer 1 */
            trace_goldfish_audio_memory_write("AUDIO_SET_WRITE_BUFFER_1_HIGH",
                                              val);
            goldfish_audio_buff_set_address_high( &s->out_buffs[0], val );
            break;
        case AUDIO_SET_WRITE_BUFFER_2:
            /* save pointer to buffer 2 */
            trace_goldfish_audio_memory_write("AUDIO_SET_WRITE_BUFFER_2", val);
            goldfish_audio_buff_set_address( &s->out_buffs[1], val );
            break;
        case AUDIO_SET_WRITE_BUFFER_2_HIGH:
            /* save pointer to buffer 2 */
            trace_goldfish_audio_memory_write("AUDIO_SET_WRITE_BUFFER_2_HIGH",
                                              val);
            goldfish_audio_buff_set_address_high( &s->out_buffs[1], val );
            break;
        case AUDIO_WRITE_BUFFER_1:
            /* record that data in buffer 1 is ready to write */
            trace_goldfish_audio_memory_write("AUDIO_WRITE_BUFFER_1", val);
            goldfish_audio_write_buffer(s, 0, val);
            s->int_status &= ~AUDIO_INT_WRITE_BUFFER_1_EMPTY;
            break;
        case AUDIO_WRITE_BUFFER_2:
            /* record that data in buffer 2 is ready to write */
            trace_goldfish_audio_memory_write("AUDIO_WRITE_BUFFER_2", val);
            goldfish_audio_write_buffer(s, 1, val);
            s->int_status &= ~AUDIO_INT_WRITE_BUFFER_2_EMPTY;
            break;

        case AUDIO_SET_READ_BUFFER:
            /* save pointer to the read buffer */
            goldfish_audio_buff_set_address( &s->in_buff, val );
            trace_goldfish_audio_memory_write("AUDIO_SET_READ_BUFFER", val);
            break;

        case AUDIO_START_READ:
            trace_goldfish_audio_memory_write("AUDIO_START_READ", val);
            start_read(s, val);
            s->int_status &= ~AUDIO_INT_READ_BUFFER_FULL;
            qemu_set_irq(s->irq, s->int_status & s->int_enable);
            break;

        case AUDIO_SET_READ_BUFFER_HIGH:
            /* save pointer to the read buffer */
            goldfish_audio_buff_set_address_high( &s->in_buff, val );
            trace_goldfish_audio_memory_write("AUDIO_SET_READ_BUFFER_HIGH",
                                              val);
            break;

        default:
            error_report ("goldfish_audio_write: Bad offset 0x" TARGET_FMT_plx,
                    offset);
    }
}

static bool goldfish_audio_flush(struct goldfish_audio_state *s, int buf,
        int *free, uint32_t *new_status)
{
    struct goldfish_audio_buff *b = &s->out_buffs[buf];
    int to_write = audio_MIN(b->length, *free);

    if (!to_write)
        return false;

    int written = AUD_write(s->voice, b->data + b->offset, to_write);
    if (!written)
        return false;

    b->offset += written;
    b->length -= written;
    *free -= written;
    trace_goldfish_audio_buff_send(written, buf + 1);

    /* If buffer is drained, set corresponding status bit. */
    if (!goldfish_audio_buff_length(b))
        *new_status |= buf ? AUDIO_INT_WRITE_BUFFER_2_EMPTY :
                AUDIO_INT_WRITE_BUFFER_1_EMPTY;

    return true;
}

static void goldfish_audio_callback(void *opaque, int free)
{
    struct goldfish_audio_state *s = opaque;
    uint32_t new_status = 0;

    if (s->current_buffer != -1) {
        int8_t i = s->current_buffer;
        int8_t j = (i + 1) % 2;

        goldfish_audio_flush(s, i, &free, &new_status);
        goldfish_audio_flush(s, j, &free, &new_status);

        if (!goldfish_audio_buff_length(&s->out_buffs[i])) {
            if (goldfish_audio_buff_length(&s->out_buffs[j])) {
                s->current_buffer = j;
            } else {
                s->current_buffer = -1;
            }
        }
    }

    if (free) /* out of samples, pause playback */
        AUD_set_active_out(s->voice, 0);

    if (new_status && new_status != s->int_status) {
        s->int_status |= new_status;
        qemu_set_irq(s->irq, s->int_status & s->int_enable);
    }
}

static void
goldfish_audio_in_callback(void *opaque, int avail)
{
    struct goldfish_audio_state *s = opaque;
    int new_status = 0;

    if (goldfish_audio_buff_available( &s->in_buff ) == 0 )
        return;

    while (avail > 0) {
        int  read = goldfish_audio_buff_recv( &s->in_buff, avail, s );
        if (read == 0)
            break;

        avail -= read;

        if (goldfish_audio_buff_available( &s->in_buff) == 0) {
            new_status |= AUDIO_INT_READ_BUFFER_FULL;
            trace_goldfish_audio_buff_full(
              goldfish_audio_buff_length( &s->in_buff ));
            break;
        }
    }

    if (new_status && new_status != s->int_status) {
        s->int_status |= new_status;
        qemu_set_irq(s->irq, s->int_status & s->int_enable);
    }
}

static const MemoryRegionOps goldfish_audio_iomem_ops = {
    .read = goldfish_audio_read,
    .write = goldfish_audio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void goldfish_audio_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    struct goldfish_audio_state *s = GOLDFISH_AUDIO(dev);

    /* MMIO must be set up regardless of whether the initialization of input
     * and output voices is successful or not. Otherwise, an assertion error
     * will occur in sysbus_mmio_map_common() (hw/core/sysbus.c).
     */
    memory_region_init_io(&s->iomem, OBJECT(s), &goldfish_audio_iomem_ops, s,
            "goldfish_audio", 0x100);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);

    /* Skip all the rest if both audio input and output are disabled. */
    if (!s->output && !s->input) {
        return;
    }

    AUD_register_card( "goldfish_audio", &s->card);

    if (s->output) {
        struct audsettings as;

        as.freq = 44100;
        as.nchannels = 2;
        as.fmt = AUD_FMT_S16;
        as.endianness = AUDIO_HOST_ENDIANNESS;
        s->voice = AUD_open_out (
            &s->card,
            NULL,
            "goldfish_audio",
            s,
            goldfish_audio_callback,
            &as
            );
        if (!s->voice) {
            error_report("warning: opening audio output failed");
        } else {
            goldfish_audio_buff_init( &s->out_buffs[0] );
            goldfish_audio_buff_init( &s->out_buffs[1] );
        }
    }
}

static Property goldfish_audio_properties[] = {
    DEFINE_PROP_BOOL("input", struct goldfish_audio_state, input, true),
    DEFINE_PROP_BOOL("output", struct goldfish_audio_state, output, true),
    DEFINE_PROP_END_OF_LIST(),
};

static void goldfish_audio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_audio_realize;
    dc->desc = "goldfish audio";
    dc->props = goldfish_audio_properties;
}

static const TypeInfo goldfish_audio_info = {
    .name          = TYPE_GOLDFISH_AUDIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct goldfish_audio_state),
    .class_init    = goldfish_audio_class_init,
};

static void goldfish_audio_register(void)
{
    type_register_static(&goldfish_audio_info);
}

type_init(goldfish_audio_register);
