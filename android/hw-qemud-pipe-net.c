/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "sockets.h"
#include "android/utils/assert.h"
#include "android/utils/panic.h"
#include "android/utils/system.h"
#include "android/async-utils.h"
#include "android/looper.h"
#include "android/hw-qemud-pipe.h"
#include "hw/goldfish_pipe.h"

/* Implement the OpenGL fast-pipe */

/* Set to 1 or 2 for debug traces */
#define  DEBUG  0

#if DEBUG >= 1
#  define D(...)   printf(__VA_ARGS__), printf("\n")
#else
#  define D(...)   ((void)0)
#endif

#if DEBUG >= 2
#  define DD(...)                       printf(__VA_ARGS__), printf("\n")
#  define DDASSERT(cond)                _ANDROID_ASSERT(cond, "Assertion failure: ", #cond)
#  define DDASSERT_INT_OP(cond,val,op)  _ANDROID_ASSERT_INT_OP(cond,val,op)
#else
#  define DD(...)                       ((void)0)
#  define DDASSERT(cond)                ((void)0)
#  define DDASSERT_INT_OP(cond,val,op)  ((void)0)
#endif

#define DDASSERT_INT_LT(cond,val)  DDASSERT_INT_OP(cond,val,<)
#define DDASSERT_INT_LTE(cond,val)  DDASSERT_INT_OP(cond,val,<=)
#define DDASSERT_INT_GT(cond,val)  DDASSERT_INT_OP(cond,val,>)
#define DDASSERT_INT_GTE(cond,val)  DDASSERT_INT_OP(cond,val,>=)
#define DDASSERT_INT_EQ(cond,val)  DDASSERT_INT_OP(cond,val,==)
#define DDASSERT_INT_NEQ(cond,val)  DDASSERT_INT_OP(cond,val,!=)


/* Forward declarations */

typedef struct NetPipeInit {
    Looper*      looper;
    SockAddress  serverAddress[1];
} NetPipeInit;

/**********************************************************************
 **********************************************************************
 *****
 *****  P I P E   M E S S A G E S
 *****
 *****/

typedef struct PipeMsg {
    struct PipeMsg* next;
    size_t           size;
    uint8_t          data[1];
} PipeMsg;

static PipeMsg*
pipeMsg_alloc( size_t  size )
{
    PipeMsg*  msg = android_alloc(sizeof(*msg) + size);

    msg->next = NULL;
    msg->size = size;
    return msg;
}

static void
pipeMsg_free( PipeMsg*  msg )
{
    AFREE(msg);
}

/**********************************************************************
 **********************************************************************
 *****
 *****  M E S S A G E   L I S T
 *****
 *****/

typedef struct {
    PipeMsg*  firstMsg;    /* first message in list */
    PipeMsg*  lastMsg;     /* last message in list */
    size_t    firstBytes;  /* bytes in firstMsg that were already sent */
    size_t    lastBytes;   /* bytes in lastMsg that were already received */
    size_t    totalBytes;  /* total bytes in list */
} MsgList;

/* Default receiver buffer size to accept incoming data */
#define  DEFAULT_RECEIVER_SIZE  8180

/* Initialize a message list - appropriate for sending them out */
static void
msgList_initSend( MsgList*  list )
{
    list->firstMsg   = NULL;
    list->lastMsg    = NULL;
    list->firstBytes = 0;
    list->lastBytes  = 0;
    list->totalBytes = 0;
}

/* Initialize a message list for receiving data */
static void
msgList_initReceive( MsgList*  list )
{
    msgList_initSend(list);
    list->firstMsg  = list->lastMsg = pipeMsg_alloc( DEFAULT_RECEIVER_SIZE );
}

/* Finalize a message list */
static void
msgList_done( MsgList*  list )
{
    PipeMsg*  msg;

    while ((msg = list->firstMsg) != NULL) {
        list->firstMsg = msg->next;
        pipeMsg_free(msg);
    }
    list->lastMsg = NULL;

    list->firstBytes  = 0;
    list->lastBytes   = 0;
    list->totalBytes  = 0;
}

