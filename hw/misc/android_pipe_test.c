/* Copyright (C) 2011 The Android Open Source Project
** Copyright (C) 2014 Linaro Limited
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
** Example uses of Android Pipes.
*/
#include "hw/misc/android_pipe.h"

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


/***********************************************************************
 ***********************************************************************
 *****
 *****    Z E R O   P I P E S
 *****
 *****/

/* A simple pipe service that mimics /dev/zero, you can write anything to
 * it, and you can always read any number of zeros from it. Useful for debugging
 * the kernel driver.
 */

typedef struct {
    void* hwpipe;
} ZeroPipe;

static void*
zeroPipe_init( void* hwpipe, void* svcOpaque, const char* args )
{
    ZeroPipe*  zpipe;

    D("%s: hwpipe=%p", __FUNCTION__, hwpipe);
    zpipe = g_malloc0(sizeof(ZeroPipe));
    zpipe->hwpipe = hwpipe;
    return zpipe;
}

static void
zeroPipe_close( void* opaque )
{
    ZeroPipe*  zpipe = opaque;

    D("%s: hwpipe=%p", __FUNCTION__, zpipe->hwpipe);
    g_free(zpipe);
}

static int
zeroPipe_sendBuffers( void* opaque, const AndroidPipeBuffer* buffers, int numBuffers )
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
zeroPipe_recvBuffers( void* opaque, AndroidPipeBuffer* buffers, int numBuffers )
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
    return PIPE_POLL_IN | PIPE_POLL_OUT;
}

static void
zeroPipe_wakeOn( void* opaque, int flags )
{
    /* nothing to do here */
}

static const AndroidPipeFuncs  zeroPipe_funcs = {
    zeroPipe_init,
    zeroPipe_close,
    zeroPipe_sendBuffers,
    zeroPipe_recvBuffers,
    zeroPipe_poll,
    zeroPipe_wakeOn,
};

#if DEBUG_ZERO_PIPE
void android_zero_pipe_init(void)
{
    android_pipe_add_type("zero", NULL, &zeroPipe_funcs);
}
#else
void android_zero_pipe_init(void) { }
#endif /* DEBUG_ZERO */

/***********************************************************************
 ***********************************************************************
 *****
 *****    P I N G   P O N G   P I P E S
 *****
 *****/

/* Similar debug service that sends back anything it receives */
/* All data is kept in a circular dynamic buffer */

/* Initial buffer size */
#define PINGPONG_SIZE  1024

typedef struct {
    void*     hwpipe;
    uint8_t*  buffer;
    size_t    size;
    size_t    pos;
    size_t    count;
    unsigned  flags;
} PingPongPipe;

static void
pingPongPipe_init0( PingPongPipe* pipe, void* hwpipe, void* svcOpaque )
{
    pipe->hwpipe = hwpipe;
    pipe->size = PINGPONG_SIZE;
    pipe->buffer = malloc(pipe->size);
    pipe->pos = 0;
    pipe->count = 0;
}

static void*
pingPongPipe_init( void* hwpipe, void* svcOpaque, const char* args )
{
    PingPongPipe*  ppipe;

    D("%s: hwpipe=%p", __FUNCTION__, hwpipe);
    ppipe = g_malloc0(sizeof(PingPongPipe));
    pingPongPipe_init0(ppipe, hwpipe, svcOpaque);
    return ppipe;
}

static void
pingPongPipe_close( void* opaque )
{
    PingPongPipe*  ppipe = opaque;

    D("%s: hwpipe=%p (pos=%zd count=%zd size=%zd)", __FUNCTION__,
      ppipe->hwpipe, ppipe->pos, ppipe->count, ppipe->size);
    free(ppipe->buffer);
    g_free(ppipe);
}

