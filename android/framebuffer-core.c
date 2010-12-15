/* Copyright (C) 2010 The Android Open Source Project
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

/*
 * Contains core-side framebuffer service that sends framebuffer updates
 * to the UI connected to the core.
 */

#include "console.h"
#include "framebuffer.h"
#include "android/looper.h"
#include "android/display-core.h"
#include "android/async-utils.h"
#include "android/framebuffer-core.h"
#include "android/utils/system.h"
#include "android/utils/debug.h"

/* Core framebuffer descriptor. */
struct CoreFramebuffer {
    /* Socket used to communicate framebuffer updates. */
    int     sock;

    /* Looper used to communicate framebuffer updates. */
    Looper* looper;

    /* I/O used to communicate framebuffer updates. */
    LoopIo  io;
};

/* Header of framebuffer update message sent to the core. */
typedef struct FBUpdateMessage {
    /* x, y, w, and h identify the rectangle that is being updated. */
    int     x;
    int     y;
    int     w;
    int     h;

    /* Contains updating rectangle copied over from the framebuffer's pixels. */
    uint8_t rect[0];
} FBUpdateMessage;

/* Framebuffer update notification descriptor to the core. */
typedef struct FBUpdateNotify {
    /* Writer used to send the message. */
    AsyncWriter     writer;

    /* Update message. */
    FBUpdateMessage message;
} FBUpdateNotify;

static void
corefb_io_func(void* opaque, int fd, unsigned events)
{
//    CoreFramebuffer* core_fb = opaque;
}

/*
 * Gets pointer in framebuffer's pixels for the given pixel.
 * Param:
 *  ds - Display state for the framebuffer.
 *  fb Framebuffer containing pixels.
 *  x, and y identify the pixel to get pointer for.
 * Return:
 * Pointer in framebuffer's pixels for the given pixel.
 */
static const uint8_t*
_pixel_offset(const DisplayState* ds, const QFrameBuffer* fb, int x, int y)
{
    return (uint8_t*)fb->pixels +
           (y / fb->width) * fb->pitch +            // Line offset
           x * ds->surface->pf.bytes_per_pixel;    // Pixel offset in line
}

/*
 * Allocates and initializes framebuffer update notification descriptor.
 * Param:
 *  ds - Display state for the framebuffer.
 *  fb Framebuffer containing pixels.
 *  x, y, w, and h identify the rectangle that is being updated.
 * Return:
 *  Initialized framebuffer update notification descriptor.
 */
static FBUpdateNotify*
fbupdatenotify_create(CoreFramebuffer* core_fb,
                      const DisplayState* ds, const QFrameBuffer* fb,
                      int x, int y, int w, int h)
{
    const uint8_t* rect_start = _pixel_offset(ds, fb, x, y);
    const uint8_t* rect_end = _pixel_offset(ds, fb, x + w, y + h);
    const size_t rect_size = rect_end - rect_start;
    FBUpdateNotify* ret = malloc(sizeof(FBUpdateNotify) + rect_size);

    ret->message.x = x;
    ret->message.y = y;
    ret->message.w = w;
    ret->message.h = h;
    memcpy(ret->message.rect, rect_start, rect_size);
    asyncWriter_init(&ret->writer, &ret->message,
                     sizeof(FBUpdateMessage) + rect_size,
                     &core_fb->io);
    return ret;
}

CoreFramebuffer*
corefb_create(int sock)
{
    CoreFramebuffer* ret;
    ANEW0(ret);
    ret->sock = sock;
    ret->looper = looper_newCore();
    loopIo_init(&ret->io, ret->looper, sock, corefb_io_func, ret);

    return ret;
}

void
corefb_destroy(CoreFramebuffer* core_fb)
{
    if (core_fb != NULL) {
        loopIo_done(&core_fb->io);
        if (core_fb->looper != NULL) {
            looper_free(core_fb->looper);
            core_fb->looper = NULL;
        }
    }
}

void
corefb_update(CoreFramebuffer* core_fb, struct DisplayState* ds,
              struct QFrameBuffer* fb, int x, int y, int w, int h)
{
    AsyncStatus status;
    FBUpdateNotify* descr = fbupdatenotify_create(core_fb, ds, fb, x, y, w, h);

    status = asyncWriter_write(&descr->writer, &core_fb->io);
    switch (status) {
        case ASYNC_COMPLETE:
            printf("Framebuffer update completed\n");
            free(descr);
            return;
        case ASYNC_ERROR:
            derror("Unable to send framebuffer update: %s\n", errno_str);
            free(descr);
            return;
        case ASYNC_NEED_MORE:
            // TODO: What we're supposed to do here?
            printf("Framebuffer update incomplete\n");
            return;
    }
}
