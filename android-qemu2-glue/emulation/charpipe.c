/* Copyright (C) 2007-2008 The Android Open Source Project
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
#include "android/utils/cbuffer.h"
#include "android/utils/debug.h"
#include "qemu/osdep.h"
#include "chardev/char-fe.h"

#define DEBUG 0

#define CPR(ptr) \
    if (!(ptr))  \
        goto Error;

#if DEBUG
#include <stdio.h>
#include "android/utils/misc.h"
#define D(...) dprint(__VA_ARGS__)
#else
#define D(...) ((void)0)
#endif

/* we want to implement a bi-directionnal communication channel
 * between two QEMU character drivers that merge well into the
 * QEMU event loop.
 *
 * each half of the channel has its own object and buffer, and
 * we implement communication through charpipe_poll() which
 * must be called by the main event loop after its call to select()
 *
 */
#define BIP_BUFFER_SIZE 512

typedef struct BipBuffer {
    struct BipBuffer* next;
    CBuffer cb[1];
    char buff[BIP_BUFFER_SIZE];
} BipBuffer;

static BipBuffer* _free_bip_buffers;

static BipBuffer* bip_buffer_alloc(void) {
    BipBuffer* bip = _free_bip_buffers;
    if (bip != NULL) {
        _free_bip_buffers = bip->next;
    } else {
        bip = malloc(sizeof(*bip));
        if (bip == NULL) {
            derror("%s: not enough memory", __FUNCTION__);
            exit(1);
        }
    }
    bip->next = NULL;
    cbuffer_reset(bip->cb, bip->buff, sizeof(bip->buff));
    return bip;
}

static void bip_buffer_free(BipBuffer* bip) {
    bip->next = _free_bip_buffers;
    _free_bip_buffers = bip;
}

/* this models each half of the charpipe */
typedef struct PipeChardev {
    Chardev parent;
    BipBuffer* bip_first;
    BipBuffer* bip_last;
    struct PipeChardev* peer; /* NULL if closed */
    QLIST_ENTRY(PipeChardev) entry;
} PipeChardev;

/** This models a charbuffer, an object used to buffer
 ** the data that is sent to a given endpoint Chardev
 ** object.
 **
 ** On the other hand, any can_read() / read() request performed
 ** by the endpoint will be passed to the BufferChardev's corresponding
 ** handlers.
 **/
typedef struct BufferChardev {
    Chardev parent;
    BipBuffer* bip_first;
    BipBuffer* bip_last;
    Chardev* endpoint; /* NULL if closed */
    QLIST_ENTRY(BufferChardev) entry;
} BufferChardev;

#define TYPE_CHARDEV_BUFFER "chardev-buffer"
#define BUFFER_CHARDEV(obj) \
    OBJECT_CHECK(BufferChardev, (obj), TYPE_CHARDEV_BUFFER)

#define TYPE_CHARDEV_ANDROID_PIPE "chardev-android-pipe"
#define ANDROID_PIPE_CHARDEV(obj) \
    OBJECT_CHECK(PipeChardev, (obj), TYPE_CHARDEV_ANDROID_PIPE)

static int charpipehalf_write(Chardev* dev, const uint8_t* buf, int len) {
    PipeChardev* ph = ANDROID_PIPE_CHARDEV(dev);
    PipeChardev* peer = ph->peer;
    BipBuffer* bip = ph->bip_last;
    int ret = 0;

    D("%s: writing %d bytes to %p: '%s'", __FUNCTION__, len, ph,
      quote_bytes((const char*)buf, len));

    if (bip == NULL && peer != NULL && peer->parent.be->chr_read != NULL) {
        /* no buffered data, try to write directly to the peer */
        while (len > 0) {
            int size;

            if (peer->parent.be->chr_can_read) {
                size = qemu_chr_be_can_write(&peer->parent);
                if (size == 0)
                    break;

                if (size > len)
                    size = len;
            } else
                size = len;

            qemu_chr_be_write(&peer->parent, (uint8_t*)buf, size);
            buf += size;
            len -= size;
            ret += size;
        }
    }

    if (len == 0)
        return ret;

    /* buffer the remaining data */
    if (bip == NULL) {
        bip = bip_buffer_alloc();
        ph->bip_first = ph->bip_last = bip;
    }

    while (len > 0) {
        int len2 = cbuffer_write(bip->cb, buf, len);

        buf += len2;
        ret += len2;
        len -= len2;
        if (len == 0)
            break;

        /* ok, we need another buffer */
        ph->bip_last = bip_buffer_alloc();
        bip->next = ph->bip_last;
        bip = ph->bip_last;
    }
    return ret;
}

