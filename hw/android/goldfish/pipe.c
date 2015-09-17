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
#include "android/utils/panic.h"
#include "android/utils/system.h"
#include "hw/android/goldfish/pipe.h"
#include "hw/android/goldfish/device.h"
#include "hw/android/goldfish/vmem.h"
#include "exec/ram_addr.h"

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

/* Maximum length of pipe service name, in characters (excluding final 0) */
#define MAX_PIPE_SERVICE_NAME_SIZE  255

#define GOLDFISH_PIPE_SAVE_VERSION  3

/***********************************************************************
 ***********************************************************************
 *****
 *****    P I P E   A P I
 *****
 *****/

#define MAX_PIPE_SERVICES  8
typedef struct {
    const char*        name;
    void*              opaque;
    AndroidPipeFuncs  funcs;
} PipeService;

typedef struct {
    int          count;
    PipeService  services[MAX_PIPE_SERVICES];
} PipeServices;

static PipeServices  _pipeServices[1];

void
android_pipe_add_type(const char*               pipeName,
                       void*                     pipeOpaque,
                       const AndroidPipeFuncs*  pipeFuncs )
{
    PipeServices* list = _pipeServices;
    int           count = list->count;

    if (count >= MAX_PIPE_SERVICES) {
        APANIC("Too many goldfish pipe services (%d)", count);
    }

    if (strlen(pipeName) > MAX_PIPE_SERVICE_NAME_SIZE) {
        APANIC("Pipe service name too long: '%s'", pipeName);
    }

    list->services[count].name   = pipeName;
    list->services[count].opaque = pipeOpaque;
    list->services[count].funcs  = pipeFuncs[0];

    list->count++;
}

static const PipeService*
goldfish_pipe_find_type(const char*  pipeName)
{
    PipeServices* list = _pipeServices;
    int           count = list->count;
    int           nn;

    for (nn = 0; nn < count; nn++) {
        if (!strcmp(list->services[nn].name, pipeName)) {
            return &list->services[nn];
        }
    }
    return NULL;
}

static const AndroidPipeHwFuncs* sPipeHwFuncs = NULL;

void android_pipe_set_hw_funcs(const AndroidPipeHwFuncs* hw_funcs) {
    sPipeHwFuncs = hw_funcs;
}

void android_pipe_close(void* hwpipe) {
    sPipeHwFuncs->closeFromHost(hwpipe);
}

void android_pipe_wake(void* hwpipe, unsigned flags) {
    sPipeHwFuncs->signalWake(hwpipe, flags);
}

/***********************************************************************
 ***********************************************************************
 *****
 *****    P I P E   C O N N E C T I O N S
 *****
 *****/

typedef struct PipeDevice  PipeDevice;

typedef struct Pipe {
    struct Pipe*              next;
    struct Pipe*              next_waked;
    PipeDevice*                device;
    uint64_t                   channel;
    void*                      opaque;
    const AndroidPipeFuncs*   funcs;
    const PipeService*         service;
    char*                      args;
    unsigned char              wanted;
    char                       closed;
} Pipe;

/* Forward */
static void*  pipeConnector_new(Pipe*  pipe);

static Pipe*
pipe_new0(PipeDevice* dev)
{
    Pipe*  pipe;
    ANEW0(pipe);
    pipe->device = dev;
    return pipe;
}

static Pipe*
pipe_new(uint64_t channel, PipeDevice* dev)
{
    Pipe*  pipe = pipe_new0(dev);
    pipe->channel = channel;
    pipe->opaque  = pipeConnector_new(pipe);
    return pipe;
}

static Pipe**
pipe_list_findp_channel( Pipe** list, uint64_t channel )
{
    Pipe** pnode = list;
    for (;;) {
        Pipe* node = *pnode;
        if (node == NULL || node->channel == channel) {
            break;
        }
        pnode = &node->next;
    }
    return pnode;
}

#if 0
static Pipe**
pipe_list_findp_opaque( Pipe** list, void* opaque )
{
    Pipe** pnode = list;
    for (;;) {
        Pipe* node = *pnode;
        if (node == NULL || node->opaque == opaque) {
            break;
        }
        pnode = &node->next;
    }
    return pnode;
}
#endif

static Pipe**
pipe_list_findp_waked( Pipe** list, Pipe* pipe )
{
    Pipe** pnode = list;
    for (;;) {
        Pipe* node = *pnode;
        if (node == NULL || node == pipe) {
            break;
        }
        pnode = &node->next_waked;
    }
    return pnode;
}


static void
pipe_list_remove_waked( Pipe** list, Pipe*  pipe )
{
    Pipe** lookup = pipe_list_findp_waked(list, pipe);
    Pipe*  node   = *lookup;

    if (node != NULL) {
        (*lookup) = node->next_waked;
        node->next_waked = NULL;
    }
}

