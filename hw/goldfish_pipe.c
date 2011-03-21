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

#define  DEBUG 0

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

/* Must match hw/goldfish_trace.h */
#define  SLOT_COMMAND   0
#define  SLOT_STATUS    0
#define  SLOT_ADDRESS   1
#define  SLOT_SIZE      2
#define  SLOT_CHANNEL   3

#define   QEMUD_PIPE_CMD_CLOSE              1
#define   QEMUD_PIPE_CMD_SEND               2
#define   QEMUD_PIPE_CMD_RECV               3
#define   QEMUD_PIPE_CMD_WAKE_ON_SEND       4
#define   QEMUD_PIPE_CMD_WAKE_ON_RECV       5

#define   QEMUD_PIPE_ERROR_INVAL            22   /* EINVAL */
#define   QEMUD_PIPE_ERROR_AGAIN            11   /* EAGAIN */
#define   QEMUD_PIPE_ERROR_CONNRESET       104   /* ECONNRESET */
#define   QEMUD_PIPE_ERROR_NOMEM            12   /* ENOMEM */

/**********************************************************************
 **********************************************************************
 **
 **  TECHNICAL NOTE ON THE FOLLOWING IMPLEMENTATION:
 **
 **    PipeService ::
 **       The global state for the QEMUD fast pipes service.
 **       Holds a tid -> ThreadState map. Registers as a Qemud
 **       service named "fast-pipes" which creates PipeClient
 **       objects on connection.
 **
 **    PipeClient ::
 **       The state of each QEMUD pipe. This handles the initial
 **       connection, message exchanges and signalling.
 **
 **    ThreadState ::
 **       Hold the thread-specific state corresponding to each guest
 **       thread that has created a least one qemud pipe. Stores the
 **       state of our 4 I/O Registers, and a map of localId -> PipeState
 **
 **
 **  The following graphics is an example corresponding to the following
 **  situation:
 **
 **     - two guest threads have opened pipe connections
 **     - the first thread has opened two different pipes
 **     - the second thread has opened only one pipe
 **
 **
 **     QEMUD-SERVICE
 **       |     |
 **       |     |________________________________________
 **       |          |                |                 |
 **       |          v                v                 v
 **       |      QEMUD-CLIENT#1    QEMUD-CLIENT#2    QEMUD-CLIENT#3
 **       |         ^                ^                  ^
 **       |         |                |                  |
 **       |         |                |                  |
 **       |         v                v                  v
 **       |      PIPE-CLIENT#1     PIPE-CLIENT#2     PIPE-CLIENT#3
 **       |         ^                ^                  ^
 **       |         |                |                  |
 **       |         |________________|                  |
 **       |         |                                   +
 **       |         |                                   |
 **       |      THREAD-STATE#1                   THREAD-STATE#2
 **       |         ^                                   ^
 **       |     ____|___________________________________|
 **       |    |
 **       |    |
 **     PIPE-SERVICE
 **
 **  Note that the QemudService and QemudClient objects are created by
 **  hw-qemud.c and not defined here.
 **
 **/

typedef struct PipeService  PipeService;
typedef struct PipeClient   PipeClient;

static void pipeService_removeState(PipeService* pipeSvc, int tid);

/**********************************************************************
 **********************************************************************
 *****
 *****  REGISTRY OF SUPPORTED PIPE SERVICES
 *****
 *****/

typedef struct {
    const char*                   pipeName;
    void*                         pipeOpaque;
    const QemudPipeHandlerFuncs*  pipeFuncs;
} PipeType;

#define MAX_PIPE_TYPES  4

static PipeType  sPipeTypes[MAX_PIPE_TYPES];
static int       sPipeTypeCount;

void
goldfish_pipe_add_type( const char*                   pipeName,
                        void*                         pipeOpaque,
                        const QemudPipeHandlerFuncs*  pipeFuncs )
{
    int count = sPipeTypeCount;

    if (count >= MAX_PIPE_TYPES) {
        APANIC("%s: Too many qemud pipe types!", __FUNCTION__);
    }

    sPipeTypes[count].pipeName   = pipeName;
    sPipeTypes[count].pipeOpaque = pipeOpaque;
    sPipeTypes[count].pipeFuncs  = pipeFuncs;
    sPipeTypeCount = ++count;
}

