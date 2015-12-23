/*
** HAX memory slot operations
**
** Copyright (c) 2015-16 Intel Corporation
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

#include "target-i386/hax-slot.h"
#include "target-i386/hax-i386.h"
#include "qemu/queue.h"

//#define DEBUG_HAX_SLOT

#ifdef DEBUG_HAX_SLOT
#define DPRINTF(fmt, ...) \
    do { fprintf(stdout, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

/**
 * HAXSlot: describes a guest physical memory region and its mapping
 *
 * @start_pa: a guest physical address marking the start of the region; must be
 *            page-aligned
 * @end_pa: a guest physical address marking the end of the region; must be
 *          page-aligned
 * @hva_pa_delta: the host virtual address to which guest physical address 0 is
 *                mapped; in other words, for any guest physical address within
 *                the region (start_pa <= pa < end_pa), the corresponding host
 *                virtual address is calculated by host_va = pa + hva_pa_delta
 * @flags: parameters for the mapping; must be non-negative
 * @entry: additional fields for linking #HAXSlot instances together
 */
typedef struct HAXSlot {
    uint64_t start_pa;
    uint64_t end_pa;
    uint64_t hva_pa_delta;
    int flags;
    QTAILQ_ENTRY(HAXSlot) entry;
} HAXSlot;

/* A doubly-linked list (actually a tail queue) of all registered slots */
static QTAILQ_HEAD(HAXSlotListHead, HAXSlot) slot_list =
    QTAILQ_HEAD_INITIALIZER(slot_list);

void hax_slot_init_registry(void)
{
    HAXSlot *initial_slot;

    g_assert(QTAILQ_EMPTY(&slot_list));

    initial_slot = (HAXSlot *) g_malloc0(sizeof(*initial_slot));
    /* Implied: initial_slot->start_pa = 0; */
    /* Ideally we want to set end_pa to 2^64, but that is too large for
     * uint64_t. We don't need to support such a large guest physical address
     * space anyway; (2^64 - TARGET_PAGE_SIZE) should be (more than) enough.
     */
    initial_slot->end_pa = TARGET_PAGE_MASK;
    /* hva_pa_delta and flags are initialized with invalid values */
    initial_slot->hva_pa_delta = ~TARGET_PAGE_MASK;
    initial_slot->flags = -1;
    QTAILQ_INSERT_TAIL(&slot_list, initial_slot, entry);
}

void hax_slot_free_registry(void)
{
    DPRINTF("%s: Deleting all registered slots\n", __func__);
    while (!QTAILQ_EMPTY(&slot_list)) {
        HAXSlot *slot = QTAILQ_FIRST(&slot_list);
        QTAILQ_REMOVE(&slot_list, slot, entry);
        g_free(slot);
    }
}

/**
 * hax_slot_dump: dumps a slot to stdout (for debugging)
 *
 * @slot: the slot to dump
 */
static void hax_slot_dump(HAXSlot *slot)
{
    DPRINTF("[ start_pa=0x%016" PRIx64 ", end_pa=0x%016" PRIx64
            ", hva_pa_delta=0x%016" PRIx64 ", flags=%d ]\n", slot->start_pa,
            slot->end_pa, slot->hva_pa_delta, slot->flags);
}

/**
 * hax_slot_dump_list: dumps @slot_list to stdout (for debugging)
 */
static void hax_slot_dump_list(void)
{
#ifdef DEBUG_HAX_SLOT
    HAXSlot *slot;
    int i = 0;

    DPRINTF("**** BEGIN HAX SLOT LIST DUMP ****\n");
    QTAILQ_FOREACH(slot, &slot_list, entry) {
        DPRINTF("Slot %d:\n\t", i++);
        hax_slot_dump(slot);
    }
    DPRINTF("**** END HAX SLOT LIST DUMP ****\n");
#endif
}

