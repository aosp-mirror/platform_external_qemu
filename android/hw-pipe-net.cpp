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

/* This file implements the 'tcp:' goldfish pipe type which allows
 * guest clients to directly connect to a TCP port through /dev/qemu_pipe.
 */

#include "android/hw-pipe-net.h"

#include "android/async-utils.h"
#include "android/base/synchronization/Lock.h"
#include "android/hw-pipe-opengles.h"
#include "android/opengles.h"
#include "android/utils/assert.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/looper.h"
#include "android/utils/panic.h"
#include "android/utils/sockets.h"
#include "android/utils/system.h"

#include "android/emulation/android_pipe.h"

#include "OpenglRender/RenderChannel.h"

// Sockets includes
#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include <stdlib.h>

/* Implement the net pipes */

/* Set to 1 or 2 for debug traces */
#define DEBUG 0

#if DEBUG >= 1
#define D(...) printf(__VA_ARGS__), printf("\n")
#else
#define D(...) ((void)0)
#endif

#if DEBUG >= 2
#define DD(...) printf(__VA_ARGS__), printf("\n")
#define DDASSERT(cond) _ANDROID_ASSERT(cond, "Assertion failure: ", #cond)
#define DDASSERT_INT_OP(cond, val, op) _ANDROID_ASSERT_INT_OP(cond, val, op)
#else
#define DD(...) ((void)0)
#define DDASSERT(cond) ((void)0)
#define DDASSERT_INT_OP(cond, val, op) ((void)0)
#endif

#define DDASSERT_INT_LT(cond, val) DDASSERT_INT_OP(cond, val, <)
#define DDASSERT_INT_LTE(cond, val) DDASSERT_INT_OP(cond, val, <=)
#define DDASSERT_INT_GT(cond, val) DDASSERT_INT_OP(cond, val, >)
#define DDASSERT_INT_GTE(cond, val) DDASSERT_INT_OP(cond, val, >=)
#define DDASSERT_INT_EQ(cond, val) DDASSERT_INT_OP(cond, val, ==)
#define DDASSERT_INT_NEQ(cond, val) DDASSERT_INT_OP(cond, val, !=)

enum State {
    STATE_INIT,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_CLOSING_GUEST,
    STATE_CLOSING_SOCKET
};

typedef struct {
    void* hwpipe;
    State state;
    int wakeWanted;
    LoopIo* io;
    AsyncConnector connector[1];
} NetPipe;

static void netPipe_free(NetPipe* pipe) {
    int fd;

    /* Close the socket */
    if (pipe->io) {
        fd = loopIo_fd(pipe->io);
        loopIo_free(pipe->io);
        socket_close(fd);
    }

    /* Release the pipe object */
    delete pipe;
}

static void netPipe_resetState(NetPipe* pipe) {
    if ((pipe->wakeWanted & PIPE_WAKE_WRITE) != 0) {
        loopIo_wantWrite(pipe->io);
    } else {
        loopIo_dontWantWrite(pipe->io);
    }

    if (pipe->state == STATE_CONNECTED &&
        (pipe->wakeWanted & PIPE_WAKE_READ) != 0) {
        loopIo_wantRead(pipe->io);
    } else {
        loopIo_dontWantRead(pipe->io);
    }
}

/* This function is only called when the socket is disconnected.
 * See netPipe_closeFromGuest() for the case when the guest requires
 * the disconnection. */
static void netPipe_closeFromSocket(void* opaque) {
    auto pipe = static_cast<NetPipe*>(opaque);

    D("%s", __FUNCTION__);

    /* If the guest already ordered the pipe to be closed, delete immediately */
    if (pipe->state == STATE_CLOSING_GUEST) {
        netPipe_free(pipe);
        return;
    }

    /* Force the closure of the QEMUD channel - if a guest is blocked
     * waiting for a wake signal, it will receive an error. */
    if (pipe->hwpipe != NULL) {
        android_pipe_close(pipe->hwpipe);
        pipe->hwpipe = NULL;
    }

    pipe->state = STATE_CLOSING_SOCKET;
    netPipe_resetState(pipe);
}

/* This is the function that gets called each time there is an asynchronous
 * event on the network pipe.
 */