static int
msgList_hasData( MsgList*  list )
{
    return list->totalBytes > 0;
}

/* Append a list of buffers to a message list.
 *
 * This is a very simple implementation that simply mallocs a single
 * new message containing all of the buffer's data, and append it to
 * our link list. This also makes the implementation of msgList_send()
 * quite simple, since there is no need to deal with the 'lastBytes'
 * pointer (it is always assumed to be 'lastMsg->size').
 */
static int
msgList_sendBuffers( MsgList*                   list,
                     const GoldfishPipeBuffer*  buffers,
                     int                        numBuffers )
{
    const GoldfishPipeBuffer*  buff    = buffers;
    const GoldfishPipeBuffer*  buffEnd = buff + numBuffers;
    PipeMsg*                   msg;
    size_t                     msgSize = 0;
    size_t                     pos;

    /* Count the total number of bytes */
    for ( ; buff < buffEnd; buff++ ) {
        msgSize += buff[0].size;
    }

    /* Allocate a new message */
    msg = pipeMsg_alloc(msgSize);
    if (msg == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /* Copy data from buffers to message */
    for ( pos = 0, buff = buffers; buff < buffEnd; buff++ ) {
        memcpy(msg->data + pos, buff->data, buff->size);
        pos += buff->size;
    }

    /* Append message to current list */
    if (list->lastMsg != NULL) {
        list->lastMsg->next = msg;
    } else {
        list->firstMsg   = msg;
        list->firstBytes = 0;
    }
    list->lastMsg = msg;

    list->totalBytes += msgSize;

    /* We are done */
    return 0;
}

/* Try to send outgoing messages in the list through non-blocking socket 'fd'.
 * Return 0 on success, and -1 on failure, where errno will be:
 *
 *    ECONNRESET - connection reset by peer
 *
 * Note that 0 will be returned if socket_send() returns EAGAIN/EWOULDBLOCK.
 */
static int
msgList_send( MsgList*  list, int  fd )
{
    int  ret  = 0;

    for (;;) {
        PipeMsg*  msg        = list->firstMsg;
        size_t    sentBytes  = list->firstBytes;
        size_t    availBytes;

        if (msg == NULL) {
            /* We sent everything */
            return 0;
        }
        DDASSERT(sentBytes < msg->size);
        availBytes = msg->size - sentBytes;

        ret = socket_send(fd, msg->data + sentBytes, availBytes);
        if (ret <= 0) {
            goto ERROR;
        }

        list->totalBytes -= ret;
        list->firstBytes += ret;

        if (list->firstBytes < msg->size) {
            continue;
        }

        /* We sent the full first packet - remove it from the head */
        list->firstBytes = 0;
        list->firstMsg   = msg->next;
        if (list->firstMsg == NULL) {
            list->lastMsg = NULL;
        }
        pipeMsg_free(msg);
    }

ERROR:
    if (ret < 0) { /* EAGAIN/EWOULDBLOCK or disconnection */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ret = 0; /* clear error - this is normal */
        } else {
            DD("%s: socket_send() returned %d: %s\n", __FUNCTION__, errno, errno_str);
            errno = ECONNRESET;
        }
    } else {
#if DEBUG >= 2
        int  err = socket_get_error(fd);
        DD("%s: socket_send() returned 0 (error=%d: %s)", __FUNCTION__,
        err, strerror(err));
#endif
        errno = ECONNRESET;
        ret   = -1;
    }
    return ret;
}


/* Try to receive data into caller-provided buffers, and return the total
 * size of bytes that were read. Returns -1 on error, with errno:
 *
 *    ECONNRESET: Connection reset by peer
 *    EAGAIN: No incoming data, wait for it to arrive.
 */
