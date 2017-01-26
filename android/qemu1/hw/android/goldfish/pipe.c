/* Copyright (C) 2011 The Android Open Source Project
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

#include "android-qemu1-glue/utils/stream.h"
#include "android/utils/panic.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include "hw/android/goldfish/pipe.h"
#include "hw/android/goldfish/device.h"
#include "hw/android/goldfish/vmem.h"
#include "exec/ram_addr.h"
#include "migration/vmstate.h"

#define  DEBUG 0

/* Set to 1 to debug i/o register reads/writes */
#define DEBUG_REGS  0

#if DEBUG >= 1
#  define D(...)  fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#  define D(...)  (void)0
#endif

#if DEBUG >= 2
#  define DD(...)  fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#  define DD(...)  (void)0
#endif

#if DEBUG_REGS >= 1
#  define DR(...)   D(__VA_ARGS__)
#else
#  define DR(...)   (void)0
#endif

#define E(...)  fprintf(stderr, "ERROR:" __VA_ARGS__), fprintf(stderr, "\n")

/* Set to 1 to enable the 'zero' pipe type, useful for debugging */
#define DEBUG_ZERO_PIPE  1

/* Set to 1 to enable the 'pingpong' pipe type, useful for debugging */
#define DEBUG_PINGPONG_PIPE 1

/* Set to 1 to enable the 'throttle' pipe type, useful for debugging */
#define DEBUG_THROTTLE_PIPE 1

#define GOLDFISH_PIPE_SAVE_VERSION  4
#define GOLDFISH_PIPE_SAVE_LEGACY_VERSION  3

/***********************************************************************
 ***********************************************************************
 *****
 *****    P I P E   C O N N E C T I O N S
 *****
 *****/

typedef struct PipeDevice  PipeDevice;

/***********************************************************************
 ***********************************************************************
 *****
 *****    G O L D F I S H   P I P E   D E V I C E
 *****
 *****/

// An HwPipe instance models the virtual hardware view of a given
// Android pipe.
typedef struct HwPipe {
    struct HwPipe* next;
    struct HwPipe* next_waked;
    uint64_t channel;
    unsigned char wakes;
    unsigned char closed;
    void* pipe;
    struct PipeDevice* device;
} HwPipe;

static HwPipe* hwpipe_new0(struct PipeDevice* device) {
    HwPipe* hwp;
    ANEW0(hwp);
    hwp->device = device;
    return hwp;
}

static HwPipe* hwpipe_new(uint64_t channel, struct PipeDevice* device) {
    HwPipe* hwp = hwpipe_new0(device);
    hwp->channel = channel;
    hwp->pipe = android_pipe_guest_open(hwp);
    return hwp;
}

static void hwpipe_free(HwPipe* hwp) {
  android_pipe_guest_close(hwp->pipe);
  free(hwp);
}

static HwPipe** hwpipe_findp_by_channel(HwPipe** list, uint64_t channel) {
    HwPipe** pnode = list;
    for (;;) {
        HwPipe* node = *pnode;
        if (!node) {
            break;
        }
        if (node->channel == channel) {
            break;
        }
        pnode = &node->next;
    }
    return pnode;
}

static HwPipe** hwpipe_findp_signaled(HwPipe** list, HwPipe* pipe) {
    HwPipe** pnode = list;
    for (;;) {
        HwPipe* node = *pnode;
        if (!node || node == pipe) {
            break;
        }
        pnode = &node->next_waked;
    }
    return pnode;
}


static void hwpipe_remove_signaled(HwPipe** list, HwPipe* pipe) {
    HwPipe** pnode = hwpipe_findp_signaled(list, pipe);
    if (*pnode) {
        *pnode = pipe->next_waked;
        pipe->next_waked = NULL;
    }
}

static void hwpipe_save(HwPipe* pipe, Stream* file) {
    /* Now save other common data */
    stream_put_be64(file, pipe->channel);
    stream_put_byte(file, (int)pipe->wakes);
    stream_put_byte(file, (int)pipe->closed);

    android_pipe_guest_save(pipe->pipe, file);
}