static int
pingPongPipe_sendBuffers( void* opaque, const AndroidPipeBuffer* buffers, int numBuffers )
{
    PingPongPipe*  pipe = opaque;
    int  ret = 0;
    int  count;
    const AndroidPipeBuffer* buff = buffers;
    const AndroidPipeBuffer* buffEnd = buff + numBuffers;

    count = 0;
    for ( ; buff < buffEnd; buff++ )
        count += buff->size;

    /* Do we need to grow the pingpong buffer? */
    while (count > pipe->size - pipe->count) {
        size_t    newsize = pipe->size*2;
        uint8_t*  newbuff = realloc(pipe->buffer, newsize);
        int       wpos    = pipe->pos + pipe->count;
        if (newbuff == NULL) {
            break;
        }
        if (wpos > pipe->size) {
            wpos -= pipe->size;
            memcpy(newbuff + pipe->size, newbuff, wpos);
        }
        pipe->buffer = newbuff;
        pipe->size   = newsize;
        D("pingpong buffer is now %zd bytes", newsize);
    }

    for ( buff = buffers; buff < buffEnd; buff++ ) {
        int avail = pipe->size - pipe->count;
        if (avail <= 0) {
            if (ret == 0)
                ret = PIPE_ERROR_AGAIN;
            break;
        }
        if (avail > buff->size) {
            avail = buff->size;
        }

        int wpos = pipe->pos + pipe->count;
        if (wpos >= pipe->size) {
            wpos -= pipe->size;
        }
        if (wpos + avail <= pipe->size) {
            memcpy(pipe->buffer + wpos, buff->data, avail);
        } else {
            int  avail2 = pipe->size - wpos;
            memcpy(pipe->buffer + wpos, buff->data, avail2);
            memcpy(pipe->buffer, buff->data + avail2, avail - avail2);
        }
        pipe->count += avail;
        ret += avail;
    }

    /* Wake up any waiting readers if we wrote something */
    if (pipe->count > 0 && (pipe->flags & PIPE_WAKE_READ)) {
        android_pipe_wake(pipe->hwpipe, PIPE_WAKE_READ);
    }

    return ret;
}

static int
pingPongPipe_recvBuffers( void* opaque, AndroidPipeBuffer* buffers, int numBuffers )
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

        if (rpos + avail <= pipe->size) {
            memcpy(buffers[0].data, pipe->buffer + rpos, avail);
        } else {
            int  avail2 = pipe->size - rpos;
            memcpy(buffers[0].data, pipe->buffer + rpos, avail2);
            memcpy(buffers[0].data + avail2, pipe->buffer, avail - avail2);
        }
        pipe->count -= avail;
        pipe->pos   += avail;
        if (pipe->pos >= pipe->size) {
            pipe->pos -= pipe->size;
        }
        ret += avail;
        numBuffers--;
        buffers++;
    }

    /* Wake up any waiting readers if we wrote something */
    if (pipe->count < PINGPONG_SIZE && (pipe->flags & PIPE_WAKE_WRITE)) {
        android_pipe_wake(pipe->hwpipe, PIPE_WAKE_WRITE);
    }

    return ret;
}

static unsigned
pingPongPipe_poll( void* opaque )
{
    PingPongPipe*  pipe = opaque;
    unsigned       ret = 0;

    if (pipe->count < pipe->size)
        ret |= PIPE_POLL_OUT;

    if (pipe->count > 0)
        ret |= PIPE_POLL_IN;

    return ret;
}

static void
pingPongPipe_wakeOn( void* opaque, int flags )
{
    PingPongPipe* pipe = opaque;
    pipe->flags |= (unsigned)flags;
}

static const AndroidPipeFuncs  pingPongPipe_funcs = {
    pingPongPipe_init,
    pingPongPipe_close,
    pingPongPipe_sendBuffers,
    pingPongPipe_recvBuffers,
    pingPongPipe_poll,
    pingPongPipe_wakeOn,
};

#if DEBUG_PINGPONG_PIPE
void android_pingpong_init(void)
{
    android_pipe_add_type("pingpong", NULL, &pingPongPipe_funcs);
}
#else
void android_pingpong_init(void) { }
#endif /* DEBUG_PINGPONG_PIPE */

/***********************************************************************
 ***********************************************************************
 *****
 *****    T H R O T T L E   P I P E S
 *****
 *****/

/* Similar to PingPongPipe, but will throttle the bandwidth to test
 * blocking I/O.
 */

typedef struct {
    PingPongPipe  pingpong;
    double        sendRate;
    int64_t       sendExpiration;
    double        recvRate;
    int64_t       recvExpiration;
    QEMUTimer*    timer;
} ThrottlePipe;

/* forward declaration */
static void throttlePipe_timerFunc( void* opaque );

static void*
throttlePipe_init( void* hwpipe, void* svcOpaque, const char* args )
{
    ThrottlePipe* pipe;

    pipe = g_malloc0(sizeof(ThrottlePipe));
    pingPongPipe_init0(&pipe->pingpong, hwpipe, svcOpaque);
    pipe->timer = timer_new(QEMU_CLOCK_VIRTUAL, SCALE_NS, throttlePipe_timerFunc, pipe);
    /* For now, limit to 500 KB/s in both directions */
    pipe->sendRate = 1e9 / (500*1024*8);
    pipe->recvRate = pipe->sendRate;
    return pipe;
}