static const PipeType*
goldfish_pipe_find_type( const char*  pipeName )
{
    const PipeType* ptype = sPipeTypes;
    const PipeType* limit = ptype + sPipeTypeCount;

    for ( ; ptype < limit; ptype++ ) {
        if (!strcmp(pipeName, ptype->pipeName)) {
            return ptype;
        }
    }
    return NULL;
}


/**********************************************************************
 **********************************************************************
 *****
 *****  THREAD-SPECIFIC STATE
 *****
 *****/

static void
pipeClient_closeFromThread( PipeClient*  pcl );

static uint32_t
pipeClient_doCommand( PipeClient* pcl,
                      uint32_t   command,
                      uint32_t   address,
                      uint32_t*  pSize );


/* For each guest thread, we will store the following state:
 *
 * - The current state of the 'address', 'size', 'localId' and 'status'
 *   I/O slots provided through the magic page by hw/goldfish_trace.c
 *
 * - A list of PipeClient objects, corresponding to all the pipes in
 *   this thread, identified by localId.
 */
typedef struct {
    uint32_t   address;
    uint32_t   size;
    uint32_t   localId;
    uint32_t   status;
    AIntMap*   pipes;
} ThreadState;

static void
threadState_free( ThreadState*  ts )
{
    /* Get rid of the localId -> PipeClient map */
    AINTMAP_FOREACH_VALUE(ts->pipes, pcl, pipeClient_closeFromThread(pcl));
    aintMap_free(ts->pipes);

    AFREE(ts);
}

static ThreadState*
threadState_new( void )
{
    ThreadState*  ts;
    ANEW0(ts);
    ts->pipes = aintMap_new();
    return ts;
}

static int
threadState_addPipe( ThreadState* ts, int  localId, PipeClient* pcl )
{
    /* We shouldn't already have a pipe for this localId */
    if (aintMap_get(ts->pipes, localId) != NULL) {
        errno = EBADF;
        return -1;
    }
    aintMap_set(ts->pipes, localId, pcl);
    return 0;
}

static void
threadState_delPipe( ThreadState* ts, int localId )
{
    aintMap_del(ts->pipes, localId);
}

static void
threadState_write( ThreadState*  ts, int  offset, uint32_t value )
{
    PipeClient*  pcl;

    switch (offset) {
        case SLOT_COMMAND:
            pcl = aintMap_get(ts->pipes, (int)ts->localId);
            if (pcl == NULL) {
                D("%s: Invalid localId (%d)",
                  __FUNCTION__, ts->localId);
                ts->status = QEMUD_PIPE_ERROR_INVAL;
            } else {
                ts->status = pipeClient_doCommand(pcl,
                                                  value,
                                                  ts->address,
                                                  &ts->size);
            }
            break;
        case SLOT_ADDRESS:
            ts->address = value;
            break;
        case SLOT_SIZE:
            ts->size = value;
            break;
        case SLOT_CHANNEL:
            ts->localId = value;
            break;
        default:
            /* XXX: PRINT ERROR? */
            ;
    }
}

static uint32_t
threadState_read( ThreadState*  ts, int offset )
{
    switch (offset) {
        case SLOT_STATUS:       return ts->status;
        case SLOT_ADDRESS:      return ts->address;
        case SLOT_SIZE:         return ts->size;
        case SLOT_CHANNEL:      return ts->localId;
        default:                return 0;
    }
}

/**********************************************************************
 **********************************************************************
 *****
 *****  PIPE CLIENT STATE
 *****
 *****/

/* Each client object points to a PipeState after it has received the
 * initial request from the guest, which shall look like:
 * <tid>:<localId>:<name>
 */
struct PipeClient {
    char*         pipeName;
    QemudClient*  client;
    PipeService*  pipeSvc;

    int                           tid;
    uint32_t                      localId;
    void*                         handler;
    const QemudPipeHandlerFuncs*  handlerFuncs;
};

static int  pipeService_addPipe(PipeService* pipeSvc, int tid, int localId, PipeClient* pcl);
static void pipeService_removePipe(PipeService* pipeSvc, int tid, int localId);

static void
pipeClient_closeFromThread( PipeClient*  pcl )
{
    qemud_client_close(pcl->client);
}

/* This function should only be invoked through qemud_client_close().
 * Never call it explicitely. */
static void
pipeClient_close( void* opaque )
{
    PipeClient*  pcl = opaque;

    if (pcl->handler && pcl->handlerFuncs && pcl->handlerFuncs->close) {
        pcl->handlerFuncs->close(pcl->handler);
    }

    D("qemud:pipe: closing client (%d,%x)", pcl->tid, pcl->localId);
    qemud_client_close(pcl->client);
    pcl->client = NULL;

    /* The guest is closing the connection, so remove it from our state */
    pipeService_removePipe(pcl->pipeSvc, pcl->tid, pcl->localId);

    AFREE(pcl->pipeName);
}