static void charpipehalf_poll(PipeChardev* dev) {
    assert(dev);
    PipeChardev* peer = dev->peer;
    int size;

    if (peer == NULL || peer->parent.be->chr_read == NULL)
        return;

    while (1) {
        BipBuffer* bip = dev->bip_first;
        uint8_t* base;
        int avail;

        if (bip == NULL)
            break;

        size = cbuffer_read_avail(bip->cb);
        if (size == 0) {
            dev->bip_first = bip->next;
            if (dev->bip_first == NULL)
                dev->bip_last = NULL;
            bip_buffer_free(bip);
            continue;
        }

        if (dev->parent.be->chr_can_read) {
            int size2 = qemu_chr_be_can_write(&peer->parent);

            if (size2 == 0)
                break;

            if (size > size2)
                size = size2;
        }

        avail = cbuffer_read_peek(bip->cb, &base);
        if (avail > size)
            avail = size;
        D("%s: sending %d bytes from %p: '%s'", __FUNCTION__, avail, dev,
          quote_bytes((const char*)base, avail));

        qemu_chr_be_write(&peer->parent, base, avail);
        cbuffer_read_step(bip->cb, avail);
    }
}

static QLIST_HEAD(, PipeChardev) s_pipes = QLIST_HEAD_INITIALIZER(s_pipes);
static bool s_pipe_poll = false;

int qemu_chr_open_charpipe(Chardev** pfirst, Chardev** psecond) {
    Error* ignored_error = NULL;
    *pfirst = NULL;
    *psecond = NULL;
    Chardev* first = NULL;
    Chardev* second = NULL;
    PipeChardev* a = NULL;
    PipeChardev* b = NULL;

    first = qemu_chardev_new(NULL, TYPE_CHARDEV_ANDROID_PIPE, NULL,
                             &ignored_error);
    CPR(first);

    second = qemu_chardev_new(NULL, TYPE_CHARDEV_ANDROID_PIPE, NULL,
                              &ignored_error);
    CPR(second);

    // Link and register the chardevs.
    a = ANDROID_PIPE_CHARDEV(first);
    b = ANDROID_PIPE_CHARDEV(second);

    a->peer = b;
    b->peer = a;

    // Note that only the entry of a will be set and
    // will occur in the list, b will not occur in this list
    QLIST_INSERT_HEAD(&s_pipes, a, entry);

    *pfirst = first;
    *psecond = second;

    D("%s: created pipe between %p <--> %p", __FUNCTION__, *pfirst, *psecond);
    return 0;

Error:
    object_unref(OBJECT(first));
    object_unref(OBJECT(second));
    return -1;
}

static int charbuffer_write(Chardev* dev, const uint8_t* buf, int len) {
    BufferChardev* cbuf = BUFFER_CHARDEV(dev);
    Chardev* peer = cbuf->endpoint;
    BipBuffer* bip = cbuf->bip_last;
    int ret = 0;

    D("%s: writing %d bytes from %p to %p: '%s'", __FUNCTION__, len, dev, peer,
      quote_bytes((const char*)buf, len));

    if (bip == NULL && peer != NULL) {
        /* no buffered data, try to write directly to the peer */
        int size = qemu_chr_fe_write(peer->be, buf, len);

        if (size < 0) /* just to be safe */
            size = 0;
        else if (size > len)
            size = len;

        buf += size;
        ret += size;
        len -= size;
    }

    if (len == 0)
        return ret;

    /* buffer the remaining data */
    if (bip == NULL) {
        bip = bip_buffer_alloc();
        cbuf->bip_first = cbuf->bip_last = bip;
    }

    while (len > 0) {
        int len2 = cbuffer_write(bip->cb, buf, len);

        buf += len2;
        ret += len2;
        len -= len2;
        if (len == 0)
            break;

        /* ok, we need another buffer */
        cbuf->bip_last = bip_buffer_alloc();
        bip->next = cbuf->bip_last;
        bip = cbuf->bip_last;
    }
    return ret;
}

static void charbuffer_poll(BufferChardev* cbuf) {
    Chardev* peer = cbuf->endpoint;

    if (peer == NULL)
        return;

    while (1) {
        BipBuffer* bip = cbuf->bip_first;
        uint8_t* base;
        int avail;
        int size;

        if (bip == NULL)
            break;

        avail = cbuffer_read_peek(bip->cb, &base);
        if (avail == 0) {
            cbuf->bip_first = bip->next;
            if (cbuf->bip_first == NULL)
                cbuf->bip_last = NULL;
            bip_buffer_free(bip);
            continue;
        }

        size = qemu_chr_fe_write(peer->be, base, avail);

        if (size < 0) /* just to be safe */
            size = 0;
        else if (size > avail)
            size = avail;

        cbuffer_read_step(bip->cb, size);

        if (size < avail)
            break;
    }
}