static void
throttlePipe_close( void* opaque )
{
    ThrottlePipe* pipe = opaque;

    timer_del(pipe->timer);
    timer_free(pipe->timer);
    pingPongPipe_close(&pipe->pingpong);
}

static void
throttlePipe_rearm( ThrottlePipe* pipe )
{
    int64_t  minExpiration = 0;

    DD("%s: sendExpiration=%" PRId64 " recvExpiration=%" PRId64"\n", __FUNCTION__, pipe->sendExpiration, pipe->recvExpiration);

    if (pipe->sendExpiration) {
        if (minExpiration == 0 || pipe->sendExpiration < minExpiration)
            minExpiration = pipe->sendExpiration;
    }

    if (pipe->recvExpiration) {
        if (minExpiration == 0 || pipe->recvExpiration < minExpiration)
            minExpiration = pipe->recvExpiration;
    }

    if (minExpiration != 0) {
        DD("%s: Arming for %" PRId64 "\n", __FUNCTION__, minExpiration);
        timer_mod(pipe->timer, minExpiration);
    }
}

static void
throttlePipe_timerFunc( void* opaque )
{
    ThrottlePipe* pipe = opaque;
    int64_t  now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    DD("%s: TICK! now=%" PRId64 " sendExpiration=%" PRId64 " recvExpiration=%" PRId64 "\n",
       __FUNCTION__, now, pipe->sendExpiration, pipe->recvExpiration);

    /* Timer has expired, signal wake up if needed */
    int      flags = 0;

    if (pipe->sendExpiration && now > pipe->sendExpiration) {
        flags |= PIPE_WAKE_WRITE;
        pipe->sendExpiration = 0;
    }
    if (pipe->recvExpiration && now > pipe->recvExpiration) {
        flags |= PIPE_WAKE_READ;
        pipe->recvExpiration = 0;
    }
    flags &= pipe->pingpong.flags;
    if (flags != 0) {
        DD("%s: WAKE %d\n", __FUNCTION__, flags);
        android_pipe_wake(pipe->pingpong.hwpipe, flags);
    }

    throttlePipe_rearm(pipe);
}

static int
throttlePipe_sendBuffers( void* opaque, const AndroidPipeBuffer* buffers, int numBuffers )
{
    ThrottlePipe*  pipe = opaque;
    int            ret;

    if (pipe->sendExpiration > 0) {
        return PIPE_ERROR_AGAIN;
    }

    ret = pingPongPipe_sendBuffers(&pipe->pingpong, buffers, numBuffers);
    if (ret > 0) {
        /* Compute next send expiration time */
        pipe->sendExpiration = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + ret*pipe->sendRate;
        throttlePipe_rearm(pipe);
    }
    return ret;
}

static int
throttlePipe_recvBuffers( void* opaque, AndroidPipeBuffer* buffers, int numBuffers )
{
    ThrottlePipe* pipe = opaque;
    int           ret;

    if (pipe->recvExpiration > 0) {
        return PIPE_ERROR_AGAIN;
    }

    ret = pingPongPipe_recvBuffers(&pipe->pingpong, buffers, numBuffers);
    if (ret > 0) {
        pipe->recvExpiration = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + ret*pipe->recvRate;
        throttlePipe_rearm(pipe);
    }
    return ret;
}

static unsigned
throttlePipe_poll( void* opaque )
{
    ThrottlePipe*  pipe = opaque;
    unsigned       ret  = pingPongPipe_poll(&pipe->pingpong);

    if (pipe->sendExpiration > 0)
        ret &= ~PIPE_POLL_OUT;

    if (pipe->recvExpiration > 0)
        ret &= ~PIPE_POLL_IN;

    return ret;
}

static void
throttlePipe_wakeOn( void* opaque, int flags )
{
    ThrottlePipe* pipe = opaque;
    pingPongPipe_wakeOn(&pipe->pingpong, flags);
}

static const AndroidPipeFuncs  throttlePipe_funcs = {
    throttlePipe_init,
    throttlePipe_close,
    throttlePipe_sendBuffers,
    throttlePipe_recvBuffers,
    throttlePipe_poll,
    throttlePipe_wakeOn,
};

#ifdef DEBUG_THROTTLE_PIPE
void android_throttle_init(void)
{
    android_pipe_add_type("throttle", NULL, &throttlePipe_funcs);
}
#else
void android_throttle_init(void) { }
#endif /* DEBUG_THROTTLE_PIPE */
