/*
 * This file contains methods common to goldfish_events and goldfish_rotary.
 * It should only be used by files implementing the goldfish_events-like
 * devices.
 */

#include "hw/input/goldfish_events_common.h"

#include "qemu/log.h"
#include "hw/sysbus.h"

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

static unsigned dequeue_event(GoldfishEvDevState *s)
{
    unsigned n;

    if (s->first == s->last) {
        return 0;
    }

    n = s->events[s->first];

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

void goldfish_enqueue_event(GoldfishEvDevState *s,
                   unsigned int type, unsigned int code, int value)
{
    int  enqueued = s->last - s->first;

    if (enqueued < 0) {
        enqueued += MAX_EVENTS;
    }

    if (enqueued + 3 > MAX_EVENTS) {
        fprintf(stderr, "##KBD: Full queue, lose event\n");
        return;
    }

    if (s->first == s->last) {
        if (s->state == STATE_LIVE) {
            qemu_irq_raise(s->irq);
        } else {
            s->state = STATE_BUFFERED;
        }
    }

    s->events[s->last] = type;
    s->last = (s->last + 1) & (MAX_EVENTS-1);
    s->events[s->last] = code;
    s->last = (s->last + 1) & (MAX_EVENTS-1);
    s->events[s->last] = value;
    s->last = (s->last + 1) & (MAX_EVENTS-1);

}

uint64_t goldfish_events_read(void *opaque, hwaddr offset, unsigned size)
{
    GoldfishEvDevState *s = (GoldfishEvDevState *)opaque;

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
        return dequeue_event(s);
    case REG_LEN:
        return get_page_len(s);
    default:
        if (offset >= REG_DATA) {
            return get_page_data(s, offset - REG_DATA);
        }
        qemu_log_mask(LOG_GUEST_ERROR,
                      "goldfish events device read: bad offset %x\n",
                      (int)offset);
        return 0;
    }
}

void goldfish_events_write(void *opaque, hwaddr offset,
                  uint64_t val, unsigned size)
{
    GoldfishEvDevState *s = (GoldfishEvDevState *)opaque;
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
}

/* set bits [bitl..bith] in the ev_bits[type] array
 */
void goldfish_events_set_bits(GoldfishEvDevState *s, int type, int bitl, int bith)
{
    uint8_t *bits;
    uint8_t maskl, maskh;
    int il, ih;
    il = bitl / 8;
    ih = bith / 8;
    if (ih >= s->ev_bits[type].len) {
        bits = g_malloc0(ih + 1);
        if (bits == NULL) {
            return;
        }
        memcpy(bits, s->ev_bits[type].bits, s->ev_bits[type].len);
        g_free(s->ev_bits[type].bits);
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
}

void goldfish_events_set_bit(GoldfishEvDevState *s, int  type, int  bit)
{
    goldfish_events_set_bits(s, type, bit, bit);
}

void
goldfish_events_clr_bit(GoldfishEvDevState *s, int type, int bit)
{
    int ii = bit / 8;
    if (ii < s->ev_bits[type].len) {
        uint8_t *bits = s->ev_bits[type].bits;
        uint8_t  mask = 0x01U << (bit & 7);
        bits[ii] &= ~mask;
    }
}