static HwPipe* hwpipe_load(PipeDevice* dev, Stream* file, int version_id) {
    HwPipe* pipe = hwpipe_new0(dev);

    char force_close = 0;
    if (version_id == GOLDFISH_PIPE_SAVE_LEGACY_VERSION) {
      pipe->pipe = android_pipe_guest_load_legacy(file, pipe, &pipe->channel,
                                                  &pipe->wakes, &pipe->closed,
                                                  &force_close);
    } else {
        pipe->channel = stream_get_be64(file);
        pipe->wakes = stream_get_byte(file);
        pipe->closed = stream_get_byte(file);
        pipe->pipe = android_pipe_guest_load(file, pipe, &force_close);
    }

    // SUBTLE: android_pipe_guest_load() may return NULL if the pipe state
    // could not be saved. In this case |force_close| will be set though, so
    // always check it first.
    if (force_close) {
        pipe->closed = 1;
    } else if (!pipe->pipe) {
        hwpipe_free(pipe);
        return NULL;
    }

    return pipe;
}

struct PipeDevice {
    struct goldfish_device dev;

    /* the list of all pipes */
    HwPipe*  pipes;

    /* the list of signalled pipes */
    HwPipe*  signaled_pipes;

    /* i/o registers */
    uint64_t  address;
    uint32_t  size;
    uint32_t  status;
    uint64_t  channel;
    uint32_t  wakes;
    uint64_t  params_addr;
};

/* Update this version number if the device's interface changes. */
#define PIPE_DEVICE_VERSION  1

typedef enum { USE_VA, USE_PA } PipeDeviceMode;

/* ============ Backward Compatibility support ============
 *
 * By default, assume old device behavior which
 * uses guest virtual addresses for read/write operations.
 * New driver will change the device mode to operate with
 * guest physical addresses by reading the version register.
 */
static PipeDeviceMode deviceMode = USE_VA;

/* Map the guest buffer into host memory.
 *
 * @xaddr    - Guest VA or PA depending on the driver version.
 * @size     - Size of the mapping
 * @is_write - Is it a read or write?
 *
 * @Return - Return a host pointer which should be unmapped later via
 * cpu_physical_memory_unmap(), or NULL if mapping failed
 * (likely because the paddr doesn't actually point at RAM).
 * Note that for RAM the "mapping" process doesn't actually involve a
 * data copy.
 */
static void *map_guest_buffer(target_ulong xaddr, size_t size, int is_write)
{
    CPUOldState* env = cpu_single_env;
    target_ulong page = xaddr & TARGET_PAGE_MASK;
    hwaddr l = size;
    hwaddr phys = xaddr;
    void *ptr;

    if (unlikely(deviceMode == USE_VA)) {
        phys = safe_get_phys_page_debug(ENV_GET_CPU(env), page);
#ifdef TARGET_X86_64
        phys = phys & TARGET_PTE_MASK;
#endif
    }

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

    if (unlikely(deviceMode == USE_VA)) {
        ptr += xaddr - page;
    }

    return ptr;
}

static void close_all_pipes(void* opaque) {
    struct PipeDevice* dev = opaque;
    HwPipe* pipe = dev->pipes;
    while(pipe) {
        HwPipe* old_pipe = pipe;
        pipe = pipe->next;
        hwpipe_remove_signaled(&dev->signaled_pipes, old_pipe);
        hwpipe_free(old_pipe);
    }
}

static void reset_pipe_device(void* opaque) {
    PipeDevice* dev = opaque;
    dev->pipes = NULL;
    dev->signaled_pipes = NULL;
    goldfish_device_set_irq(&dev->dev, 0, 0);
}

