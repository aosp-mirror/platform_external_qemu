/* Copyright (C) 2015 The Android Open Source Project
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
**
** Description
**
** A virtual device to exchange short, fixed size messages with timestamps
** between the guest and host. This device can be used for event logging
** across the host and guest with (nearly) proper ordering.
**
*/

#include "android/emulation/android_pipe.h"
#include "android/emulation/android_twitter.h"
#include "android/utils/tempfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_TWITTER

#define E(fmt, ...) do { fprintf(stderr, "twitter: " fmt "\n", ## __VA_ARGS__); } while (0)

#ifdef DEBUG_TWITTER
#define D(fmt, ...) do { fprintf(stdout, "tweet: " fmt "\n", ## __VA_ARGS__); } while (0)
#else
#define D(fmt, ...)  do { /* nothing */ } while (0)
#endif // DEBUG_TWITTER

/*****************************************************************************/

#define  MAX_TWEET_LEN 128
struct _tweet_t_ {
    unsigned long long tsc; // guest's timestamp
    char msg[MAX_TWEET_LEN];
};
typedef struct _tweet_t_ tweet_t;
static FILE *twitter_stream = NULL;

/*****************************************************************************/

/* All data is kept in a circular fixed size buffer */

typedef struct {
    void*     hwpipe;
    uint8_t*  buffer;
    size_t    size;
    size_t    pos;
    size_t    count;
    unsigned  flags;
} TwitterPipe;

unsigned long long time_stamp(void)
{
    unsigned int low = 0;
    unsigned int hi = 0;
    __asm__ __volatile__ ("rdtsc" : "=a"(low), "=d"(hi));
    return ((unsigned long long)low) | (((unsigned long long)hi)<<32 );
}

static void
twitterPipe_init0( TwitterPipe* pipe, void* hwpipe, void* svcOpaque )
{
    pipe->hwpipe = hwpipe;
    pipe->size = sizeof(tweet_t);
    pipe->buffer = static_cast<uint8_t *>(malloc(pipe->size));
    pipe->pos = 0;
    pipe->count = 0;
}

static void*
twitterPipe_init( void* hwpipe, void* svcOpaque, const char* args )
{
    if(twitter_stream == NULL) {
        E("Cannot init twitter pipe without log file");
        return NULL;
    }

    TwitterPipe* ppipe = NULL;
    D("%s: hwpipe=%p", __FUNCTION__, hwpipe);
    ppipe = static_cast<TwitterPipe *>(malloc(sizeof(TwitterPipe)));
    if(ppipe == NULL) {
        E("Failed to allocate memory for new Twitter pipe");
        return NULL;
    }
    memset(ppipe, 0, sizeof(TwitterPipe));
    twitterPipe_init0(ppipe, hwpipe, svcOpaque);
    return ppipe;
}

static void
twitterPipe_close( void* opaque )
{
    TwitterPipe* ppipe = static_cast<TwitterPipe *>(opaque);

    D("%s: hwpipe=%p size=%zd)", __FUNCTION__,
      ppipe->hwpipe, ppipe->size);
    free(ppipe->buffer);
    free(ppipe);

    // just flush the stream; it will be closed and deleted at exit
    fflush(twitter_stream);
}

static int
twitterPipe_sendBuffers( void* opaque, const AndroidPipeBuffer* buffers, int numBuffers )
{
    TwitterPipe* pipe = static_cast<TwitterPipe *>(opaque);
    const AndroidPipeBuffer* buff = buffers;

    // Twitter buffers are short so only support one buffer to keep things simple
    if(numBuffers > 1)
        return PIPE_ERROR_INVAL;

    // simply overwrite anything already in the pipe buffer
    memcpy(pipe->buffer, buff->data, sizeof(tweet_t));
    pipe->count = sizeof(tweet_t);

    // Make sure a string is properly terminated
    tweet_t *tweet = (tweet_t *) pipe->buffer;
    tweet->msg[MAX_TWEET_LEN-1] = 0;

    D("send : h %016lld : g %016lld : m %s",
      time_stamp(), tweet->tsc, tweet->msg);
    android_stweet(tweet->tsc, tweet->msg);

    return sizeof(tweet_t);
}