static int
msgList_recvBuffers( MsgList*             list,
                     GoldfishPipeBuffer*  buffers,
                     int                  numBuffers )
{
    GoldfishPipeBuffer*  buff = buffers;
    GoldfishPipeBuffer*  buffEnd = buff + numBuffers;
    size_t               buffStart = 0;
    PipeMsg*             msg   = list->firstMsg;
    size_t               msgStart = list->firstBytes;
    size_t               totalSize = 0;

    DDASSERT(msg != NULL);

    D("%s: ENTER list.firstBytes=%d list.lastBytes=%d list.totalBytes=%d list.firstSize=%d list.lastSize=%d list.firstEqualLast=%d",
      __FUNCTION__, list->firstBytes, list->lastBytes, list->totalBytes,
      list->firstMsg->size, list->lastMsg->size, list->firstMsg == list->lastMsg);

    /* If there is no incoming data, return EAGAIN */
    if (list->totalBytes == 0) {
        errno = EAGAIN;
        return -1;
    }

    /* Now try to transfer as much from the list of incoming messages
     * into the buffers.
     */
    while (msg != NULL && buff < buffEnd) {
        DDASSERT(msgStart < msg->size);
        DDASSERT(buffStart < buff->size);

        /* Copy data from current message into next buffer.
         * For a given message, first determine the start and end
         * of available data. Then try to see how much of these
         * we can copy to the current buffer.
         */
        size_t  msgEnd = msg->size;

        if (msg == list->lastMsg) {
            msgEnd = list->lastBytes;
        }

        size_t  msgAvail = msgEnd - msgStart;
        size_t  buffAvail = buff->size - buffStart;

        if (msgAvail > buffAvail) {
            msgAvail = buffAvail;
        }

        DDASSERT(msgAvail > 0);

        D("%s: transfer %d bytes (msgStart=%d msgSize=%d buffStart=%d buffSize=%d)",
          __FUNCTION__, msgAvail, msgStart, msg->size, buffStart, buff->size);
        memcpy(buff->data + buffStart, msg->data + msgStart, msgAvail);

        /* Advance cursors */
        msgStart   += msgAvail;
        buffStart  += msgAvail;
        totalSize  += msgAvail;

        /* Did we fill up the current buffer? */
        if (buffStart >= buff->size) {
            buffStart = 0;
            buff++;
        }

        /* Did we empty the current message? */
        if (msgStart >= msgEnd) {
            msgStart = 0;
            /* If this is the last message, reset the 'first' and 'last'
             * pointers to reuse it for the next recv(). */
            if (msg == list->lastMsg) {
                list->lastBytes = 0;
                msg = NULL;
            } else {
                /* Otherwise, delete the message, and jump to the next one */
                list->firstMsg = msg->next;
                pipeMsg_free(msg);
                msg = list->firstMsg;
            }
        }

    }
    list->firstBytes  = msgStart;
    list->totalBytes -= totalSize;

    D("%s: EXIT list.firstBytes=%d list.lastBytes=%d list.totalBytes=%d list.firstSize=%d list.lastSize=%d list.firstEqualLast=%d",
      __FUNCTION__, list->firstBytes, list->lastBytes, list->totalBytes,
      list->firstMsg->size, list->lastMsg->size, list->firstMsg == list->lastMsg);

    return (int)totalSize;
}


/* Try to receive data from non-blocking socket 'fd'.
 * Return 0 on success, or -1 on error, where errno can be:
 *
 *    ECONNRESET         - connection reset by peer
 *    ENOMEM             - full message list, no room to receive more data
 */