static void
pipe_save( Pipe* pipe, QEMUFile* file )
{
    if (pipe->service == NULL) {
        /* pipe->service == NULL means we're still using a PipeConnector */
        /* Write a zero to indicate this condition */
        qemu_put_byte(file, 0);
    } else {
        /* Otherwise, write a '1' then the service name */
        qemu_put_byte(file, 1);
        qemu_put_string(file, pipe->service->name);
    }

    /* Now save other common data */
    qemu_put_be64(file, pipe->channel);
    qemu_put_byte(file, (int)pipe->wanted);
    qemu_put_byte(file, (int)pipe->closed);

    /* Write 1 + args, if any, or simply 0 otherwise */
    if (pipe->args != NULL) {
        qemu_put_byte(file, 1);
        qemu_put_string(file, pipe->args);
    } else {
        qemu_put_byte(file, 0);
    }

    if (pipe->funcs->save) {
        pipe->funcs->save(pipe->opaque, file);
    }
}

static Pipe*
pipe_load(PipeDevice* dev, QEMUFile* file)
{
    Pipe*              pipe;
    const PipeService* service = NULL;
    int   state = qemu_get_byte(file);
    uint64_t channel;

    if (state != 0) {
        /* Pipe is associated with a service. */
        char* name = qemu_get_string(file);
        if (name == NULL)
            return NULL;

        service = goldfish_pipe_find_type(name);
        if (service == NULL) {
            D("No QEMU pipe service named '%s'", name);
            AFREE(name);
            return NULL;
        }
    }

    channel = qemu_get_be64(file);

    pipe = pipe_new(channel, dev);
    pipe->wanted  = qemu_get_byte(file);
    pipe->closed  = qemu_get_byte(file);
    if (qemu_get_byte(file) != 0) {
        pipe->args = qemu_get_string(file);
    }

    pipe->service = service;
    if (service != NULL) {
        pipe->funcs = &service->funcs;
    }

    if (pipe->funcs->load) {
        pipe->opaque = pipe->funcs->load(pipe, service ? service->opaque : NULL, pipe->args, file);
        if (pipe->opaque == NULL) {
            AFREE(pipe);
            return NULL;
        }
    } else {
        /* Force-close the pipe on load */
        pipe->closed = 1;
    }
    return pipe;
}

static void
pipe_free( Pipe* pipe )
{
    /* Call close callback */
    if (pipe->funcs->close) {
        pipe->funcs->close(pipe->opaque);
    }
    /* Free stuff */
    AFREE(pipe->args);
    AFREE(pipe);
}

/***********************************************************************
 ***********************************************************************
 *****
 *****    P I P E   C O N N E C T O R S
 *****
 *****/

/* These are used to handle the initial connection attempt, where the
 * client is going to write the name of the pipe service it wants to
 * connect to, followed by a terminating zero.
 */
typedef struct {
    Pipe*  pipe;
    char   buffer[128];
    int    buffpos;
} PipeConnector;

static const AndroidPipeFuncs  pipeConnector_funcs;  // forward

void*
pipeConnector_new(Pipe*  pipe)
{
    PipeConnector*  pcon;

    ANEW0(pcon);
    pcon->pipe  = pipe;
    pipe->funcs = &pipeConnector_funcs;
    return pcon;
}

static void
pipeConnector_close( void* opaque )
{
    PipeConnector*  pcon = opaque;
    AFREE(pcon);
}

