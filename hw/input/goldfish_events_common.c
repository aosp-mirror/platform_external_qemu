/*
 * This file contains methods common to goldfish_events and goldfish_rotary.
 * It should only be used by files implementing the goldfish_events-like
 * devices.
 */

#include "hw/input/goldfish_events_common.h"

#include "qemu/log.h"
#include "hw/sysbus.h"

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

/* Protected by s->lock. */
static int get_page_len(GoldfishEvDevState *s)
{
    int page = s->page;
    if (page == PAGE_NAME) {
        const char *name = s->name;
        return strlen(name);
    }
    if (page >= PAGE_EVBITS && page <= PAGE_EVBITS + EV_MAX) {
        return s->ev_bits[page - PAGE_EVBITS].len;
    }
    if (page == PAGE_ABSDATA) {
        return s->abs_info_count * sizeof(s->abs_info[0]);
    }
    return 0;
}

/* Protected by s->lock. */
static int get_page_data(GoldfishEvDevState *s, int offset)
{
    int page_len = get_page_len(s);
    int page = s->page;
    if (offset > page_len) {
        return 0;
    }
    if (page == PAGE_NAME) {
        const char *name = s->name;
        return name[offset];
    }
    if (page >= PAGE_EVBITS && page <= PAGE_EVBITS + EV_MAX) {
        return s->ev_bits[page - PAGE_EVBITS].bits[offset];
    }
    if (page == PAGE_ABSDATA) {
        return s->abs_info[offset / sizeof(s->abs_info[0])];
    }
    return 0;
}

/* Protected by s->lock. */
static unsigned dequeue_event(GoldfishEvDevState *s)
{
    unsigned n;
    unsigned int ev_index;
    uint64_t ev_time_us;
    struct timeval tv;
    float duration_ms;
    if (s->measure_latency) {
        gettimeofday(&tv, 0);
        ev_time_us = tv.tv_usec + tv.tv_sec * 1000000ULL;
    }

    if (s->first == s->last) {
        return 0;
    }

    ev_index = s->first;
    n = s->events[s->first];

    if (s->measure_latency) {
        s->dequeue_times_us[ev_index] = ev_time_us;
        duration_ms =
            (s->dequeue_times_us[ev_index] -
             s->enqueue_times_us[ev_index]) / 1000.0f;

#define LONG_EVENT_PROCESSING_THRESHOLD_MS 2.0f // 2 ms is quite long to handle interrupt

        if (duration_ms > LONG_EVENT_PROCESSING_THRESHOLD_MS) {
            fprintf(stderr, "%s: warning: long input event processing: "
                            "event %u processed in %f ms\n", __func__,
                            ev_index, duration_ms);
        }
    }

    s->first = (s->first + 1) & (MAX_EVENTS - 1);

    if (s->first == s->last) {
        qemu_irq_lower(s->irq);
    }
#ifdef TARGET_I386
    /*
     * Adding the logic to handle edge-triggered interrupts for x86
     * because the exisiting goldfish events device basically provides
     * level-trigger interrupts only.
     *
     * Logic: When an event (including the type/code/value) is fetched
     * by the driver, if there is still another event in the event
     * queue, the goldfish event device will re-assert the IRQ so that
     * the driver can be notified to fetch the event again.
     */
    else if (((s->first + 2) & (MAX_EVENTS - 1)) < s->last ||
               (s->first & (MAX_EVENTS - 1)) > s->last) {
        /* if there still is an event */
        qemu_irq_lower(s->irq);
        qemu_irq_raise(s->irq);
    }
#endif
    return n;
}

static int g_events_dropped = 0;

int goldfish_event_drop_count() {
    return g_events_dropped;
}

void goldfish_evdev_lock(GoldfishEvDevState* s) {
#ifdef _WIN32
    AcquireSRWLockExclusive(&s->lock);
#else
    pthread_mutex_lock(&s->lock);
#endif
}

void goldfish_evdev_unlock(GoldfishEvDevState* s) {
#ifdef _WIN32
    ReleaseSRWLockExclusive(&s->lock);
#else
    pthread_mutex_unlock(&s->lock);
#endif
}