static void
pipeClient_recv( void* opaque, uint8_t* msg, int msgLen, QemudClient* client )
{
    PipeClient*      pcl = opaque;
    const PipeType*  ptype;

    const char* p;
    int         failure = 1;
    char        answer[64];

    if (pcl->pipeName != NULL) {
        /* Should never happen, the only time we'll receive something
         * is when the connection is created! Simply ignore any incoming
         * message.
         */
        return;
    }

    /* The message format is <tid>:<localIdHex>:<name> */
    if (sscanf((char*)msg, "%d:%x:", &pcl->tid, &pcl->localId) != 2) {
        goto BAD_FORMAT;
    }
    p = strchr((const char*)msg, ':');
    if (p != NULL) {
        p = strchr(p+1, ':');
        if (p != NULL)
            p += 1;
    }
    if (p == NULL || *p == '\0') {
        goto BAD_FORMAT;
    }

    pcl->pipeName = ASTRDUP(p);

    ptype = goldfish_pipe_find_type(pcl->pipeName);
    if (ptype == NULL) {
        goto UNKNOWN_PIPE_TYPE;
    }

    pcl->handlerFuncs = ptype->pipeFuncs;
    pcl->handler      = ptype->pipeFuncs->init( client, ptype->pipeOpaque );
    if (pcl->handler == NULL) {
        goto BAD_INIT;
    }

    if (pipeService_addPipe(pcl->pipeSvc, pcl->tid, pcl->localId, pcl) < 0) {
        goto DUPLICATE_REQUEST;
    }

    D("qemud:pipe: Added new client: %s", msg);
    failure = 0;
    snprintf(answer, sizeof answer, "OK");
    goto SEND_ANSWER;

BAD_INIT:
    /* Initialization failed for some reason! */
    E("qemud:pipe: Could not initialize pipe: '%s'", pcl->pipeName);
    snprintf(answer, sizeof answer, "KO:%d:Could not initialize pipe",
                QEMUD_PIPE_ERROR_INVAL);
    goto SEND_ANSWER;

UNKNOWN_PIPE_TYPE:
    E("qemud:pipe: Unknown pipe type: '%s'", p);
    snprintf(answer, sizeof answer, "KO:%d:Unknown pipe type name",
             QEMUD_PIPE_ERROR_INVAL);
    goto SEND_ANSWER;

BAD_FORMAT:
    E("qemud:pipe: Invalid connection request: '%s'", msg);
    snprintf(answer, sizeof answer, "KO:%d:Invalid connection request",
             QEMUD_PIPE_ERROR_INVAL);
    goto SEND_ANSWER;

DUPLICATE_REQUEST:
    E("qemud:pipe: Duplicate connection request: '%s'", msg);
    snprintf(answer, sizeof answer, "KO:%d:Duplicate connection request",
             QEMUD_PIPE_ERROR_INVAL);
    goto SEND_ANSWER;

SEND_ANSWER:
    qemud_client_send(client, (uint8_t*)answer, strlen(answer));
    if (failure) {
        qemud_client_close(client);
    } else {
        /* Disable framing for the rest of signalling */
        qemud_client_set_framing(client, 0);
    }
}

/* Count the number of GoldfishPipeBuffers we will need to transfer
 * memory from/to [address...address+size)
 */
static int
_countPipeBuffers( uint32_t  address, uint32_t  size )
{
    CPUState*  env   = cpu_single_env;
    int        count = 0;

    while (size > 0) {
        uint32_t   vstart = address & TARGET_PAGE_MASK;
        uint32_t   vend   = vstart + TARGET_PAGE_SIZE;
        uint32_t   next   = address + size;

        DD("%s: trying to map (0x%x - 0x%0x, %d bytes) -> page=0x%x - 0x%x",
          __FUNCTION__, address, address+size, size, vstart, vend);

        if (next > vend) {
            next = vend;
        }

        /* Check that the address is valid */
        if (cpu_get_phys_page_debug(env, vstart) == -1) {
            DD("%s: bad guest address!", __FUNCTION__);
            return -1;
        }

        count++;

        size   -= (next - address);
        address = next;
    }
    return count;
}