static int
pipeConnector_sendBuffers( void* opaque, const AndroidPipeBuffer* buffers, int numBuffers )
{
    PipeConnector* pcon = opaque;
    const AndroidPipeBuffer*  buffers_limit = buffers + numBuffers;
    int ret = 0;

    DD("%s: channel=0x%llx numBuffers=%d", __FUNCTION__,
       (unsigned long long)pcon->pipe->channel,
       numBuffers);

    while (buffers < buffers_limit) {
        int  avail;

        DD("%s: buffer data (%3d bytes): '%.*s'", __FUNCTION__,
           buffers[0].size, buffers[0].size, buffers[0].data);

        if (buffers[0].size == 0) {
            buffers++;
            continue;
        }

        avail = sizeof(pcon->buffer) - pcon->buffpos;
        if (avail > buffers[0].size)
            avail = buffers[0].size;

        if (avail > 0) {
            memcpy(pcon->buffer + pcon->buffpos, buffers[0].data, avail);
            pcon->buffpos += avail;
            ret += avail;
        }
        buffers++;
    }

    /* Now check that our buffer contains a zero-terminated string */
    if (memchr(pcon->buffer, '\0', pcon->buffpos) != NULL) {
        /* Acceptable formats for the connection string are:
         *
         *   pipe:<name>
         *   pipe:<name>:<arguments>
         */
        char* pipeName;
        char* pipeArgs;

        D("%s: connector: '%s'", __FUNCTION__, pcon->buffer);

        if (memcmp(pcon->buffer, "pipe:", 5) != 0) {
            /* Nope, we don't handle these for now. */
            D("%s: Unknown pipe connection: '%s'", __FUNCTION__, pcon->buffer);
            return PIPE_ERROR_INVAL;
        }

        pipeName = pcon->buffer + 5;
        pipeArgs = strchr(pipeName, ':');

        if (pipeArgs != NULL) {
            *pipeArgs++ = '\0';
            if (!*pipeArgs)
                pipeArgs = NULL;
        }

        Pipe* pipe = pcon->pipe;
        const PipeService* svc = goldfish_pipe_find_type(pipeName);
        if (svc == NULL) {
            D("%s: Unknown server!", __FUNCTION__);
            return PIPE_ERROR_INVAL;
        }

        void*  peer = svc->funcs.init(pipe, svc->opaque, pipeArgs);
        if (peer == NULL) {
            D("%s: Initialization failed!", __FUNCTION__);
            return PIPE_ERROR_INVAL;
        }

        /* Do the evil switch now */
        pipe->opaque = peer;
        pipe->service = svc;
        pipe->funcs  = &svc->funcs;
        pipe->args   = ASTRDUP(pipeArgs);
        AFREE(pcon);
    }

    return ret;
}

static int
pipeConnector_recvBuffers( void* opaque, AndroidPipeBuffer* buffers, int numBuffers )
{
    return PIPE_ERROR_IO;
}

static unsigned
pipeConnector_poll( void* opaque )
{
    return PIPE_POLL_OUT;
}

static void
pipeConnector_wakeOn( void* opaque, int flags )
{
    /* nothing, really should never happen */
}

static void
pipeConnector_save( void* pipe, QEMUFile* file )
{
    PipeConnector*  pcon = pipe;
    qemu_put_sbe32(file, pcon->buffpos);
    qemu_put_sbuffer(file, (const int8_t*)pcon->buffer, pcon->buffpos);
}

static void*
pipeConnector_load( void* hwpipe, void* pipeOpaque, const char* args, QEMUFile* file )
{
    PipeConnector*  pcon;

    int len = qemu_get_sbe32(file);
    if (len < 0 || len > sizeof(pcon->buffer)) {
        return NULL;
    }
    pcon = pipeConnector_new(hwpipe);
    pcon->buffpos = len;
    if (qemu_get_buffer(file, (uint8_t*)pcon->buffer, pcon->buffpos) != pcon->buffpos) {
        AFREE(pcon);
        return NULL;
    }
    return pcon;
}

static const AndroidPipeFuncs  pipeConnector_funcs = {
    NULL,  /* init */
    pipeConnector_close,        /* should rarely happen */
    pipeConnector_sendBuffers,  /* the interesting stuff */
    pipeConnector_recvBuffers,  /* should not happen */
    pipeConnector_poll,         /* should not happen */
    pipeConnector_wakeOn,       /* should not happen */
    pipeConnector_save,
    pipeConnector_load,
};

/***********************************************************************
 ***********************************************************************
 *****
 *****    G O L D F I S H   P I P E   D E V I C E
 *****
 *****/

struct PipeDevice {
    struct goldfish_device dev;

    /* the list of all pipes */
    Pipe*  pipes;

