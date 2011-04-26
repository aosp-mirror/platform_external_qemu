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
#include "android/utils/intmap.h"
#include "android/utils/panic.h"
#include "android/utils/reflist.h"
#include "android/utils/system.h"
#include "android/hw-qemud.h"
#include "hw/goldfish_pipe.h"
#include "hw/goldfish_device.h"

#define  DEBUG 2

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

#define E(...)  fprintf(stderr, "ERROR:" __VA_ARGS__), fprintf(stderr, "\n")

/* Set to 1 to enable the 'zero' pipe type, useful for debugging */
#define DEBUG_ZERO_PIPE  1

/* Set to 1 to enable the 'pingpong' pipe type, useful for debugging */
#define DEBUG_PINGPONG_PIPE 1

/***********************************************************************
 ***********************************************************************
 *****
 *****   P I P E   S E R V I C E   R E G I S T R A T I O N
 *****
 *****/

#define MAX_PIPE_SERVICES  8
typedef struct {
    const char*        name;
    void*              opaque;
    GoldfishPipeFuncs  funcs;
} PipeService;

typedef struct {
    int          count;
    PipeService  services[MAX_PIPE_SERVICES];
} PipeServices;

static PipeServices  _pipeServices[1];

void
goldfish_pipe_add_type(const char*               pipeName,
                       void*                     pipeOpaque,
                       const GoldfishPipeFuncs*  pipeFuncs )
{
    PipeServices* list = _pipeServices;
    int           count = list->count;

    if (count >= MAX_PIPE_SERVICES) {
        APANIC("Too many goldfish pipe services (%d)", count);
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


/***********************************************************************
 ***********************************************************************
 *****
 *****    P I P E   C O N N E C T I O N S
 *****
 *****/

typedef struct PipeDevice  PipeDevice;

typedef struct Pipe {
    struct Pipe*             next;
    struct Pipe*             next_waked;
    PipeDevice*               device;
    uint32_t                  channel;
    void*                     opaque;
    const GoldfishPipeFuncs*  funcs;
    unsigned                  wanted;
} Pipe;

/* Forward */
static void*  pipeConnector_new(Pipe*  pipe);

Pipe*
pipe_new(uint32_t channel, PipeDevice* dev)
{
    Pipe*  pipe;
    ANEW0(pipe);
    pipe->channel = channel;
    pipe->device  = dev;
    pipe->opaque  = pipeConnector_new(pipe);
    return pipe;
}

static Pipe**
pipe_list_findp_channel( Pipe** list, uint32_t channel )
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

static const GoldfishPipeFuncs  pipeConnector_funcs;  // forward

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
pipeConnector_sendBuffers( void* opaque, const GoldfishPipeBuffer* buffers, int numBuffers )
{
    PipeConnector* pcon = opaque;
    const GoldfishPipeBuffer*  buffers_limit = buffers + numBuffers;
    int ret = 0;

    DD("%s: channel=0x%x numBuffers=%d", __FUNCTION__,
       pcon->pipe->channel,
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
        /* OK, we now have a valid name */
        Pipe* pipe = pcon->pipe;
        const PipeService* svc = goldfish_pipe_find_type(pcon->buffer);
        if (svc == NULL) {
            // UNKNOWN SERVICE
            return PIPE_ERROR_INVAL;
        }

        void*  peer = svc->funcs.init(pipe, svc->opaque);
        if (peer == NULL) {
            // COULD NOT INITIALIZE, WHAT?
            return PIPE_ERROR_INVAL;
        }

        /* Do the evil switch now */
        pipe->opaque = peer;
        pipe->funcs  = &svc->funcs;
        AFREE(pcon);
    }

    return ret;
}

static int
pipeConnector_recvBuffers( void* opaque, GoldfishPipeBuffer* buffers, int numBuffers )
{
    return PIPE_ERROR_IO;
}

static unsigned
pipeConnector_poll( void* opaque )
{
    return PIPE_WAKE_WRITE;
}

static void
pipeConnector_wakeOn( void* opaque, int flags )
{
    /* nothing, really should never happen */
}

static const GoldfishPipeFuncs  pipeConnector_funcs = {
    NULL,  /* init */
    pipeConnector_close,        /* should rarely happen */
    pipeConnector_sendBuffers,  /* the interesting stuff */
    pipeConnector_recvBuffers,  /* should not happen */
    pipeConnector_poll,         /* should not happen */
    pipeConnector_wakeOn,       /* should not happen */
};

/***********************************************************************
 ***********************************************************************
 *****
 *****    Z E R O   S E R V I C E
 *****
 *****/

/* A simple pipe service that mimics /dev/zero, you can write anything to
 * it, and you can always any number of zeros from it. Useful for debugging
 * the kernel driver.
 */
#if DEBUG_ZERO_PIPE

typedef struct {
    void* hwpipe;
} ZeroPipe;

static void*
zeroPipe_init( void* hwpipe, void* svcOpaque )
{
    ZeroPipe*  zpipe;

    D("%s: hwpipe=%p", __FUNCTION__, hwpipe);
    ANEW0(zpipe);
    zpipe->hwpipe = hwpipe;
    return zpipe;
}

static void
zeroPipe_close( void* opaque )
{
    ZeroPipe*  zpipe = opaque;

    D("%s: hwpipe=%p", __FUNCTION__, zpipe->hwpipe);
    AFREE(zpipe);
}

static int
zeroPipe_sendBuffers( void* opaque, const GoldfishPipeBuffer* buffers, int numBuffers )
{
    int  ret = 0;
    while (numBuffers > 0) {
        ret += buffers[0].size;
        buffers++;
        numBuffers--;
    }
    return ret;
}

static int
zeroPipe_recvBuffers( void* opaque, GoldfishPipeBuffer* buffers, int numBuffers )
{
    int  ret = 0;
    while (numBuffers > 0) {
        ret += buffers[0].size;
        memset(buffers[0].data, 0, buffers[0].size);
        buffers++;
        numBuffers--;
    }
    return ret;
}

static unsigned
zeroPipe_poll( void* opaque )
{
    return PIPE_WAKE_READ | PIPE_WAKE_WRITE;
}

static void
zeroPipe_wakeOn( void* opaque, int flags )
{
    /* nothing to do here */
}

static const GoldfishPipeFuncs  zeroPipe_funcs = {
    zeroPipe_init,
    zeroPipe_close,
    zeroPipe_sendBuffers,
    zeroPipe_recvBuffers,
    zeroPipe_poll,
    zeroPipe_wakeOn,
};

#endif /* DEBUG_ZERO */

/***********************************************************************
 ***********************************************************************
 *****
 *****    P I N G   P O N G   S E R V I C E
 *****
 *****/

/* Similar debug service that sends back anything it receives */

#if DEBUG_PINGPONG_PIPE

#define PINGPONG_SIZE  15000

typedef struct {
    void*    hwpipe;
    uint8_t  buffer[PINGPONG_SIZE];
    int      pos;
    int      count;
    unsigned flags;
} PingPongPipe;

static void*
pingPongPipe_init( void* hwpipe, void* svcOpaque )
{
    PingPongPipe*  ppipe;

    D("%s: hwpipe=%p", __FUNCTION__, hwpipe);
    ANEW0(ppipe);
    ppipe->hwpipe = hwpipe;
    ppipe->pos = 0;
    ppipe->count = 0;
    return ppipe;
}

static void
pingPongPipe_close( void* opaque )
{
    PingPongPipe*  ppipe = opaque;

    D("%s: hwpipe=%p (pos=%d count=%d)", __FUNCTION__, ppipe->hwpipe, ppipe->pos, ppipe->count);
    AFREE(ppipe);
}

static int
pingPongPipe_sendBuffers( void* opaque, const GoldfishPipeBuffer* buffers, int numBuffers )
{
    PingPongPipe*  pipe = opaque;
    int  ret = 0;

    while (numBuffers > 0) {
        int avail = PINGPONG_SIZE - pipe->count;
        if (avail <= 0) {
            if (ret == 0)
                ret = PIPE_ERROR_AGAIN;
            break;
        }
        if (avail > buffers[0].size) {
            avail = buffers[0].size;
        }

        int wpos = pipe->pos + pipe->count;
        if (wpos >= PINGPONG_SIZE)
            wpos -= PINGPONG_SIZE;

        if (wpos + avail <= PINGPONG_SIZE) {
            memcpy(pipe->buffer + wpos, buffers[0].data, avail);
        } else {
            int  avail2 = PINGPONG_SIZE - wpos;
            memcpy(pipe->buffer + wpos, buffers[0].data, avail2);
            memcpy(pipe->buffer, buffers[0].data + avail2, avail - avail2);
        }
        pipe->count += avail;
        ret += avail;
        numBuffers--;
        buffers++;
    }

    /* Wake up any waiting readers if we wrote something */
    if (pipe->count > 0 && (pipe->flags & PIPE_WAKE_READ)) {
        goldfish_pipe_wake(pipe->hwpipe, PIPE_WAKE_READ);
    }

    return ret;
}

static int
pingPongPipe_recvBuffers( void* opaque, GoldfishPipeBuffer* buffers, int numBuffers )
{
    PingPongPipe*  pipe = opaque;
    int  ret = 0;

    while (numBuffers > 0) {
        int avail = pipe->count;
        if (avail <= 0) {
            if (ret == 0)
                ret = PIPE_ERROR_AGAIN;
            break;
        }
        if (avail > buffers[0].size) {
            avail = buffers[0].size;
        }

        int rpos = pipe->pos;

        if (rpos + avail <= PINGPONG_SIZE) {
            memcpy(buffers[0].data, pipe->buffer + rpos, avail);
        } else {
            int  avail2 = PINGPONG_SIZE - rpos;
            memcpy(buffers[0].data, pipe->buffer + rpos, avail2);
            memcpy(buffers[0].data + avail2, pipe->buffer, avail - avail2);
        }
        pipe->count -= avail;
        pipe->pos   += avail;
        if (pipe->pos >= PINGPONG_SIZE) {
            pipe->pos -= PINGPONG_SIZE;
        }
        ret += avail;
        numBuffers--;
        buffers++;
    }

    /* Wake up any waiting readers if we wrote something */
    if (pipe->count < PINGPONG_SIZE && (pipe->flags & PIPE_WAKE_WRITE)) {
        goldfish_pipe_wake(pipe->hwpipe, PIPE_WAKE_WRITE);
    }

    return ret;
}

static unsigned
pingPongPipe_poll( void* opaque )
{
    PingPongPipe*  pipe = opaque;
    unsigned       ret = 0;

    if (pipe->count < PINGPONG_SIZE)
        ret |= PIPE_WAKE_WRITE;

    if (pipe->count > 0)
        ret |= PIPE_WAKE_READ;

    return ret;
}

static void
pingPongPipe_wakeOn( void* opaque, int flags )
{
    PingPongPipe* pipe = opaque;
    pipe->flags |= (unsigned)flags;
}

static const GoldfishPipeFuncs  pingPongPipe_funcs = {
    pingPongPipe_init,
    pingPongPipe_close,
    pingPongPipe_sendBuffers,
    pingPongPipe_recvBuffers,
    pingPongPipe_poll,
    pingPongPipe_wakeOn,
};

#endif /* DEBUG_PINGPONG_PIPE */

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
    uint32_t  address;
    uint32_t  size;
    uint32_t  status;
    uint32_t  channel;
};


static void
pipeDevice_doCommand( PipeDevice* dev, uint32_t command )
{
    Pipe** lookup = pipe_list_findp_channel(&dev->pipes, dev->channel);
    Pipe*  pipe   = *lookup;
    CPUState* env = cpu_single_env;

    /* Check that we're referring a known pipe channel */
    if (command != PIPE_CMD_OPEN && pipe == NULL) {
        dev->status = PIPE_ERROR_INVAL;
        return;
    }

    switch (command) {
    case PIPE_CMD_OPEN:
        DD("%s: CMD_OPEN", __FUNCTION__);
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
        DD("%s: CMD_CLOSE", __FUNCTION__);
        /* Remove from device's lists */
        *lookup = pipe->next;
        pipe->next = NULL;
        pipe_list_remove_waked(&dev->signaled_pipes, pipe);
        /* Call close callback */
        if (pipe->funcs->close) {
            pipe->funcs->close(pipe->opaque);
        }
        /* Free stuff */
        AFREE(pipe);
        break;

    case PIPE_CMD_POLL:
        DD("%s: CMD_POLL", __FUNCTION__);
        break;

    case PIPE_CMD_READ_BUFFER: {
        DD("%s: CMD_READ_BUFFER", __FUNCTION__);
        /* Translate virtual address into physical one, into emulator memory. */
        GoldfishPipeBuffer  buffer;
        uint32_t            address = dev->address;
        uint32_t            page    = address & TARGET_PAGE_MASK;
        target_phys_addr_t  phys = cpu_get_phys_page_debug(env, page);
        buffer.data = qemu_get_ram_ptr(phys) + (address - page);
        buffer.size = dev->size;
        dev->status = pipe->funcs->recvBuffers(pipe->opaque, &buffer, 1);
        break;
    }

    case PIPE_CMD_WRITE_BUFFER: {
        DD("%s: CMD_WRITE_BUFFER", __FUNCTION__);
        /* Translate virtual address into physical one, into emulator memory. */
        GoldfishPipeBuffer  buffer;
        uint32_t            address = dev->address;
        uint32_t            page    = address & TARGET_PAGE_MASK;
        target_phys_addr_t  phys = cpu_get_phys_page_debug(env, page);
        buffer.data = qemu_get_ram_ptr(phys) + (address - page);
        buffer.size = dev->size;
        dev->status = pipe->funcs->sendBuffers(pipe->opaque, &buffer, 1);
        break;
    }

    case PIPE_CMD_WAKE_ON_READ:
        DD("%s: CMD_WAKE_ON_READ", __FUNCTION__);
        if ((pipe->wanted & PIPE_WAKE_READ) == 0) {
            pipe->wanted |= PIPE_WAKE_READ;
            pipe->funcs->wakeOn(pipe->opaque, pipe->wanted);
        }
        dev->status = 0;
        break;

    case PIPE_CMD_WAKE_ON_WRITE:
        DD("%s: CMD_WAKE_ON_WRITE", __FUNCTION__);
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

static void pipe_dev_write(void *opaque, target_phys_addr_t offset, uint32_t value)
{
    PipeDevice *s = (PipeDevice *)opaque;

    switch (offset) {
    case PIPE_REG_COMMAND:
        DD("%s: command=%d (0x%x)", __FUNCTION__, value, value);
        pipeDevice_doCommand(s, value);
        break;

    case PIPE_REG_SIZE:
        DD("%s: size=%d (0x%x)", __FUNCTION__, value, value);
        s->size = value;
        break;

    case PIPE_REG_ADDRESS:
        DD("%s: address=%d (0x%x)", __FUNCTION__, value, value);
        s->address = value;
        break;

    case PIPE_REG_CHANNEL:
        DD("%s: channel=%d (0x%x)", __FUNCTION__, value, value);
        s->channel = value;
        break;

    default:
        D("%s: offset=%d (0x%x) value=%d (0x%x)\n", __FUNCTION__, offset,
            offset, value, value);
        break;
    }
}

/* I/O read */
static uint32_t pipe_dev_read(void *opaque, target_phys_addr_t offset)
{
    PipeDevice *dev = (PipeDevice *)opaque;

    switch (offset) {
    case PIPE_REG_STATUS:
        DD("%s: status=%d (0x%x)", __FUNCTION__, dev->status, dev->status);
        return dev->status;

    case PIPE_REG_CHANNEL:
        if (dev->signaled_pipes != NULL) {
            Pipe* pipe = dev->signaled_pipes;
            DD("%s: channel=0x%x wanted=%d", __FUNCTION__,
               pipe->channel, pipe->wanted);
            dev->status = pipe->wanted;
            dev->signaled_pipes = pipe->next_waked;
            pipe->next_waked = NULL;
            if (dev->signaled_pipes == NULL) {
                goldfish_device_set_irq(&dev->dev, 0, 1);
                DD("%s: lowering IRQ", __FUNCTION__);
            }
            return pipe->channel;
        }
        DD("%s: no signaled channels?", __FUNCTION__);
        return 0;

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

/* initialize the trace device */
void pipe_dev_init()
{
    PipeDevice *s;

    s = (PipeDevice *) qemu_mallocz(sizeof(*s));

    s->dev.name = "qemu_pipe";
    s->dev.id = -1;
    s->dev.base = 0;       // will be allocated dynamically
    s->dev.size = 0x2000;
    s->dev.irq = 0;
    s->dev.irq_count = 1;

    goldfish_device_add(&s->dev, pipe_dev_readfn, pipe_dev_writefn, s);

#if DEBUG_ZERO_PIPE
    goldfish_pipe_add_type("zero", NULL, &zeroPipe_funcs);
#endif
#if DEBUG_PINGPONG_PIPE
    goldfish_pipe_add_type("pingpong", NULL, &pingPongPipe_funcs);
#endif
}

void
goldfish_pipe_wake( void* hwpipe, unsigned flags )
{
    Pipe*  pipe = hwpipe;
    Pipe** lookup;
    PipeDevice*  dev = pipe->device;

    DD("%s: channel=0x%x flags=%d", __FUNCTION__, pipe->channel, flags);

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

void
goldfish_pipe_close( void* hwpipe )
{
    /* XXX: TODO */
}
