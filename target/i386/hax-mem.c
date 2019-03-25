/*
 * HAX memory mapping operations
 *
 * Copyright (c) 2015-16 Intel Corporation
 * Copyright 2016 Google, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/address-spaces.h"
#include "exec/exec-all.h"
#include "exec/ram_addr.h"
#include "exec/memory-remap.h"

#include "target/i386/hax-i386.h"
#include "qemu/queue.h"
#include "qemu/abort.h"

// Memory slots/////////////////////////////////////////////////////////////////

static void hax_dump_memslots(void)
{
    hax_slot *slot;
    int i;

    for (i = 0; i < HAX_MAX_SLOTS; ++i) {
        slot = &hax_global.memslots[i];
        if (slot->flags & HAX_RAM_INFO_INVALID)
            continue;
        fprintf(stderr, "Slot %d start %016llx end %016llx hva %p flags %x\n",
                i,
                slot->start,
                slot->size + slot->start,
                slot->hva,
                slot->flags);
    }
}

void* hax_gpa2hva(uint64_t gpa, bool *found)
{
    int i = 0;
    *found = false;

    for (; i < HAX_MAX_SLOTS; i++) {
        hax_slot *hslot;
        hslot = &hax_global.memslots[i];
        // TODO (hshan):
        // Skip slots that are not part of hax
        if (hslot->flags & HAX_RAM_INFO_INVALID) continue;
        if (gpa >= hslot->start &&
            gpa < hslot->start + hslot->size) {
            *found = true;
            return (void *)((char *)(hslot->hva) + (gpa - hslot->start));
        }
    }

    return NULL;
}

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

int hax_hva2gpa(void *hva, uint64_t length, int array_size,
                uint64_t *gpa, uint64_t *size)
{
    int count = 0, i = 0;

    for (; i < HAX_MAX_SLOTS; i++) {
        hax_slot *hslot;
        uintptr_t hva_start_num;
        uintptr_t hva_num;
        hslot = &hax_global.memslots[i];
        // TODO (hshan):
        // Skip slots that are not part of hax
        if (hslot->flags & HAX_RAM_INFO_INVALID) continue;

        hva_start_num = (uintptr_t)hslot->hva;
        hva_num = (uintptr_t)hva;

        // Start of this hva region is in this slot.
        if (hva_num >= hva_start_num &&
            hva_num < hva_start_num + hslot->size) {
            if (count < array_size) {
                gpa[count] = hslot->start + (hva_num - hva_start_num);
                size[count] = min(length,
                                  hslot->size - (hva_num - hva_start_num));
            }
            count++;
        // End of this hva region is in this slot.
        // Its start is outside of this slot.
        } else if (hva_num + length <= hva_start_num + hslot->size &&
                   hva_num + length > hva_start_num) {
            if (count < array_size) {
                gpa[count] = hslot->start;
                size[count] = hva_num + length - hva_start_num;
            }
            count++;
        // This slot belongs to this hva region completely.
        } else if (hva_num + length > hva_start_num +  hslot->size &&
                   hva_num < hva_start_num)  {
            if (count < array_size) {
                gpa[count] = hslot->start;
                size[count] = hslot->size;
            }
            count++;
        }
    }
    return count;
}

static bool is_compatible_mapping(hax_slot *slot,
        uint64_t start_gpa, uint64_t size,
        int flags, void *hva) {
    return slot->flags == flags &&
           ((uintptr_t)slot->hva - (uintptr_t)hva == slot->start - start_gpa);
}

void hax_set_phys_mem_general(void* hva, uint64_t start, uint64_t size, bool add, int flags)
{
#ifdef HAX_SLOT_DEBUG
    fprintf(stderr, "%s begin=======================================================\n", __func__);
#endif

    hax_slot *mem;
    uint64_t end;
    int i;
    int free_slots[HAX_MAX_SLOTS];
    int overlap_slots[HAX_MAX_SLOTS];
    int free_slot_count;
    int overlap_slot_count;
    int split_slot_id;
    int split_needed_free_slots;

    end = start + size;

    memset(free_slots, 0, HAX_MAX_SLOTS * sizeof(int));
    memset(overlap_slots, 0, HAX_MAX_SLOTS * sizeof(int));
    free_slot_count = 0;
    overlap_slot_count = 0;
    split_slot_id = -1;
    split_needed_free_slots = 0;

#ifdef HAX_SLOT_DEBUG

#define HAX_SET_PHYS_MEM_DUMP(fmt,...) do {\
    fprintf(stderr, "%s: region: [0x%llx 0x%llx] flags 0x%x for %s\n", __func__, (unsigned long long)start, (unsigned long long)end, flags, add ? "ADD" : "REMOVE"); \
    fprintf(stderr, "%s: msg: " fmt "\n", __func__, ##__VA_ARGS__); \
    fprintf(stderr, "%s: current slot data:\n", __func__); \
    hax_dump_memslots(); \
    fprintf(stderr, "%s: free slots (%d): ", __func__, free_slot_count); for (i = 0; i < HAX_MAX_SLOTS; ++i) { if (free_slots[i]) fprintf(stderr, "%d ", i); } fprintf(stderr, "\n"); \
    fprintf(stderr, "%s: overlap slots (%d): ", __func__, overlap_slot_count); for (i = 0; i < HAX_MAX_SLOTS; ++i) { if (overlap_slots[i]) fprintf(stderr, "%d ", i); } fprintf(stderr, "\n"); \
    fprintf(stderr, "%s: split slot id: %d needed free for split: %d\n", __func__, split_slot_id, split_needed_free_slots); \
} while (0); \

#else

#define HAX_SET_PHYS_MEM_DUMP(fmt,...)

#endif

    HAX_SET_PHYS_MEM_DUMP("Begin")

    // Free slot finding phase:
    // Loop over all slots and record whether we can use them for new slots.
    for (i = 0; i < HAX_MAX_SLOTS; i++) {
        hax_slot *slot;
        slot = &hax_global.memslots[i];
        free_slots[i] = slot->flags & HAX_RAM_INFO_INVALID ? 1 : 0;
        free_slot_count += free_slots[i];
    }

    HAX_SET_PHYS_MEM_DUMP("Found free slots")

    // Topology phase:
    //
    // Loop over all overlapping slots, indicating which ones overlap with the
    // region and which one slot needs to get split.
    //
    // Split: It's also possible to create 1 new region if we are removing
    // slots + the slot to remove slot is properly contained in an existing
    // slot.
    //
    // It's possible to exit early here if we are adding a region and it
    // wouldn't change the set of regions at all.
    for (i = 0; i < HAX_MAX_SLOTS; i++) {
        int overlap;
        int will_split;
        uint64_t slot_start;
        uint64_t slot_end;
        hax_slot *slot;
        int split_condition;

        slot = &hax_global.memslots[i];
        overlap_slots[i] = 0;

        if (!slot->size || slot->flags & HAX_RAM_INFO_INVALID) continue;

        slot_start = slot->start;
        slot_end = slot->start + slot->size;

        overlap = start < slot_end && end > slot_start;

        overlap_slots[i] = overlap;

        if (!overlap) continue;

        // Is the overlap exact and we are adding a region
        // with the same flags? Then just exit out of this entire function.
        if (add &&
            start == slot_start &&
            end == slot_end &&
            flags == slot->flags &&
            hva == slot->hva) {
            HAX_SET_PHYS_MEM_DUMP("Coincident slot added, no-op.")
            goto end;
        }

        // Ok, we might have to split then.
        // First, even if the interval is "split" by the new region,
        // we might not have to do anything.
        // Split if:
        // 1. We are removing a region
        // 2. The region to be added has different flags
        // In case (2), we need 2 free slots.
        {
            int removing = !add;
            int mapping_changing = add && !is_compatible_mapping(slot, start, size, flags, hva);
            int should_split = removing || mapping_changing;

            will_split = should_split &&
                         start > slot_start && end < slot_end;

            if (will_split) {
                split_slot_id = i;
                split_needed_free_slots = mapping_changing ? 2 : 1;
            } else {
                split_slot_id = -1;
                split_needed_free_slots = 0;
            }

            // Record whether we are adding the slot to the middle of an
            // existing slot with the same flags (in other words, no-op)
            if (add && !mapping_changing) {
                HAX_SET_PHYS_MEM_DUMP("Subset of existing slot added, no-op.")
                goto end;
            }
        }

        ++overlap_slot_count;
    }

    HAX_SET_PHYS_MEM_DUMP("Found overlap slots")

    // Validation: check that if we have more than one overlap, we should not
    // need to split.
    if (overlap_slot_count > 1 &&
        split_slot_id != -1) {
        qemu_abort("%s: FATAL: Have more than one overlap slot, "
                   "but we are splitting a single slot.\n", __func__);
    }

    // Remove all overlapped regions
    for (i = 0; i < HAX_MAX_SLOTS; ++i) {
        hax_slot *slot;

        if (!overlap_slots[i]) continue;

        slot = &hax_global.memslots[i];

        if (hax_set_ram(slot->start, slot->size, 0, HAX_RAM_INFO_INVALID)) {
            qemu_abort("%s: Failed to reset overlapping slot\n", __func__);
        }
    }

    // Restore all partially overlapped regions, except the split one,
    // which is dealt with separately.
    for (i = 0; overlap_slot_count > 0 && i < HAX_MAX_SLOTS; ++i) {
        hax_slot *slot;
        uint64_t slot_start;
        uint64_t slot_end;
        uint64_t overlap_start;
        uint64_t overlap_end;
        uint64_t remaining_start;
        uint64_t remaining_end;

        if (!overlap_slots[i] || i == split_slot_id) continue;

        slot = &hax_global.memslots[i];

        slot_start = slot->start;
        slot_end = slot->start + slot->size;

        // This is not the split region, so we can use the following
        // calculation of overlap interval and remaining interval.
        overlap_start = start < slot_start ? slot_start : start;
        overlap_end = end > slot_end ? slot_end : end;

        // This region is totally overlapped; we may not want to restore it.
        if (overlap_start == slot_start && overlap_end == slot_end) continue;

        // Restore the partially overlapped region.
        remaining_start = overlap_start > slot_start ? slot_start : overlap_end;
        remaining_end = overlap_end < slot_end ? slot_end : overlap_start;

        if (remaining_start >= remaining_end) {
            qemu_abort("%s: FATAL: Invalid slot to restore\n", __func__);
        }

        if (hax_set_ram(
                remaining_start,
                remaining_end - remaining_start,
                (uint64_t)(uintptr_t)
                (slot->hva + (uintptr_t)(remaining_start - slot_start)),
                slot->flags)) {
            qemu_abort("%s: Failed to restore overlapping slot\n", __func__);
        }

        slot->start = remaining_start;
        slot->size = remaining_end - remaining_start;
        slot->hva = (slot->hva + (uintptr_t)(remaining_start - slot_start));

        // Remove from the overlapped set.
        overlap_slots[i] = false;
    }

    HAX_SET_PHYS_MEM_DUMP("Restored partially overlapped slots")

    // Now the only regions to deal with are completely contained in the region
    // that we want to add / remove, or the split region.
    // For the first group, set flags to invalid and add to free slots.
    for (i = 0; i < HAX_MAX_SLOTS; ++i) {
        hax_slot *slot;

        if (!overlap_slots[i] || i == split_slot_id) continue;

        slot = &hax_global.memslots[i];
        slot->hva = 0;
        slot->size = 0;
        slot->flags = HAX_RAM_INFO_INVALID;

        free_slots[i] = 1;
        ++free_slot_count;
    }

    HAX_SET_PHYS_MEM_DUMP("Updated new free slot info")

    // Validation: check that if we have to split, we have the free slots.
    if (split_needed_free_slots > free_slot_count) {
        qemu_abort("%s: FATAL: needed to split a slot, "
                   "but we have no free slots (%d needed).\n",
                   __func__,
                   split_needed_free_slots);
    }

    // Process the split region.
    if (split_slot_id != -1) {
        hax_slot *slot;
        hax_slot *right_slot;
        uint64_t slot_start;
        uint64_t slot_end;

        uint64_t split_left_start;
        uint64_t split_left_end;

        uint64_t split_right_start;
        uint64_t split_right_end;

        slot = &hax_global.memslots[split_slot_id];

        slot_start = slot->start;
        slot_end = slot->start + slot->size;

        split_left_start = slot_start;
        split_left_end = start;

        split_right_start = end;
        split_right_end = slot_end;

        int free_slot_id = -1;
        for (i = 0; i < HAX_MAX_SLOTS; ++i) {
            if (free_slots[i]) {
                free_slot_id = i;
                free_slots[i] = 0;
                --free_slot_count;
                break;
            }
        }

        if (free_slot_id == -1) {
            qemu_abort("%s: FATAL: splitting slots, "
                       "could not find a free slot.\n",
                       __func__);
        }

        right_slot = &hax_global.memslots[free_slot_id];

        right_slot->start = split_right_start;
        right_slot->size = split_right_end - split_right_start;
        right_slot->hva =
            slot->hva + (uintptr_t)(split_right_start - slot_start);
        right_slot->flags = slot->flags;

        if (hax_set_ram(right_slot->start,
                        right_slot->size,
                        (uint64_t)(uintptr_t)right_slot->hva,
                        right_slot->flags)) {
            qemu_abort("%s: FATAL: splitting slots, "
                       "could not allocate new slot\n",
                       __func__);
        }

        // Update left slot

        slot->size = split_left_end - split_left_start;

        if (hax_set_ram(slot->start,
                        slot->size,
                        (uint64_t)(uintptr_t)slot->hva,
                        slot->flags)) {
            qemu_abort("%s: FATAL: splitting slots, "
                       "could not allocate new slot\n",
                       __func__);
        }

        HAX_SET_PHYS_MEM_DUMP("Processed split slot")
    }

    // Now, if we are not adding a new region, we are done.
    if (!add) goto end;

    HAX_SET_PHYS_MEM_DUMP("Begin addition of new slot")

    // Now make a new slot.
    // Validation: Check that we have free slots.
    if (free_slot_count == 0) {
        qemu_abort("%s: FATAL: no free slots for adding new region\n",
                   __func__);
    }

    // Obtain the free slot.
    {
        int ret;
        hax_slot* slot = NULL;
        for (i = 0; i < HAX_MAX_SLOTS; ++i) {
            if (free_slots[i]) {
                slot = &hax_global.memslots[i];
                free_slots[i] = 0;
                --free_slot_count;
                break;
            }
        }

        // Validation: Check that our state isn't corrupted.
        if (!slot) {
            qemu_abort("%s: FATAL: free slot count "
                       "does not match free slot state\n",
                       __func__);
        }

        // Validation: Check that the slot itself is marked as free.
        if (!(slot->flags & HAX_RAM_INFO_INVALID)) {
            qemu_abort("%s: FATAL: free slot state "
                       "does not match slot flags\n",
                       __func__);
        }

        // Set parameters of the new slot, and also pass through to HAXM.
        slot->start = start;
        slot->size = end - start;
        slot->hva = hva;
        slot->flags = flags;
        ret = hax_set_ram(slot->start,
                          slot->size,
                          (uint64_t)(uintptr_t)slot->hva,
                          slot->flags);
        if (ret) {
            qemu_abort("%s: FATAL: final hax_set_ram() failed, ret=%d\n",
                       __func__, ret);
        }
        HAX_SET_PHYS_MEM_DUMP("After addition of new slot")
    }
end:

#ifdef HAX_SLOT_DEBUG
    fprintf(stderr, "%s end=========================================================\n", __func__);
#endif

    return;
}

void hax_set_phys_mem_from_memory_region(MemoryRegionSection *section, bool add)
{
#ifdef HAX_SLOT_DEBUG
    fprintf(stderr, "%s begin=======================================================\n", __func__);
#endif

    MemoryRegion *area = section->mr;
    uint64_t start, size, end;
    int flags;
    void *hva;

    start = section->offset_within_address_space;
    size = int128_get64(section->size);
    end = start + size;

    if (!memory_region_is_ram(area)) {
#ifdef HAX_SLOT_DEBUG
        fprintf(stderr, "%s: is not ram, skip [0x%llx 0x%llx] for %s\n",
                __func__,
                (unsigned long long)start,
                (unsigned long long)end, add ? "ADD" : "REMOVE");
#endif
        return;
    }

    if (memory_region_is_user_backed(area)) {
#ifdef HAX_SLOT_DEBUG
        fprintf(stderr, "%s: is user-backed ram, skip [0x%llx 0x%llx] for %s\n",
                __func__,
                (unsigned long long)start,
                (unsigned long long)end, add ? "ADD" : "REMOVE");
#endif
        return;
    }

    hva = memory_region_get_ram_ptr(area) + section->offset_within_region;

    if (memory_region_is_rom(area)) {
        flags = HAX_RAM_INFO_ROM;
    } else {
        flags = 0;
    }

    hax_set_phys_mem_general(hva, start, size, add, flags);
    return;
}

static void hax_region_add(MemoryListener * listener,
                           MemoryRegionSection * section)
{
    hax_set_phys_mem_from_memory_region(section, true);
}

static void hax_region_del(MemoryListener * listener,
                           MemoryRegionSection * section)
{
    hax_set_phys_mem_from_memory_region(section, false);
}

/* currently we fake the dirty bitmap sync, always dirty */
static void hax_log_sync(MemoryListener *listener,
                         MemoryRegionSection *section)
{
    MemoryRegion *mr = section->mr;

    if (!memory_region_is_ram(mr)) {
        /* Skip MMIO regions */
        return;
    }

    memory_region_set_dirty(mr, 0, int128_get64(section->size));
}

