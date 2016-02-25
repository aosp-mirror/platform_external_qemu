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

#include "android/emulation/android_pipe_twitter.h"

#include "android/emulation/android_pipe.h"
#include "android/emulation/android_twitter.h"
#include "android/utils/debug.h"
#include "android/utils/tempfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEBUG 1

#define E(...) derror(__VA_ARGS__)
#if DEBUG
#define D(...) VERBOSE_PRINT(twitter, __VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

/*****************************************************************************/

/* All data is kept in a circular fixed size buffer */

typedef struct {
    void* hwpipe;
    uint8_t* buffer;
    size_t size;
    size_t pos;
    size_t count;
    unsigned flags;
} TwitterPipe;

/******************************************************************************/

static void twitterPipe_init0(TwitterPipe* pipe,
                              void* hwpipe,
                              void* svcOpaque) {
    pipe->hwpipe = hwpipe;
    pipe->size = sizeof(tweet_t);
    pipe->buffer = static_cast<uint8_t*>(malloc(pipe->size));
    pipe->pos = 0;
    pipe->count = 0;
}

static void* twitterPipe_init(void* hwpipe, void* svcOpaque, const char* args) {
    TwitterPipe* ppipe = NULL;
    D("%s: hwpipe=%p", __FUNCTION__, hwpipe);
    ppipe = static_cast<TwitterPipe*>(malloc(sizeof(TwitterPipe)));
    if (ppipe == NULL) {
        E("Failed to allocate memory for new Twitter pipe");
        return NULL;
    }
    memset(ppipe, 0, sizeof(TwitterPipe));
    twitterPipe_init0(ppipe, hwpipe, svcOpaque);
    return ppipe;
}

static void twitterPipe_close(void* opaque) {
    TwitterPipe* ppipe = static_cast<TwitterPipe*>(opaque);

    D("%s: hwpipe=%p size=%zd)", __FUNCTION__, ppipe->hwpipe, ppipe->size);
    if (ppipe->buffer)
        free(ppipe->buffer);
    free(ppipe);
}

static int twitterPipe_sendBuffers(void* opaque,
                                   const AndroidPipeBuffer* buffers,
                                   int numBuffers) {
    TwitterPipe* pipe = static_cast<TwitterPipe*>(opaque);
    const AndroidPipeBuffer* buff = buffers;

    // Twitter buffers are short so only support one buffer to keep things
    // simple
    if (numBuffers != 1)
        return PIPE_ERROR_INVAL;

    // simply overwrite anything already in the pipe buffer
    memcpy(pipe->buffer, buff->data, sizeof(tweet_t));
    pipe->count = sizeof(tweet_t);

    tweet_t* tweet = (tweet_t*)pipe->buffer;
    tweet->msg[MAX_TWEET_LEN - 1] = 0;

    android_twitter_tweet(tweet);

    return sizeof(tweet_t);
}

static int twitterPipe_recvBuffers(void* opaque,
                                   AndroidPipeBuffer* buffers,
                                   int numBuffers) {
    TwitterPipe* pipe = static_cast<TwitterPipe*>(opaque);
    const AndroidPipeBuffer* buff = buffers;

    // Twitter buffers are short so only support one buffer to keep things
    // simple
    if (numBuffers > 1)
        return PIPE_ERROR_INVAL;

    // receiving is blocking and requires one ore more (possibly overwritten)
    // messages to have been sent before
    if (pipe->count == 0)
        return PIPE_ERROR_AGAIN;

    memcpy(buff->data, pipe->buffer, sizeof(tweet_t));
    pipe->count = 0;

    tweet_t* tweet = (tweet_t*)pipe->buffer;
    tweet->msg[MAX_TWEET_LEN - 1] = 0;

    android_twitter_tweet(tweet);

    return sizeof(tweet_t);
}

static unsigned twitterPipe_poll(void* opaque) {
    TwitterPipe* pipe = static_cast<TwitterPipe*>(opaque);
    unsigned ret = 0;

    if (pipe->count < pipe->size)
        ret |= PIPE_POLL_OUT;

    if (pipe->count > 0)
        ret |= PIPE_POLL_IN;

    tweet_t tweet = {.tsc = 0, .msg = {0}};
    const char* msg = "twitter polling, null msg";
    memcpy(tweet.msg, msg, strlen(msg));
    android_twitter_tweet(&tweet);

    return ret;
}

static void twitterPipe_wakeOn(void* opaque, int flags) {
    TwitterPipe* pipe = static_cast<TwitterPipe*>(opaque);
    pipe->flags |= (unsigned)flags;
}

static const AndroidPipeFuncs twitterPipe_funcs = {
        twitterPipe_init,        twitterPipe_close, twitterPipe_sendBuffers,
        twitterPipe_recvBuffers, twitterPipe_poll,  twitterPipe_wakeOn,
};

extern "C" {
void android_pipe_add_type_twitter(void) {
    if (android_twitter_is_init() == 0) {
        D("File stream not initialized, won't add twitter pipe");
        return;
    }

    android_pipe_add_type("twitter", NULL, &twitterPipe_funcs);
    D("Added twitter pipe");

    return;
}
}