/* Fill the pipe buffers to prepare memory transfer from/to
 * [address...address+size). This assumes 'buffers' points to an array
 * which size corresponds to _countPipeBuffers(address, size)
 */
static void
_fillPipeBuffers( uint32_t  address, uint32_t  size, GoldfishPipeBuffer*  buffers )
{
    CPUState*  env = cpu_single_env;

    while (size > 0) {
        uint32_t   vstart = address & TARGET_PAGE_MASK;
        uint32_t   vend   = vstart + TARGET_PAGE_SIZE;
        uint32_t   next   = address + size;

        if (next > vend) {
            next = vend;
        }

        /* Translate virtual address into physical one, into emulator
         * memory. */
        target_phys_addr_t  phys = cpu_get_phys_page_debug(env, vstart);

        buffers[0].data = qemu_get_ram_ptr(phys) + (address - vstart);
        buffers[0].size = next - address;
        buffers++;

        size   -= (next - address);
        address = next;
    }
}

static uint32_t
pipeClient_doCommand( PipeClient* pcl,
                      uint32_t   command,
                      uint32_t   address,
                      uint32_t*  pSize )
{
    uint32_t  size = *pSize;

    D("%s: TID=%4d CHANNEL=%08x COMMAND=%d ADDRESS=%08x SIZE=%d\n",
           __FUNCTION__, pcl->tid, pcl->localId, command, address, size);

    /* XXX: TODO */
    switch (command) {
        case QEMUD_PIPE_CMD_CLOSE:
            /* The client is asking us to close the connection, so
             * just do that. */
            qemud_client_close(pcl->client);
            return 0;

        case QEMUD_PIPE_CMD_SEND: {
            /* First, try to allocate a buffer from the handler */
            void*                opaque = pcl->handler;
            GoldfishPipeBuffer*  buffers;
            int                  numBuffers = 0;

            /* Count the number of buffers we need, allocate them,
             * then fill them. */
            numBuffers = _countPipeBuffers(address, size);
            if (numBuffers < 0) {
                D("%s: Invalid guest address range 0x%x - 0x%x (%d bytes)",
                  __FUNCTION__, address, address+size, size);
                  return QEMUD_PIPE_ERROR_NOMEM;
            }
            buffers = alloca( sizeof(*buffers) * numBuffers );
            _fillPipeBuffers(address, size, buffers);

            D("%s: Sending %d bytes using %d buffers", __FUNCTION__,
              size, numBuffers);

            /* Send the data */
            if (pcl->handlerFuncs->sendBuffers(opaque, buffers, numBuffers) < 0) {
                /* When .sendBuffers() returns -1, it usually means
                * that the handler isn't ready to accept a new message. There
                * is however once exception: when the message is too large
                * and it wasn't possible to allocate the buffer.
                *
                * Differentiate between these two cases by looking at errno.
                */
                if (errno == ENOMEM)
                    return QEMUD_PIPE_ERROR_NOMEM;
                else
                    return QEMUD_PIPE_ERROR_AGAIN;
            }
            return 0;
        }

        case QEMUD_PIPE_CMD_RECV: {
            void*     opaque = pcl->handler;
            GoldfishPipeBuffer* buffers;
            int                 numBuffers, ret;

            /* Count the number of buffers we have */
            numBuffers = _countPipeBuffers(address, size);
            if (numBuffers < 0) {
                D("%s: Invalid guest address range 0x%x - 0x%x (%d bytes)",
                  __FUNCTION__, address, address+size, size);
                  return QEMUD_PIPE_ERROR_NOMEM;
            }
            buffers    = alloca(sizeof(*buffers)*numBuffers);
            _fillPipeBuffers(address, size, buffers);

            /* Receive data */
            ret = pcl->handlerFuncs->recvBuffers(opaque, buffers, numBuffers);
            if (ret < 0) {
                if (errno == ENOMEM) {
                    // XXXX: TODO *pSize = msgSize;
                    return QEMUD_PIPE_ERROR_NOMEM;
                } else {
                    return QEMUD_PIPE_ERROR_AGAIN;
                }
            }
            *pSize = ret;
            return 0;
        }

        case QEMUD_PIPE_CMD_WAKE_ON_SEND: {
            pcl->handlerFuncs->wakeOn(pcl->handler, QEMUD_PIPE_WAKE_ON_SEND);
            return 0;
        }

        case QEMUD_PIPE_CMD_WAKE_ON_RECV: {
            pcl->handlerFuncs->wakeOn(pcl->handler, QEMUD_PIPE_WAKE_ON_RECV);
            return 0;
        }

        default:
            return QEMUD_PIPE_ERROR_CONNRESET;
    }
}