/**
 * hax_slot_find: locates the slot containing a guest physical address
 *
 * Traverses @slot_list, starting from @start_slot, and returns the slot which
 * contains @pa. There should be one and only one such slot, because:
 *
 * 1) @slot_list is initialized with a slot which covers all valid @pa values.
 *    This coverage stays unchanged as new slots are inserted into @slot_list.
 * 2) @slot_list does not contain overlapping slots.
 *
 * @start_slot: the first slot from which @slot_list is traversed and searched;
 *              must not be %NULL
 * @pa: the guest physical address to locate; must not be less than the lower
 *      bound of @start_slot
 */
static HAXSlot * hax_slot_find(HAXSlot *start_slot, uint64_t pa)
{
    HAXSlot *slot;

    g_assert(start_slot);
    g_assert(start_slot->start_pa <= pa);

    slot = start_slot;
    do {
        if (slot->end_pa > pa) {
            return slot;
        }
        slot = QTAILQ_NEXT(slot, entry);
    } while (slot);

    /* Should never reach here */
    g_assert_not_reached();
    return NULL;
}

/**
 * hax_slot_split: splits a slot into two
 *
 * Shrinks @slot and creates a new slot from the vacated region. Returns the
 * new slot.
 *
 * @slot: the slot to be split/shrinked
 * @pa: the splitting point; must be page-aligned and within @slot
 */
static HAXSlot * hax_slot_split(HAXSlot *slot, uint64_t pa)
{
    HAXSlot *new_slot;

    g_assert(slot);
    g_assert(pa > slot->start_pa && pa < slot->end_pa);
    g_assert(!(pa & ~TARGET_PAGE_MASK));

    new_slot = (HAXSlot *) g_malloc0(sizeof(*new_slot));
    new_slot->start_pa = pa;
    new_slot->end_pa = slot->end_pa;
    new_slot->hva_pa_delta = slot->hva_pa_delta;
    new_slot->flags = slot->flags;

    slot->end_pa = pa;
    QTAILQ_INSERT_AFTER(&slot_list, slot, new_slot, entry);
    return new_slot;
}

/**
 * hax_slot_can_merge: tests if two slots are compatible
 *
 * Two slots are considered compatible if they share the same memory mapping
 * attributes. Compatible slots can be merged if they overlap or are adjacent.
 *
 * Returns %true if @slot1 and @slot2 are compatible.
 *
 * @slot1: one of the slots to be tested; must not be %NULL
 * @slot2: the other slot to be tested; must not be %NULL
 */
static bool hax_slot_can_merge(HAXSlot *slot1, HAXSlot *slot2)
{
    g_assert(slot1 && slot2);

    return slot1->hva_pa_delta == slot2->hva_pa_delta
           && slot1->flags == slot2->flags;
}

/**
 * hax_slot_insert: inserts a slot into @slot_list, with the potential side
 *                  effect of creating/updating memory mappings
 *
 * Causes memory mapping attributes of @slot to override those of overlapping
 * slots (including partial slots) in @slot_list. For any slot whose mapping
 * attributes have changed, performs an ioctl to enforce the new mapping.
 *
 * Aborts QEMU on error.
 *
 * @slot: the slot to be inserted
 */
