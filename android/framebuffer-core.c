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

    /* I/O associated with this descriptor. */
    LoopIo  io;

    /* Looper used to communicate framebuffer updates. */
    Looper* looper;
};

/* Header of framebuffer update message sent to the core. */
typedef struct FBUpdateMessage {
    /* x, y, w, and h identify the rectangle that is being updated. */
    uint16_t    x;
    uint16_t    y;
    uint16_t    w;
    uint16_t    h;

    /* Number of bits used to encode a single pixel. */
    uint8_t     bits_per_pixel;

    /* Contains updating rectangle copied over from the framebuffer's pixels. */
    uint8_t rect[0];
} FBUpdateMessage;

/* Framebuffer update notification descriptor to the core. */
typedef struct FBUpdateNotify {
    /* Links all pending FB update notifications. */
    struct FBUpdateNotify*  next_fb_update;

    /* Core framebuffer instance that owns the message. */
    CoreFramebuffer*        core_fb;

    /* Size of the message to transfer. */
    size_t                  message_size;

    /* Update message. */
    FBUpdateMessage         message;
} FBUpdateNotify;

/* Writer used to send FB update notification messages. */
static AsyncWriter  fb_update_writer;

/* Head of the list of pending FB update notifications. */
static FBUpdateNotify*  fb_update_head = NULL;

/* Tail of the list of pending FB update notifications. */
static FBUpdateNotify*  fb_update_tail = NULL;

static void fbupdatenotify_delete(FBUpdateNotify* desc);

static void
corefb_io_func(void* opaque, int fd, unsigned events)
{
    CoreFramebuffer* core_fb = opaque;

    while (fb_update_head != NULL) {
        FBUpdateNotify* current_update = fb_update_head;
        // Lets continue writing of the current notification.
        const AsyncStatus status =
            asyncWriter_write(&fb_update_writer, &core_fb->io);
        switch (status) {
            case ASYNC_COMPLETE:
                // Done with the current update. Move on to the next one.
                break;
            case ASYNC_ERROR:
                // Done with the current update. Move on to the next one.
                loopIo_dontWantWrite(&core_fb->io);
                break;

            case ASYNC_NEED_MORE:
                // Transfer will eventually come back into this routine.
                return;
        }

        // Advance the list of updates
        fb_update_head = current_update->next_fb_update;
        if (fb_update_head == NULL) {
            fb_update_tail = NULL;
        }
        fbupdatenotify_delete(current_update);

        if (fb_update_head != NULL) {
            // Schedule the next one.
            asyncWriter_init(&fb_update_writer, &fb_update_head->message,
                             fb_update_head->message_size, &core_fb->io);
        }
    }
}

/*
 * Gets pointer in framebuffer's pixels for the given pixel.
 * Param:
 *  fb Framebuffer containing pixels.
 *  x, and y identify the pixel to get pointer for.
 * Return:
 *  Pointer in framebuffer's pixels for the given pixel.
 */
static const uint8_t*
_pixel_offset(const QFrameBuffer* fb, int x, int y)
{
    return (const uint8_t*)fb->pixels + y * fb->pitch + x * fb->bytes_per_pixel;
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
fbupdatenotify_create(CoreFramebuffer* core_fb, const QFrameBuffer* fb,
                      int x, int y, int w, int h)
{
    const uint8_t* rect_start = _pixel_offset(fb, x, y);
    const uint8_t* rect_end = _pixel_offset(fb, x + w, y + h);
    const size_t rect_size = rect_end - rect_start;
    FBUpdateNotify* ret = malloc(sizeof(FBUpdateNotify) + rect_size);

    ret->next_fb_update = NULL;
    ret->core_fb = core_fb;
    ret->message_size = sizeof(FBUpdateMessage) + rect_size;
    ret->message.x = x;
    ret->message.y = y;
    ret->message.w = w;
    ret->message.h = h;
    ret->message.bits_per_pixel = fb->bits_per_pixel;
    memcpy(ret->message.rect, rect_start, rect_size);
    return ret;
}

static void
fbupdatenotify_delete(FBUpdateNotify* desc)
{
    if (desc != NULL) {
        free(desc);
    }
}

CoreFramebuffer*
corefb_create(int sock, const char* protocol)
{
    // At this point we're implementing the -raw protocol only.
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
        if (core_fb->looper != NULL) {
            loopIo_done(&core_fb->io);
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
    FBUpdateNotify* descr = fbupdatenotify_create(core_fb, fb, x, y, w, h);

    printf(">>>>>>>>>>>>>>>>>>>> UPDATE: %p\n", fb_update_tail);

    // Lets see if we should list it behind other pending updates.
    if (fb_update_tail != NULL) {
        fb_update_tail->next_fb_update = descr;
        return;
    }

    // We're first in the list. Just send it now.
    fb_update_head = fb_update_tail = descr;
    asyncWriter_init(&fb_update_writer, &fb_update_head->message,
                     fb_update_head->message_size, &core_fb->io);
    status = asyncWriter_write(&fb_update_writer, &core_fb->io);
    switch (status) {
        case ASYNC_COMPLETE:
            fbupdatenotify_delete(descr);
            fb_update_head = fb_update_tail = NULL;
            printf("<<<<<<<<<< Completed immediately\n");
            return;
        case ASYNC_ERROR:
            derror("!!!!!!!!! Failed immediately: %s\n", errno_str);
            fbupdatenotify_delete(descr);
            fb_update_head = fb_update_tail = NULL;
            return;
        case ASYNC_NEED_MORE:
            // Update transfer will eventually complete in corefb_io_func
            printf("^^^^^^^^^^ Scheduled\n");
            return;
    }
}