    /* the list of signalled pipes */
    Pipe*  signaled_pipes;

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

static void
pipeDevice_doCommand( PipeDevice* dev, uint32_t command )
{
    Pipe** lookup = pipe_list_findp_channel(&dev->pipes, dev->channel);
    Pipe*  pipe   = *lookup;

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
        break;

    case PIPE_CMD_CLOSE:
        DD("%s: CMD_CLOSE channel=0x%llx", __FUNCTION__, (unsigned long long)dev->channel);
        /* Remove from device's lists */
        *lookup = pipe->next;
        pipe->next = NULL;
        pipe_list_remove_waked(&dev->signaled_pipes, pipe);
        pipe_free(pipe);
        break;

    case PIPE_CMD_POLL:
        dev->status = pipe->funcs->poll(pipe->opaque);
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
        dev->status = pipe->funcs->recvBuffers(pipe->opaque, &buffer, 1);
        DD("%s: CMD_READ_BUFFER channel=0x%llx address=0x%16llx size=%d > status=%d",
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
        dev->status = pipe->funcs->sendBuffers(pipe->opaque, &buffer, 1);
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
            pipe->funcs->wakeOn(pipe->opaque, pipe->wanted);
        }
        dev->status = 0;
        break;

    case PIPE_CMD_WAKE_ON_WRITE:
        DD("%s: CMD_WAKE_ON_WRITE channel=0x%llx", __FUNCTION__, (unsigned long long)dev->channel);
        if ((pipe->wanted & PIPE_WAKE_WRITE) == 0) {
            pipe->wanted |= PIPE_WAKE_WRITE;
            pipe->funcs->wakeOn(pipe->opaque, pipe->wanted);
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
        D("%s: offset=%d (0x%x) value=%d (0x%x)\n", __FUNCTION__, offset,
            offset, value, value);
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
            Pipe* pipe = dev->signaled_pipes;
            DR("%s: channel=0x%llx wanted=%d", __FUNCTION__,
               (unsigned long long)pipe->channel, pipe->wanted);
            dev->wakes = pipe->wanted;
            pipe->wanted = 0;
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
            Pipe* pipe = dev->signaled_pipes;
            DR("%s: channel_high=0x%llx wanted=%d", __FUNCTION__,
               (unsigned long long)pipe->channel, pipe->wanted);
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
        return PIPE_DEVICE_VERSION;

    default:
        D("%s: offset=%d (0x%x)\n", __FUNCTION__, offset, offset);
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
    Pipe* pipe;

    qemu_put_be64(file, dev->address);
    qemu_put_be32(file, dev->size);
    qemu_put_be32(file, dev->status);
    qemu_put_be64(file, dev->channel);
    qemu_put_be32(file, dev->wakes);
    qemu_put_be64(file, dev->params_addr);

    /* Count the number of pipe connections */
    int count = 0;
    for ( pipe = dev->pipes; pipe; pipe = pipe->next )
        count++;

    qemu_put_sbe32(file, count);

    /* Now save each pipe one after the other */
    for ( pipe = dev->pipes; pipe; pipe = pipe->next ) {
        pipe_save(pipe, file);
    }
}

static int
goldfish_pipe_load( QEMUFile* file, void* opaque, int version_id )
{
    PipeDevice* dev = opaque;
    Pipe*       pipe;

    if (version_id != GOLDFISH_PIPE_SAVE_VERSION) {
        return -EINVAL;
    }
    dev->address = qemu_get_be64(file);
    dev->size    = qemu_get_be32(file);
    dev->status  = qemu_get_be32(file);
    dev->channel = qemu_get_be64(file);

    dev->wakes   = qemu_get_be32(file);
    dev->params_addr   = qemu_get_be64(file);

    /* Count the number of pipe connections */
    int count = qemu_get_sbe32(file);

    /* Load all pipe connections */
    for ( ; count > 0; count-- ) {
        pipe = pipe_load(dev, file);
        if (pipe == NULL) {
            return -EIO;
        }
        pipe->next = dev->pipes;
        dev->pipes = pipe;
    }

    /* Now we need to wake/close all relevant pipes */
    for ( pipe = dev->pipes; pipe; pipe = pipe->next ) {
        if (pipe->wanted != 0)
            android_pipe_wake(pipe, pipe->wanted);
        if (pipe->closed != 0)
            android_pipe_close(pipe);
    }
    return 0;
}

static void goldfish_pipe_wake(void* hwpipe, unsigned flags) {
    Pipe*  pipe = hwpipe;
    Pipe** lookup;
    PipeDevice*  dev = pipe->device;

    DD("%s: channel=0x%llx flags=%d", __FUNCTION__, (unsigned long long)pipe->channel, flags);

    /* If not already there, add to the list of signaled pipes */
    lookup = pipe_list_findp_waked(&dev->signaled_pipes, pipe);
    if (!*lookup) {
        pipe->next_waked = dev->signaled_pipes;
        dev->signaled_pipes = pipe;
    }
    pipe->wanted |= (unsigned)flags;

    /* Raise IRQ to indicate there are items on our list ! */
    goldfish_device_set_irq(&dev->dev, 0, 1);
    DD("%s: raising IRQ", __FUNCTION__);
}

static void goldfish_pipe_close(void* hwpipe) {
    Pipe* pipe = hwpipe;

    D("%s: channel=0x%llx (closed=%d)", __FUNCTION__, (unsigned long long)pipe->channel, pipe->closed);

    if (!pipe->closed) {
        pipe->closed = 1;
        goldfish_pipe_wake(hwpipe, PIPE_WAKE_CLOSED);
    }
}

static const AndroidPipeHwFuncs goldfish_pipe_hw_funcs = {
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

#if DEBUG_ZERO_PIPE
    android_pipe_add_type_zero();
#endif
#if DEBUG_PINGPONG_PIPE
    android_pipe_add_type_pingpong();
#endif
#if DEBUG_THROTTLE_PIPE
    android_pipe_add_type_pingpong();
#endif
}