static void hax_slot_insert(HAXSlot *slot)
{
    HAXSlot *low_slot, *high_slot;
    HAXSlot *low_slot_prev, *high_slot_next;
    HAXSlot *old_slot, *old_slot_next;

    g_assert(!QTAILQ_EMPTY(&slot_list));

    low_slot = hax_slot_find(QTAILQ_FIRST(&slot_list), slot->start_pa);
    g_assert(low_slot);
    low_slot_prev = QTAILQ_PREV(low_slot, HAXSlotListHead, entry);

    /* Adjust slot and/or low_slot such that their lower bounds (start_pa)
     * align.
     */
    if (hax_slot_can_merge(low_slot, slot)) {
        slot->start_pa = low_slot->start_pa;
    } else if (slot->start_pa == low_slot->start_pa && low_slot_prev
               && hax_slot_can_merge(low_slot_prev, slot)) {
        low_slot = low_slot_prev;
        slot->start_pa = low_slot->start_pa;
    } else if (slot->start_pa != low_slot->start_pa) {
        /* low_slot->start_pa < slot->start_pa < low_slot->end_pa */
        low_slot = hax_slot_split(low_slot, slot->start_pa);
        g_assert(low_slot);
    }
    /* Now we have slot->start_pa == low_slot->start_pa */

    high_slot = hax_slot_find(low_slot, slot->end_pa - 1);
    g_assert(high_slot);
    high_slot_next = QTAILQ_NEXT(high_slot, entry);

    /* Adjust slot and/or high_slot such that their upper bounds (end_pa)
     * align.
     */
    if (hax_slot_can_merge(slot, high_slot)) {
        slot->end_pa = high_slot->end_pa;
    } else if (slot->end_pa == high_slot->end_pa && high_slot_next
               && hax_slot_can_merge(slot, high_slot_next)) {
        high_slot = high_slot_next;
        slot->end_pa = high_slot->end_pa;
    } else if (slot->end_pa != high_slot->end_pa) {
        /* high_slot->start_pa < slot->end_pa < high_slot->end_pa */
        high_slot_next = hax_slot_split(high_slot, slot->end_pa);
        g_assert(high_slot_next);
    }
    /* Now we have slot->end_pa == high_slot->end_pa */

    /* We are ready for substitution: replace all slots between low_slot and
     * high_slot (inclusive) with slot. */

    /* Step 1: insert slot into the list, before low_slot */
    QTAILQ_INSERT_BEFORE(low_slot, slot, entry);

    /* Step 2: remove low_slot..high_slot, one by one */
    for (old_slot = low_slot;
         /* This condition always evaluates to 1. See:
          * https://en.wikipedia.org/wiki/Comma_operator
          */
         old_slot_next = QTAILQ_NEXT(old_slot, entry), 1;
         old_slot = old_slot_next) {
        g_assert(old_slot);

        QTAILQ_REMOVE(&slot_list, old_slot, entry);
        if (!hax_slot_can_merge(slot, old_slot)) {
            /* Mapping for guest memory region [old_slot->start_pa,
             * old_slot->end_pa) has changed - must do ioctl. */
            /* TODO: Further reduce the number of ioctl calls by preprocessing
             * the low_slot..high_slot sublist and combining any two adjacent
             * slots that are both incompatible with slot.
             */
            uint32_t size = old_slot->end_pa - old_slot->start_pa;
            uint64_t host_va = old_slot->start_pa + slot->hva_pa_delta;
            int err;

            DPRINTF("%s: Doing ioctl (size=0x%08" PRIx32 ")\n", __func__, size);
            /* Use the new host_va and flags */
            err = hax_set_ram(old_slot->start_pa, size, host_va, slot->flags);
            if (err) {
                fprintf(stderr, "%s: Failed to set memory mapping (err=%d)\n",
                        __func__, err);
                abort();
            }
        }
        g_free(old_slot);

        /* Exit the infinite loop following the removal of high_slot */
        if (old_slot == high_slot) {
            break;
        }
    }
}

void hax_slot_register(uint64_t start_pa, uint32_t size, uint64_t host_va,
                       int flags)
{
    uint64_t end_pa = start_pa + size;
    HAXSlot *slot;

    g_assert(!(start_pa & ~TARGET_PAGE_MASK));
    g_assert(!(end_pa & ~TARGET_PAGE_MASK));
    g_assert(start_pa < end_pa);
    g_assert(host_va);
    g_assert(flags >= 0);

    slot = g_malloc0(sizeof(*slot));
    slot->start_pa = start_pa;
    slot->end_pa = end_pa;
    slot->hva_pa_delta = host_va - start_pa;
    slot->flags = flags;

    DPRINTF("%s: Inserting slot:\n\t", __func__);
    hax_slot_dump(slot);
    hax_slot_dump_list();

    hax_slot_insert(slot);

    DPRINTF("%s: Done\n", __func__);
    hax_slot_dump_list();
}
