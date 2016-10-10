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
} PipeCmd;

enum {
    MAX_BUFFERS_COUNT = 160,
    COMMAND_BUFFER_SIZE = 4096,
};

#define TYPE_ANDROID_PIPE "android_pipe"
#define ANDROID_PIPE(obj) \
    OBJECT_CHECK(AndroidPipeState, (obj), TYPE_ANDROID_PIPE)

/* Update this version number if the device's interface changes. */
enum {
    PIPE_DEVICE_VERSION = 2,
    PIPE_DEVICE_VERSION_v1 = 1,
    MAX_SUPPORTED_DRIVER_VERSION = 2,
    PIPE_DRIVER_VERSION_v1 = 0,  // used to not report its version at all
};

/* from AOSP version include/hw/android/goldfish/device.h
 * FIXME?: needs to use proper qemu abstractions
 */
static inline void uint64_set_low(uint64_t* addr, uint32 value) {
    *addr = (*addr & ~(0xFFFFFFFFULL)) | value;
}

static inline void uint64_set_high(uint64_t* addr, uint32 value) {
    *addr = (*addr & 0xFFFFFFFFULL) | ((uint64_t)value << 32);
}

typedef struct HwPipe HwPipe;
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
    void (*wanted_list_add)(PipeDevice* dev, HwPipe* pipe);
    void (*close_all)(PipeDevice* dev);
    void (*save)(Stream* stream, PipeDevice* dev);

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

    PipeDevice* dev;
} AndroidPipeState;

typedef struct PipeCommand {
    int32_t cmd;
    int32_t id;
    int32_t status;
    int32_t padding;
    union {
        struct {
            uint64_t ptrs[MAX_BUFFERS_COUNT];
            uint32_t sizes[MAX_BUFFERS_COUNT];
            uint32_t buffers_count;
            int32_t consumed_size;
        } rw_params;
    };
} PipeCommand;

typedef struct HwPipe {
    struct HwPipe* wanted_next;
    struct HwPipe* wanted_prev;
    PipeDevice* dev;
    uint32_t id;  // pipe ID is its index into the PipeDevice::pipes array
    unsigned char wanted;
    char closed;
    void* pipe_impl_by_user;
    uint64_t command_buffer_addr;
    PipeCommand* command_buffer;

    // v1-specific fields
    struct HwPipe* next;
    uint64_t channel; /* opaque kernel handle */
} HwPipe;

typedef struct GuestSignalledPipe {
    uint32_t id;
    uint32_t flags;
} GuestSignalledPipe;

typedef struct OpenCommandParams {
    uint64_t command_buffer_ptr;
} OpenCommandParams;

typedef struct PipeDevice {
    AndroidPipeState* ps;  // backlink to instance state
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
} PipeDevice;


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

static unsigned char get_and_clear_pipe_wanted(HwPipe* pipe) {
    unsigned char val = pipe->wanted;
    pipe->wanted = 0;
    return val;
}

static void set_pipe_wanted_bits(HwPipe* pipe, unsigned char val) {
    pipe->wanted |= val;
}

static HwPipe* pipe_new0(PipeDevice* dev) {
    HwPipe* pipe;
    pipe = g_malloc0(sizeof(HwPipe));
    pipe->dev = dev;
    return pipe;
}

static HwPipe* pipe_new(uint32_t id, uint64_t channel, PipeDevice* dev) {
    HwPipe* pipe = pipe_new0(dev);
    pipe->id = id;
    pipe->channel = channel;
    pipe->pipe_impl_by_user = android_pipe_guest_open(pipe);
    return pipe;
}

