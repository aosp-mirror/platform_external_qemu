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

#include "android/opengles.h"
#include "hw/misc/android_pipe.h"

#include "qemu-common.h"
#include "qemu/main-loop.h"
#include "qemu/sockets.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

/* Set to 1 or 2 for debug traces */
#define  DEBUG  0

#if DEBUG >= 1
#  define D(...)   printf(__VA_ARGS__), printf("\n")
#else
#  define D(...)   ((void)0)
#endif

#if DEBUG >= 2
#  define DD(...)  printf(__VA_ARGS__), printf("\n")
#else
#  define DD(...)  ((void)0)
#endif


/* Network pipe implementation */
enum {
    STATE_INIT,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_CLOSING_GUEST,
    STATE_CLOSING_SOCKET
};

typedef struct {
    void* hwpipe;
    int fd;
    int state;
    int wakeWanted;
    int wakeActual;
} NetPipe;

static void net_pipe_free(NetPipe* pipe) {
    DD("%s: fd=%d\n", __FUNCTION__, pipe->fd);
    /* Close the socket */
    int fd = pipe->fd;
    qemu_set_fd_handler(fd, NULL, NULL, NULL);
    closesocket(fd);

    /* Release the pipe object */
    g_free(pipe);
}

static void net_pipe_handle_read(void* opaque);
static void net_pipe_handle_write(void* opaque);

static void net_pipe_reset_state(NetPipe* pipe) {
    IOHandler* read_handler = NULL;
    IOHandler* write_handler = NULL;
    if ((pipe->wakeWanted & PIPE_WAKE_WRITE) != 0) {
        write_handler = net_pipe_handle_write;
    }
    if (pipe->state == STATE_CONNECTED && (pipe->wakeWanted & PIPE_WAKE_READ) != 0) {
        read_handler = net_pipe_handle_read;
    }
    qemu_set_fd_handler(pipe->fd, read_handler, write_handler, pipe);
}


/* This function is only called when the socket is disconnected.
 * See net_pipe_close_from_guest() for the case when the guest requires
 * the disconnection. */
static void net_pipe_close_from_socket(void* opaque) {
    NetPipe* pipe = opaque;

    D("%s: fd=%d", __FUNCTION__, pipe->fd);

    /* If the guest already ordered the pipe to be closed, delete immediately */
    if (pipe->state == STATE_CLOSING_GUEST) {
        net_pipe_free(pipe);
        return;
    }

    /* Force the closure of the pipe channel - if a guest is blocked
     * waiting for a wake signal, it will receive an error. */
    if (pipe->hwpipe != NULL) {
        android_pipe_close(pipe->hwpipe);
        pipe->hwpipe = NULL;
    }

    pipe->state = STATE_CLOSING_SOCKET;
    net_pipe_reset_state(pipe);
    DD("%s: fd=%d: waiting for guest to close pipe", __FUNCTION__, pipe->fd);
}

/* Called whenever data is available from the pipe socket. */
static void net_pipe_handle_read(void* opaque) {
    NetPipe* pipe = opaque;

    DD("%s: fd=%d", __FUNCTION__, pipe->fd);

    pipe->wakeActual |= PIPE_WAKE_READ;

    /* Send wake signal to guest if needed */
    if ((pipe->wakeWanted & PIPE_WAKE_READ) != 0) {
        android_pipe_wake(pipe->hwpipe, PIPE_WAKE_READ);
        pipe->wakeWanted &= ~PIPE_WAKE_READ;
    }


    /* Reset state */
    net_pipe_reset_state(pipe);
}

/* Called whenever the pipe socket is ready to receive data. */
static void net_pipe_handle_write(void* opaque) {
    NetPipe* pipe = opaque;

    DD("%s: fd=%d", __FUNCTION__, pipe->fd);

    pipe->wakeActual |= PIPE_WAKE_WRITE;

    /* Send wake signal to guest if needed */
    if ((pipe->wakeWanted & PIPE_WAKE_WRITE) != 0) {
        android_pipe_wake(pipe->hwpipe, PIPE_WAKE_WRITE);
        pipe->wakeWanted &= ~PIPE_WAKE_WRITE;
    }

    /* Reset state */
    net_pipe_reset_state(pipe);
}

/* Called when async connection to the pipe destination completes (fd >= 0)
 * or fails (fd < 0). */