static int
twitterPipe_recvBuffers( void* opaque, AndroidPipeBuffer* buffers, int numBuffers )
{
    TwitterPipe* pipe = static_cast<TwitterPipe *>(opaque);
    const AndroidPipeBuffer* buff = buffers;

    // Twitter buffers are short so only support one buffer to keep things simple
    if(numBuffers > 1)
        return PIPE_ERROR_INVAL;

    // receiving is blocking and requires one ore more (possibly overwritten)
    // messages to have been sent before
    if(pipe->count == 0)
        return PIPE_ERROR_AGAIN;

    memcpy(buff->data, pipe->buffer, sizeof(tweet_t));
    // count and pos are ignored for any reason other than
    pipe->count = 0;

    // Make sure a string is properly terminated
    tweet_t *tweet = (tweet_t *) pipe->buffer;
    tweet->msg[MAX_TWEET_LEN-1] = 0;

    D("recv : h %016lld : g %016lld : m %s",
      time_stamp(), tweet->tsc, tweet->msg);
    android_stweet(tweet->tsc, tweet->msg);

    return sizeof(tweet_t);
}

static unsigned
twitterPipe_poll( void* opaque )
{
    unsigned long long now = time_stamp();
    TwitterPipe* pipe = static_cast<TwitterPipe *>(opaque);
    unsigned ret = 0;

    if (pipe->count < pipe->size)
        ret |= PIPE_POLL_OUT;

    if (pipe->count > 0)
        ret |= PIPE_POLL_IN;

    D("poll : h %016lld", now);
    android_stweet(now, "poll");

    return ret;
}

static void
twitterPipe_wakeOn( void* opaque, int flags )
{
    TwitterPipe* pipe = static_cast<TwitterPipe *>(opaque);
    pipe->flags |= (unsigned)flags;
}

static const AndroidPipeFuncs  twitterPipe_funcs = {
    twitterPipe_init,
    twitterPipe_close,
    twitterPipe_sendBuffers,
    twitterPipe_recvBuffers,
    twitterPipe_poll,
    twitterPipe_wakeOn,
};

/*****************************************************************************/
// android interface
/*****************************************************************************/

/*****************************************************************************/
/*
** Initialize timestamped log stream as a temp file; delete on exit
** TODO : replace with persistent storage under /tmp or equivalent
**/
int android_twitter_init( void )
{
    TempFile *tf = tempfile_create();
    const char *tfpath = tempfile_path(tf);
    twitter_stream = fopen(tfpath, "w");
    if(twitter_stream == NULL) {
        E("Failed to open twitter temp log file");
        return -1;
    } else {
        int fd =  fileno(twitter_stream);
        atexit_close_fd(fd);
        D("New twitter log at %s", tfpath);
    }

    return 0;
}

/*****************************************************************************/
/* save a short log message */
int android_tweet(unsigned long long ts, const char *msg)
{
    int ret = 0;
    unsigned long long now = time_stamp();

    D("H: %016lld G: %016lld M: %s\n", now, ts, msg);
    ret = fprintf(twitter_stream, "H: %016lld G: %016lld M: %s\n", now, ts, msg);

    return ret; // TODO: only count user msg length --- sub 45
}

/*****************************************************************************/
/* save a short log message atomic (synchronized writes to log file) */
int android_stweet(unsigned long long ts, const char *msg)
{
    int ret = 0;
    // TODO: Synchronization
    ret = android_tweet(ts, msg);

    return ret;
}

/*****************************************************************************/
/* open twitter pipe */
void android_pipe_add_type_twitter( void )
{
    if(twitter_stream == NULL && android_twitter_init() < 0) {
        E("File stream not initialized, won't add twitter pipe");
        return;
    }

    android_pipe_add_type("twitter", NULL, &twitterPipe_funcs);
    D("Added twitter pipe");

    return;
}
