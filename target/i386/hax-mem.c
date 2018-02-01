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

#include "target/i386/hax-i386.h"
#include "qemu/queue.h"
#include "qemu/abort.h"

#define DEBUG_HAX_MEM 0

#define DPRINTF(fmt, ...) \
    do { \
        if (DEBUG_HAX_MEM) { \
            fprintf(stdout, fmt, ## __VA_ARGS__); \
        } \
    } while (0)

// Memory slots/////////////////////////////////////////////////////////////////

static void hax_dump_memslot(void)
{
    hax_slot *slot;
    int x = 0;

    for (; x < HAX_SLOTS_COUNT; ++x) {
        slot = &hax_global.hax_slots[x];
        if (slot->flags & HAX_RAM_INFO_INVALID)
            continue;
        DPRINTF("Slot %d start %016llx end %016llx hva %p flags %x\n",
                x,
                slot->start,
                slot->size + slot->start,
                slot->hva,
                slot->flags);
    }
}

hax_slot *hax_find_overlap_slot(uint64_t start, uint64_t end) {
    hax_slot *slot;
    int x = 0;
    for (; x < HAX_SLOTS_COUNT; ++x) {
        slot = &hax_global.hax_slots[x];
        if (slot->size && start < (slot->start + slot->size) && end > slot->start)
            return slot;
    }
    return NULL;
}

#define ALIGN(x, y)  (((x)+(y)-1) & ~((y)-1))

void* hax_gpa2hva(uint64_t gpa, bool* found) {
    struct hax_slot *hslot;
    *found = false;
    int i = 0;

    for (; i < HAX_SLOTS_COUNT; i++) {
        hslot = &hax_global.hax_slots[i];
        if (!(hslot->flags | HAX_RAM_INFO_INVALID)) continue;
        if (gpa >= hslot->start &&
            gpa < hslot->start + hslot->size) {
            *found = true;
            return (void*)((char*)(hslot->hva) + (gpa - hslot->start));
        }
    }

    return 0;
}

int hax_hva2gpa(void* hva, uint64_t length, int array_size,
                uint64_t* gpa, uint64_t* size) {
    struct hax_slot *hslot;
    int count = 0, i = 0;

    for (; i < HAX_SLOTS_COUNT; i++) {
        hslot = &hax_global.hax_slots[i];
        if (!(hslot->flags | HAX_RAM_INFO_INVALID)) continue;

        uintptr_t hva_start_num = (uintptr_t)hslot->hva;
        uintptr_t hva_num = (uintptr_t)hva;
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

void hax_set_phys_mem(MemoryRegionSection* section, bool add) {
    hax_slot *mem;
    MemoryRegion *area = section->mr;
    uint64_t start, size;
    void *hva;

    if (!memory_region_is_ram(area)) return;

    start = section->offset_within_address_space;
    size = int128_get64(section->size);
    hva = memory_region_get_ram_ptr(area) + section->offset_within_region;

    mem = hax_find_overlap_slot(start, start + size);

    if (mem && add) {
        if (mem->size == size && mem->start == start && mem->hva == hva)
            return; // Same region was attempted to register, go away.
    }

    if (!(add || (mem->start <= start &&
                mem->start + mem->size >= start + size)))
        qemu_abort("%s: slot to be deleted not fully within existing slots\n",
                __func__);

    // Region needs to be reset. set the hva to 0 and remap it.
    if (mem) {
        if (hax_set_ram(start, size, 0, HAX_RAM_INFO_INVALID)) {
            qemu_abort("%s: Failed to reset overlapping slot\n", __func__);
        }
        if (mem->start == start && mem->size == size) {
            mem->start = mem->size = 0;
            mem->hva = 0;
            mem->flags = HAX_RAM_INFO_INVALID;
        } else if (mem->start == start) {
            g_assert(mem->size - size);
            mem->start += size;
            mem->hva += size;
            mem->size = mem->size - size;
        } else {
            uint64_t size2 = mem->start + mem->size - start - size;
            void *hva2 = mem->hva + mem->size - start - size;
            mem->size = start - mem->start;
            if (size2) {
                int x, flags = mem->flags;

                for (x = 0; x < HAX_SLOTS_COUNT; ++x) {
                    mem = &hax_global.hax_slots[x];
                    if (mem->flags & HAX_RAM_INFO_INVALID)
                        break;
                }
                if (x == HAX_SLOTS_COUNT) {
                    qemu_abort("%s: no free slots\n", __func__);
                }
                mem->start = start + size;
                mem->size = size2;
                mem->hva = hva2;
                mem->flags = flags;
            }
        }
#ifdef HAX_SLOT_DEBUG
        fprintf(stderr, "Dump Memslot After Removal\n");
        hax_dump_memslot();
#endif
    }

    if (!add) return;

    // Now make a new slot.
    int x, flags = 0;

    if (memory_region_is_rom(area))
        flags = HAX_RAM_INFO_ROM;

    for (x = 0; x < HAX_SLOTS_COUNT; ++x) {
        mem = &hax_global.hax_slots[x];
        if (mem->flags == flags &&
                mem->start + mem->size == start &&
                mem->hva + mem->size == hva)
            break;
    }

    if (x == HAX_SLOTS_COUNT)
        for (x = 0; x < HAX_SLOTS_COUNT; ++x) {
            mem = &hax_global.hax_slots[x];
            if (mem->flags & HAX_RAM_INFO_INVALID)
                break;
        }

    if (x == HAX_SLOTS_COUNT) {
        qemu_abort("%s: no free slots\n", __func__);
    }

    if (mem->flags & HAX_RAM_INFO_INVALID) {
        mem->start = start;
        mem->hva = hva;
    }
    mem->flags = flags;
    mem->size += size;

    if (hax_set_ram(start, size, (uint64_t)(size_t)hva, flags)) {
        qemu_abort("%s: error regsitering new memory slot\n", __func__);
    }
#ifdef HAX_SLOT_DEBUG
    fprintf(stderr, "Dump Memslot After Insertion\n");
    hax_dump_memslot();
#endif
}

static void hax_region_add(MemoryListener * listener,
                           MemoryRegionSection * section)
{
    hax_set_phys_mem(section, true);
}

static void hax_region_del(MemoryListener * listener,
                           MemoryRegionSection * section)
{
    hax_set_phys_mem(section, false);
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

static MemoryListener hax_memory_listener = {
    .region_add = hax_region_add,
    .region_del = hax_region_del,
    .log_sync = hax_log_sync,
    .priority = 10,
};

static MemoryListener hax_io_listener = {
    .priority = 10,
};

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
}