static void net_pipe_handle_connect(int fd, Error *errp, void* opaque) {
    NetPipe *pipe = opaque;

    DD("%s: fd=%d result=%d", __FUNCTION__, pipe->fd, fd);
    if (fd < 0) {
        /* Could not connect, tell our client by closing the channel. */
        net_pipe_close_from_socket(pipe);
        return;
    }

    pipe->state = STATE_CONNECTED;
    net_pipe_reset_state(pipe);
    return;
}

static void* net_pipe_init_from_address(void* hwpipe, const char* address) {
    NetPipe* pipe = g_new0(NetPipe, 1);
    DD("%s: address=[%s]", __FUNCTION__, address);
    pipe->hwpipe = hwpipe;
    pipe->state  = STATE_CONNECTING;
    if (address[0] == ':') {
        pipe->fd = inet_nonblocking_connect(address, net_pipe_handle_connect,
                                            pipe, NULL);
    } else {
        pipe->fd = unix_nonblocking_connect(address, net_pipe_handle_connect,
                                            pipe, NULL);
    }
    return pipe;
}


/* Called when the guest wants to close the channel. This is different
 * from net_pipe_close_from_socket() which is called when the socket is
 * disconnected. */
static void net_pipe_close_from_guest(void* opaque) {
    NetPipe *pipe = opaque;
    DD("%s: fd=%d", __FUNCTION__, pipe->fd);
    net_pipe_free(pipe);
}

static int net_pipe_ready_send(NetPipe *pipe) {
    if (pipe->state == STATE_CONNECTED)
        return 0;
    else if (pipe->state == STATE_CONNECTING)
        return PIPE_ERROR_AGAIN;
    else if (pipe->hwpipe == NULL)
        return PIPE_ERROR_INVAL;
    else
        return PIPE_ERROR_IO;
}

static int net_pipe_send_buffers(void* opaque,
                                 const AndroidPipeBuffer* buffers,
                                 int numBuffers) {
    NetPipe *pipe = opaque;
    int count = 0;
    int ret = 0;
    size_t buffStart = 0;
    const AndroidPipeBuffer* buff = buffers;
    const AndroidPipeBuffer* buffEnd = buff + numBuffers;

    ret = net_pipe_ready_send(pipe);
    if (ret != 0) {
        DD("%s: fd=%d error=%d", __FUNCTION__, pipe->fd, errno);
        return ret;
    }
    for (; buff < buffEnd; buff++) {
        count += buff->size;
    }
    DD("%s: fd=%d count=%d num_buffers=%d", __FUNCTION__, pipe->fd,
       count, numBuffers);
    buff = buffers;
    while (count > 0) {
        int  avail = buff->size - buffStart;
        int  len = send_all(pipe->fd, buff->data + buffStart, avail);

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
            if (ret == 0) {
                DD("%s: fd=%d EOF", __FUNCTION__, pipe->fd);
                ret = PIPE_ERROR_IO;
            }
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
            DD("%s: fd=%d I/O error [%s]", __FUNCTION__, pipe->fd,
               strerror(errno));
            ret = PIPE_ERROR_IO;
        }
        break;
    }

    if (count > 0) {
        pipe->wakeActual &= ~PIPE_WAKE_WRITE;
    }

    return ret;
}

static int net_pipe_recv_buffers(void *opaque,
                                 AndroidPipeBuffer* buffers,
                                 int numBuffers) {
    NetPipe *pipe = opaque;
    int count = 0;
    int ret   = 0;
    size_t buffStart = 0;
    AndroidPipeBuffer* buff = buffers;
    AndroidPipeBuffer* buffEnd = buff + numBuffers;

    for (; buff < buffEnd; buff++) {
        count += buff->size;
    }
    DD("%s: fd=%d count=%d num_buffers=%d", __FUNCTION__, pipe->fd,
       count, numBuffers);
    buff = buffers;
    while (count > 0) {
        int avail = buff->size - buffStart;
        int len = recv_all(pipe->fd, buff->data + buffStart, avail, true);

        /* the read succeeded */
        if (len > 0) {
            buffStart += len;
            if (buffStart >= buff->size) {
                buff++;
                buffStart = 0;
            }
            count -= len;
            ret   += len;
            continue;
        }

        /* we reached the end of stream? */
        if (len == 0) {
            if (ret == 0) {
                DD("%s: fd=%d EOS", __FUNCTION__, pipe->fd);
                ret = PIPE_ERROR_IO;
            }
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
            DD("%s: fd=%d I/O error [%s]", __FUNCTION__, pipe->fd,
               strerror(errno));
            ret = PIPE_ERROR_IO;
        }
        break;
    }

    if (count > 0) {
        pipe->wakeActual &= ~PIPE_WAKE_READ;
    }

    return ret;
}