void goldfish_enqueue_event(GoldfishEvDevState *s,
                   unsigned int type, unsigned int code, int value)
{
    goldfish_evdev_lock(s);

    int  enqueued = s->last - s->first;

    if (enqueued < 0) {
        enqueued += MAX_EVENTS;
    }

    if (enqueued + 3 > MAX_EVENTS) {
        g_events_dropped++;
        fprintf(stderr, "##KBD: Full queue, dropping event, current drop count: %d\n", g_events_dropped);
    } else {
        unsigned int ev_index_0;
        unsigned int ev_index_1;
        unsigned int ev_index_2;
        uint64_t ev_time_us = 0;
        struct timeval tv;

        g_events_dropped = 0;
        if (s->state == STATE_LIVE) {
            qemu_irq_lower(s->irq);
            qemu_irq_raise(s->irq);
        } else {
            s->state = STATE_BUFFERED;
        }

        ev_index_0 = s->last;
        s->events[s->last] = type;
        s->last = (s->last + 1) & (MAX_EVENTS-1);

        ev_index_1 = s->last;
        s->events[s->last] = code;
        s->last = (s->last + 1) & (MAX_EVENTS-1);

        ev_index_2 = s->last;
        s->events[s->last] = value;
        s->last = (s->last + 1) & (MAX_EVENTS-1);

        if (s->measure_latency) {
            gettimeofday(&tv, 0);
            ev_time_us = tv.tv_usec + tv.tv_sec * 1000000ULL;
            s->enqueue_times_us[ev_index_0] = ev_time_us;
            s->enqueue_times_us[ev_index_1] = ev_time_us;
            s->enqueue_times_us[ev_index_2] = ev_time_us;
        }
    }

    goldfish_evdev_unlock(s);
}

uint64_t goldfish_events_read(void *opaque, hwaddr offset, unsigned size)
{
    GoldfishEvDevState *s = (GoldfishEvDevState *)opaque;
    uint64_t res = 0;

    goldfish_evdev_lock(s);

    /* This gross hack below is used to ensure that we
     * only raise the IRQ when the kernel driver is
     * properly ready! If done before this, the driver
     * becomes confused and ignores all input events
     * as soon as one was buffered!
     */
    if (offset == REG_LEN && s->page == PAGE_ABSDATA) {
        if (s->state == STATE_BUFFERED) {
            qemu_irq_raise(s->irq);
        }
        s->state = STATE_LIVE;
    }

    switch (offset) {
    case REG_READ:
        res = dequeue_event(s);
        break;
    case REG_LEN:
        res = get_page_len(s);
        break;
    default:
        if (offset >= REG_DATA) {
            res = get_page_data(s, offset - REG_DATA);
        }
        qemu_log_mask(LOG_GUEST_ERROR,
                      "goldfish events device read: bad offset %x\n",
                      (int)offset);
        break;
    }

    goldfish_evdev_unlock(s);

    return res;
}

void goldfish_events_write(void *opaque, hwaddr offset,
                  uint64_t val, unsigned size)
{
    GoldfishEvDevState *s = (GoldfishEvDevState *)opaque;

    goldfish_evdev_lock(s);

    switch (offset) {
    case REG_SET_PAGE:
        s->page = val;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "goldfish events device write: bad offset %x\n",
                      (int)offset);
        break;
    }
    goldfish_evdev_unlock(s);
}

/* set bits [bitl..bith] in the ev_bits[type] array
 */
void goldfish_events_set_bits(GoldfishEvDevState *s, int type, int bitl, int bith)
{
    uint8_t *bits;
    uint8_t maskl, maskh;
    int il, ih;

    goldfish_evdev_lock(s);

    il = bitl / 8;
    ih = bith / 8;
    if (ih >= s->ev_bits[type].len) {
        bits = g_malloc0(ih + 1);
        if (bits == NULL) {
            goldfish_evdev_unlock(s);
            return;
        }
        if (s->ev_bits[type].bits) {
            memcpy(bits, s->ev_bits[type].bits, s->ev_bits[type].len);
            g_free(s->ev_bits[type].bits);
        }
        s->ev_bits[type].bits = bits;
        s->ev_bits[type].len = ih + 1;
    } else {
        bits = s->ev_bits[type].bits;
    }
    maskl = 0xffU << (bitl & 7);
    maskh = 0xffU >> (7 - (bith & 7));
    if (il >= ih) {
        maskh &= maskl;
    } else {
        bits[il] |= maskl;
        while (++il < ih) {
            bits[il] = 0xff;
        }
    }
    bits[ih] |= maskh;

    goldfish_evdev_unlock(s);
}

void goldfish_events_set_bit(GoldfishEvDevState *s, int  type, int  bit)
{
    goldfish_events_set_bits(s, type, bit, bit);
}

void
goldfish_events_clr_bit(GoldfishEvDevState *s, int type, int bit)
{
    goldfish_evdev_lock(s);
    int ii = bit / 8;
    if (ii < s->ev_bits[type].len) {
        uint8_t *bits = s->ev_bits[type].bits;
        uint8_t  mask = 0x01U << (bit & 7);
        bits[ii] &= ~mask;
    }
    goldfish_evdev_unlock(s);
}