static QLIST_HEAD(, BufferChardev)
        s_charbuffers = QLIST_HEAD_INITIALIZER(s_charbuffers);

Chardev* qemu_chr_open_buffer(Chardev* endpoint) {
    Error* error = NULL;
    Chardev* dev = qemu_chardev_new(NULL, TYPE_CHARDEV_BUFFER, NULL, &error);
    if (!dev) {
        return NULL;
    }

    BufferChardev* buffer = BUFFER_CHARDEV(dev);
    buffer->endpoint = endpoint;

    QLIST_INSERT_HEAD(&s_charbuffers, buffer, entry);

    D("%s: created buffered chardev %p with endpoint: %p", __FUNCTION__, dev,
      buffer->endpoint);
    return dev;
}

void qemu_charpipe_poll(void) {
    /**
     * Look ma! No locks.
     *
     * There are 2 cases where locking is needed:
     *
     * - We are introducing new devices when this loop is active
     *   - This does not happen in the case of android emulator.
     *     All the devices are constructed before the execution of
     *     qemu main_loop. So new elements will not be added to
     *     any of the lists that we iterate over.
     *
     * - We are removing devices when this loop is active:
     *   - QEMU does not decrease the refcount of any of its created devices
     *     (yet). Because of this finalize is never called on any of the
     *     objects, and hence we will never decrease the refcount to the point
     *     where we will have to remove a device while this loops is active.
     */
    PipeChardev* cps;
    BufferChardev* bc;

    // Polling loop has been activated. If you need support for dynamically
    // adding/removing of devices you will need to turn this into a mutex.
    s_pipe_poll = true;

    /* poll the charpipes */
    QLIST_FOREACH(cps, &s_pipes, entry) {
        charpipehalf_poll(cps);
        charpipehalf_poll(cps->peer);
    }

    /* poll the buffers */
    QLIST_FOREACH(bc, &s_charbuffers, entry) { charbuffer_poll(bc); }
}

static void charbuffer_finalize(Object* obj) {
    // We don't support deletion of devices once we started
    // the polling loop, as we don't have locks around our list
    // access
    assert(!s_pipe_poll);
    BufferChardev* cbuf = BUFFER_CHARDEV(obj);
    while (cbuf->bip_first) {
        BipBuffer* bip = cbuf->bip_first;
        cbuf->bip_first = bip->next;
        bip_buffer_free(bip);
    }
    cbuf->bip_last = NULL;
    cbuf->endpoint = NULL;

    if (cbuf->endpoint != NULL) {
        object_unparent(OBJECT(cbuf->endpoint));
        cbuf->endpoint = NULL;
    }

    QLIST_REMOVE(cbuf, entry);
}

static void charbuffer_class_init(ObjectClass* oc, void* data) {
    ChardevClass* cc = CHARDEV_CLASS(oc);
    cc->chr_write = charbuffer_write;
}

static const TypeInfo charbuffer_type_info = {
        .name = TYPE_CHARDEV_BUFFER,
        .parent = TYPE_CHARDEV,
        .instance_size = sizeof(BufferChardev),
        .instance_finalize = charbuffer_finalize,
        .class_init = charbuffer_class_init,
};

static void charpipe_finalize(Object* obj) {
    // We don't support deletion of devices once we started
    // the polling loop, as we don't have locks around our list
    // access
    assert(!s_pipe_poll);

    PipeChardev* ph = ANDROID_PIPE_CHARDEV(obj);
    while (ph->bip_first) {
        BipBuffer* bip = ph->bip_first;
        ph->bip_first = bip->next;
        bip_buffer_free(bip);
    }
    ph->bip_last = NULL;
    ph->peer = NULL;

    QLIST_REMOVE(ph, entry);
}

static void charpipe_class_init(ObjectClass* oc, void* data) {
    ChardevClass* cc = CHARDEV_CLASS(oc);
    cc->chr_write = charpipehalf_write;
}

static const TypeInfo charpipe_type_info = {
        .name = TYPE_CHARDEV_ANDROID_PIPE,
        .parent = TYPE_CHARDEV,
        .instance_size = sizeof(PipeChardev),
        .instance_finalize = charpipe_finalize,
        .class_init = charpipe_class_init,
};

static void register_types(void) {
    type_register_static(&charbuffer_type_info);
    type_register_static(&charpipe_type_info);
}

// Note that this is a static constructor that gets called upon
// loading of this library. This only works if one function in this
// file gets actually linked into the final executable. Two cases:
// 1. This does not get linked in:
//   -- This is fine as the devices registered are only referenced
//      in this file.
// 2. This does get linked in:
//   -- The qemu types will be registered and therefore available.
type_init(register_types);