static unsigned net_pipe_poll(void *opaque) {
    NetPipe* pipe = opaque;
    unsigned mask = pipe->wakeActual;
    unsigned ret  = 0;

    if (mask & PIPE_WAKE_READ)
        ret |= PIPE_POLL_IN;
    if (mask & PIPE_WAKE_WRITE)
        ret |= PIPE_POLL_OUT;

    DD("%s: fd=%d result=%d", __FUNCTION__, pipe->fd, ret);
    return ret;
}

static void net_pipe_wake_on(void* opaque, int flags) {
    NetPipe *pipe = opaque;

    DD("%s: fd=%d flags=%d", __FUNCTION__, pipe->fd, flags);

    pipe->wakeWanted |= flags;
    net_pipe_reset_state(pipe);
}


static void* net_pipe_init_tcp(void* hwpipe, void* _looper, const char* args) {
    if (args == NULL) {
        D("%s: Missing address!", __FUNCTION__);
        return NULL;
    }
    D("%s: Port is '%s'", __FUNCTION__, args);

    /* Now, look at the port number */
    uint16_t port = 0;
    {
        char *end;
        long val = strtol(args, &end, 10);
        if (end == NULL || *end != '\0' || val <= 0 || val > 65535) {
            D("%s: Invalid port number: '%s'", __FUNCTION__, args);
        }
        port = (uint16_t)val;
    }
    char address[32];
    snprintf(address, sizeof(address), ":%d", port);
    void *ret = net_pipe_init_from_address(hwpipe, address);
    return ret;
}

#ifndef _WIN32
static void* net_pipe_init_unix(void* hwpipe, void* _looper, const char* args) {
    return net_pipe_init_from_address(hwpipe, args);
}
#endif

/**********************************************************************
 **********************************************************************
 *****
 *****  N E T W O R K   P I P E   M E S S A G E S
 *****
 *****/

static const AndroidPipeFuncs  netPipeTcp_funcs = {
    net_pipe_init_tcp,
    net_pipe_close_from_guest,
    net_pipe_send_buffers,
    net_pipe_recv_buffers,
    net_pipe_poll,
    net_pipe_wake_on,
    NULL,  /* we can't save these */
    NULL,  /* we can't load these */
};

#ifndef _WIN32
static const AndroidPipeFuncs  netPipeUnix_funcs = {
    net_pipe_init_unix,
    net_pipe_close_from_guest,
    net_pipe_send_buffers,
    net_pipe_recv_buffers,
    net_pipe_poll,
    net_pipe_wake_on,
    NULL,  /* we can't save these */
    NULL,  /* we can't load these */
};
#endif

static void*
openglesPipe_init( void* hwpipe, void* _looper, const char* args )
{
    NetPipe *pipe;

    char server_addr[PATH_MAX];
    android_gles_server_path(server_addr, sizeof(server_addr));
#ifndef _WIN32
    if (true) {
        pipe = (NetPipe *)net_pipe_init_unix(hwpipe, _looper, server_addr);
        D("Creating Unix OpenGLES pipe for GPU emulation: %s", server_addr);
    } else {
#else /* _WIN32 */
    {
#endif
        /* Connect through TCP as a fallback */
        pipe = (NetPipe *)net_pipe_init_tcp(hwpipe, _looper, server_addr);
        D("Creating TCP OpenGLES pipe for GPU emulation!");
    }
    if (pipe != NULL) {
        // Disable TCP nagle algorithm to improve throughput of small packets
        socket_set_nodelay(pipe->fd);

    // On Win32, adjust buffer sizes
#ifdef _WIN32
        {
            int sndbuf = 128 * 1024;
            int len = sizeof(sndbuf);
            if (setsockopt(pipe->fd, SOL_SOCKET, SO_SNDBUF,
                        (char*)&sndbuf, len) == SOCKET_ERROR) {
                D("Failed to set SO_SNDBUF to %d error=0x%x\n",
                sndbuf, WSAGetLastError());
            }
        }
#endif /* _WIN32 */
    }

    return pipe;
}

static const AndroidPipeFuncs  openglesPipe_funcs = {
    openglesPipe_init,
    net_pipe_close_from_guest,
    net_pipe_send_buffers,
    net_pipe_recv_buffers,
    net_pipe_poll,
    net_pipe_wake_on,
    NULL,  /* we can't save these */
    NULL,  /* we can't load these */
};

void android_net_pipes_init(void) {
    android_pipe_add_type("opengles", NULL, &openglesPipe_funcs);
}