static int
msgList_recv( MsgList*  list, int  fd )
{
    int  ret = 0;

    D("%s: ENTER list.firstBytes=%d list.lastBytes=%d list.totalBytes=%d list.firstSize=%d list.lastSize=%d list.firstEqualLast=%d",
      __FUNCTION__, list->firstBytes, list->lastBytes, list->totalBytes,
      list->firstMsg->size, list->lastMsg->size, list->firstMsg == list->lastMsg);

    for (;;) {
        PipeMsg*  last = list->lastMsg;
        size_t    lastBytes = list->lastBytes;
        size_t    availBytes;

        /* Compute how many bytes we can receive in the last buffer*/
        DDASSERT(last != NULL);
        DDASSERT(last->size > 0);
        DDASSERT(lastBytes < last->size);

        availBytes = last->size - lastBytes;

        /* Try to receive the data, act on errors */
        ret = socket_recv(fd, last->data + lastBytes, availBytes);
        if (ret <= 0) {
            goto ERROR;
        }

        /* Acknowledge received data */
        list->lastBytes  += ret;
        list->totalBytes += ret;

        if (list->lastBytes < last->size) {
            continue;
        }

        /* We filled-up the last message buffer, allocate a new one */
        last = pipeMsg_alloc( DEFAULT_RECEIVER_SIZE );
        list->lastMsg->next = last;
        list->lastMsg       = last;
        list->lastBytes     = 0;
    }

ERROR:
    if (ret < 0) { /* EAGAIN/EWOULDBLOCK or disconnection */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ret = 0;  /* clear error - this is normal */
        } else {
            DD("%s: socket_send() returned %d: %s\n", __FUNCTION__, errno, errno_str);
            errno = ECONNRESET;
        }
    } else /* ret == 0 */ {
#if DEBUG >= 2
        int  err = socket_get_error(fd);
        DD("%s: socket_send() returned 0 (error=%d: %s)", __FUNCTION__,
        err, strerror(err));
#endif
        errno = ECONNRESET;
        ret   = -1;
    }
    D("%s: EXIT list.firstBytes=%d list.lastBytes=%d list.totalBytes=%d list.firstSize=%d list.lastSize=%d list.firstEqualLast=%d",
      __FUNCTION__, list->firstBytes, list->lastBytes, list->totalBytes,
      list->firstMsg->size, list->lastMsg->size, list->firstMsg == list->lastMsg);

    return ret;
}



/**********************************************************************
 **********************************************************************
 *****
 *****  P I P E   H A N D L E R S
 *****
 *****/

/* Technical Note:
 *
 * Each NetPipe object is connected to the following:
 *
 *  - a remote rendering process through a normal TCP socket.
 *  - a Goldfish pipe (see hw/goldfish_pipe.h) to exchange messages with the guest.
 *  - a Qemud client  (see android/hw-qemud.h) to signal state changes to the guest.
 *
 *      REMOTE  <---socket---> PIPE <------> GOLDFISH PIPE
 *     PROCESS                      <--+
 *                                     |
 *                                     +---> QEMUD CHANNEL (android/hw-qemud.h)
 *
 */
enum {
    STATE_INIT,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_CLOSING_GUEST,
    STATE_CLOSING_SOCKET
};

#define  DEFAULT_INCOMING_SIZE  4000

#define  MAX_IN_BUFFERS  4

typedef struct {
    QemudClient*  client;
    int           state;
    int           wakeWanted;

    MsgList       outList[1];
    MsgList       inList[1];

    LoopIo          io[1];
    AsyncConnector  connector[1];

    GoldfishPipeBuffer  outBuffer[1];
    GoldfishPipeBuffer  inBuffers[MAX_IN_BUFFERS];
    int                 inBufferCount;

} NetPipe;

static void
netPipe_free( NetPipe*  pipe )
{
    int  fd;

    /* Removing any pending incoming packet */
    msgList_done(pipe->outList);
    msgList_done(pipe->inList);

    /* Close the socket */
    fd = pipe->io->fd;
    loopIo_done(pipe->io);
    socket_close(fd);

    /* Release the pipe object */
    AFREE(pipe);
}


static void
netPipe_resetState( NetPipe* pipe )
{
    /* If there is a pending outgoing packet, open the valve */
    if (msgList_hasData(pipe->outList)) {
        loopIo_wantWrite(pipe->io);
    } else {
        loopIo_dontWantWrite(pipe->io);
    }

    /* Accept incoming data if we are not closing, and our input list isn't full */
    if (pipe->state == STATE_CONNECTED) {
        loopIo_wantRead(pipe->io);
    } else {
        loopIo_dontWantRead(pipe->io);
    }
}