QemudClient*
pipeClient_connect( void* opaque, QemudService*  svc, int channel )
{
    PipeClient*   pcl;

    ANEW0(pcl);
    pcl->pipeSvc = opaque;
    pcl->client  = qemud_client_new( svc, channel, pcl,
                                     pipeClient_recv,
                                     pipeClient_close,
                                     NULL,   /* TODO: NO SNAPSHOT SAVE */
                                     NULL ); /* TODO: NO SNAPHOT LOAD */

    /* Only for the initial connection message */
    qemud_client_set_framing(pcl->client, 1);
    return pcl->client;
}

/**********************************************************************
 **********************************************************************
 *****
 *****  GLOBAL PIPE STATE
 *****
 *****/

struct PipeService {
    AIntMap*  threadMap;  /* maps tid to ThreadState */
};

#if 0
static void
pipeService_done(PipeService*  pipeSvc)
{
    /* Get rid of the tid -> ThreadState map */
    AINTMAP_FOREACH_VALUE(pipeSvc->threadMap, ts, threadState_free(ts));
    aintMap_free(pipeSvc->threadMap);
    pipeSvc->threadMap = NULL;
}
#endif

static void
pipeService_init(PipeService*  pipeSvc)
{
    pipeSvc->threadMap = aintMap_new();

    qemud_service_register( "fast-pipe", 0, pipeSvc,
                            pipeClient_connect,
                            NULL,  /* TODO: NO SNAPSHOT SAVE SUPPORT */
                            NULL   /* TODO: NO SNAPSHOT LOAD SUPPORT */ );
}

static ThreadState*
pipeService_getState(PipeService* pipeSvc, int tid)
{
    if (pipeSvc->threadMap == NULL)
        pipeSvc->threadMap = aintMap_new();

    return (ThreadState*) aintMap_get(pipeSvc->threadMap, tid);
}

static void
pipeService_removeState(PipeService* pipeSvc, int tid)
{
    ThreadState* ts = pipeService_getState(pipeSvc, tid);

    if (ts == NULL)
        return;

    aintMap_del(pipeSvc->threadMap,tid);
    threadState_free(ts);
}

static int
pipeService_addPipe(PipeService* pipeSvc, int  tid, int  localId, PipeClient*  pcl)
{
    ThreadState*  ts = pipeService_getState(pipeSvc, tid);

    if (ts == NULL) {
        ts = threadState_new();
        aintMap_set(pipeSvc->threadMap, tid, ts);
    }

    return threadState_addPipe(ts, localId, pcl);
}

static void
pipeService_removePipe(PipeService* pipeSvc, int tid, int localId)
{
    ThreadState* ts = pipeService_getState(pipeSvc, tid);

    if (ts == NULL)
        return;

    threadState_delPipe(ts, localId);
}

/**********************************************************************
 **********************************************************************
 *****
 *****  HARDWARE API - AS SEEN FROM hw/goldfish_trace.c
 *****
 *****/

static PipeService  _globalState[1];

void  init_qemud_pipes(void)
{
    pipeService_init(_globalState);
}

void
goldfish_pipe_thread_death(int  tid)
{
    PipeService*  pipeSvc = _globalState;
    pipeService_removeState(pipeSvc, tid);
}

void
goldfish_pipe_write(int tid, int offset, uint32_t value)
{
    PipeService*  pipeSvc = _globalState;
    DD("%s: tid=%d offset=%d value=%d (0x%x)", __FUNCTION__, tid,
      offset, value, value);

    ThreadState*  ts      = pipeService_getState(pipeSvc, tid);

    if (ts == NULL) {
        D("%s: no thread state for tid=%d", __FUNCTION__, tid);
        return;
    }
    threadState_write(ts, offset, value);
}

uint32_t
goldfish_pipe_read(int tid, int offset)
{
    PipeService*  pipeSvc = _globalState;
    uint32_t      ret;

    DD("%s: tid=%d offset=%d", __FUNCTION__, tid, offset);

    ThreadState*  ts      = pipeService_getState(pipeSvc, tid);

    if (ts == NULL) {
        D("%s: no thread state for tid=%d", __FUNCTION__, tid);
        return 0;
    }
    ret = threadState_read(ts, offset);
    DD("%s:    result=%d (0x%d)", __FUNCTION__, ret, ret);
    return ret;
}