// User backed memory region API
static int
user_backed_flags_to_hax(int flags) {
    /* HAX_RAM_INFO_STANDALONE is ignored if not supported by HAXM */
    int hax_flags = HAX_RAM_INFO_STANDALONE;
    if (!(flags & USER_BACKED_RAM_FLAGS_READ)) {
        hax_flags |= HAX_RAM_INFO_ROM;
    }
    return hax_flags;
}

static void hax_user_backed_ram_map(uint64_t gpa, void* hva, uint64_t size, int flags) {
    if (!hax_global.supports_implicit_ramblock) {
        /* This code path is for older HAXM and does not work well. */
        int ret = hax_populate_ram((uint64_t)(uintptr_t)hva, size);
        if (ret < 0) {
            fprintf(stderr, "%s: Failed to add RAM block, ret=%d\n", __func__, ret);
            abort();
        }
    }
    hax_set_phys_mem_general(hva, gpa, size, true /* add */, user_backed_flags_to_hax(flags));
}

static void hax_user_backed_ram_unmap(uint64_t gpa, uint64_t size) {
    /* HAXM will take care of freeing the RAM block [hva, hva + size) */
    hax_set_phys_mem_general(0, gpa, size, false /* del */, HAX_RAM_INFO_INVALID);
}

static MemoryListener hax_memory_listener = {
    .region_add = hax_region_add,
    .region_del = hax_region_del,
    .log_sync = hax_log_sync,
    .priority = 10,
};

int hax_gpa_protect(uint64_t gpa, uint64_t size, uint64_t flags) {
    int res = hax_protect_ram(gpa, size, flags);
    return res;
}

static void hax_ram_block_added(RAMBlockNotifier *n, void *host, size_t size)
{
    /*
     * In HAX, QEMU allocates the virtual address, and HAX kernel
     * populates the memory with physical memory. Currently we have no
     * paging, so user should make sure enough free memory in advance.
     */
    if (hax_populate_ram((uint64_t)(uintptr_t)host, size) < 0) {
        fprintf(stderr, "HAX failed to populate RAM");
        abort();
    }
}

static struct RAMBlockNotifier hax_ram_notifier = {
    .ram_block_added = hax_ram_block_added,
};

void hax_memory_init(void)
{
    ram_block_notifier_add(&hax_ram_notifier);
    memory_listener_register(&hax_memory_listener, &address_space_memory);
    qemu_set_user_backed_mapping_funcs(
        hax_user_backed_ram_map,
        hax_user_backed_ram_unmap);
}