/* This function is only called when the socket is disconnected.
 * See netPipe_closeFromGuest() for the case when the guest requires
 * the disconnection. */
static void
netPipe_closeFromSocket( void* opaque )
{
    NetPipe*  pipe = opaque;

    /* If the guest already ordered the pipe to be closed, delete immediately */
    if (pipe->state == STATE_CLOSING_GUEST) {
        netPipe_free(pipe);
        return;
    }

    /* Force the closure of the QEMUD channel - if a guest is blocked
     * waiting for a wake signal, it will receive an error. */
    if (pipe->client != NULL) {
        qemud_client_close(pipe->client);
        pipe->client = NULL;
    }

    /* Remove any outgoing packets - they won't go anywhere */
    msgList_done(pipe->outList);

    pipe->state = STATE_CLOSING_SOCKET;
    netPipe_resetState(pipe);
}


/* This is the function that gets called each time there is an asynchronous
 * event on the network pipe.
 */
static void
netPipe_io_func( void* opaque, int fd, unsigned events )
{
    NetPipe*  pipe = opaque;
    int         wakeFlags = 0;

    /* Run the connector if we are in the CONNECTING state     */
    /* TODO: Add some sort of time-out, to deal with the case */
    /*        where the renderer process is wedged.            */
    if (pipe->state == STATE_CONNECTING) {
        AsyncStatus  status = asyncConnector_run(pipe->connector);
        if (status == ASYNC_NEED_MORE) {
            return;
        }
        else if (status == ASYNC_ERROR) {
            /* Could not connect, tell our client by closing the channel. */

            netPipe_closeFromSocket(pipe);
            return;
        }
        pipe->state = STATE_CONNECTED;
        netPipe_resetState(pipe);
        return;
    }

    /* Otherwise, accept incoming data */
    if ((events & LOOP_IO_READ) != 0) {
        int ret;

        if ((pipe->wakeWanted & QEMUD_PIPE_WAKE_ON_RECV) != 0) {
            wakeFlags |= QEMUD_PIPE_WAKE_ON_RECV;
        }

        ret = msgList_recv(pipe->inList, fd);
        if (ret < 0) {
            wakeFlags &= ~QEMUD_PIPE_WAKE_ON_RECV;

            if (errno == ENOMEM) { /* shouldn't happen */
                DD("%s: msgList_recv() return ENOMEM!?\n", __FUNCTION__);
            } else {
                /* errno == ECONNRESET */
                DD("%s: msgList_recv() error, closing pipe\n", __FUNCTION__);
                netPipe_closeFromSocket(pipe);
                return;
            }
        }
    }

    if ((events & LOOP_IO_WRITE) != 0) {
        int  ret;

        DDASSERT(msgList_hasData(pipe->outList));

        ret = msgList_send(pipe->outList, fd);
        if (ret < 0) {
            DD("%s: msgList_send() error, closing pipe\n", __FUNCTION__);
            netPipe_closeFromSocket(pipe);
            return;
        }

        if ((pipe->wakeWanted & QEMUD_PIPE_WAKE_ON_SEND) != 0) {
            wakeFlags |= QEMUD_PIPE_WAKE_ON_SEND;
        }
    }

    /* Send wake signal to the guest if needed */
    if (wakeFlags != 0) {
        uint8_t  byte = (uint8_t) wakeFlags;
        DD("%s: Sending wake flags %d (wanted=%d)", __FUNCTION__, byte, pipe->wakeWanted);
        qemud_client_send(pipe->client, &byte, 1);
        pipe->wakeWanted &= ~wakeFlags;
    }

    /* Reset state */
    netPipe_resetState(pipe);
}