static void pipe_free(HwPipe* pipe) {
    android_pipe_guest_close(pipe->pipe_impl_by_user);
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

static void close_all_pipes_v1(PipeDevice* dev) {
    HwPipe* pipe = dev->pipes_list;
    while (pipe) {
        HwPipe* next = pipe->next;
        pipe_free(pipe);
        pipe = next;
    }
    dev->pipes_list = NULL;
}

static void close_all_pipes_v2(PipeDevice* dev) {
    int i = 0;
    for (; i < dev->pipes_capacity; ++i) {
        HwPipe* pipe = dev->pipes[i];
        if (pipe) {
            unmap_command_buffer(pipe->command_buffer);
            pipe_free(pipe);
            dev->pipes[i] = NULL;
        }
    }
}

static void reset_pipe_device(PipeDevice* dev) {
    dev->wanted_pipes_first = NULL;
    dev->wanted_pipe_after_channel_high = NULL;
    g_hash_table_remove_all(dev->pipes_by_channel);
    qemu_set_irq(dev->ps->irq, 0);
}

static void pipeDevice_doCommand_v1(PipeDevice* dev, uint32_t command) {
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
        pipe = pipe_new(0, dev->channel, dev);
        pipe->next = dev->pipes_list;
        dev->pipes_list = pipe;
        dev->status = 0;
        g_hash_table_insert(dev->pipes_by_channel,
                            hash_create_key_from_channel(dev->channel), pipe);
        break;

    case PIPE_CMD_CLOSE: {
        DD("%s: CMD_CLOSE channel=0x%llx", __FUNCTION__, (unsigned long long)dev->channel);
        // Remove from device's lists.
        // This linear lookup is potentially slow, but we don't delete pipes
        // often enough for it to become noticable.
        HwPipe** pnode = &dev->pipes_list;
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
        wanted_pipes_remove_v1(dev, pipe);

        pipe_free(pipe);
        break;
    }

    case PIPE_CMD_POLL:
        dev->status = android_pipe_guest_poll(pipe->pipe_impl_by_user);
        DD("%s: CMD_POLL > status=%d", __FUNCTION__, dev->status);
        break;

    case PIPE_CMD_READ: {
        /* Translate guest physical address into emulator memory. */
        AndroidPipeBuffer  buffer;
        buffer.data = map_guest_buffer(dev->address, dev->size, /*is_write*/1);
        if (!buffer.data) {
            dev->status = PIPE_ERROR_INVAL;
            break;
        }
        buffer.size = dev->size;
        dev->status = android_pipe_guest_recv(pipe->pipe_impl_by_user, &buffer, 1);
        DD("%s: CMD_READ channel=0x%llx address=0x%16llx size=%d > status=%d",
           __FUNCTION__, (unsigned long long)dev->channel, (unsigned long long)dev->address,
           dev->size, dev->status);
        cpu_physical_memory_unmap(buffer.data, dev->size,
                                  /*is_write*/1, dev->size);
        break;
    }

    case PIPE_CMD_WRITE: {
        /* Translate guest physical address into emulator memory. */
        AndroidPipeBuffer  buffer;
        buffer.data = map_guest_buffer(dev->address, dev->size, /*is_write*/0);
        if (!buffer.data) {
            dev->status = PIPE_ERROR_INVAL;
            break;
        }
        buffer.size = dev->size;
        dev->status = android_pipe_guest_send(pipe->pipe_impl_by_user, &buffer, 1);
        DD("%s: CMD_WRITE_BUFFER channel=0x%llx address=0x%16llx size=%d > status=%d",
           __FUNCTION__, (unsigned long long)dev->channel, (unsigned long long)dev->address,
           dev->size, dev->status);
        cpu_physical_memory_unmap(buffer.data, dev->size,
                                  /*is_write*/0, dev->size);
        break;
    }

    case PIPE_CMD_WAKE_ON_READ:
        DD("%s: CMD_WAKE_ON_READ channel=0x%llx", __FUNCTION__, (unsigned long long)dev->channel);
        if ((pipe->wanted & PIPE_WAKE_READ) == 0) {
            pipe->wanted |= PIPE_WAKE_READ;
            android_pipe_guest_wake_on(pipe->pipe_impl_by_user, pipe->wanted);
        }
        dev->status = 0;
        break;

    case PIPE_CMD_WAKE_ON_WRITE:
        DD("%s: CMD_WAKE_ON_WRITE channel=0x%llx", __FUNCTION__, (unsigned long long)dev->channel);
        if ((pipe->wanted & PIPE_WAKE_WRITE) == 0) {
            pipe->wanted |= PIPE_WAKE_WRITE;
            android_pipe_guest_wake_on(pipe->pipe_impl_by_user, pipe->wanted);
        }
        dev->status = 0;
        break;

    default:
        D("%s: command=%d (0x%x)\n", __FUNCTION__, command, command);
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
    if (commandBuffer->id != id) {
        commandBuffer->status = PIPE_ERROR_INVAL;
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
        commandBuffer->status = PIPE_ERROR_INVAL;
        unmap_command_buffer(commandBuffer);
        return;
    }
    if (id >= dev->pipes_capacity) {
        int newCapacity = (id + 1 > 2 * dev->pipes_capacity)
                                  ? id + 1
                                  : 2 * dev->pipes_capacity;
        HwPipe** pipes = calloc(newCapacity, sizeof(HwPipe*));
        if (!pipes) {
            commandBuffer->status = PIPE_ERROR_NOMEM;
            unmap_command_buffer(commandBuffer);
            return;
        }
        memcpy(pipes, dev->pipes, sizeof(HwPipe*) * dev->pipes_capacity);
        dev->pipes = pipes;
        dev->pipes_capacity = newCapacity;
    }

    HwPipe* pipe = pipe_new(id, 0, dev);
    if (!pipe || !pipe->pipe_impl_by_user) {
        free(pipe);
        commandBuffer->status = PIPE_ERROR_NOMEM;
        unmap_command_buffer(commandBuffer);
        return;
    }

    pipe->command_buffer_addr = dev->open_command->command_buffer_ptr;
    pipe->command_buffer = commandBuffer;
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
        pipe->command_buffer->status = PIPE_ERROR_IO;
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
            pipe_free(pipe);
            break;
        }

        case PIPE_CMD_POLL:
            pipe->command_buffer->status = android_pipe_guest_poll(pipe->pipe_impl_by_user);
            DD("%s: CMD_POLL > status=%d", __func__,
               pipe->command_buffer->status);
            break;

        case PIPE_CMD_READ:
        case PIPE_CMD_WRITE: {
            bool willModifyData = command == PIPE_CMD_READ;
            pipe->command_buffer->rw_params.consumed_size = 0;
            unsigned buffers_count =
                    pipe->command_buffer->rw_params.buffers_count;
            if (buffers_count > MAX_BUFFERS_COUNT) {
                buffers_count = MAX_BUFFERS_COUNT;
            }

            AndroidPipeBuffer buffers[MAX_BUFFERS_COUNT];
            unsigned i;
            for (i = 0; i < buffers_count; ++i) {
                buffers[i].size = pipe->command_buffer->rw_params.sizes[i];
                buffers[i].data = map_guest_buffer(
                        pipe->command_buffer->rw_params.ptrs[i],
                        pipe->command_buffer->rw_params.sizes[i],
                        willModifyData);
                if (!buffers[i].data) {
                    pipe->command_buffer->status = PIPE_ERROR_INVAL;
                    goto done;
                }
            }

            pipe->command_buffer->status =
                    willModifyData
                            ? android_pipe_guest_recv(pipe->pipe_impl_by_user, buffers,
                                                      buffers_count)
                            : android_pipe_guest_send(pipe->pipe_impl_by_user, buffers,
                                                      buffers_count);
            // TODO(zyy): create an extended version of send()/recv() functions
            // to
            //  return both transferred size and resulting status in single
            //  call.
            pipe->command_buffer->rw_params.consumed_size =
                    pipe->command_buffer->status < 0
                            ? 0
                            : pipe->command_buffer->status;
            DD("%s: CMD_%s id=%d buffers=%d > status=%d", __func__,
               (willModifyData ? "READ" : "WRITE"), (int)pipe->id,
               (int)buffers_count, pipe->command_buffer->status);

            // C doesn't allow a label on a declaration, so this has to be
            // before the labelled statement.
            unsigned j;
        done:
            for (j = 0; j < i; ++j) {
                cpu_physical_memory_unmap(buffers[j].data, buffers[j].size,
                                          willModifyData, buffers[j].size);
            }
            break;
        }

        case PIPE_CMD_WAKE_ON_READ:
        case PIPE_CMD_WAKE_ON_WRITE: {
            bool read = (command == PIPE_CMD_WAKE_ON_READ);
            DD("%s: CMD_WAKE_ON_%s id=%d", __func__, (read ? "READ" : "WRITE"),
               (int)pipe->id);
            if ((pipe->wanted & (read ? PIPE_WAKE_READ : PIPE_WAKE_WRITE)) ==
                0) {
                pipe->wanted |= (read ? PIPE_WAKE_READ : PIPE_WAKE_WRITE);
                android_pipe_guest_wake_on(pipe->pipe_impl_by_user, pipe->wanted);
            }
            pipe->command_buffer->status = 0;
            break;
        }
        default:
            D("%s: command=%d (0x%x)\n", __func__, command, command);
    }
}