static void netPipe_io_func(void* opaque, int fd, unsigned events) {
    auto pipe = static_cast<NetPipe*>(opaque);
    int wakeFlags = 0;

    /* Run the connector if we are in the CONNECTING state     */
    /* TODO: Add some sort of time-out, to deal with the case */
    /*        when the server is wedged.                      */
    if (pipe->state == STATE_CONNECTING) {
        AsyncStatus status = asyncConnector_run(pipe->connector);
        if (status == ASYNC_NEED_MORE) {
            return;
        } else if (status == ASYNC_ERROR) {
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
        if ((pipe->wakeWanted & PIPE_WAKE_READ) != 0) {
            wakeFlags |= PIPE_WAKE_READ;
        }
    }

    if ((events & LOOP_IO_WRITE) != 0) {
        if ((pipe->wakeWanted & PIPE_WAKE_WRITE) != 0) {
            wakeFlags |= PIPE_WAKE_WRITE;
        }
    }

    /* Send wake signal to the guest if needed */
    if (wakeFlags != 0) {
        android_pipe_wake(pipe->hwpipe, wakeFlags);
        pipe->wakeWanted &= ~wakeFlags;
    }

    /* Reset state */
    netPipe_resetState(pipe);
}

static void* netPipe_initFromAddress(void* hwpipe,
                                     const SockAddress* address,
                                     Looper* looper) {
    NetPipe* pipe = new NetPipe();
    *pipe = {};
    pipe->hwpipe = hwpipe;
    pipe->state = STATE_INIT;

    {
        AsyncStatus status;

        int fd = socket_create(sock_address_get_family(address), SOCKET_STREAM);
        if (fd < 0) {
            D("%s: Could create socket from address family!", __FUNCTION__);
            netPipe_free(pipe);
            return NULL;
        }

        pipe->io = loopIo_new(looper, fd, netPipe_io_func, pipe);
        status = asyncConnector_init(pipe->connector, address, pipe->io);
        pipe->state = STATE_CONNECTING;

        if (status == ASYNC_ERROR) {
            D("%s: Could not connect to socket: %s", __FUNCTION__, errno_str);
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
static void netPipe_closeFromGuest(void* opaque) {
    auto pipe = static_cast<NetPipe*>(opaque);
    netPipe_free(pipe);
}

static int netPipeReadySend(NetPipe* pipe) {
    if (pipe->state == STATE_CONNECTED)
        return 0;
    else if (pipe->state == STATE_CONNECTING)
        return PIPE_ERROR_AGAIN;
    else if (pipe->hwpipe == NULL)
        return PIPE_ERROR_INVAL;
    else
        return PIPE_ERROR_IO;
}

#ifdef _WIN32
int qemu_windows_send(int fd, const void* _buf, int len1) {
    int ret, len;
    auto buf = (const char*)_buf;

    len = len1;
    while (len > 0) {
        ret = send(fd, buf, len, 0);
        if (ret < 0) {
            errno = WSAGetLastError();
            if (errno != WSAEWOULDBLOCK) {
                return -1;
            }
        } else if (ret == 0) {
            break;
        } else {
            buf += ret;
            len -= ret;
            break;
        }
    }
    return len1 - len;
}

int qemu_windows_recv(int fd, void* _buf, int len1, bool single_read) {
    int ret, len;
    char* buf = (char*)_buf;

    len = len1;
    while (len > 0) {
        ret = recv(fd, buf, len, 0);
        if (ret < 0) {
            errno = WSAGetLastError();
            if (errno != WSAEWOULDBLOCK) {
                return -1;
            }
            continue;
        } else {
            if (single_read) {
                return ret;
            }
            buf += ret;
            len -= ret;
            break;
        }
    }
    return len1 - len;
}

#endif

static int netPipe_sendBuffers(void* opaque,
                               const AndroidPipeBuffer* buffers,
                               int numBuffers) {
    auto pipe = static_cast<NetPipe*>(opaque);
    int count = 0;
    int ret = 0;
    size_t buffStart = 0;
    const AndroidPipeBuffer* buff = buffers;
    const AndroidPipeBuffer* buffEnd = buff + numBuffers;

    ret = netPipeReadySend(pipe);
    if (ret != 0)
        return ret;

    for (; buff < buffEnd; buff++)
        count += buff->size;

    buff = buffers;
    while (count > 0) {
        int  avail = buff->size - buffStart;
        // NOTE:
        // Please take care to send data in a way that does not cause
        // pipe corruption.
        //
        // For instance:
        // One may want to call send() multiple times here, but the important
        // thing to notice is, what happens when multiple send()'s are issued,
        // especially when the last send() results in an error (e.g., EAGAIN)?
        //
        // If the last send() results in EAGAIN, we need to ensure that no
        // data was sent. If we end up sending data successfully through
        // send and then get EAGAIN on the last send() call, the pipe driver
        // will believe there has really not been any data sent,
        // which in turn will cause retransmission of actually-sent data,
        // corrupting the pipe.
        //
        // Otherwise, we need to retry when getting EAGAIN,
        // until the transfer completely goes through.
        // Only the following two situations are correct:
        //
        // a) transfer completely or partially succeeds,
        // returning # bytes written
        // b) nothing is transferred + error code
        //
        // We currently employ two solutions depending on platform:
        // 1. Spin on EAGAIN, with multiple send() calls.
        // 2. Only allow one possible send() success, which makes it easier
        // to deal with EAGAIN.
        //
        // We cannot use #1 on POSIX hosts at least, because if we keep
        // retrying on EAGAIN, it's possible to lock up the emulator.
        // This is because currently, there is a global lock
        // for all render threads.
        // If the wrong thread is waiting for the lock at the same time
        // we are retrying on the EAGAIN signal, no progress will be made.
        // So we just use #2: HANDLE_EINTR(send(...)), allowing only
        // one successful send() and returning EAGAIN without the possibility
        // of previous successful sends.
        //
        // Curiously, on Windows hosts, solution #2 will cause the emulator
        // to not be able to boot up, with repeated surfaceflinger failures.
        // On Windows hosts, solution #1 (spin on EAGAIN) must be used.
#ifndef _WIN32
        // Solution #2: Allow only one successful send()
        int  len = HANDLE_EINTR(send(loopIo_fd(pipe->io),
                                     buff->data + buffStart,
                                     avail,
                                     0));
#else
        // Solution #1: Spin on EAGAIN (WSAEWOULDBLOCK)
        int  len = qemu_windows_send(loopIo_fd(pipe->io),
                                     buff->data + buffStart,
                                 avail);
#endif

        /* the write succeeded */
        if (len > 0) {
            buffStart += len;
            if (buffStart >= buff->size) {
                buff++;
                buffStart = 0;
            }
            count -= len;
            ret += len;
            continue;
        }

        /* we reached the end of stream? */
        if (len == 0) {
            if (ret == 0)
                ret = PIPE_ERROR_IO;
            break;
        }

        /* if we already wrote some stuff, simply return */
        if (ret > 0) {
            break;
        }

        /* need to return an appropriate error code */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ret = PIPE_ERROR_AGAIN;
        } else {
            ret = PIPE_ERROR_IO;
        }
        break;
    }

    return ret;
}

static int netPipe_recvBuffers(void* opaque,
                               AndroidPipeBuffer* buffers,
                               int numBuffers) {
    auto pipe = static_cast<NetPipe*>(opaque);
    int count = 0;
    int ret = 0;
    size_t buffStart = 0;
    AndroidPipeBuffer* buff = buffers;
    AndroidPipeBuffer* buffEnd = buff + numBuffers;

    for (; buff < buffEnd; buff++)
        count += buff->size;

    buff = buffers;
    while (count > 0) {
        int  avail = buff->size - buffStart;
        // See comment in netPipe_sendBuffers.
        // Although we have not observed it yet,
        // pipe corruption can potentially happen here too.
        // We use the same solutions to be safe.
#ifndef _WIN32
        int  len = HANDLE_EINTR(recv(loopIo_fd(pipe->io),
                                buff->data + buffStart,
                                avail,
                                0));
#else
        int  len = qemu_windows_recv(loopIo_fd(pipe->io),
                                     buff->data + buffStart,
                                     avail,
                                     true);
#endif

        /* the read succeeded */
        if (len > 0) {
            buffStart += len;
            if (buffStart >= buff->size) {
                buff++;
                buffStart = 0;
            }
            count -= len;
            ret += len;
            continue;
        }

        /* we reached the end of stream? */
        if (len == 0) {
            if (ret == 0)
                ret = PIPE_ERROR_IO;
            break;
        }

        /* if we already read some stuff, simply return */
        if (ret > 0) {
            break;
        }

        /* need to return an appropriate error code */
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ret = PIPE_ERROR_AGAIN;
        } else {
            ret = PIPE_ERROR_IO;
        }
        break;
    }
    return ret;
}

static unsigned netPipe_poll(void* opaque) {
    auto pipe = static_cast<NetPipe*>(opaque);
    unsigned mask = loopIo_poll(pipe->io);
    unsigned ret = 0;

    if (mask & LOOP_IO_READ)
        ret |= PIPE_POLL_IN;
    if (mask & LOOP_IO_WRITE)
        ret |= PIPE_POLL_OUT;

    return ret;
}

static void netPipe_wakeOn(void* opaque, int flags) {
    auto pipe = static_cast<NetPipe*>(opaque);

    DD("%s: flags=%d", __FUNCTION__, flags);

    pipe->wakeWanted |= flags;
    netPipe_resetState(pipe);
}

void* netPipe_initTcp(void* hwpipe, void* _looper, const char* args) {
    /* Build SockAddress from arguments. Acceptable formats are:
     *   <port>
     */
    SockAddress address;
    uint16_t port;
    void* ret;

    if (args == NULL) {
        D("%s: Missing address!", __FUNCTION__);
        return NULL;
    }
    D("%s: Port is '%s'", __FUNCTION__, args);

    /* Now, look at the port number */
    {
        char* end;
        long val = strtol(args, &end, 10);
        if (end == NULL || *end != '\0' || val <= 0 || val > 65535) {
            D("%s: Invalid port number: '%s'", __FUNCTION__, args);
        }
        port = (uint16_t)val;
    }
    sock_address_init_inet(&address, SOCK_ADDRESS_INET_LOOPBACK, port);

    ret = netPipe_initFromAddress(hwpipe, &address,
                                  static_cast<Looper*>(_looper));

    sock_address_done(&address);
    return ret;
}

#ifndef _WIN32
void* netPipe_initUnix(void* hwpipe, void* _looper, const char* args) {
    /* Build SockAddress from arguments. Acceptable formats are:
     *
     *   <path>
     */
    SockAddress address;
    void* ret;

    if (args == NULL || args[0] == '\0') {
        D("%s: Missing address!", __FUNCTION__);
        return NULL;
    }
    D("%s: Address is '%s'", __FUNCTION__, args);

    sock_address_init_unix(&address, args);

    ret = netPipe_initFromAddress(hwpipe, &address,
                                  static_cast<Looper*>(_looper));

    sock_address_done(&address);
    return ret;
}
#endif

/**********************************************************************
 **********************************************************************
 *****
 *****  N E T W O R K   P I P E   M E S S A G E S
 *****
 *****/

static const AndroidPipeFuncs netPipeTcp_funcs = {
        netPipe_initTcp,
        netPipe_closeFromGuest,
        netPipe_sendBuffers,
        netPipe_recvBuffers,
        netPipe_poll,
        netPipe_wakeOn,
        NULL, /* we can't save these */
        NULL, /* we can't load these */
};

#ifndef _WIN32
static const AndroidPipeFuncs netPipeUnix_funcs = {
        netPipe_initUnix,
        netPipe_closeFromGuest,
        netPipe_sendBuffers,
        netPipe_recvBuffers,
        netPipe_poll,
        netPipe_wakeOn,
        NULL, /* we can't save these */
        NULL, /* we can't load these */
};
#endif

void android_net_pipes_init(void) {
    android::OpenglEsPipe::registerPipeType();

    Looper* looper = looper_getForThread();

    android_pipe_add_type("tcp", looper, &netPipeTcp_funcs);
#ifndef _WIN32
    android_pipe_add_type("unix", looper, &netPipeUnix_funcs);
#endif
}