void*
netPipe_init( QemudClient*  qcl, void*  pipeOpaque )
{
    NetPipe*      pipe;
    NetPipeInit*  pipeSvc = pipeOpaque;

    ANEW0(pipe);

    pipe->client = qcl;
    pipe->state  = STATE_INIT;

    msgList_initSend(pipe->outList);
    msgList_initReceive(pipe->inList);

#define DEFAULT_OPENGLES_PORT  22468

    {
        AsyncStatus  status;

        int  fd = socket_create_inet( SOCKET_STREAM );
        if (fd < 0) {
            netPipe_free(pipe);
            return NULL;
        }

        loopIo_init(pipe->io, pipeSvc->looper, fd, netPipe_io_func, pipe);
        asyncConnector_init(pipe->connector, pipeSvc->serverAddress, pipe->io);
        pipe->state = STATE_CONNECTING;

        status = asyncConnector_run(pipe->connector);
        if (status == ASYNC_ERROR) {
            D("%s: Could not create to renderer process: %s",
              __FUNCTION__, errno_str);
            netPipe_free(pipe);
            return NULL;
        }
        if (status == ASYNC_COMPLETE) {
            pipe->state = STATE_CONNECTED;
            netPipe_resetState(pipe);
        }
    }

    return pipe;
}

/* Called when the guest wants to close the channel. This is different
 * from netPipe_closeFromSocket() which is called when the socket is
 * disconnected. */
static void
netPipe_closeFromGuest( void* opaque )
{
    NetPipe*  pipe = opaque;

    /* The qemud client is gone when we reach this code */
    pipe->client = NULL;

    /* Remove input messages */
    msgList_done(pipe->inList);

    /* If the socket is already closed, or if there are no
     * outgoing messages, delete immediately */
    if (pipe->state == STATE_CLOSING_SOCKET ||
        !msgList_hasData(pipe->outList)) {
        netPipe_free(pipe);
        return;
    }

    /* Otherwise, mark our pipe as closing, and wait until everything is
     * sent before deleting the object. */
    pipe->state = STATE_CLOSING_GUEST;
    netPipe_resetState(pipe);
}


static int
netPipe_sendBuffers( void* opaque, const GoldfishPipeBuffer* buffers, int numBuffers )
{
    NetPipe*  pipe = opaque;
    int         ret;

    ret = msgList_sendBuffers(pipe->outList, buffers, numBuffers);
    netPipe_resetState(pipe);
    return ret;
}

static int
netPipe_recvBuffers( void* opaque, GoldfishPipeBuffer*  buffers, int  numBuffers )
{
    NetPipe*  pipe = opaque;
    int         ret;

    ret = msgList_recvBuffers(pipe->inList, buffers, numBuffers);
    netPipe_resetState(pipe);
    return ret;
}

static void
netPipe_wakeOn( void* opaque, int flags )
{
    NetPipe*  pipe = opaque;

    pipe->wakeWanted |= flags;
}

/**********************************************************************
 **********************************************************************
 *****
 *****  N E T W O R K   P I P E   M E S S A G E S
 *****
 *****/

static const QemudPipeHandlerFuncs  net_pipe_handler_funcs = {
    netPipe_init,
    netPipe_closeFromGuest,
    netPipe_sendBuffers,
    netPipe_recvBuffers,
    netPipe_wakeOn,
};


static NetPipeInit  _netPipeService[1];

/**********************************************************************
 **********************************************************************
 *****
 *****  O P E N G L E S   P I P E   S E R V I C E
 *****
 *****/

void
android_hw_opengles_init(void)
{
    NetPipeInit*  svc = _netPipeService;
    int           ret;

    DD("%s: Registering service\n", __FUNCTION__);

    svc->looper = looper_newCore();

    ret = sock_address_init_resolve(svc->serverAddress,
                                    "127.0.0.1",
                                    DEFAULT_OPENGLES_PORT,
                                    0);
    if (ret < 0) {
        APANIC("Could not resolve renderer process address!");
    }

    goldfish_pipe_add_type( "opengles", svc, &net_pipe_handler_funcs );
}