static void
pipeDevice_doCommand( PipeDevice* dev, uint32_t command )
{
    HwPipe** lookup = hwpipe_findp_by_channel(&dev->pipes, dev->channel);
    HwPipe*  pipe = *lookup;

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
        pipe = hwpipe_new(dev->channel, dev);
        pipe->next = dev->pipes;
        dev->pipes = pipe;
        dev->status = 0;
        break;

    case PIPE_CMD_CLOSE:
        DD("%s: CMD_CLOSE channel=0x%llx", __FUNCTION__, (unsigned long long)dev->channel);
        /* Remove from device's lists */
        *lookup = pipe->next;
        pipe->next = NULL;
        hwpipe_remove_signaled(&dev->signaled_pipes, pipe);
        hwpipe_free(pipe);
        break;

    case PIPE_CMD_POLL:
      dev->status = android_pipe_guest_poll(pipe->pipe);
      DD("%s: CMD_POLL > status=%d", __FUNCTION__, dev->status);
      break;

    case PIPE_CMD_READ_BUFFER: {
        /* Map guest buffer identified by dev->address
         * (guest PA or VA depending on the driver version)
         * into host memory.
         */
        AndroidPipeBuffer  buffer;
        buffer.data = map_guest_buffer(dev->address, dev->size, 1);
        if (!buffer.data) {
            dev->status = PIPE_ERROR_INVAL;
            break;
        }
        buffer.size = dev->size;
        dev->status = android_pipe_guest_recv(pipe->pipe, &buffer, 1);
        DD("%s: CMD_READ_BUFFER channel=0x%llx address=0x%llx size=%d > status=%d",
           __FUNCTION__, (unsigned long long)dev->channel, (unsigned long long)dev->address,
           dev->size, dev->status);
        cpu_physical_memory_unmap(buffer.data, dev->size, 1, dev->size);
        break;
    }

    case PIPE_CMD_WRITE_BUFFER: {
        /* Map guest buffer identified by dev->address
         * (guest PA or VA depending on the driver version)
         * into host memory.
         */
        AndroidPipeBuffer  buffer;
        buffer.data = map_guest_buffer(dev->address, dev->size, 0);
        if (!buffer.data) {
            dev->status = PIPE_ERROR_INVAL;
            break;
        }
        buffer.size = dev->size;
        dev->status = android_pipe_guest_send(pipe->pipe, &buffer, 1);
        DD("%s: CMD_WRITE_BUFFER channel=0x%llx address=0x%llx size=%d > status=%d",
           __FUNCTION__, (unsigned long long)dev->channel,
           (unsigned long long)dev->address,
           dev->size, dev->status);
        cpu_physical_memory_unmap(buffer.data, dev->size, 0, dev->size);
        break;
    }

    case PIPE_CMD_WAKE_ON_READ:
        DD("%s: CMD_WAKE_ON_READ channel=0x%llx", __FUNCTION__,
           (unsigned long long)dev->channel);
        if ((pipe->wakes & PIPE_WAKE_READ) == 0) {
            pipe->wakes |= PIPE_WAKE_READ;
            android_pipe_guest_wake_on(pipe->pipe, pipe->wakes);
        }
        dev->status = 0;
        break;

    case PIPE_CMD_WAKE_ON_WRITE:
        DD("%s: CMD_WAKE_ON_WRITE channel=0x%llx", __FUNCTION__,
           (unsigned long long)dev->channel);
        if ((pipe->wakes & PIPE_WAKE_WRITE) == 0) {
            pipe->wakes |= PIPE_WAKE_WRITE;
            android_pipe_guest_wake_on(pipe->pipe, pipe->wakes);
        }
        dev->status = 0;
        break;

    default:
        D("%s: command=%d (0x%x)\n", __FUNCTION__, command, command);
    }
}

static void pipe_dev_write(void *opaque, hwaddr offset, uint32_t value)
{
    PipeDevice *s = (PipeDevice *)opaque;

    switch (offset) {
    case PIPE_REG_COMMAND:
        DR("%s: command=%d (0x%x)", __FUNCTION__, value, value);
        pipeDevice_doCommand(s, value);
        break;

    case PIPE_REG_SIZE:
        DR("%s: size=%d (0x%x)", __FUNCTION__, value, value);
        s->size = value;
        break;

    case PIPE_REG_ADDRESS:
        DR("%s: address=%d (0x%x)", __FUNCTION__, value, value);
        uint64_set_low(&s->address, value);
        break;

    case PIPE_REG_ADDRESS_HIGH:
        DR("%s: address_high=%d (0x%x)", __FUNCTION__, value, value);
        uint64_set_high(&s->address, value);
        break;

    case PIPE_REG_CHANNEL:
        DR("%s: channel=%d (0x%x)", __FUNCTION__, value, value);
        uint64_set_low(&s->channel, value);
        break;

    case PIPE_REG_CHANNEL_HIGH:
        DR("%s: channel_high=%d (0x%x)", __FUNCTION__, value, value);
        uint64_set_high(&s->channel, value);
        break;

    case PIPE_REG_PARAMS_ADDR_HIGH:
        s->params_addr = (s->params_addr & ~(0xFFFFFFFFULL << 32) ) |
                          ((uint64_t)value << 32);
        break;

    case PIPE_REG_PARAMS_ADDR_LOW:
        s->params_addr = (s->params_addr & ~(0xFFFFFFFFULL) ) | value;
        break;

    case PIPE_REG_ACCESS_PARAMS:
    {
        struct access_params aps;
        struct access_params_64 aps64;
        uint32_t cmd;

        /* Don't touch aps.result if anything wrong */
        if (s->params_addr == 0)
            break;

        if (goldfish_guest_is_64bit()) {
            cpu_physical_memory_read(s->params_addr, (void*)&aps64,
                                     sizeof(aps64));
        } else {
            cpu_physical_memory_read(s->params_addr, (void*)&aps,
                                     sizeof(aps));
        }
        /* sync pipe device state from batch buffer */
        if (goldfish_guest_is_64bit()) {
            s->channel = aps64.channel;
            s->size = aps64.size;
            s->address = aps64.address;
            cmd = aps64.cmd;
        } else {
            s->channel = aps.channel;
            s->size = aps.size;
            s->address = aps.address;
            cmd = aps.cmd;
        }
        if ((cmd != PIPE_CMD_READ_BUFFER) && (cmd != PIPE_CMD_WRITE_BUFFER))
            break;

        pipeDevice_doCommand(s, cmd);
        if (goldfish_guest_is_64bit()) {
            aps64.result = s->status;
            cpu_physical_memory_write(s->params_addr, (void*)&aps64,
                                      sizeof(aps64));
        } else {
            aps.result = s->status;
            cpu_physical_memory_write(s->params_addr, (void*)&aps,
                                      sizeof(aps));
        }
    }
    break;

    default:
        D("%s: offset=%lld (0x%llx) value=%lld (0x%llx)\n", __FUNCTION__,
          (long long)offset, (long long)offset, (long long)value,
          (long long)value);
        break;
    }
}