static void pipe_dev_write(void* opaque,
                           hwaddr offset,
                           uint64_t value,
                           unsigned size) {
    AndroidPipeState* state = opaque;
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
    AndroidPipeState* s = (AndroidPipeState*)opaque;
    PipeDevice* dev = s->dev;
    if (offset == PIPE_REG_VERSION) {
        // PIPE_REG_VERSION is issued on probe, which means that
        // we should clean up all existing stale pipes.
        // This helps keep the right state on rebooting.
        dev->ops->close_all(dev);
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

static void pipe_dev_write_v2(PipeDevice* dev,
                                  hwaddr offset,
                                  uint64_t value) {
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
                APANIC("%s: failed to map opend command buffer\n", __func__);
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
}

static uint64_t pipe_dev_read_v1(PipeDevice* dev, hwaddr offset)
{
    switch (offset) {
    case PIPE_REG_STATUS:
        DR("%s: REG_STATUS status=%d (0x%x)", __FUNCTION__, dev->status, dev->status);
        return dev->status;

    case PIPE_REG_CHANNEL: {
        HwPipe* wanted_pipe = wanted_pipes_pop_first_v1(dev);
        if (wanted_pipe != NULL) {
            dev->wakes = get_and_clear_pipe_wanted(wanted_pipe);
            DR("%s: channel=0x%llx wanted=%d", __FUNCTION__,
               (unsigned long long)wanted_pipe->channel, dev->wakes);
            return (uint32_t)(wanted_pipe->channel & 0xFFFFFFFFUL);
        } else {
            qemu_set_irq(dev->ps->irq, 0);
            DD("%s: no signaled channels, lowering IRQ", __FUNCTION__);
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
            DR("%s: channel_high=0x%llx wanted=%d", __FUNCTION__,
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
        DR("%s: wakes %d", __FUNCTION__, dev->wakes);
        return dev->wakes;

    case PIPE_REG_PARAMS_ADDR_HIGH:
        return (uint32_t)(dev->params_addr >> 32);

    case PIPE_REG_PARAMS_ADDR_LOW:
        return (uint32_t)(dev->params_addr & 0xFFFFFFFFUL);

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: unknown register %" HWADDR_PRId
                      " (0x%" HWADDR_PRIx ")\n", __FUNCTION__, offset, offset);
    }
    return 0;
}

static uint64_t pipe_dev_read_v2(PipeDevice* dev, hwaddr offset) {
    switch (offset) {
        case PIPE_REG_GET_SIGNALLED: {
            int count = 0;
            HwPipe* pipe;
            while (count < dev->signalled_pipe_buffer_size &&
                   (pipe = wanted_pipes_pop_first_v2(dev)) != NULL) {
                dev->signalled_pipe_buffer[count].id = pipe->id;
                dev->signalled_pipe_buffer[count].flags =
                        get_and_clear_pipe_wanted(pipe);
                ++count;
            }
            if (!dev->wanted_pipes_first) {
                qemu_set_irq(dev->ps->irq, 0);  // we've passed all wanted pipes
            }
            return count;
        }
        default:
            qemu_log_mask(LOG_GUEST_ERROR, "%s: unknown register %" HWADDR_PRId
                                           " (0x%" HWADDR_PRIx ")\n",
                          __func__, offset, offset);
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

// Don't change this version unless you want to break forward compatibility.
// Instead, use the different device version as a first saved field.
enum {
    ANDROID_PIPE_SAVE_VERSION = 1,
};

static void android_pipe_save(QEMUFile* f, void* opaque) {
    AndroidPipeState* s = opaque;
    PipeDevice* dev = s->dev;
    Stream* stream = stream_from_qemufile(f);
    dev->ops->save(stream, dev);
    stream_free(stream);
}

static void android_pipe_save_v1(Stream* stream, PipeDevice* dev) {
    assert(dev->device_version == PIPE_DEVICE_VERSION_v1);
    stream_put_be32(stream, dev->device_version);

    /* Save the device version */
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
    for (pipe = dev->pipes_list; pipe; pipe = pipe->next) {
        ++pipe_count;
    }
    stream_put_be32(stream, pipe_count);
    for (pipe = dev->pipes_list; pipe; pipe = pipe->next) {
        stream_put_be64(stream, pipe->channel);
        stream_put_byte(stream, pipe->closed);
        stream_put_byte(stream, pipe->wanted);
        android_pipe_guest_save(pipe->pipe_impl_by_user, stream);
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
}

static void android_pipe_save_v2(Stream* stream, PipeDevice* dev) {
    assert(dev->device_version == PIPE_DEVICE_VERSION);
    stream_put_be32(stream, dev->device_version);

    /* Save the device data. */
    stream_put_be32(stream, dev->driver_version);
    stream_put_be32(stream, dev->signalled_pipe_buffer_size);
    stream_put_be64(stream, dev->signalled_pipe_buffer_addr);
    stream_put_be64(stream, dev->open_command_addr);
    stream_put_be32(stream, dev->pipes_capacity);

    /* Save the pipes. */
    int i;
    int pipe_count = 0;
    for (i = 0; i < dev->pipes_capacity; ++i) {
        if (dev->pipes[i]) {
            ++pipe_count;
        }
    }
    stream_put_be32(stream, pipe_count);

    for (i = 0; i < dev->pipes_capacity; ++i) {
        HwPipe* pipe = dev->pipes[i];
        if (!pipe) {
            continue;
        }
        stream_put_be32(stream, pipe->id);
        stream_put_be64(stream, pipe->command_buffer_addr);
        stream_put_byte(stream, pipe->closed);
        stream_put_byte(stream, pipe->wanted);
        android_pipe_guest_save(pipe->pipe_impl_by_user, stream);
    }

    /* Save wanted pipes list. */
    HwPipe* pipe;
    int wanted_pipes_count = 0;
    for (pipe = dev->wanted_pipes_first; pipe; pipe = pipe->wanted_next) {
        wanted_pipes_count++;
    }
    stream_put_be32(stream, wanted_pipes_count);
    for (pipe = dev->wanted_pipes_first; pipe; pipe = pipe->wanted_next) {
        stream_put_be32(stream, pipe->id);
    }
}

static int android_pipe_load_v1(Stream* stream, PipeDevice* dev) {
    HwPipe* pipe;

    assert(dev->device_version == PIPE_DEVICE_VERSION_v1);
    dev->driver_version = PIPE_DRIVER_VERSION_v1;

    /* Load i/o registers. */
    dev->address = stream_get_be64(stream);
    dev->size = stream_get_be32(stream);
    dev->status = stream_get_be32(stream);
    dev->channel = stream_get_be64(stream);
    dev->wakes = stream_get_be32(stream);
    dev->params_addr = stream_get_be64(stream);

    /* Clean up old pipe objects. */
    dev->ops->close_all(dev);
    dev->ops = &pipe_ops_v1;

    /* Restore pipes. */
    int pipe_count = stream_get_be32(stream);
    int pipe_n;
    HwPipe** pipe_list_end = &dev->pipes_list;
    *pipe_list_end = NULL;
    uint64_t* force_closed_pipes = malloc(sizeof(uint64_t) * pipe_count);
    int force_closed_pipes_count = 0;
    for (pipe_n = 0; pipe_n < pipe_count; pipe_n++) {
        HwPipe* pipe = pipe_new0(dev);
        char force_close = 0;
        pipe->channel = stream_get_be64(stream);
        pipe->closed = stream_get_byte(stream);
        pipe->wanted = stream_get_byte(stream);
        pipe->pipe_impl_by_user = android_pipe_guest_load(stream, pipe, &force_close);
        pipe->wanted_next = pipe->wanted_prev = NULL;

        // |pipe| might be NULL in case it couldn't be saved. However,
        // in that case |force_close| will be set by android_pipe_guest_load,
        // so |force_close| should be checked first so that the function
        // can continue.
        if (force_close) {
            pipe->closed = 1;
            force_closed_pipes[force_closed_pipes_count++] = pipe->channel;
        } else if (!pipe->pipe_impl_by_user) {
            pipe_free(pipe);
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

    free(force_closed_pipes);
    return 0;
}

static int android_pipe_load_v2(Stream* stream, PipeDevice* dev) {
    int res = -EIO;
    uint32_t* force_closed_pipes = NULL;

    /* Load the device. */
    dev->device_version = stream_get_be32(stream);
    if (dev->device_version > PIPE_DEVICE_VERSION) {
        goto done;
    } else if (dev->device_version == PIPE_DEVICE_VERSION_v1) {
        // redirect to the v1 loader if this is an old pipe.
        return android_pipe_load_v1(stream, dev);
    }

    dev->driver_version = stream_get_be32(stream);
    if (dev->driver_version > MAX_SUPPORTED_DRIVER_VERSION) {
        goto done;
    }

    dev->signalled_pipe_buffer_size = stream_get_be32(stream);
    dev->signalled_pipe_buffer_addr = stream_get_be64(stream);
    dev->signalled_pipe_buffer = (GuestSignalledPipe*)map_guest_buffer(
            dev->signalled_pipe_buffer_addr,
            sizeof(*dev->signalled_pipe_buffer) *
                    dev->signalled_pipe_buffer_size,
            /*is_write*/1);
    if (!dev->signalled_pipe_buffer) {
        goto done;
    }
    dev->open_command_addr = stream_get_be64(stream);
    dev->open_command = (OpenCommandParams*)map_guest_buffer(
            dev->open_command_addr, sizeof(*dev->open_command), /*is_write*/0);
    if (!dev->open_command) {
        goto done;
    }

    /* Clean up old pipe objects. */
    dev->wanted_pipes_first = NULL;
    dev->ops->close_all(dev);
    dev->ops = &pipe_ops_v2;

    int pipes_capacity = stream_get_be32(stream);
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

    int pipe_count = stream_get_be32(stream);
    force_closed_pipes = malloc(sizeof(*force_closed_pipes) * pipe_count);
    if (!force_closed_pipes) {
        goto done;
    }
    int i;
    int force_closed_pipes_count = 0;
    for (i = 0; i < pipe_count; ++i) {
        HwPipe* pipe = pipe_new0(dev);
        pipe->id = stream_get_be32(stream);
        pipe->command_buffer_addr = stream_get_be64(stream);
        pipe->command_buffer = (PipeCommand*)map_guest_buffer(
                pipe->command_buffer_addr, COMMAND_BUFFER_SIZE, /*is_write*/1);
        if (!pipe->command_buffer) {
            pipe_free(pipe);
            goto done;
        }
        pipe->closed = stream_get_byte(stream);
        pipe->wanted = stream_get_byte(stream);

        char force_close = 0;
        pipe->pipe_impl_by_user = android_pipe_guest_load(stream, pipe, &force_close);

        // |pipe| might be NULL in case it couldn't be saved. However,
        // in that case |force_close| will be set by android_pipe_guest_load,
        // so |force_close| should be checked first so that the function
        // can continue.
        if (force_close) {
            pipe->closed = 1;
            force_closed_pipes[force_closed_pipes_count++] = pipe->id;
        } else if (!pipe->pipe_impl_by_user) {
            unmap_command_buffer(pipe->command_buffer);
            pipe_free(pipe);
            goto done;
        }

        if (dev->pipes[pipe->id]) {
            unmap_command_buffer(pipe->command_buffer);
            pipe_free(pipe);
            goto done;
        }
        dev->pipes[pipe->id] = pipe;
    }

    /* Reconstruct wanted pipes list. */
    int wanted_pipes_count = stream_get_be32(stream);
    for (i = 0; i < wanted_pipes_count; ++i) {
        int id = stream_get_be32(stream);
        HwPipe* pipe = dev->pipes[id];
        if (!pipe) {
            goto done;
        }
        wanted_pipes_add_v2(dev, pipe);
    }

    /* Add forcibly closed pipes to wanted pipes list */
    for (i = 0; i < force_closed_pipes_count; ++i) {
        HwPipe* pipe = dev->pipes[force_closed_pipes[i]];
        set_pipe_wanted_bits(pipe, PIPE_WAKE_CLOSED);
        pipe->closed = 1;
        wanted_pipes_add_v2(dev, pipe);
    }

    res = 0;

done:
    free(force_closed_pipes);
    return res;
}

static int android_pipe_load(QEMUFile* f, void* opaque, int version_id) {
    AndroidPipeState* s = opaque;
    PipeDevice* dev = s->dev;
    Stream* stream = stream_from_qemufile(f);
    int res = android_pipe_load_v2(stream, dev);
    stream_free(stream);
    return res;
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

static void android_pipe_realize(DeviceState* dev, Error** errp) {
    SysBusDevice* sbdev = SYS_BUS_DEVICE(dev);
    AndroidPipeState* s = ANDROID_PIPE(dev);

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

    memory_region_init_io(&s->iomem, OBJECT(s), &android_pipe_iomem_ops, s,
                          "android_pipe", 0x2000 /*TODO: ?how big?*/);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);

    android_pipe_set_hw_funcs(&qemu2_android_pipe_hw_funcs);

    register_savevm_with_post_load(
            dev, "android_pipe", 0, ANDROID_PIPE_SAVE_VERSION,
            android_pipe_save, android_pipe_load, android_pipe_post_load, s);
}

static void qemu2_android_pipe_reset(void* hwpipe, void* internal_pipe) {
    HwPipe* pipe = hwpipe;
    pipe->pipe_impl_by_user = internal_pipe;
}

static void qemu2_android_pipe_host_signal_wake(void* hwpipe, unsigned flags) {
    HwPipe* pipe = hwpipe;
    PipeDevice* dev = pipe->dev;

    DD("%s: id=%d channel=0x%llx flags=%d", __func__,
        (int)pipe->id, pipe->channel, flags);

    set_pipe_wanted_bits(pipe, (unsigned char)flags);
    dev->ops->wanted_list_add(dev, pipe);

    /* Raise IRQ to indicate there are items on our list ! */
    qemu_set_irq(dev->ps->irq, 1);
    DD("%s: raising IRQ", __func__);
}

static void qemu2_android_pipe_host_close(void* hwpipe) {
    HwPipe* pipe = hwpipe;

    D("%s: id=%d channel=0x%llx (closed=%d)", __func__, (int)pipe->id,
        pipe->channel, pipe->closed);

    if (!pipe->closed) {
        pipe->closed = 1;
        qemu2_android_pipe_host_signal_wake(hwpipe, PIPE_WAKE_CLOSED);
    }
}

static void android_pipe_class_init(ObjectClass* klass, void* data) {
    DeviceClass* dc = DEVICE_CLASS(klass);
    dc->realize = android_pipe_realize;
    dc->desc = "android pipe";
}

static const TypeInfo android_pipe_info = {
        .name = TYPE_ANDROID_PIPE,
        .parent = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(AndroidPipeState),
        .class_init = android_pipe_class_init};

static void android_pipe_register(void) {
    type_register_static(&android_pipe_info);
}

type_init(android_pipe_register);

static const PipeOperations pipe_ops_v2 = {
        .wanted_list_add = &wanted_pipes_add_v2,
        .close_all = &close_all_pipes_v2,
        .save = &android_pipe_save_v2,
        .dev_read = &pipe_dev_read_v2,
        .dev_write = &pipe_dev_write_v2
};

static const PipeOperations pipe_ops_v1 = {
        .wanted_list_add = &wanted_pipes_add_v1,
        .close_all = &close_all_pipes_v1,
        .save = &android_pipe_save_v1,
        .dev_read = &pipe_dev_read_v1,
        .dev_write = &pipe_dev_write_v1
};
