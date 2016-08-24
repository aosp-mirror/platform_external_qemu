/* Copyright (C) 2011 The Android Open Source Project
** Copyright (C) 2014 Linaro Limited
** Copyright (C) 2015 Intel Corporation
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** Description
**
** This device provides a virtual pipe device (originally called
** goldfish_pipe and latterly qemu_pipe). This allows the android
** running under the emulator to open a fast connection to the host
** for various purposes including the adb debug bridge and
** (eventually) the opengles pass-through. This file contains only the
** basic pipe infrastructure and a couple of test pipes. Additional
** pipes are registered with the android_pipe_add_type() call.
**
** Open Questions
**
** Since this was originally written there have been a number of other
** virtual devices added to QEMU using the virtio infrastructure. We
** should give some thought to if this needs re-writing to take
** advantage of that infrastructure to create the pipes.
*/
#include "hw/misc/android_pipe.h"

#include "android-qemu2-glue/utils/stream.h"
#include "android/emulation/android_pipe_device.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "qemu/error-report.h"

#include <assert.h>
#include <glib.h>

/* Set to > 0 for debug output */
#define PIPE_DEBUG 0

/* Set to 1 to debug i/o register reads/writes */
#define PIPE_DEBUG_REGS 0

#if PIPE_DEBUG >= 1
#define D(fmt, ...) \
    do { fprintf(stdout, "android_pipe: " fmt "\n", ## __VA_ARGS__); } while (0)
#else
#define D(fmt, ...)  do { /* nothing */ } while (0)
#endif

#if PIPE_DEBUG >= 2
#define DD(fmt, ...) \
    do { fprintf(stdout, "android_pipe: " fmt "\n", ## __VA_ARGS__); } while (0)
#else
#define DD(fmt, ...)  do { /* nothing */ } while (0)
#endif

#if PIPE_DEBUG_REGS >= 1
#  define DR(...)   D(__VA_ARGS__)
#else
#  define DR(...)   do { /* nothing */ } while (0)
#endif

#define E(fmt, ...)  \
    do { fprintf(stdout, "ERROR:" fmt "\n", ## __VA_ARGS__); } while (0)

#define APANIC(...)                     \
    do {                                \
        error_report(__VA_ARGS__);      \
        exit(1);                        \
    } while (0)

/* The following definitions must match those in the kernel driver
 * found at android.googlesource.com/kernel/goldfish.git
 *
 *    drivers/platform/goldfish/goldfish_pipe.c
 */

/* pipe device registers */
#define PIPE_REG_COMMAND            0x00  /* write: value = command */
#define PIPE_REG_STATUS             0x04  /* read */
#define PIPE_REG_CHANNEL            0x08  /* read/write: channel id */
#define PIPE_REG_SIZE               0x0c  /* read/write: buffer size */
#define PIPE_REG_ADDRESS            0x10  /* write: physical address */
#define PIPE_REG_WAKES              0x14  /* read: wake flags */
/* read/write: parameter buffer address */
#define PIPE_REG_PARAMS_ADDR_LOW     0x18
#define PIPE_REG_PARAMS_ADDR_HIGH    0x1c
/* write: access with paremeter buffer */
#define PIPE_REG_ACCESS_PARAMS       0x20
#define PIPE_REG_VERSION             0x24 /* read: device version */
#define PIPE_REG_CHANNEL_HIGH        0x30 /* read/write: high 32 bit channel id */
#define PIPE_REG_ADDRESS_HIGH        0x34 /* write: high 32 bit physical address */

/* list of commands for PIPE_REG_COMMAND */
#define PIPE_CMD_OPEN               1  /* open new channel */
#define PIPE_CMD_CLOSE              2  /* close channel (from guest) */
#define PIPE_CMD_POLL               3  /* poll read/write status */

/* The following commands are related to write operations */
#define PIPE_CMD_WRITE_BUFFER       4  /* send a user buffer to the emulator */
#define PIPE_CMD_WAKE_ON_WRITE      5  /* tell the emulator to wake us when writing is possible */

/* The following commands are related to read operations, they must be
 * listed in the same order than the corresponding write ones, since we
 * will use (CMD_READ_BUFFER - CMD_WRITE_BUFFER) as a special offset
 * in qemu_pipe_read_write() below.
 */
#define PIPE_CMD_READ_BUFFER        6  /* receive a page-contained buffer from the emulator */
#define PIPE_CMD_WAKE_ON_READ       7  /* tell the emulator to wake us when reading is possible */

struct access_params_32 {
    uint32_t channel;   /* 0x00 */
    uint32_t size;      /* 0x04 */
    uint32_t address;   /* 0x08 */
    uint32_t cmd;       /* 0x0c */
    uint32_t result;    /* 0x10 */
    /* reserved for future extension */
    uint32_t flags;     /* 0x14 */
};

struct access_params_64 {
    uint64_t channel;   /* 0x00 */
    uint32_t size;      /* 0x08 */
    uint64_t address;   /* 0x0c */
    uint32_t cmd;       /* 0x14 */
    uint32_t result;    /* 0x18 */
    /* reserved for future extension */
    uint32_t flags;     /* 0x1c */
};

union access_params {
    struct access_params_32 aps32;
    struct access_params_64 aps64;
};

/* Maximum length of pipe service name, in characters (excluding final 0) */
#define MAX_PIPE_SERVICE_NAME_SIZE  255

/* from AOSP version include/hw/android/goldfish/device.h
 * FIXME?: needs to use proper qemu abstractions
 */
static inline void uint64_set_low(uint64_t *addr, uint32 value)
{
    *addr = (*addr & ~(0xFFFFFFFFULL)) | value;
}

static inline void uint64_set_high(uint64_t *addr, uint32 value)
{
    *addr = (*addr & 0xFFFFFFFFULL) | ((uint64_t)value << 32);
}

#define TYPE_ANDROID_PIPE "android_pipe"
#define ANDROID_PIPE(obj) \
    OBJECT_CHECK(AndroidPipeState, (obj), TYPE_ANDROID_PIPE)

typedef struct PipeDevice  PipeDevice;

typedef struct {
    SysBusDevice parent;
    MemoryRegion iomem;
    qemu_irq irq;

    /* TODO: roll into shared state */
    PipeDevice *dev;
} AndroidPipeState;


/***********************************************************************
 ***********************************************************************
 *****
 *****    P I P E   C O N N E C T I O N S
 *****
 *****/

typedef struct HwPipe {
    struct HwPipe               *next;
    struct HwPipe               *wanted_next;
    struct HwPipe               *wanted_prev;
    PipeDevice                  *device;
    uint64_t                    channel; /* opaque kernel handle */
    unsigned char               wanted;
    char                        closed;
    void                        *pipe;
} HwPipe;

static HwPipe* hwpipe_new0(struct PipeDevice* device) {
    HwPipe* hwp;
    ANEW0(hwp);
    hwp->device = device;
    return hwp;
}

static void hwpipe_free(HwPipe* hwp) {
    android_pipe_guest_close(hwp->pipe);
    free(hwp);
}

static unsigned char get_and_clear_pipe_wanted(HwPipe* pipe) {
    unsigned char val = pipe->wanted;
    pipe->wanted = 0;
    return val;
}

static void set_pipe_wanted_bits(HwPipe* pipe, unsigned char val) {
    pipe->wanted |= val;
}

static HwPipe*
pipe_new0(PipeDevice* dev)
{
    HwPipe*  pipe;
    pipe = g_malloc0(sizeof(HwPipe));
    pipe->device = dev;
    return pipe;
}

static HwPipe*
pipe_new(uint64_t channel, PipeDevice* dev)
{
    HwPipe*  pipe = pipe_new0(dev);
    pipe->channel = channel;
    pipe->pipe  = android_pipe_guest_open(pipe);
    return pipe;
}

static void pipe_free(HwPipe* pipe)
{
    android_pipe_guest_close(pipe->pipe);
    g_free(pipe);
}

/***********************************************************************
 ***********************************************************************
 *****
 *****    G O L D F I S H   P I P E   D E V I C E
 *****
 *****/

struct PipeDevice {
    AndroidPipeState *ps;       // FIXME: backlink to instance state

    // The list of all pipes.
    HwPipe*  pipes;
    // The list of the pipes that signalled some 'wanted' state.
    HwPipe*  wanted_pipes_first;
    // A cached wanted pipe after the guest's _CHANNEL_HIGH call. We need to
    // return the very same pipe during the following *_CHANNEL call.
    HwPipe*  wanted_pipe_after_channel_high;

    // Cache of the pipes by channel for a faster lookup.
    GHashTable* pipes_by_channel;

    // i/o registers
    uint64_t  address;
    uint32_t  size;
    uint32_t  status;
    uint64_t  channel;
    uint32_t  wakes;
    uint64_t  params_addr;
};

// hashtable-related functions
// 64-bit emulator just casts uint64_t to gpointer to get a key for the
// GLib's hash table. 32-bit one has to dynamically allocate an 8-byte chunk
// of memory and copy the channel value there, storing pointer as a key.
static uint64_t hash_channel_from_key(gconstpointer key) {
#ifdef __x86_64__
    // keys are just channels
    return (uint64_t)key;
#else
    // keys are pointers to channels
    return *(uint64_t*)key;
#endif
}

static gpointer hash_create_key_from_channel(uint64_t channel) {
#ifdef __x86_64__
    return (gpointer)channel;
#else
    uint64_t* on_heap = (uint64_t*)malloc(sizeof(channel));
    if (!on_heap) {
        APANIC("%s: failed to allocate RAM for a channel value\n", __func__);
    }
    *on_heap = channel;
    return on_heap;
#endif
}

static gconstpointer hash_cast_key_from_channel(const uint64_t* channel) {
#ifdef __x86_64__
    return (gconstpointer)*channel;
#else
    return (gconstpointer)channel;
#endif
}

static guint hash_channel(gconstpointer key) {
    uint64_t channel = hash_channel_from_key(key);
    return (guint)(channel ^ (channel >> 6));
}

static gboolean hash_channel_equal(gconstpointer a, gconstpointer b) {
    return hash_channel_from_key(a) == hash_channel_from_key(b);
}

#ifdef __x86_64__
// we don't need to free channels in 64-bit emulator as we store them in-place
static void (*hash_channel_destroy)(gpointer a) = NULL;
#else
static void hash_channel_destroy(gpointer a) {
    free((uint64_t*)a);
}
#endif

// Wanted pipe linked list operations
static HwPipe* wanted_pipes_pop_first(PipeDevice* dev) {
    if (dev->wanted_pipe_after_channel_high) {
        HwPipe* val = dev->wanted_pipe_after_channel_high;
        dev->wanted_pipe_after_channel_high = NULL;
        return val;
    }
    HwPipe* pipe = dev->wanted_pipes_first;
    if (pipe) {
        dev->wanted_pipes_first = pipe->wanted_next;
        assert(pipe->wanted_prev == NULL);
        pipe->wanted_next = NULL;
        if (dev->wanted_pipes_first) {
            dev->wanted_pipes_first->wanted_prev = NULL;
        }
    }
    return pipe;
}

static void wanted_pipes_add(PipeDevice* dev, HwPipe* pipe) {
    if (!pipe->wanted_next && !pipe->wanted_prev
        && dev->wanted_pipes_first != pipe
        && dev->wanted_pipe_after_channel_high != pipe) {
        pipe->wanted_next = dev->wanted_pipes_first;
        if (dev->wanted_pipes_first) {
            dev->wanted_pipes_first->wanted_prev = pipe;
        }
        dev->wanted_pipes_first = pipe;
    }
}

static void wanted_pipes_remove(PipeDevice* dev, HwPipe* pipe) {
    if (dev->wanted_pipe_after_channel_high == pipe) {
        dev->wanted_pipe_after_channel_high = NULL;
    }

    if (pipe->wanted_next) {
        pipe->wanted_next->wanted_prev = pipe->wanted_prev;
    }
    if (pipe->wanted_prev) {
        pipe->wanted_prev->wanted_next = pipe->wanted_next;
        pipe->wanted_prev = NULL;
    } else if (dev->wanted_pipes_first == pipe) {
        dev->wanted_pipes_first = pipe->wanted_next;
    }
    pipe->wanted_next = NULL;
}

/* Update this version number if the device's interface changes. */
#define PIPE_DEVICE_VERSION  1

/* Map the guest buffer specified by the guest paddr 'phys'.
 * Returns a host pointer which should be unmapped later via
 * cpu_physical_memory_unmap(), or NULL if mapping failed (likely
 * because the paddr doesn't actually point at RAM).
 * Note that for RAM the "mapping" process doesn't actually involve a
 * data copy.
 */
static void *map_guest_buffer(hwaddr phys, size_t size, int is_write)
{
    hwaddr l = size;
    void *ptr;

    ptr = cpu_physical_memory_map(phys, &l, is_write);
    if (!ptr) {
        /* Can't happen for RAM */
        return NULL;
    }
    if (l != size) {
        /* This will only happen if the address pointed at non-RAM,
         * or if the size means the buffer end is beyond the end of
         * the RAM block.
         */
        cpu_physical_memory_unmap(ptr, l, 0, 0);
        return NULL;
    }

    return ptr;
}

static void pipeDevice_doCommand(PipeDevice* dev, uint32_t command) {
    HwPipe* pipe = (HwPipe*)g_hash_table_lookup(
            dev->pipes_by_channel, hash_cast_key_from_channel(&dev->channel));

    /* Check that we're referring a known pipe channel */
    if (command != PIPE_CMD_OPEN && pipe == NULL) {
        dev->status = PIPE_ERROR_INVAL;
        return;
    }

    /* If the pipe is closed by the host, return an error */
    if (pipe != NULL && pipe->closed && command != PIPE_CMD_CLOSE) {
        dev->status = PIPE_ERROR_IO;
        return;
    }

    switch (command) {
    case PIPE_CMD_OPEN:
        DD("%s: CMD_OPEN channel=0x%llx", __FUNCTION__, (unsigned long long)dev->channel);
        if (pipe != NULL) {
            dev->status = PIPE_ERROR_INVAL;
            break;
        }
        pipe = pipe_new(dev->channel, dev);
        pipe->next = dev->pipes;
        dev->pipes = pipe;
        dev->status = 0;
        g_hash_table_insert(dev->pipes_by_channel,
                            hash_create_key_from_channel(dev->channel), pipe);
        break;

    case PIPE_CMD_CLOSE: {
        DD("%s: CMD_CLOSE channel=0x%llx", __FUNCTION__, (unsigned long long)dev->channel);
        // Remove from device's lists.
        // This linear lookup is potentially slow, but we don't delete pipes
        // often enough for it to become noticable.
        HwPipe** pnode = &dev->pipes;
        while (*pnode && *pnode != pipe) {
            pnode = &(*pnode)->next;
        }
        if (!*pnode) {
            dev->status = PIPE_ERROR_INVAL;
            break;
        }
        *pnode = pipe->next;
        pipe->next = NULL;
        g_hash_table_remove(dev->pipes_by_channel,
                            hash_cast_key_from_channel(&pipe->channel));
        wanted_pipes_remove(dev, pipe);

        pipe_free(pipe);
        break;
    }

    case PIPE_CMD_POLL:
        dev->status = android_pipe_guest_poll(pipe->pipe);
        DD("%s: CMD_POLL > status=%d", __FUNCTION__, dev->status);
        break;

    case PIPE_CMD_READ_BUFFER: {
        /* Translate guest physical address into emulator memory. */
        AndroidPipeBuffer  buffer;
        buffer.data = map_guest_buffer(dev->address, dev->size, 1);
        if (!buffer.data) {
            dev->status = PIPE_ERROR_INVAL;
            break;
        }
        buffer.size = dev->size;
        dev->status = android_pipe_guest_recv(pipe->pipe, &buffer, 1);
        DD("%s: CMD_READ_BUFFER channel=0x%llx address=0x%16llx size=%d > status=%d",
           __FUNCTION__, (unsigned long long)dev->channel, (unsigned long long)dev->address,
           dev->size, dev->status);
        cpu_physical_memory_unmap(buffer.data, dev->size, 1, dev->size);
        break;
    }

    case PIPE_CMD_WRITE_BUFFER: {
        /* Translate guest physical address into emulator memory. */
        AndroidPipeBuffer  buffer;
        buffer.data = map_guest_buffer(dev->address, dev->size, 0);
        if (!buffer.data) {
            dev->status = PIPE_ERROR_INVAL;
            break;
        }
        buffer.size = dev->size;
        dev->status = android_pipe_guest_send(pipe->pipe, &buffer, 1);
        DD("%s: CMD_WRITE_BUFFER channel=0x%llx address=0x%16llx size=%d > status=%d",
           __FUNCTION__, (unsigned long long)dev->channel, (unsigned long long)dev->address,
           dev->size, dev->status);
        cpu_physical_memory_unmap(buffer.data, dev->size, 0, dev->size);
        break;
    }

    case PIPE_CMD_WAKE_ON_READ:
        DD("%s: CMD_WAKE_ON_READ channel=0x%llx", __FUNCTION__, (unsigned long long)dev->channel);
        if ((pipe->wanted & PIPE_WAKE_READ) == 0) {
            pipe->wanted |= PIPE_WAKE_READ;
            android_pipe_guest_wake_on(pipe->pipe, pipe->wanted);
        }
        dev->status = 0;
        break;

    case PIPE_CMD_WAKE_ON_WRITE:
        DD("%s: CMD_WAKE_ON_WRITE channel=0x%llx", __FUNCTION__, (unsigned long long)dev->channel);
        if ((pipe->wanted & PIPE_WAKE_WRITE) == 0) {
            pipe->wanted |= PIPE_WAKE_WRITE;
            android_pipe_guest_wake_on(pipe->pipe, pipe->wanted);
        }
        dev->status = 0;
        break;

    default:
        D("%s: command=%d (0x%x)\n", __FUNCTION__, command, command);
    }
}

static void pipe_dev_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    AndroidPipeState *state = (AndroidPipeState *) opaque;
    PipeDevice *s = state->dev;

    DR("%s: offset = 0x%" HWADDR_PRIx " value=%" PRIu64 "/0x%" PRIx64,
       __func__, offset, value, value);
    switch (offset) {
    case PIPE_REG_COMMAND:
        pipeDevice_doCommand(s, value);
        break;

    case PIPE_REG_SIZE:
        s->size = value;
        break;

    case PIPE_REG_ADDRESS:
        uint64_set_low(&s->address, value);
        break;

    case PIPE_REG_ADDRESS_HIGH:
        uint64_set_high(&s->address, value);
        break;

    case PIPE_REG_CHANNEL:
        uint64_set_low(&s->channel, value);
        break;

    case PIPE_REG_CHANNEL_HIGH:
        uint64_set_high(&s->channel, value);
        break;

    case PIPE_REG_PARAMS_ADDR_HIGH:
        uint64_set_high(&s->params_addr, value);
        break;

    case PIPE_REG_PARAMS_ADDR_LOW:
        uint64_set_low(&s->params_addr, value);
        break;

    case PIPE_REG_ACCESS_PARAMS:
    {
        union access_params aps;
        uint32_t cmd;
        bool is_64bit = true;

        /* Don't touch aps.result if anything wrong */
        if (s->params_addr == 0)
            break;

        cpu_physical_memory_read(s->params_addr, (void*)&aps, sizeof(aps.aps32));

        /* This auto-detection of 32bit/64bit ness relies on the
         * currently unused flags parameter. As the 32 bit flags
         * overlaps with the 64 bit cmd parameter. As cmd != 0 if we
         * find it as 0 it's 32bit
         */
        if (aps.aps32.flags == 0) {
            is_64bit = false;
        } else {
            cpu_physical_memory_read(s->params_addr, (void*)&aps, sizeof(aps.aps64));
        }

        if (is_64bit) {
            s->channel = aps.aps64.channel;
            s->size = aps.aps64.size;
            s->address = aps.aps64.address;
            cmd = aps.aps64.cmd;
        } else {
            s->channel = aps.aps32.channel;
            s->size = aps.aps32.size;
            s->address = aps.aps32.address;
            cmd = aps.aps32.cmd;
        }

        if ((cmd != PIPE_CMD_READ_BUFFER) && (cmd != PIPE_CMD_WRITE_BUFFER))
            break;

        pipeDevice_doCommand(s, cmd);

        if (is_64bit) {
            aps.aps64.result = s->status;
            cpu_physical_memory_write(s->params_addr, (void*)&aps, sizeof(aps.aps64));
        } else {
            aps.aps32.result = s->status;
            cpu_physical_memory_write(s->params_addr, (void*)&aps, sizeof(aps.aps32));
        }
    }
    break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: unknown register offset = 0x%"
                      HWADDR_PRIx " value=%" PRIu64 "/0x%" PRIx64 "\n",
                      __func__, offset, value, value);
        break;
    }
}

static void close_all_pipes(void* opaque) {
    PipeDevice* dev = opaque;
    HwPipe* pipe = dev->pipes;
    while(pipe) {
        HwPipe* old_pipe = pipe;
        pipe = pipe->next;
        hwpipe_free(old_pipe);
    }
}

static void reset_pipe_device(void* opaque) {
    PipeDevice* dev = opaque;
    dev->pipes = NULL;
    dev->wanted_pipes_first = NULL;
    dev->wanted_pipe_after_channel_high = NULL;
    g_hash_table_remove_all(dev->pipes_by_channel);
    qemu_set_irq(dev->ps->irq, 0);
}

/* I/O read */
static uint64_t pipe_dev_read(void *opaque, hwaddr offset, unsigned size)
{
    AndroidPipeState *s = (AndroidPipeState *)opaque;
    PipeDevice *dev = s->dev;

    switch (offset) {
    case PIPE_REG_STATUS:
        DR("%s: REG_STATUS status=%d (0x%x)", __FUNCTION__, dev->status, dev->status);
        return dev->status;

    case PIPE_REG_CHANNEL: {
        HwPipe* wanted_pipe = wanted_pipes_pop_first(dev);
        if (wanted_pipe != NULL) {
            dev->wakes = get_and_clear_pipe_wanted(wanted_pipe);
            DR("%s: channel=0x%llx wanted=%d", __FUNCTION__,
               (unsigned long long)wanted_pipe->channel, dev->wakes);
            return (uint32_t)(wanted_pipe->channel & 0xFFFFFFFFUL);
        } else {
            qemu_set_irq(s->irq, 0);
            DD("%s: no signaled channels, lowering IRQ", __FUNCTION__);
            return 0;
        }
    }

    case PIPE_REG_CHANNEL_HIGH: {
        // TODO(zyy): this call is really dangerous; currently the device would
        //  stop the calls as soon as we return 0 here; but it means that if the
        //  channel's upper 32 bits are zeroes (that happens), we won't be able
        //  to wake either that channel or any following ones.
        //  I think we should create a new pipe protocol to deal with this
        //  issue and also reduce chattiness of the pipe communication at the
        //  same time.
        HwPipe* wanted_pipe = wanted_pipes_pop_first(dev);
        if (wanted_pipe != NULL) {
            dev->wanted_pipe_after_channel_high = wanted_pipe;
            DR("%s: channel_high=0x%llx wanted=%d", __FUNCTION__,
               (unsigned long long)wanted_pipe->channel, wanted_pipe->wanted);
            assert((uint32_t)(wanted_pipe->channel >> 32) != 0);
            return (uint32_t)(wanted_pipe->channel >> 32);
        } else {
            qemu_set_irq(s->irq, 0);
            DD("%s: no signaled channels (for high), lowering IRQ", __func__);
            return 0;
        }
    }

    case PIPE_REG_WAKES:
        DR("%s: wakes %d", __FUNCTION__, dev->wakes);
        return dev->wakes;

    case PIPE_REG_PARAMS_ADDR_HIGH:
        return (uint32_t)(dev->params_addr >> 32);

    case PIPE_REG_PARAMS_ADDR_LOW:
        return (uint32_t)(dev->params_addr & 0xFFFFFFFFUL);

    case PIPE_REG_VERSION:
        // PIPE_REG_VERSION is issued on probe, which means that
        // we should clean up all existing stale pipes.
        // This helps keep the right state on rebooting.
        close_all_pipes(dev);
        reset_pipe_device(dev);
        return PIPE_DEVICE_VERSION;

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: unknown register %" HWADDR_PRId
                      " (0x%" HWADDR_PRIx ")\n", __FUNCTION__, offset, offset);
    }
    return 0;
}

static const MemoryRegionOps android_pipe_iomem_ops = {
    .read = pipe_dev_read,
    .write = pipe_dev_write,
    .endianness = DEVICE_NATIVE_ENDIAN
};

static void qemu2_android_pipe_reset(void* hwpipe, void* internal_pipe);
static void qemu2_android_pipe_host_signal_wake(void* hwpipe, unsigned flags);
static void qemu2_android_pipe_host_close(void* hwpipe);

static const AndroidPipeHwFuncs qemu2_android_pipe_hw_funcs = {
    .resetPipe = qemu2_android_pipe_reset,
    .closeFromHost = qemu2_android_pipe_host_close,
    .signalWake = qemu2_android_pipe_host_signal_wake,
};

#define ANDROID_PIPE_SAVE_VERSION 1

static void android_pipe_save(QEMUFile* f, void* opaque) {
    AndroidPipeState* s = opaque;
    PipeDevice* dev = s->dev;
    Stream* stream = stream_from_qemufile(f);

    /* Save i/o registers. */
    stream_put_be64(stream, dev->address);
    stream_put_be32(stream, dev->size);
    stream_put_be32(stream, dev->status);
    stream_put_be64(stream, dev->channel);
    stream_put_be32(stream, dev->wakes);
    stream_put_be64(stream, dev->params_addr);

    /* Save the pipe count and state of the pipes. */
    int pipe_count = 0;
    HwPipe* pipe;
    for (pipe = dev->pipes; pipe; pipe = pipe->next) {
        ++pipe_count;
    }
    stream_put_be32(stream, pipe_count);
    for (pipe = dev->pipes; pipe; pipe = pipe->next) {
        stream_put_be64(stream, pipe->channel);
        stream_put_byte(stream, pipe->closed);
        stream_put_byte(stream, pipe->wanted);
        android_pipe_guest_save(pipe->pipe, stream);
    }

    /* Save wanted pipes list. */
    int wanted_pipes_count = 0;
    for (pipe = dev->wanted_pipes_first; pipe; pipe = pipe->wanted_next) {
        wanted_pipes_count++;
    }
    stream_put_be32(stream, wanted_pipes_count);
    for (pipe = dev->wanted_pipes_first; pipe; pipe = pipe->wanted_next) {
        stream_put_be64(stream, pipe->channel);
    }
    if (dev->wanted_pipe_after_channel_high) {
        stream_put_byte(stream, 1);
        stream_put_be64(stream, dev->wanted_pipe_after_channel_high->channel);
    } else {
        stream_put_byte(stream, 0);
    }

    stream_free(stream);
}

static int android_pipe_load(QEMUFile* f, void* opaque, int version_id) {
    AndroidPipeState* s = opaque;
    PipeDevice* dev = s->dev;
    Stream* stream = stream_from_qemufile(f);
    HwPipe* pipe;

    /* Load i/o registers. */
    dev->address = stream_get_be64(stream);
    dev->size = stream_get_be32(stream);
    dev->status = stream_get_be32(stream);
    dev->channel = stream_get_be64(stream);
    dev->wakes = stream_get_be32(stream);
    dev->params_addr = stream_get_be64(stream);

    /* Clean up old pipe objects. */
    close_all_pipes(dev);

    /* Restore pipes. */
    int pipe_count = stream_get_be32(stream);
    int pipe_n;
    HwPipe** pipe_list_end = &(dev->pipes);
    *pipe_list_end = NULL;
    uint64_t* force_closed_pipes = malloc(sizeof(uint64_t) * pipe_count);
    int force_closed_pipes_count = 0;
    for (pipe_n = 0; pipe_n < pipe_count; pipe_n++) {
        HwPipe* pipe = hwpipe_new0(dev);
        char force_close = 0;
        pipe->channel = stream_get_be64(stream);
        pipe->closed = stream_get_byte(stream);
        pipe->wanted = stream_get_byte(stream);
        pipe->pipe = android_pipe_guest_load(stream, pipe, &force_close);
        pipe->wanted_next = pipe->wanted_prev = NULL;

        // |pipe| might be NULL in case it couldn't be saved. However,
        // in that case |force_close| will be set by android_pipe_guest_load,
        // so |force_close| should be checked first so that the function
        // can continue.
        if (force_close) {
            pipe->closed = 1;
            force_closed_pipes[force_closed_pipes_count++] = pipe->channel;
        } else if (!pipe->pipe) {
            hwpipe_free(pipe);
            free(force_closed_pipes);
            return -EIO;
        }

        pipe->next = NULL;
        *pipe_list_end = pipe;
        pipe_list_end = &(pipe->next);
    }

    /* Rebuild the pipes-by-channel table. */
    g_hash_table_remove_all(dev->pipes_by_channel); /* Clean up old data. */
    for(pipe = dev->pipes; pipe; pipe = pipe->next) {
        g_hash_table_insert(dev->pipes_by_channel,
                            hash_create_key_from_channel(pipe->channel),
                            pipe);
    }

    /* Reconstruct wanted pipes list. */
    HwPipe** wanted_pipe_list_end = &(dev->wanted_pipes_first);
    *wanted_pipe_list_end = NULL;
    int wanted_pipes_count = stream_get_be32(stream);
    uint64_t channel;
    for (pipe_n = 0; pipe_n < wanted_pipes_count; ++pipe_n) {
        channel = stream_get_be64(stream);
        HwPipe* pipe = g_hash_table_lookup(dev->pipes_by_channel,
                                           hash_cast_key_from_channel(&channel));
        if (pipe) {
            pipe->wanted_prev = *wanted_pipe_list_end;
            pipe->wanted_next = NULL;
            *wanted_pipe_list_end = pipe;
            wanted_pipe_list_end = &(pipe->wanted_next);
        } else {
            stream_free(stream);
            free(force_closed_pipes);
            return -EIO;
        }
    }
    if (stream_get_byte(stream)) {
        channel = stream_get_be64(stream);
        dev->wanted_pipe_after_channel_high =
            g_hash_table_lookup(dev->pipes_by_channel,
                                hash_cast_key_from_channel(&channel));
        if (!dev->wanted_pipe_after_channel_high) {
            stream_free(stream);
            free(force_closed_pipes);
            return -EIO;
        }
    } else {
        dev->wanted_pipe_after_channel_high = NULL;
    }

    /* Add forcibly closed pipes to wanted pipes list */
    for (pipe_n = 0; pipe_n < force_closed_pipes_count; pipe_n++) {
        HwPipe* pipe =
            g_hash_table_lookup(dev->pipes_by_channel,
                                hash_cast_key_from_channel(
                                    &force_closed_pipes[pipe_n]));
        set_pipe_wanted_bits(pipe, PIPE_WAKE_CLOSED);
        pipe->closed = 1;
        if (!pipe->wanted_next &&
            !pipe->wanted_prev &&
            pipe != dev->wanted_pipe_after_channel_high) {
            pipe->wanted_prev = *wanted_pipe_list_end;
            *wanted_pipe_list_end = pipe;
            wanted_pipe_list_end = &(pipe->wanted_next);
        }
    }

    stream_free(stream);
    free(force_closed_pipes);
    return 0;
}

static void android_pipe_post_load(void* opaque) {
    /* This function gets invoked after all VM state has
     * been loaded. Raising IRQ in the load handler causes
     * problems.
     */
    PipeDevice* dev = ((AndroidPipeState*)opaque)->dev;
    if (dev->wanted_pipes_first) {
        qemu_set_irq(dev->ps->irq, 1);
    } else {
        qemu_set_irq(dev->ps->irq, 0);
    }
}

static void android_pipe_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    AndroidPipeState *s = ANDROID_PIPE(dev);

    s->dev = (PipeDevice *) g_malloc0(sizeof(PipeDevice));
    s->dev->ps = s; /* HACK: backlink */
    s->dev->wanted_pipes_first = NULL;
    s->dev->wanted_pipe_after_channel_high = NULL;
    s->dev->pipes_by_channel = g_hash_table_new_full(
                                   hash_channel, hash_channel_equal,
                                   hash_channel_destroy, NULL);
    if (!s->dev->pipes_by_channel) {
        APANIC("%s: failed to initialize pipes hash\n", __func__);
    }
    memory_region_init_io(&s->iomem, OBJECT(s), &android_pipe_iomem_ops, s,
                          "android_pipe", 0x2000 /*TODO: ?how big?*/);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);

    android_pipe_set_hw_funcs(&qemu2_android_pipe_hw_funcs);

    register_savevm_with_post_load
        (dev,
         "android_pipe",
         0,
         ANDROID_PIPE_SAVE_VERSION,
         android_pipe_save,
         android_pipe_load,
         android_pipe_post_load,
         s);
}

static void qemu2_android_pipe_reset(void* hwpipe, void* internal_pipe) {
    HwPipe* pipe = hwpipe;
    pipe->pipe = internal_pipe;
}

static void qemu2_android_pipe_host_signal_wake( void* hwpipe, unsigned flags )
{
    HwPipe*  pipe = hwpipe;
    PipeDevice*  dev = pipe->device;

    DD("%s: channel=0x%llx flags=%d", __FUNCTION__, (unsigned long long)pipe->channel, flags);

    set_pipe_wanted_bits(pipe, (unsigned char)flags);
    wanted_pipes_add(dev, pipe);

    /* Raise IRQ to indicate there are items on our list ! */
    qemu_set_irq(dev->ps->irq, 1);
    DD("%s: raising IRQ", __FUNCTION__);
}

static void qemu2_android_pipe_host_close( void* hwpipe )
{
    HwPipe* pipe = hwpipe;

    D("%s: channel=0x%llx (closed=%d)", __FUNCTION__, (unsigned long long)pipe->channel, pipe->closed);

    if (!pipe->closed) {
        pipe->closed = 1;
        qemu2_android_pipe_host_signal_wake( hwpipe, PIPE_WAKE_CLOSED );
    }
}

static void android_pipe_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = android_pipe_realize;
    dc->desc = "android pipe";
}

static const TypeInfo android_pipe_info = {
    .name          = TYPE_ANDROID_PIPE,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AndroidPipeState),
    .class_init    = android_pipe_class_init
};

static void android_pipe_register(void)
{
    type_register_static(&android_pipe_info);
}

type_init(android_pipe_register);