/* I/O read */
static uint32_t pipe_dev_read(void *opaque, hwaddr offset)
{
    PipeDevice *dev = (PipeDevice *)opaque;

    switch (offset) {
    case PIPE_REG_STATUS:
        DR("%s: REG_STATUS status=%d (0x%x)", __FUNCTION__, dev->status, dev->status);
        return dev->status;

    case PIPE_REG_CHANNEL:
        if (dev->signaled_pipes != NULL) {
            HwPipe* pipe = dev->signaled_pipes;
            DR("%s: channel=0x%llx wakes=%d", __FUNCTION__,
               (unsigned long long)pipe->channel, pipe->wakes);
            dev->wakes = pipe->wakes;
            pipe->wakes = 0;
            dev->signaled_pipes = pipe->next_waked;
            pipe->next_waked = NULL;
            if (dev->signaled_pipes == NULL) {
                goldfish_device_set_irq(&dev->dev, 0, 0);
                DD("%s: lowering IRQ", __FUNCTION__);
            }
            return (uint32_t)(pipe->channel & 0xFFFFFFFFUL);
        }
        DR("%s: no signaled channels", __FUNCTION__);
        return 0;

    case PIPE_REG_CHANNEL_HIGH:
        if (dev->signaled_pipes != NULL) {
            HwPipe* pipe = dev->signaled_pipes;
            DR("%s: channel_high=0x%llx wakes=%d", __FUNCTION__,
               (unsigned long long)pipe->channel, pipe->wakes);
            return (uint32_t)(pipe->channel >> 32);
        }
        DR("%s: no signaled channels", __FUNCTION__);
        return 0;

    case PIPE_REG_WAKES:
        DR("%s: wakes %d", __FUNCTION__, dev->wakes);
        return dev->wakes;

    case PIPE_REG_PARAMS_ADDR_HIGH:
        return (uint32_t)(dev->params_addr >> 32);

    case PIPE_REG_PARAMS_ADDR_LOW:
        return (uint32_t)(dev->params_addr & 0xFFFFFFFFUL);

    case PIPE_REG_VERSION:
        deviceMode = USE_PA;
        // PIPE_REG_VERSION is issued on probe, which means that
        // we should clean up all existing stale pipes.
        // This helps keep the right state on rebooting.
        close_all_pipes(dev);
        reset_pipe_device(dev);
        return PIPE_DEVICE_VERSION;

    default:
        D("%s: offset=%lld (0x%llx)\n", __FUNCTION__, (long long)offset,
          (long long)offset);
    }
    return 0;
}

static CPUReadMemoryFunc *pipe_dev_readfn[] = {
   pipe_dev_read,
   pipe_dev_read,
   pipe_dev_read
};

static CPUWriteMemoryFunc *pipe_dev_writefn[] = {
   pipe_dev_write,
   pipe_dev_write,
   pipe_dev_write
};

static void
goldfish_pipe_save( QEMUFile* file, void* opaque )
{
    PipeDevice* dev = opaque;

    Stream* stream = stream_from_qemufile(file);
    
    android_pipe_guest_pre_save(stream);

    stream_put_be64(stream, dev->address);
    stream_put_be32(stream, dev->size);
    stream_put_be32(stream, dev->status);
    stream_put_be64(stream, dev->channel);
    stream_put_be32(stream, dev->wakes);
    stream_put_be64(stream, dev->params_addr);

    /* Count the number of pipe connections */
    HwPipe* pipe;
    int count = 0;
    for (pipe = dev->pipes; pipe; pipe = pipe->next) {
        count++;
    }
    stream_put_be32(stream, count);

    /* Now save each pipe one after the other */
    for (pipe = dev->pipes; pipe; pipe = pipe->next) {
        hwpipe_save(pipe, stream);
    }

    android_pipe_guest_post_save(stream);
    stream_free(stream);
}

