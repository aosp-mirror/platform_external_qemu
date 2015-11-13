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

#include "android/emulation/android_pipe.h"

#include "android/utils/panic.h"
#include "android/utils/system.h"

#include <string.h>

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


/* Maximum length of pipe service name, in characters (excluding final 0) */
#define MAX_PIPE_SERVICE_NAME_SIZE  255

/***********************************************************************
 ***********************************************************************
 *****
 *****    P I P E   A P I
 *****
 *****/

#define MAX_PIPE_SERVICES  16
typedef struct {
    const char* name;
    void* opaque;
    AndroidPipeFuncs funcs;
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
android_pipe_service_find(const char* serviceName)
{
    const PipeServices* list = _pipeServices;
    int count = list->count;
    int nn;

    for (nn = 0; nn < count; nn++) {
        if (!strcmp(list->services[nn].name, serviceName)) {
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

typedef struct Pipe {
    void* opaque;
    const AndroidPipeFuncs* funcs;
    const PipeService* service;
    char* args;
    void* hwpipe;
} Pipe;

/* Forward */
static void*  pipeConnector_new(Pipe*  pipe);

static Pipe* pipe_new0(void* hwpipe) {
    Pipe* pipe;
    ANEW0(pipe);
    pipe->hwpipe = hwpipe;
    return pipe;
}

void* android_pipe_new(void* hwpipe) {
    Pipe* pipe = pipe_new0(hwpipe);
    pipe->opaque = pipeConnector_new(pipe);
    return pipe;
}

void android_pipe_free(void* pipe_) {
    if (!pipe_) {
        return;
    }

    Pipe* pipe = pipe_;
    /* Call close callback */
    if (pipe->funcs->close) {
        pipe->funcs->close(pipe->opaque);
    }
    /* Free stuff */
    AFREE(pipe->args);
    AFREE(pipe);
}

void android_pipe_save(void* pipe_, Stream* file) {
    Pipe* pipe = pipe_;
    if (pipe->service == NULL) {
        /* pipe->service == NULL means we're still using a PipeConnector */
        /* Write a zero to indicate this condition */
        stream_put_byte(file, 0);
    } else {
        /* Otherwise, write a '1' then the service name */
        stream_put_byte(file, 1);
        stream_put_string(file, pipe->service->name);
    }

    /* Write 1 + args, if any, or simply 0 otherwise */
    if (pipe->args != NULL) {
        stream_put_byte(file, 1);
        stream_put_string(file, pipe->args);
    } else {
        stream_put_byte(file, 0);
    }

    if (pipe->funcs->save) {
        pipe->funcs->save(pipe->opaque, file);
    }
}

// Load an optional service name. On success, return 0 and
// sets |*service| to either NULL (the name was not present)
// or a PipeService pointer. Return -1 on failure.
static int pipe_load_service_name(Stream* file,
                                  const PipeService** service) {
    *service = NULL;
    int ret = 0;
    int state = stream_get_byte(file);
    if (state != 0) {
        /* Pipe is associated with a service. */
        char* name = stream_get_string(file);
        if (!name) {
            return -1;
        }
        *service = android_pipe_service_find(name);
        if (!*service) {
            D("No QEMU pipe service named '%s'", name);
            ret = -1;
        }
        AFREE(name);
    }
    return ret;
}

// Load the missing state of a Pipe instance.
// This is used by both pipe_load() and pipe_load_legacy().
//
// |file| is the input stream.
// |hwpipe| is the hardware-side view of the pipe.
// |service| is either NULL or a PipeService instance pointer.
// The NULL value indicates that the pipe is still receiving data
// through a PipeConnector.
// |*force_close| will be set to 1 to indicate that the pipe must be
// force-closed after creation.
//
// Return new Pipe instance, or NULL on error.
static Pipe* pipe_load_state(Stream* file,
                             void* hwpipe,
                             const PipeService* service,
                             char* force_close) {
    *force_close = 0;

    Pipe* pipe = pipe_new0(hwpipe);
    if (!service) {
        // Pipe being opened.
        pipe->opaque = pipeConnector_new(pipe);
    } else {
        // Optional service arguments.
        if (stream_get_byte(file) != 0) {
            pipe->args = stream_get_string(file);
        }
        pipe->funcs = &service->funcs;
    }

    if (pipe->funcs->load) {
        pipe->opaque = pipe->funcs->load(
                hwpipe, service ? service->opaque : NULL, pipe->args, file);
        if (!pipe->opaque) {
            android_pipe_free(pipe);
            return NULL;
        }
    } else {
        *force_close = 1;
    }
    return pipe;
}

void* android_pipe_load(Stream* file, void* hwpipe, char* force_close) {
    const PipeService* service;
    if (pipe_load_service_name(file, &service) < 0) {
        return NULL;
    }
    return pipe_load_state(file, hwpipe, service, force_close);
}

void* android_pipe_load_legacy(Stream* file,
                               void* hwpipe,
                               uint64_t* channel,
                               unsigned char* wakes,
                               unsigned char* closed,
                               char* force_close) {
    const PipeService* service;
    if (pipe_load_service_name(file, &service) < 0) {
        return NULL;
    }

    *channel = stream_get_be64(file);
    *wakes = stream_get_byte(file);
    *closed = stream_get_byte(file);

    return pipe_load_state(file, hwpipe, service, force_close);
}

unsigned android_pipe_poll(void* pipe_) {
    Pipe* pipe = pipe_;
    return pipe->funcs->poll(pipe->opaque);
}

int android_pipe_recv(void* pipe_, AndroidPipeBuffer* buffers, int numBuffers) {
    Pipe* pipe = pipe_;
    return pipe->funcs->recvBuffers(pipe->opaque, buffers, numBuffers);
}

int android_pipe_send(void* pipe_, const AndroidPipeBuffer* buffers, int numBuffers) {
    Pipe* pipe = pipe_;
    return pipe->funcs->sendBuffers(pipe->opaque, buffers, numBuffers);
}

void android_pipe_wake_on(void* pipe_, unsigned wakes) {
    Pipe* pipe = pipe_;
    pipe->funcs->wakeOn(pipe->opaque, wakes);
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

    DD("%s: pipe=%p numBuffers=%d", __FUNCTION__, pcon->pipe, numBuffers);

    while (buffers < buffers_limit) {
        int  avail;

        DD("%s: buffer data (%3d bytes): '%.*s'", __FUNCTION__,
           (int)buffers[0].size, (int)buffers[0].size, buffers[0].data);

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

#ifdef ANDROID_QEMU2_SPECIFIC
        // Qemu2 uses its own implementation of ADB connector, which talks
        // through raw pipes; make sure the qemud:adb pipes don't go through
        // qemud multiplexer
        if (pipeArgs && pipeArgs - pipeName == 5
                && strncmp(pipeName, "qemud", 5) == 0
                && strncmp(pipeArgs + 1, "adb", 3) == 0) {
            pipeArgs = strchr(pipeArgs + 1, ':');
        }
#endif

        if (pipeArgs != NULL) {
            *pipeArgs++ = '\0';
            if (!*pipeArgs)
                pipeArgs = NULL;
        }

        Pipe* pipe = pcon->pipe;
        const PipeService* svc = android_pipe_service_find(pipeName);
        if (svc == NULL) {
            D("%s: Unknown server with name %s!", __FUNCTION__, pipeName);
            return PIPE_ERROR_INVAL;
        }

        void* peer = svc->funcs.init(pipe->hwpipe, svc->opaque, pipeArgs);
        if (peer == NULL) {
            D("%s: Initialization failed for pipe %s!", __FUNCTION__, pipeName);
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
pipeConnector_save( void* pipe, Stream* file )
{
    PipeConnector*  pcon = pipe;
    stream_put_be32(file, pcon->buffpos);
    stream_write(file, (const uint8_t*)pcon->buffer, pcon->buffpos);
}

static void*
pipeConnector_load(void* hwpipe, void* pipeOpaque, const char* args,
                   Stream* file)
{
    PipeConnector*  pcon;

    int len = stream_get_be32(file);
    if (len < 0 || len > sizeof(pcon->buffer)) {
        return NULL;
    }
    pcon = pipeConnector_new(hwpipe);
    pcon->buffpos = len;
    if (stream_read(file, (uint8_t*)pcon->buffer, pcon->buffpos) != pcon->buffpos) {
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

