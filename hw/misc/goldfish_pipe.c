/* Copyright (c) 2011-2016 The Android Open Source Project
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
** pipes are registered with the goldfish_pipe_add_type() call.
**
** Open Questions
**
** Since this was originally written there have been a number of other
** virtual devices added to QEMU using the virtio infrastructure. We
** should give some thought to if this needs re-writing to take
** advantage of that infrastructure to create the pipes.
*/
#include "hw/misc/goldfish_pipe.h"

#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/sysbus.h"

#include "qemu-common.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "qemu/error-report.h"

#include <assert.h>
#include <glib.h>

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

/* Set to > 0 for debug output */
#define PIPE_DEBUG 0

/* Set to 1 to debug i/o register reads/writes */
#define PIPE_DEBUG_REGS 0

#if PIPE_DEBUG >= 1
#define D(fmt, ...) \
    do { fprintf(stdout, "goldfish_pipe: " fmt "\n", ## __VA_ARGS__); } while (0)
#else
#define D(fmt, ...)  do { /* nothing */ } while (0)
#endif

#if PIPE_DEBUG >= 2
#define DD(fmt, ...) \
    do { fprintf(stdout, "goldfish_pipe: " fmt "\n", ## __VA_ARGS__); } while (0)
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
 *    drivers/platform/goldfish/goldfish_pipe*
 */

typedef enum PipeRegs {
    PIPE_REG_CMD = 0,

    PIPE_REG_SIGNAL_BUFFER_HIGH = 4,
    PIPE_REG_SIGNAL_BUFFER = 8,
    PIPE_REG_SIGNAL_BUFFER_COUNT = 12,

    PIPE_REG_OPEN_BUFFER_HIGH = 20,
    PIPE_REG_OPEN_BUFFER = 24,

    PIPE_REG_VERSION = 36,

    PIPE_REG_GET_SIGNALLED = 48,

    // v1-specific registers
    PIPE_REG_STATUS = 0x04,  /* read */
    PIPE_REG_CHANNEL = 0x08,  /* read/write: channel id */
    PIPE_REG_SIZE = 0x0c,  /* read/write: buffer size */
    PIPE_REG_ADDRESS = 0x10,  /* write: physical address */
    PIPE_REG_WAKES = 0x14,  /* read: wake flags */

    /* read/write: parameter buffer address */
    PIPE_REG_PARAMS_ADDR_LOW = 0x18,
    PIPE_REG_PARAMS_ADDR_HIGH = 0x1c,

    /* write: access with paremeter buffer */
    PIPE_REG_ACCESS_PARAMS = 0x20,

    PIPE_REG_CHANNEL_HIGH = 0x30, /* read/write: high 32 bit channel id */
    PIPE_REG_ADDRESS_HIGH = 0x34, /* write: high 32 bit physical address */
} PipeRegs;

typedef enum PipeCmd {
    PIPE_CMD_OPEN = 1,  // to be used by the pipe device itself
    PIPE_CMD_CLOSE,
    PIPE_CMD_POLL,
    PIPE_CMD_WRITE,
    PIPE_CMD_WAKE_ON_WRITE,
    PIPE_CMD_READ,
    PIPE_CMD_WAKE_ON_READ,
    PIPE_CMD_WAKE_ON_DONE_IO,
    PIPE_CMD_DMA_MAPHOST,
    PIPE_CMD_DMA_UNMAPHOST,
    PIPE_CMD_CALL,
} PipeCmd;

enum {
    COMMAND_BUFFER_SIZE = 4096,
};

#define TYPE_GOLDFISH_PIPE "goldfish_pipe"
#define GOLDFISH_PIPE(obj) \
    OBJECT_CHECK(GoldfishPipeState, (obj), TYPE_GOLDFISH_PIPE)

/* Update this version number if the device's interface changes. */
enum {
    PIPE_DEVICE_VERSION = 2,
    PIPE_DEVICE_VERSION_v1 = 1,
    // Currently we support {v3,v4} driver for v2 pipe device, and anything
    // else for v1 device.
    MAX_SUPPORTED_DRIVER_VERSION = 4,
    PIPE_DRIVER_VERSION_v1 = 0,  // used to not report its version at all
};

/* These default callbacks are provided to detect when emulation setup
 * didn't register a pipe service implementation correctly! There is no
 * need to provide other GoldfishPipeServiceOps callbacks, since they
 * cannot be called if one could not open or create a host pipe.
 *
 * NOTE: Returning NULL will force-close the pipe as soon as it is
 *       opened by the guest. */
static GoldfishHostPipe *null_guest_open(GoldfishHwPipe *hw_pipe)
{
    E("Android guest tried to open a pipe before service registration!\n"
      "Please call goldfish_pipe_set_service_ops() at setup time!");
    (void)hw_pipe;
    return NULL;
}

static GoldfishHostPipe *null_guest_load(QEMUFile *file,
                                         GoldfishHwPipe *hw_pipe,
                                         char *force_close)
{
    E("Trying to load a pipe before service registration!\n"
      "Please call goldfish_pipe_set_service_ops() at setup time!");
    (void)file;
    (void)hw_pipe;
    (void)force_close;
    return NULL;
}

static void null_guest_pre_post_save_load(QEMUFile *file) {
    E("Trying to save/load a pipe before service registration!\n"
      "Please call goldfish_pipe_set_service_ops() at setup time!");
    (void)file;
}

static const GoldfishPipeServiceOps s_null_service_ops = {
    .guest_open = null_guest_open,
    .guest_load = null_guest_load,
    .guest_pre_load = null_guest_pre_post_save_load,
    .guest_post_load = null_guest_pre_post_save_load,
    .guest_pre_save = null_guest_pre_post_save_load,
    .guest_post_save = null_guest_pre_post_save_load,
};

static const GoldfishPipeServiceOps* service_ops = &s_null_service_ops;

void goldfish_pipe_set_service_ops(const GoldfishPipeServiceOps* ops) {
    service_ops = ops ? ops : &s_null_service_ops;
}

const GoldfishPipeServiceOps* goldfish_pipe_get_service_ops() {
    return service_ops;
}

/* from AOSP version include/hw/android/goldfish/device.h
 * FIXME?: needs to use proper qemu abstractions
 */
static inline void uint64_set_low(uint64_t* addr, uint32_t value) {
    *addr = (*addr & ~(0xFFFFFFFFULL)) | value;
}

static inline void uint64_set_high(uint64_t* addr, uint32_t value) {
    *addr = (*addr & 0xFFFFFFFFULL) | ((uint64_t)value << 32);
}

typedef struct PipeDevice PipeDevice;

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

/* A set of version-specific pipe operations */
typedef struct {
    void (*wanted_list_add)(PipeDevice* dev, GoldfishHwPipe* pipe);
    void (*close_all)(PipeDevice* dev, GoldfishPipeCloseReason reason);
    void (*save)(QEMUFile* file, PipeDevice* dev);

    void (*dev_write)(PipeDevice* dev, hwaddr offset, uint64_t value);
    uint64_t (*dev_read)(PipeDevice* dev, hwaddr offset);
} PipeOperations;

/* Operations for the old and the latest pipe device versions */
static const PipeOperations pipe_ops_v2;
static const PipeOperations pipe_ops_v1;

typedef struct {
    SysBusDevice parent;
    MemoryRegion iomem;
    qemu_irq irq;

    /* TODO: roll into shared state */
    PipeDevice *dev;
} GoldfishPipeState;

typedef struct PipeCommand {
    int32_t cmd;
    int32_t id;
    int32_t status;
    int32_t padding;
    union {
        struct {
            uint32_t buffers_count;
            int32_t consumed_size;
            // There are two arrays here, uint64_t ptrs[max_size]
            // and uint32_t sizes[max_size].
            // max_size is supplied at startup, so we can't reserve the space
            // but need to have a function to calculate the pointers.
            // The two arrays are followed by the index of the first read
            // buffer. Index is only used by PIPE_CMD_CALL commands.
        } rw_params;
        struct {
            uint64_t dma_paddr;
            uint64_t sz;
        } dma_maphost_params;
    };
} PipeCommand;

struct GoldfishHwPipe {
    struct GoldfishHwPipe *wanted_next;
    struct GoldfishHwPipe *wanted_prev;
    PipeDevice* dev;
    uint32_t id;  // pipe ID is its index into the PipeDevice::pipes array
    unsigned char wanted;
    char closed;
    GoldfishHostPipe *host_pipe;
    uint64_t command_buffer_addr;
    PipeCommand* command_buffer;
    uint32_t rw_params_max_count;

    // v1-specific fields
    struct GoldfishHwPipe* next;
    uint64_t channel; /* opaque kernel handle */
};

typedef GoldfishHwPipe HwPipe;
typedef GoldfishHostPipe HostPipe;

typedef struct GuestSignalledPipe {
    uint32_t id;
    uint32_t flags;
} GuestSignalledPipe;

typedef struct OpenCommandParams {
    uint64_t command_buffer_ptr;
    uint32_t rw_params_max_count;
} OpenCommandParams;

struct PipeDevice {
    GoldfishPipeState* ps;  // backlink to instance state
    int device_version;    // host device verion
    int driver_version;    // guest's driver version

    const PipeOperations* ops;  // a set of version-specific pipe operations

    // A vector of all pipes.
    HwPipe** pipes;
    // Capacity of the vector
    unsigned pipes_capacity;

    // The list of the pipes that signalled some 'wanted' state.
    HwPipe* wanted_pipes_first;

    uint64_t signalled_pipe_buffer_addr;
    uint64_t open_command_addr;

    GuestSignalledPipe* signalled_pipe_buffer;
    unsigned signalled_pipe_buffer_size;

    OpenCommandParams* open_command;

    // v1-specific fields

    // The list of all pipes.
    HwPipe* pipes_list;
    // A cached wanted pipe after the guest's _CHANNEL_HIGH call. We need to
    // return the very same pipe during the following *_CHANNEL call.
    HwPipe* wanted_pipe_after_channel_high;

    // Cache of the pipes by channel for a faster lookup.
    GHashTable* pipes_by_channel;

    // i/o registers
    uint64_t address;
    uint32_t size;
    uint32_t status;
    uint64_t channel;
    uint32_t wakes;
    uint64_t params_addr;

    // Benchmarking
    bool measure_latency;
    uint64_t write_start_us;
    uint64_t write_end_us;
    uint64_t wake_us;
    uint64_t read_start_us;
    uint64_t read_end_us;
};


// RW params getter functions for |ptrs| and |sizes| arrays.
static uint64_t* hwpipe_get_command_rw_ptrs(HwPipe* pipe) {
    // |ptrs| is right next to the |consumed_size| member.
    return (uint64_t*)(
                &pipe->command_buffer->rw_params.consumed_size + 1);
}

static uint32_t* hwpipe_get_command_rw_sizes(HwPipe* pipe) {
    // |sizes| follows the |ptrs|.
    return (uint32_t*)(
                hwpipe_get_command_rw_ptrs(pipe) + pipe->rw_params_max_count);
}

static uint32_t hwpipe_get_command_rw_read_index(HwPipe* pipe) {
    // |read_index| follows the |sizes|.
    return *((uint32_t*)(
                hwpipe_get_command_rw_sizes(pipe) + pipe->rw_params_max_count));
}

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

static unsigned char hwpipe_get_and_clear_wanted(HwPipe* pipe) {
    unsigned char val = pipe->wanted;
    pipe->wanted = 0;
    return val;
}

static void hwpipe_set_wanted(HwPipe* pipe, unsigned char val) {
    pipe->wanted |= val;
}

static HwPipe* hwpipe_new0(PipeDevice* dev) {
    HwPipe* pipe;
    pipe = g_malloc0(sizeof(HwPipe));
    pipe->dev = dev;
    return pipe;
}

static HwPipe* hwpipe_new(uint32_t id, uint64_t channel, PipeDevice* dev) {
    HwPipe* pipe = hwpipe_new0(dev);
    pipe->id = id;
    pipe->channel = channel;
    pipe->host_pipe = service_ops->guest_open(pipe);
    return pipe;
}

static void hwpipe_free(HwPipe* pipe, GoldfishPipeCloseReason reason) {
    if (pipe->host_pipe)
        service_ops->guest_close(pipe->host_pipe, reason);

    g_free(pipe);
}

// Wanted pipe linked list operations
static HwPipe* wanted_pipes_pop_first_v2(PipeDevice* dev) {
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

static void wanted_pipes_add_v2(PipeDevice* dev, HwPipe* pipe) {
    if (!pipe->wanted_next && !pipe->wanted_prev &&
        dev->wanted_pipes_first != pipe) {
        pipe->wanted_next = dev->wanted_pipes_first;
        if (dev->wanted_pipes_first) {
            dev->wanted_pipes_first->wanted_prev = pipe;
        }
        dev->wanted_pipes_first = pipe;
    }
}

static void wanted_pipes_remove_v2(PipeDevice* dev, HwPipe* pipe) {
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

// same for v1
static HwPipe* wanted_pipes_pop_first_v1(PipeDevice* dev) {
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

static void wanted_pipes_add_v1(PipeDevice* dev, HwPipe* pipe) {
    if (!pipe->wanted_next && !pipe->wanted_prev &&
        dev->wanted_pipes_first != pipe &&
        dev->wanted_pipe_after_channel_high != pipe) {
        pipe->wanted_next = dev->wanted_pipes_first;
        if (dev->wanted_pipes_first) {
            dev->wanted_pipes_first->wanted_prev = pipe;
        }
        dev->wanted_pipes_first = pipe;
    }
}

static void wanted_pipes_remove_v1(PipeDevice* dev, HwPipe* pipe) {
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

/* Map the guest buffer specified by the guest paddr 'phys'.
 * Returns a host pointer which should be unmapped later via
 * cpu_physical_memory_unmap(), or NULL if mapping failed (likely
 * because the paddr doesn't actually point at RAM).
 * Note that for RAM the "mapping" process doesn't actually involve a
 * data copy.
 */
static void* map_guest_buffer(hwaddr phys, size_t size, int is_write) {
    hwaddr l = size;
    void* ptr;

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

static void unmap_command_buffer(void* buffer) {
    cpu_physical_memory_unmap(buffer, COMMAND_BUFFER_SIZE, 1,
                              COMMAND_BUFFER_SIZE);
}

static void close_all_pipes_v1(PipeDevice* dev, GoldfishPipeCloseReason reason) {
    HwPipe* pipe = dev->pipes_list;
    while (pipe) {
        HwPipe* next = pipe->next;
        hwpipe_free(pipe, reason);
        pipe = next;
    }
    dev->pipes_list = NULL;
}

static void close_all_pipes_v2(PipeDevice* dev, GoldfishPipeCloseReason reason) {
    int i = 0;
    for (; i < dev->pipes_capacity; ++i) {
        HwPipe* pipe = dev->pipes[i];
        if (pipe) {
            // Make sure to use the guest physical address to
            // obtain the host address to unmap;
            // it is possible a remap occurred.
            //
            // map_guest_buffer increments refcount
            // on the memory region, so we need to explicitly
            // unmap this.
            void* current_buffer =
                map_guest_buffer(
                    pipe->command_buffer_addr,
                    COMMAND_BUFFER_SIZE, 1);

            // Unref for telling QEMU we are done with this command buffer.
            cpu_physical_memory_unmap(
                current_buffer, COMMAND_BUFFER_SIZE, 1,
                COMMAND_BUFFER_SIZE);

            // Explicitly unmap again because we just called
            // map_guest_buffer.
            cpu_physical_memory_unmap(
                current_buffer, COMMAND_BUFFER_SIZE, 1,
                COMMAND_BUFFER_SIZE);

            hwpipe_free(pipe, reason);
            dev->pipes[i] = NULL;
        }
    }
}

static void reset_pipe_device(PipeDevice* dev) {
    dev->wanted_pipes_first = NULL;
    dev->wanted_pipe_after_channel_high = NULL;
    g_hash_table_remove_all(dev->pipes_by_channel);
    qemu_set_irq(dev->ps->irq, 0);
    service_ops->dma_reset_host_mappings();
}

static void pipeDevice_doCommand_v1(PipeDevice* dev, uint32_t command) {
    HwPipe* pipe = (HwPipe*)g_hash_table_lookup(
            dev->pipes_by_channel, hash_cast_key_from_channel(&dev->channel));

    /* Check that we're referring a known pipe channel */
    if (command != PIPE_CMD_OPEN && pipe == NULL) {
        dev->status = GOLDFISH_PIPE_ERROR_INVAL;
        return;
    }

    /* If the pipe is closed by the host, return an error */
    if (pipe != NULL && pipe->closed && command != PIPE_CMD_CLOSE) {
        dev->status = GOLDFISH_PIPE_ERROR_IO;
        return;
    }

    switch (command) {
    case PIPE_CMD_OPEN:
        DD("%s: CMD_OPEN channel=0x%llx", __func__,
           (unsigned long long)dev->channel);
        if (pipe != NULL) {
            dev->status = GOLDFISH_PIPE_ERROR_INVAL;
            break;
        }
        pipe = hwpipe_new(0, dev->channel, dev);
        if (!pipe->host_pipe) {
            hwpipe_free(pipe, GOLDFISH_PIPE_CLOSE_ERROR);
            dev->status = GOLDFISH_PIPE_ERROR_INVAL;
            break;
        }
        pipe->next = dev->pipes_list;
        dev->pipes_list = pipe;
        dev->status = 0;
        g_hash_table_insert(dev->pipes_by_channel,
                            hash_create_key_from_channel(dev->channel), pipe);
        break;

    case PIPE_CMD_CLOSE: {
        DD("%s: CMD_CLOSE channel=0x%llx", __func__,
           (unsigned long long)dev->channel);
        // Remove from device's lists.
        // This linear lookup is potentially slow, but we don't delete pipes
        // often enough for it to become noticable.
        HwPipe** pnode = &dev->pipes_list;
        while (*pnode && *pnode != pipe) {
            pnode = &(*pnode)->next;
        }
        if (!*pnode) {
            dev->status = GOLDFISH_PIPE_ERROR_INVAL;
            break;
        }
        *pnode = pipe->next;
        pipe->next = NULL;
        g_hash_table_remove(dev->pipes_by_channel,
                            hash_cast_key_from_channel(&pipe->channel));
        wanted_pipes_remove_v1(dev, pipe);

        hwpipe_free(pipe, GOLDFISH_PIPE_CLOSE_GRACEFUL);
        break;
    }

    case PIPE_CMD_POLL:
        dev->status = service_ops->guest_poll(pipe->host_pipe);
        DD("%s: CMD_POLL > status=%d", __func__, dev->status);
        break;

    case PIPE_CMD_READ: {
        /* Translate guest physical address into emulator memory. */
        GoldfishPipeBuffer buffer;
        buffer.data = map_guest_buffer(dev->address, dev->size, /*is_write*/1);
        if (!buffer.data) {
            dev->status = GOLDFISH_PIPE_ERROR_INVAL;
            break;
        }
        buffer.size = dev->size;
        dev->status = service_ops->guest_recv(pipe->host_pipe, &buffer, 1);
        DD("%s: CMD_READ channel=0x%llx address=0x%16llx size=%d > status=%d",
           __func__, (unsigned long long)dev->channel,
           (unsigned long long)dev->address,
           dev->size, dev->status);
        cpu_physical_memory_unmap(buffer.data, dev->size,
                                  /*is_write*/1, dev->size);
        break;
    }

    case PIPE_CMD_WRITE: {
        /* Translate guest physical address into emulator memory. */
        GoldfishPipeBuffer  buffer;
        buffer.data = map_guest_buffer(dev->address, dev->size, /*is_write*/0);
        if (!buffer.data) {
            dev->status = GOLDFISH_PIPE_ERROR_INVAL;
            break;
        }
        buffer.size = dev->size;
        dev->status = service_ops->guest_send(&pipe->host_pipe, &buffer, 1);
        DD("%s: CMD_WRITE_BUFFER channel=0x%llx address=0x%16llx size=%d > "
           "status=%d", __func__, (unsigned long long)dev->channel,
           (unsigned long long)dev->address, dev->size, dev->status);
        cpu_physical_memory_unmap(buffer.data, dev->size,
                                  /*is_write*/0, dev->size);
        break;
    }

    case PIPE_CMD_WAKE_ON_READ:
        DD("%s: CMD_WAKE_ON_READ channel=0x%llx", __func__,
           (unsigned long long)dev->channel);
        if ((pipe->wanted & GOLDFISH_PIPE_WAKE_READ) == 0) {
            pipe->wanted |= GOLDFISH_PIPE_WAKE_READ;
            service_ops->guest_wake_on(pipe->host_pipe, pipe->wanted);
        }
        dev->status = 0;
        break;

    case PIPE_CMD_WAKE_ON_WRITE:
        DD("%s: CMD_WAKE_ON_WRITE channel=0x%llx", __func__,
           (unsigned long long)dev->channel);
        if ((pipe->wanted & GOLDFISH_PIPE_WAKE_WRITE) == 0) {
            pipe->wanted |= GOLDFISH_PIPE_WAKE_WRITE;
            service_ops->guest_wake_on(pipe->host_pipe, pipe->wanted);
        }
        dev->status = 0;
        break;
    default:
        D("%s: command=%d (0x%x)\n", __func__, command, command);
    }
}

static void pipeDevice_doOpenClose_v2(PipeDevice* dev, uint32_t id) {
    PipeCommand* commandBuffer = (PipeCommand*)map_guest_buffer(
            dev->open_command->command_buffer_ptr,
            COMMAND_BUFFER_SIZE, /*is_write*/1);
    if (!commandBuffer) {
        // well, what can we do here?
        return;
    }
    if (dev->open_command->rw_params_max_count < 1) {
        commandBuffer->status = GOLDFISH_PIPE_ERROR_INVAL;
        unmap_command_buffer(commandBuffer);
        return;
    }
    if (commandBuffer->id != id) {
        commandBuffer->status = GOLDFISH_PIPE_ERROR_INVAL;
        unmap_command_buffer(commandBuffer);
        return;
    }
    if (commandBuffer->cmd == PIPE_CMD_CLOSE) {
        // it's OK to close an already closed pipe
        commandBuffer->status = 0;
        unmap_command_buffer(commandBuffer);
        return;
    }
    if (commandBuffer->cmd != PIPE_CMD_OPEN) {
        commandBuffer->status = GOLDFISH_PIPE_ERROR_INVAL;
        unmap_command_buffer(commandBuffer);
        return;
    }
    if (id >= dev->pipes_capacity) {
        int newCapacity = (id + 1 > 2 * dev->pipes_capacity)
                                  ? id + 1
                                  : 2 * dev->pipes_capacity;
        HwPipe** pipes = calloc(newCapacity, sizeof(HwPipe*));
        if (!pipes) {
            commandBuffer->status = GOLDFISH_PIPE_ERROR_NOMEM;
            unmap_command_buffer(commandBuffer);
            return;
        }
        memcpy(pipes, dev->pipes, sizeof(HwPipe*) * dev->pipes_capacity);
        dev->pipes = pipes;
        dev->pipes_capacity = newCapacity;
    }

    HwPipe* pipe = hwpipe_new(id, 0, dev);
    if (!pipe || !pipe->host_pipe) {
        hwpipe_free(pipe, GOLDFISH_PIPE_CLOSE_ERROR);
        commandBuffer->status = GOLDFISH_PIPE_ERROR_NOMEM;
        unmap_command_buffer(commandBuffer);
        return;
    }

    pipe->command_buffer_addr = dev->open_command->command_buffer_ptr;
    pipe->command_buffer = commandBuffer;
    pipe->rw_params_max_count = dev->open_command->rw_params_max_count;
    dev->pipes[id] = pipe;
    commandBuffer->status = 0;
}

static void pipeDevice_doCommand_v2(HwPipe* pipe) {
    assert(pipe);
    assert(pipe->command_buffer->cmd != PIPE_CMD_OPEN);

    PipeDevice* dev = pipe->dev;
    PipeCmd command = pipe->command_buffer->cmd;

    /* If the pipe is closed by the host, return an error */
    if (pipe->closed && command != PIPE_CMD_CLOSE) {
        pipe->command_buffer->status = GOLDFISH_PIPE_ERROR_IO;
        return;
    }

    switch (command) {
        case PIPE_CMD_CLOSE: {
            DD("%s: CMD_CLOSE id=%d", __func__, (int)pipe->id);
            // Remove from device's lists.
            dev->pipes[pipe->id] = NULL;
            wanted_pipes_remove_v2(dev, pipe);
            pipe->command_buffer->status = 0;
            unmap_command_buffer(pipe->command_buffer);
            hwpipe_free(pipe, GOLDFISH_PIPE_CLOSE_GRACEFUL);
            break;
        }

        case PIPE_CMD_POLL:
            pipe->command_buffer->status =
                    service_ops->guest_poll(pipe->host_pipe);
            DD("%s: CMD_POLL > status=%d", __func__,
               pipe->command_buffer->status);
            break;

        case PIPE_CMD_READ:
        case PIPE_CMD_WRITE:
        case PIPE_CMD_CALL: {
            const bool willModifyData = command != PIPE_CMD_WRITE;
            const bool isCall = command == PIPE_CMD_CALL;
            pipe->command_buffer->rw_params.consumed_size = 0;
            unsigned buffers_count =
                    pipe->command_buffer->rw_params.buffers_count;
            if (buffers_count > pipe->rw_params_max_count) {
                buffers_count = pipe->rw_params_max_count;
            }

            // This isn't supposed to happen, what's the puprose of calling
            // us with no data?
            assert(buffers_count);

            // We know that the |rw_params| fit into single page as of now, so
            // we're free to estimate the maximum size this way.
            uint64_t* const rwPtrs = hwpipe_get_command_rw_ptrs(pipe);
            uint32_t* const rwSizes = hwpipe_get_command_rw_sizes(pipe);
            assert(buffers_count <=
                   COMMAND_BUFFER_SIZE / (sizeof(*rwPtrs) + sizeof(*rwSizes)));
            GoldfishPipeBuffer buffers[
                    COMMAND_BUFFER_SIZE / (sizeof(*rwPtrs) + sizeof(*rwSizes))];

#if defined(TARGET_MIPS)
// MIPS Ranchu memory layout has device IO space hole between
// 0x1f000000 - 0x20000000 and has two RAM regions for
// each of them, below 0x1f000000 and after 0x20000000.
// Tweak RAM_SPLIT_BOUNDARY accordingly to use interpolated
// guest buffer mappings optimization on MIPS.
#define RAM_SPLIT_BOUNDARY    0x20000000
#elif defined(TARGET_I386)
// If given memory around or bigger then 4G, emulator will split physical
// memory to two parts, one is mapped below 4g, another one is mapped over
// 4G. This is for reserving address space for PCI devices. So actually we
// can't always map guest physical address to host virtual address with only
// one offset. We have to use two offsets: one for memory below 4G and one
// for memory over 4G.
#define RAM_SPLIT_BOUNDARY    0xFFFFFFFF
#elif defined(TARGET_ARM)
// ARM Memory layout:
// 0..128MB is space for a flash device bootrom code such as UEFI.
// 128MB..256MB is used for miscellaneous device I/O.
// 256MB..1GB is reserved for possible future PCI support (ie where the
// PCI memory window will go if we add a PCI host controller).
// 1GB and up is RAM (which may happily spill over into the
// high memory region beyond 4GB).
// For ARM this means, there will always be one single RAM region
// (no splitting) so all buffer address interpolation will be done
// using diffFromGuestPostSplit.
#define RAM_SPLIT_BOUNDARY    0x40000000
#else
#error Unsupported architecture!
#endif
            // All passed buffers are allocated in the same guest process, so
            // know they all have the same offset from the host address if they
            // are in the same RAM region: below RAM_SPLIT_BOUNDARY or after.
            // Use '1' as invalid value since real offsets always are aligned
            // to page size.
            ptrdiff_t diffFromGuestPreSplit = 1, diffFromGuestPostSplit = 1;
            int count = 0;
            int unmapIndex[2];
            unsigned i;
            for (i = 0; i < buffers_count; ++i) {
                if ((rwPtrs[i] <= RAM_SPLIT_BOUNDARY && diffFromGuestPreSplit == 1) ||
                    (rwPtrs[i] > RAM_SPLIT_BOUNDARY && diffFromGuestPostSplit == 1)) {
                    buffers[i].data =
                        map_guest_buffer(rwPtrs[i], rwSizes[i], willModifyData);
                    if (rwPtrs[i] <= RAM_SPLIT_BOUNDARY) {
                        diffFromGuestPreSplit =
                            (intptr_t)buffers[i].data - (intptr_t)rwPtrs[i];
                    } else {
                        diffFromGuestPostSplit =
                            (intptr_t)buffers[i].data - (intptr_t)rwPtrs[i];
                    }
                    unmapIndex[count++] = i;
                } else {
                    if (rwPtrs[i] <= RAM_SPLIT_BOUNDARY) {
                        buffers[i].data =
                            (void*)(intptr_t)(rwPtrs[i] + diffFromGuestPreSplit);
                    } else {
                        buffers[i].data =
                            (void*)(intptr_t)(rwPtrs[i] + diffFromGuestPostSplit);
                    }
                }
                buffers[i].size = rwSizes[i];
                assert(buffers[i].data != NULL);
                assert(buffers[i].size != 0);
            }

#ifndef NDEBUG
            // Verify that our interpolated mappings are actually correct
            for (i = 1; i < buffers_count; ++i) {
                void* const mapping = map_guest_buffer(rwPtrs[i], rwSizes[i],
                                                       willModifyData);
                assert(mapping == buffers[i].data);
                cpu_physical_memory_unmap(mapping, rwSizes[i],
                                          willModifyData, rwSizes[i]);
            }
#endif

            GoldfishPipeBuffer* send_buffers = NULL;
            unsigned send_buffers_count = 0;
            GoldfishPipeBuffer* recv_buffers = NULL;
            unsigned recv_buffers_count = 0;

            if (willModifyData) {
                // CALL commands perform send/recv using a single command.
                // |read_index| is the first buffer used for receiving.
                if (isCall) {
                    uint32_t read_index = hwpipe_get_command_rw_read_index(pipe);
                    assert(read_index <= buffers_count);
                    send_buffers = buffers;
                    send_buffers_count = read_index;
                    recv_buffers = &buffers[read_index];
                    recv_buffers_count = buffers_count - read_index;
                } else {
                    recv_buffers = buffers;
                    recv_buffers_count = buffers_count;
                }
            } else {
                send_buffers = buffers;
                send_buffers_count = buffers_count;
            }

            int32_t status = 0;
            int32_t consumed_size = 0;
            if (send_buffers_count) {
                status = service_ops->guest_send(&pipe->host_pipe,
                                                 send_buffers,
                                                 send_buffers_count);
                if (status > 0) {
                    consumed_size += status;
                }
            }
            if (status >= 0 && recv_buffers_count) {
                status = service_ops->guest_recv(pipe->host_pipe,
                                                 recv_buffers,
                                                 recv_buffers_count);
                if (status > 0) {
                    consumed_size += status;
                }
            }
            // TODO(zyy): create an extended version of send()/recv() functions
            // to return both transferred size and resulting status in single
            // call.
            if (isCall) {
                pipe->command_buffer->rw_params.consumed_size = consumed_size;
                pipe->command_buffer->status = consumed_size > 0 ? 0 : status;
            } else {
                pipe->command_buffer->status = status;
                pipe->command_buffer->rw_params.consumed_size =
                    status < 0 ? 0 : status;
            }

            DD("%s: CMD_%s id=%d buffers=%d > status=%d", __func__,
               (willModifyData ? (isCall ? "CALL" : "READ") : "WRITE"),
               (int)pipe->id, (int)buffers_count, pipe->command_buffer->status);

            for (i = 0; i < count; ++i) {
                const int j = unmapIndex[i];
                cpu_physical_memory_unmap(buffers[j].data, buffers[j].size,
                                          willModifyData, buffers[j].size);
            }
            break;
        }

        case PIPE_CMD_WAKE_ON_READ:
        case PIPE_CMD_WAKE_ON_WRITE: {
            bool read = (command == PIPE_CMD_WAKE_ON_READ);
            int wake_flags = read
                    ? GOLDFISH_PIPE_WAKE_READ : GOLDFISH_PIPE_WAKE_WRITE;
            DD("%s: CMD_WAKE_ON_%s id=%d", __func__, (read ? "READ" : "WRITE"),
               (int)pipe->id);
            if ((pipe->wanted & wake_flags) == 0) {
                pipe->wanted |= wake_flags;
                service_ops->guest_wake_on(pipe->host_pipe, pipe->wanted);
            }
            pipe->command_buffer->status = 0;
            break;
        }
        case PIPE_CMD_DMA_MAPHOST:
            service_ops->dma_add_buffer(
                    pipe,
                    pipe->command_buffer->dma_maphost_params.dma_paddr,
                    pipe->command_buffer->dma_maphost_params.sz);
            pipe->command_buffer->status = 0;
            break;
        case PIPE_CMD_DMA_UNMAPHOST:
            service_ops->dma_remove_buffer(
                    pipe->command_buffer->dma_maphost_params.dma_paddr);
            pipe->command_buffer->status = 0;
            break;
        default:
            D("%s: command=%d (0x%x)\n", __func__, command, command);
    }
}

static void pipe_dev_write(void* opaque,
                           hwaddr offset,
                           uint64_t value,
                           unsigned size) {
    GoldfishPipeState* state = opaque;
    PipeDevice* dev = state->dev;

    DR("%s: offset = 0x%" HWADDR_PRIx " value=%" PRIu64 "/0x%" PRIx64, __func__,
       offset, value, value);
    if (offset == PIPE_REG_VERSION) {
        dev->driver_version = value;
    } else {
        dev->ops->dev_write(dev, offset, value);
    }
}

static uint64_t pipe_dev_read(void* opaque, hwaddr offset, unsigned size) {
    GoldfishPipeState* s = (GoldfishPipeState*)opaque;
    PipeDevice* dev = s->dev;
    if (offset == PIPE_REG_VERSION) {
        // PIPE_REG_VERSION is issued on probe, which means that
        // we should clean up all existing stale pipes.
        // This helps keep the right state on rebooting.
        dev->ops->close_all(dev, GOLDFISH_PIPE_CLOSE_REBOOT);
        reset_pipe_device(dev);
        if (dev->driver_version < MAX_SUPPORTED_DRIVER_VERSION) {
            // Old driver used to not report its version at all.
            dev->device_version = PIPE_DEVICE_VERSION_v1;
            dev->ops = &pipe_ops_v1;
        } else {
            dev->device_version = PIPE_DEVICE_VERSION;
            dev->ops = &pipe_ops_v2;
        }
        return dev->device_version;
    }
    return dev->ops->dev_read(dev, offset);
}

static void pipe_dev_write_v1(PipeDevice* dev,
                              hwaddr offset,
                              uint64_t value) {
    switch (offset) {
    case PIPE_REG_CMD:
        pipeDevice_doCommand_v1(dev, value);
        break;

    case PIPE_REG_SIZE:
        dev->size = value;
        break;

    case PIPE_REG_ADDRESS:
        uint64_set_low(&dev->address, value);
        break;

    case PIPE_REG_ADDRESS_HIGH:
        uint64_set_high(&dev->address, value);
        break;

    case PIPE_REG_CHANNEL:
        uint64_set_low(&dev->channel, value);
        break;

    case PIPE_REG_CHANNEL_HIGH:
        uint64_set_high(&dev->channel, value);
        break;

    case PIPE_REG_PARAMS_ADDR_HIGH:
        uint64_set_high(&dev->params_addr, value);
        break;

    case PIPE_REG_PARAMS_ADDR_LOW:
        uint64_set_low(&dev->params_addr, value);
        break;

    case PIPE_REG_ACCESS_PARAMS:
    {
        union access_params aps;
        uint32_t cmd;
        bool is_64bit = true;

        /* Don't touch aps.result if anything wrong */
        if (dev->params_addr == 0)
            break;

        cpu_physical_memory_read(dev->params_addr, (void*)&aps, sizeof(aps.aps32));

        /* This auto-detection of 32bit/64bit ness relies on the
         * currently unused flags parameter. As the 32 bit flags
         * overlaps with the 64 bit cmd parameter. As cmd != 0 if we
         * find it as 0 it's 32bit
         */
        if (aps.aps32.flags == 0) {
            is_64bit = false;
        } else {
            cpu_physical_memory_read(dev->params_addr, (void*)&aps, sizeof(aps.aps64));
        }

        if (is_64bit) {
            dev->channel = aps.aps64.channel;
            dev->size = aps.aps64.size;
            dev->address = aps.aps64.address;
            cmd = aps.aps64.cmd;
        } else {
            dev->channel = aps.aps32.channel;
            dev->size = aps.aps32.size;
            dev->address = aps.aps32.address;
            cmd = aps.aps32.cmd;
        }

        if ((cmd != PIPE_CMD_READ) && (cmd != PIPE_CMD_WRITE))
            break;

        pipeDevice_doCommand_v1(dev, cmd);

        if (is_64bit) {
            aps.aps64.result = dev->status;
            cpu_physical_memory_write(dev->params_addr, (void*)&aps, sizeof(aps.aps64));
        } else {
            aps.aps32.result = dev->status;
            cpu_physical_memory_write(dev->params_addr, (void*)&aps, sizeof(aps.aps32));
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

static uint64_t pipe_dev_curr_time_us() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_usec + tv.tv_sec * 1000000ULL;
}

#define LONG_PIPE_TRANSACTION_THRESHOLD_MS 1.0f

static void pipe_dev_warn_long_duration(
    const char* tag, uint64_t start_us, uint64_t end_us) {
    float ms = (end_us - start_us) / 1000.0f;
    if (ms > LONG_PIPE_TRANSACTION_THRESHOLD_MS) {
        fprintf(stderr, "%s: high latency for [%s]: %f ms\n",
                __func__, tag, ms);
    }
}

static void pipe_dev_write_v2(PipeDevice* dev,
                                  hwaddr offset,
                                  uint64_t value) {
    if (dev->measure_latency) {
        dev->write_start_us = pipe_dev_curr_time_us();
    }

    switch (offset) {
        case PIPE_REG_SIGNAL_BUFFER_HIGH:
            dev->signalled_pipe_buffer_addr = value << 32;
            break;
        case PIPE_REG_SIGNAL_BUFFER:
            dev->signalled_pipe_buffer_addr |= (uintptr_t)value;
            break;
        case PIPE_REG_SIGNAL_BUFFER_COUNT:
            dev->signalled_pipe_buffer_size = value;
            dev->signalled_pipe_buffer = (GuestSignalledPipe*)map_guest_buffer(
                    dev->signalled_pipe_buffer_addr,
                    sizeof(*dev->signalled_pipe_buffer) *
                            dev->signalled_pipe_buffer_size,
                    /*is_write*/1);
            if (!dev->signalled_pipe_buffer) {
                APANIC("%s: failed to map signalled pipe buffer\n", __func__);
            }
            break;
        case PIPE_REG_OPEN_BUFFER_HIGH:
            dev->open_command_addr = value << 32;
            break;
        case PIPE_REG_OPEN_BUFFER:
            dev->open_command_addr |= (uintptr_t)value;
            dev->open_command = (OpenCommandParams*)map_guest_buffer(
                    dev->open_command_addr, sizeof(*dev->open_command),
                    /*is_write*/0);
            if (!dev->open_command) {
                APANIC("%s: failed to map open command buffer\n", __func__);
            }
            break;
        case PIPE_REG_CMD: {
            unsigned id = value;
            if (id < dev->pipes_capacity && dev->pipes[id]) {
                pipeDevice_doCommand_v2(dev->pipes[id]);
            } else {
                pipeDevice_doOpenClose_v2(dev, id);
            }
            break;
        }
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "%s: unknown register offset = 0x%" HWADDR_PRIx
                          " value=%" PRIu64 "/0x%" PRIx64 "\n",
                          __func__, offset, value, value);
            break;
    }

    if (dev->measure_latency) {
        dev->write_end_us = pipe_dev_curr_time_us();
        pipe_dev_warn_long_duration(
            "pipe_write",
            dev->write_start_us,
            dev->write_end_us);
    }
}

static uint64_t pipe_dev_read_v1(PipeDevice* dev, hwaddr offset)
{
    switch (offset) {
    case PIPE_REG_STATUS:
        DR("%s: REG_STATUS status=%d (0x%x)", __func__, dev->status, dev->status);
        return dev->status;

    case PIPE_REG_CHANNEL: {
        HwPipe* wanted_pipe = wanted_pipes_pop_first_v1(dev);
        if (wanted_pipe != NULL) {
            dev->wakes = hwpipe_get_and_clear_wanted(wanted_pipe);
            DR("%s: channel=0x%llx wanted=%d", __func__,
               (unsigned long long)wanted_pipe->channel, dev->wakes);
            return (uint32_t)(wanted_pipe->channel & 0xFFFFFFFFUL);
        } else {
            qemu_set_irq(dev->ps->irq, 0);
            DD("%s: no signaled channels, lowering IRQ", __func__);
            return 0;
        }
    }

    case PIPE_REG_CHANNEL_HIGH: {
        //  This call is really dangerous; currently the device would
        //  stop the calls as soon as we return 0 here; but it means that if the
        //  channel's upper 32 bits are zeroes (that happens), we won't be able
        //  to wake either that channel or any following ones.
        //  It is gone in v2.
        HwPipe* wanted_pipe = wanted_pipes_pop_first_v1(dev);
        if (wanted_pipe != NULL) {
            dev->wanted_pipe_after_channel_high = wanted_pipe;
            DR("%s: channel_high=0x%llx wanted=%d", __func__,
               (unsigned long long)wanted_pipe->channel, wanted_pipe->wanted);
            assert((uint32_t)(wanted_pipe->channel >> 32) != 0);
            return (uint32_t)(wanted_pipe->channel >> 32);
        } else {
            qemu_set_irq(dev->ps->irq, 0);
            DD("%s: no signaled channels (for high), lowering IRQ", __func__);
            return 0;
        }
    }

    case PIPE_REG_WAKES:
        DR("%s: wakes %d", __func__, dev->wakes);
        return dev->wakes;

    case PIPE_REG_PARAMS_ADDR_HIGH:
        return (uint32_t)(dev->params_addr >> 32);

    case PIPE_REG_PARAMS_ADDR_LOW:
        return (uint32_t)(dev->params_addr & 0xFFFFFFFFUL);

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: unknown register %" HWADDR_PRId
                      " (0x%" HWADDR_PRIx ")\n", __func__, offset, offset);
    }
    return 0;
}

static uint64_t pipe_dev_read_v2(PipeDevice* dev, hwaddr offset) {
    uint64_t res = 0;

    if (dev->measure_latency) {
        dev->read_start_us = pipe_dev_curr_time_us();
    }

    switch (offset) {
        case PIPE_REG_GET_SIGNALLED: {
            int count = 0;
            HwPipe* pipe;
            while (count < dev->signalled_pipe_buffer_size &&
                   (pipe = wanted_pipes_pop_first_v2(dev)) != NULL) {
                dev->signalled_pipe_buffer[count].id = pipe->id;
                dev->signalled_pipe_buffer[count].flags =
                        hwpipe_get_and_clear_wanted(pipe);
                ++count;
            }
            if (!dev->wanted_pipes_first) {
                qemu_set_irq(dev->ps->irq, 0);  // we've passed all wanted pipes
            }
            res = count;
            break;
        }
        default:
            qemu_log_mask(LOG_GUEST_ERROR, "%s: unknown register %" HWADDR_PRId
                                           " (0x%" HWADDR_PRIx ")\n",
                          __func__, offset, offset);
    }

    if (dev->measure_latency) {
        dev->read_end_us = pipe_dev_curr_time_us();
        pipe_dev_warn_long_duration(
            "pipe_read",
            dev->read_start_us,
            dev->read_end_us);
        pipe_dev_warn_long_duration(
            "pipe_wake",
            dev->wake_us,
            dev->read_start_us);
    }

    return res;
}

static const MemoryRegionOps goldfish_pipe_iomem_ops = {
        .read = pipe_dev_read,
        .write = pipe_dev_write,
        .endianness = DEVICE_NATIVE_ENDIAN
};

// Don't change this version unless you want to break forward compatibility.
// Instead, use the different device version as a first saved field.
enum {
    GOLDFISH_PIPE_SAVE_VERSION = 1,
};

static void goldfish_pipe_save(QEMUFile* f, void* opaque) {
    GoldfishPipeState* s = opaque;
    PipeDevice* dev = s->dev;
    service_ops->guest_pre_save(f);
    dev->ops->save(f, dev);
    service_ops->guest_post_save(f);
}

static void goldfish_pipe_save_v1(QEMUFile* file, PipeDevice* dev) {
    assert(dev->device_version == PIPE_DEVICE_VERSION_v1);
    qemu_put_be32(file, dev->device_version);

    /* Save the device version */
    /* Save i/o registers. */
    qemu_put_be64(file, dev->address);
    qemu_put_be32(file, dev->size);
    qemu_put_be32(file, dev->status);
    qemu_put_be64(file, dev->channel);
    qemu_put_be32(file, dev->wakes);
    qemu_put_be64(file, dev->params_addr);

    /* Save the pipe count and state of the pipes. */
    int pipe_count = 0;
    HwPipe* pipe;
    for (pipe = dev->pipes_list; pipe; pipe = pipe->next) {
        ++pipe_count;
    }
    qemu_put_be32(file, pipe_count);
    for (pipe = dev->pipes_list; pipe; pipe = pipe->next) {
        qemu_put_be64(file, pipe->channel);
        qemu_put_byte(file, pipe->closed);
        qemu_put_byte(file, pipe->wanted);
        // It's possible to get a 'save' command right after the 'load' one,
        // when some force-closed pipes are still on the list.
        if (pipe->host_pipe) {
            qemu_put_byte(file, 1);
            service_ops->guest_save(pipe->host_pipe, file);
        } else {
            qemu_put_byte(file, 0);
        }
    }

    /* Save wanted pipes list. */
    int wanted_pipes_count = 0;
    for (pipe = dev->wanted_pipes_first; pipe; pipe = pipe->wanted_next) {
        wanted_pipes_count++;
    }
    qemu_put_be32(file, wanted_pipes_count);
    for (pipe = dev->wanted_pipes_first; pipe; pipe = pipe->wanted_next) {
        qemu_put_be64(file, pipe->channel);
    }
    if (dev->wanted_pipe_after_channel_high) {
        qemu_put_byte(file, 1);
        qemu_put_be64(file, dev->wanted_pipe_after_channel_high->channel);
    } else {
        qemu_put_byte(file, 0);
    }
}

static void goldfish_pipe_save_v2(QEMUFile* file, PipeDevice* dev) {
    assert(dev->device_version == PIPE_DEVICE_VERSION);
    qemu_put_be32(file, dev->device_version);

    /* Save the device data. */
    qemu_put_be32(file, dev->driver_version);
    qemu_put_be32(file, dev->signalled_pipe_buffer_size);
    qemu_put_be64(file, dev->signalled_pipe_buffer_addr);
    qemu_put_be64(file, dev->open_command_addr);
    qemu_put_be32(file, dev->pipes_capacity);

    /* Save the pipes. */
    int i;
    int pipe_count = 0;
    for (i = 0; i < dev->pipes_capacity; ++i) {
        if (dev->pipes[i]) {
            ++pipe_count;
        }
    }
    qemu_put_be32(file, pipe_count);

    for (i = 0; i < dev->pipes_capacity; ++i) {
        HwPipe* pipe = dev->pipes[i];
        if (!pipe) {
            continue;
        }
        qemu_put_be32(file, pipe->id);
        qemu_put_be64(file, pipe->command_buffer_addr);
        qemu_put_be32(file, pipe->rw_params_max_count);
        qemu_put_byte(file, pipe->closed);
        qemu_put_byte(file, pipe->wanted);
        // It's possible to get a 'save' command right after the 'load' one,
        // when some force-closed pipes are still on the list.
        if (pipe->host_pipe) {
            qemu_put_byte(file, 1);
            service_ops->guest_save(pipe->host_pipe, file);
        } else {
            qemu_put_byte(file, 0);
        }
    }

    /* Save wanted pipes list. */
    HwPipe* pipe;
    int wanted_pipes_count = 0;
    for (pipe = dev->wanted_pipes_first; pipe; pipe = pipe->wanted_next) {
        wanted_pipes_count++;
    }
    qemu_put_be32(file, wanted_pipes_count);
    for (pipe = dev->wanted_pipes_first; pipe; pipe = pipe->wanted_next) {
        qemu_put_be32(file, pipe->id);
    }

    /* Invalidate all guest DMA buffer -> host ptr mappings. */
    service_ops->dma_invalidate_host_mappings();

    /* Save DMA map. */
    service_ops->dma_save_mappings(file);
}

static int goldfish_pipe_load_v1(QEMUFile* file, PipeDevice* dev) {
    HwPipe* pipe;

    assert(dev->device_version == PIPE_DEVICE_VERSION_v1);
    dev->driver_version = PIPE_DRIVER_VERSION_v1;

    /* Load i/o registers. */
    dev->address = qemu_get_be64(file);
    dev->size = qemu_get_be32(file);
    dev->status = qemu_get_be32(file);
    dev->channel = qemu_get_be64(file);
    dev->wakes = qemu_get_be32(file);
    dev->params_addr = qemu_get_be64(file);

    dev->ops = &pipe_ops_v1;

    /* Restore pipes. */
    int pipe_count = qemu_get_be32(file);
    int pipe_n;
    HwPipe** pipe_list_end = &dev->pipes_list;
    *pipe_list_end = NULL;
    uint64_t* force_closed_pipes = malloc(sizeof(uint64_t) * pipe_count);
    int force_closed_pipes_count = 0;
    for (pipe_n = 0; pipe_n < pipe_count; pipe_n++) {
        HwPipe* pipe = hwpipe_new0(dev);
        char force_close = 1;
        pipe->channel = qemu_get_be64(file);
        pipe->closed = qemu_get_byte(file);
        pipe->wanted = qemu_get_byte(file);
        char has_host_pipe = qemu_get_byte(file);
        if (has_host_pipe) {
            pipe->host_pipe = service_ops->guest_load(file, pipe, &force_close);
            if (pipe->host_pipe) {
              force_close = 0;
            }
        }
        pipe->wanted_next = pipe->wanted_prev = NULL;

        // |pipe| might be NULL in case it couldn't be saved. However,
        // in that case |force_close| will be set by goldfish_pipe_guest_load,
        // so |force_close| should be checked first so that the function
        // can continue.
        if (force_close) {
            pipe->closed = 1;
            force_closed_pipes[force_closed_pipes_count++] = pipe->channel;
        } else if (!pipe->host_pipe) {
            hwpipe_free(pipe, GOLDFISH_PIPE_CLOSE_LOAD_SNAPSHOT);
            free(force_closed_pipes);
            return -EIO;
        }

        pipe->next = NULL;
        *pipe_list_end = pipe;
        pipe_list_end = &(pipe->next);
    }

    /* Rebuild the pipes-by-channel table. */
    g_hash_table_remove_all(dev->pipes_by_channel); /* Clean up old data. */
    for(pipe = dev->pipes_list; pipe; pipe = pipe->next) {
        g_hash_table_insert(dev->pipes_by_channel,
                            hash_create_key_from_channel(pipe->channel),
                            pipe);
    }

    /* Reconstruct wanted pipes list. */
    HwPipe** wanted_pipe_list_end = &dev->wanted_pipes_first;
    *wanted_pipe_list_end = NULL;
    int wanted_pipes_count = qemu_get_be32(file);
    uint64_t channel;
    for (pipe_n = 0; pipe_n < wanted_pipes_count; ++pipe_n) {
        channel = qemu_get_be64(file);
        HwPipe* pipe = g_hash_table_lookup(dev->pipes_by_channel,
                                           hash_cast_key_from_channel(&channel));
        if (pipe) {
            pipe->wanted_prev = *wanted_pipe_list_end;
            pipe->wanted_next = NULL;
            *wanted_pipe_list_end = pipe;
            wanted_pipe_list_end = &(pipe->wanted_next);
        } else {
            free(force_closed_pipes);
            return -EIO;
        }
    }
    if (qemu_get_byte(file)) {
        channel = qemu_get_be64(file);
        dev->wanted_pipe_after_channel_high =
            g_hash_table_lookup(dev->pipes_by_channel,
                                hash_cast_key_from_channel(&channel));
        if (!dev->wanted_pipe_after_channel_high) {
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
        hwpipe_set_wanted(pipe, GOLDFISH_PIPE_WAKE_CLOSED);
        pipe->closed = 1;
        if (!pipe->wanted_next &&
            !pipe->wanted_prev &&
            pipe != dev->wanted_pipe_after_channel_high) {
            pipe->wanted_prev = *wanted_pipe_list_end;
            *wanted_pipe_list_end = pipe;
            wanted_pipe_list_end = &(pipe->wanted_next);
        }
    }

    free(force_closed_pipes);
    return 0;
}

static int goldfish_pipe_load_v2(QEMUFile* file, PipeDevice* dev) {
    int res = -EIO;
    uint32_t* force_closed_pipes = NULL;

    /* Load the device. */
    dev->device_version = qemu_get_be32(file);
    if (dev->device_version > PIPE_DEVICE_VERSION) {
        goto done;
    } else if (dev->device_version == PIPE_DEVICE_VERSION_v1) {
        // redirect to the v1 loader if this is an old pipe.
        return goldfish_pipe_load_v1(file, dev);
    }

    dev->driver_version = qemu_get_be32(file);
    if (dev->driver_version > MAX_SUPPORTED_DRIVER_VERSION) {
        goto done;
    }

    dev->signalled_pipe_buffer_size = qemu_get_be32(file);
    dev->signalled_pipe_buffer_addr = qemu_get_be64(file);
    dev->signalled_pipe_buffer = (GuestSignalledPipe*)map_guest_buffer(
            dev->signalled_pipe_buffer_addr,
            sizeof(*dev->signalled_pipe_buffer) *
                    dev->signalled_pipe_buffer_size,
            /*is_write*/1);
    if (!dev->signalled_pipe_buffer) {
        goto done;
    }
    dev->open_command_addr = qemu_get_be64(file);
    dev->open_command = (OpenCommandParams*)map_guest_buffer(
            dev->open_command_addr, sizeof(*dev->open_command), /*is_write*/0);
    if (!dev->open_command) {
        goto done;
    }

    dev->ops = &pipe_ops_v2;

    int pipes_capacity = qemu_get_be32(file);
    if (dev->pipes_capacity < pipes_capacity) {
        void* pipes = calloc(pipes_capacity, sizeof(*dev->pipes));
        if (!pipes) {
            goto done;
        }
        // No need to memcpy() the old array - we've already freed
        // all pipes there.
        dev->pipes = pipes;
    }
    dev->pipes_capacity = pipes_capacity;

    int pipe_count = qemu_get_be32(file);
    force_closed_pipes = malloc(sizeof(*force_closed_pipes) * pipe_count);
    if (!force_closed_pipes) {
        goto done;
    }
    int i;
    int force_closed_pipes_count = 0;
    for (i = 0; i < pipe_count; ++i) {
        HwPipe* pipe = hwpipe_new0(dev);
        pipe->id = qemu_get_be32(file);
        pipe->command_buffer_addr = qemu_get_be64(file);
        pipe->command_buffer = (PipeCommand*)map_guest_buffer(
                pipe->command_buffer_addr, COMMAND_BUFFER_SIZE, /*is_write*/1);
        if (!pipe->command_buffer) {
            hwpipe_free(pipe, GOLDFISH_PIPE_CLOSE_ERROR);
            goto done;
        }
        pipe->rw_params_max_count = qemu_get_be32(file);
        pipe->closed = qemu_get_byte(file);
        pipe->wanted = qemu_get_byte(file);

        char force_close = 0;
        char has_host_pipe = qemu_get_byte(file);
        if (has_host_pipe) {
            pipe->host_pipe = service_ops->guest_load(file, pipe, &force_close);
        } else {
            force_close = 1;
        }

        // |pipe| might be NULL in case it couldn't be saved. However,
        // in that case |force_close| will be set by goldfish_pipe_guest_load,
        // so |force_close| should be checked first so that the function
        // can continue.
        if (force_close) {
            pipe->closed = 1;
            force_closed_pipes[force_closed_pipes_count++] = pipe->id;
        } else if (!pipe->host_pipe) {
            unmap_command_buffer(pipe->command_buffer);
            hwpipe_free(pipe, GOLDFISH_PIPE_CLOSE_LOAD_SNAPSHOT);
            goto done;
        }

        if (dev->pipes[pipe->id]) {
            unmap_command_buffer(pipe->command_buffer);
            hwpipe_free(pipe, GOLDFISH_PIPE_CLOSE_ERROR);
            goto done;
        }
        dev->pipes[pipe->id] = pipe;
    }

    /* Reconstruct wanted pipes list. */
    int wanted_pipes_count = qemu_get_be32(file);
    for (i = 0; i < wanted_pipes_count; ++i) {
        int id = qemu_get_be32(file);
        HwPipe* pipe = dev->pipes[id];
        if (!pipe) {
            goto done;
        }
        wanted_pipes_add_v2(dev, pipe);
    }

    /* Add forcibly closed pipes to wanted pipes list */
    for (i = 0; i < force_closed_pipes_count; ++i) {
        HwPipe* pipe = dev->pipes[force_closed_pipes[i]];
        hwpipe_set_wanted(pipe, GOLDFISH_PIPE_WAKE_CLOSED);
        pipe->closed = 1;
        wanted_pipes_add_v2(dev, pipe);
    }

    res = 0;

    /* Load DMA mappings. */
    service_ops->dma_load_mappings(file);

    /* Invalidate all guest DMA buffer -> host ptr mappings. */
    service_ops->dma_invalidate_host_mappings();

done:
    free(force_closed_pipes);
    return res;
}

static int goldfish_pipe_load(QEMUFile* f, void* opaque, int version_id) {
    GoldfishPipeState* s = opaque;
    PipeDevice* dev = s->dev;

    /* As a first step close all old pipes to make sure they don't interfere
     * with the loading process.
     */
    dev->wanted_pipes_first = NULL;
    dev->wanted_pipe_after_channel_high = NULL;
    dev->ops->close_all(dev, GOLDFISH_PIPE_CLOSE_LOAD_SNAPSHOT);

    service_ops->guest_pre_load(f);
    int res = goldfish_pipe_load_v2(f, dev);
    service_ops->guest_post_load(f);
    return res;
}

static void goldfish_pipe_post_load(void* opaque) {
    /* This function gets invoked after all VM state has
     * been loaded. Raising IRQ in the load handler causes
     * problems.
     */
    PipeDevice* dev = ((GoldfishPipeState*)opaque)->dev;
    qemu_set_irq(dev->ps->irq, 1);
}

static GoldfishPipeState* s_goldfish_pipe_state = NULL;

static void goldfish_pipe_realize(DeviceState* dev, Error** errp) {
    SysBusDevice* sbdev = SYS_BUS_DEVICE(dev);
    GoldfishPipeState* s = GOLDFISH_PIPE(dev);
    s_goldfish_pipe_state = s;

    s->dev = (PipeDevice*)g_malloc0(sizeof(PipeDevice));
    s->dev->ps = s; /* HACK: backlink */

    s->dev->ops = &pipe_ops_v2;
    s->dev->device_version = PIPE_DEVICE_VERSION;
    s->dev->driver_version = PIPE_DRIVER_VERSION_v1;

    s->dev->pipes_capacity = 64;
    s->dev->pipes = calloc(s->dev->pipes_capacity, sizeof(*s->dev->pipes));
    if (!s->dev->pipes) {
        APANIC("%s: failed to initialize pipes array\n", __func__);
    }

    s->dev->pipes_by_channel = g_hash_table_new_full(
            hash_channel, hash_channel_equal, hash_channel_destroy, NULL);
    if (!s->dev->pipes_by_channel) {
        APANIC("%s: failed to initialize pipes hash\n", __func__);
    }

    memory_region_init_io(&s->iomem, OBJECT(s), &goldfish_pipe_iomem_ops, s,
                          "goldfish_pipe", 0x2000 /*TODO: ?how big?*/);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);

    s->dev->measure_latency = false;
    {
        char* android_emu_trace_env_var =
            getenv("ANDROID_EMU_TRACING");
        s->dev->measure_latency =
            android_emu_trace_env_var &&
            !strcmp("1", android_emu_trace_env_var);
    }

    register_savevm_with_post_load(
            dev, "goldfish_pipe", 0, GOLDFISH_PIPE_SAVE_VERSION,
            goldfish_pipe_save, goldfish_pipe_load, goldfish_pipe_post_load, s);
}

void goldfish_pipe_reset(GoldfishHwPipe *pipe, GoldfishHostPipe *host_pipe) {
    pipe->host_pipe = host_pipe;
}

void goldfish_pipe_signal_wake(GoldfishHwPipe *pipe,
                               GoldfishPipeWakeFlags flags) {
    if (!pipe) return;

    PipeDevice *dev = pipe->dev;

    DD("%s: id=%d channel=0x%llx flags=%d", __func__, (int)pipe->id,
       pipe->channel, flags);

    hwpipe_set_wanted(pipe, (unsigned char)flags);
    dev->ops->wanted_list_add(dev, pipe);

    /* Raise IRQ to indicate there are items on our list ! */
    qemu_set_irq(dev->ps->irq, 1);
    DD("%s: raising IRQ", __func__);

    if (dev->measure_latency) {
        dev->wake_us = pipe_dev_curr_time_us();
    }
}

/* Function to look up hwpipe by pipe id and vice versa. */
int goldfish_pipe_get_id(GoldfishHwPipe* pipe) {
    assert(pipe->dev->device_version == PIPE_DEVICE_VERSION);
    return pipe->id;
}

GoldfishHwPipe* goldfish_pipe_lookup_by_id(int id) {
    assert(s_goldfish_pipe_state->dev->device_version == PIPE_DEVICE_VERSION);
    return s_goldfish_pipe_state->dev->pipes[id];
}

void goldfish_pipe_close_from_host(GoldfishHwPipe *pipe)
{
    D("%s: id=%d channel=0x%llx (closed=%d)", __func__, (int)pipe->id,
        pipe->channel, pipe->closed);

    if (!pipe->closed) {
        pipe->closed = 1;
        goldfish_pipe_signal_wake(pipe, GOLDFISH_PIPE_WAKE_CLOSED);
    }
}

static void goldfish_pipe_class_init(ObjectClass* klass, void* data) {
    DeviceClass* dc = DEVICE_CLASS(klass);
    dc->realize = goldfish_pipe_realize;
    dc->desc = "goldfish pipe";
}

static const TypeInfo goldfish_pipe_info = {
    .name          = TYPE_GOLDFISH_PIPE,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(GoldfishPipeState),
    .class_init    = goldfish_pipe_class_init
};

static void goldfish_pipe_register(void) {
    type_register_static(&goldfish_pipe_info);
}

type_init(goldfish_pipe_register);

static const PipeOperations pipe_ops_v2 = {
        .wanted_list_add = &wanted_pipes_add_v2,
        .close_all = &close_all_pipes_v2,
        .save = &goldfish_pipe_save_v2,
        .dev_read = &pipe_dev_read_v2,
        .dev_write = &pipe_dev_write_v2
};

static const PipeOperations pipe_ops_v1 = {
        .wanted_list_add = &wanted_pipes_add_v1,
        .close_all = &close_all_pipes_v1,
        .save = &goldfish_pipe_save_v1,
        .dev_read = &pipe_dev_read_v1,
        .dev_write = &pipe_dev_write_v1
};