static void goldfish_pipe_wake(void* hwpipe, unsigned flags);  // forward
static void goldfish_pipe_close(void* hwpipe);                 // forward

static int
goldfish_pipe_load( QEMUFile* file, void* opaque, int version_id )
{
    PipeDevice* dev = opaque;

    if ((version_id != GOLDFISH_PIPE_SAVE_VERSION) &&
        (version_id != GOLDFISH_PIPE_SAVE_LEGACY_VERSION)) {
        return -EINVAL;
    }

    Stream* stream = stream_from_qemufile(file);

    android_pipe_guest_pre_load(stream);

    dev->address = stream_get_be64(stream);
    dev->size    = stream_get_be32(stream);
    dev->status  = stream_get_be32(stream);
    dev->channel = stream_get_be64(stream);

    dev->wakes = stream_get_be32(stream);
    dev->params_addr = stream_get_be64(stream);

    /* Count the number of pipe connections */
    HwPipe* pipe;
    int count = stream_get_be32(stream);

    /* Load all pipe connections */
    for (; count > 0; count--) {
        pipe = hwpipe_load(dev, stream, version_id);
        if (pipe == NULL) {
            stream_free(stream);
            return -EIO;
        }
        pipe->next = dev->pipes;
        dev->pipes = pipe;
    }

    /* Now we need to wake/close all relevant pipes */
    for (pipe = dev->pipes; pipe; pipe = pipe->next) {
        if (pipe->wakes != 0) {
          goldfish_pipe_wake(pipe, pipe->wakes);
        }
        if (pipe->closed != 0) {
          goldfish_pipe_close(pipe);
        }
    }

    android_pipe_guest_post_load(stream);
    
    stream_free(stream);
    return 0;
}

static void goldfish_pipe_reset(void* hwpipe, void* internal_pipe) {
    HwPipe*  pipe = hwpipe;
    pipe->pipe = internal_pipe;
}

static void goldfish_pipe_wake(void* hwpipe, unsigned flags) {
    HwPipe*  pipe = hwpipe;
    HwPipe** lookup;
    PipeDevice*  dev = pipe->device;

    DD("%s: channel=0x%llx flags=%d", __FUNCTION__,
       (unsigned long long)pipe->channel, flags);

    /* If not already there, add to the list of signaled pipes */
    lookup = hwpipe_findp_signaled(&dev->signaled_pipes, pipe);
    if (!*lookup) {
        pipe->next_waked = dev->signaled_pipes;
        dev->signaled_pipes = pipe;
    }
    pipe->wakes |= (unsigned)flags;

    /* Raise IRQ to indicate there are items on our list ! */
    goldfish_device_set_irq(&dev->dev, 0, 1);
    DD("%s: raising IRQ", __FUNCTION__);
}

static void goldfish_pipe_close(void* hwpipe) {
    HwPipe* pipe = hwpipe;

    D("%s: channel=0x%llx (closed=%d)", __FUNCTION__,
      (unsigned long long)pipe->channel, pipe->closed);

    pipe->closed = 1;
    goldfish_pipe_wake(hwpipe, PIPE_WAKE_CLOSED);
}

static const AndroidPipeHwFuncs goldfish_pipe_hw_funcs = {
    .resetPipe = goldfish_pipe_reset,
    .closeFromHost = goldfish_pipe_close,
    .signalWake = goldfish_pipe_wake,
};

/* initialize the trace device */
void pipe_dev_init(bool newDeviceNaming)
{
    PipeDevice *s;

    s = (PipeDevice *) g_malloc0(sizeof(*s));

    s->dev.name = newDeviceNaming ? "goldfish_pipe" : "qemu_pipe";
    s->dev.id = -1;
    s->dev.base = 0;       // will be allocated dynamically
    s->dev.size = 0x2000;
    s->dev.irq = 0;
    s->dev.irq_count = 1;

    goldfish_device_add(&s->dev, pipe_dev_readfn, pipe_dev_writefn, s);

    register_savevm(NULL,
                    "goldfish_pipe",
                    0,
                    GOLDFISH_PIPE_SAVE_VERSION,
                    goldfish_pipe_save,
                    goldfish_pipe_load,
                    s);

    android_pipe_set_hw_funcs(&goldfish_pipe_hw_funcs);
}
