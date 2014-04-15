/*
 * Declarations for cpu physical memory functions
 *
 * Copyright 2011 Red Hat, Inc. and/or its affiliates
 *
 * Authors:
 *  Avi Kivity <avi@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or
 * later.  See the COPYING file in the top-level directory.
 *
 */

/*
 * This header is for use by exec.c and memory.c ONLY.  Do not include it.
 * The functions declared here will be removed soon.
 */

#ifndef RAM_ADDR_H
#define RAM_ADDR_H

#ifndef CONFIG_USER_ONLY
#include "hw/xen/xen.h"

ram_addr_t qemu_ram_alloc_from_ptr(DeviceState *dev, const char *name,
                                   ram_addr_t size, void *host);
ram_addr_t qemu_ram_alloc(DeviceState *dev, const char *name, ram_addr_t size);
/* This should only be used for ram local to a device.  */
void *qemu_get_ram_ptr(ram_addr_t addr);
/* This should only be used for ram local to a device.  */
/* Same but slower, to use for migration, where the order of
 * RAMBlocks must not change. */
void *qemu_safe_ram_ptr(ram_addr_t addr);
/* This should not be used by devices.  */
int qemu_ram_addr_from_host(void *ptr, ram_addr_t *ram_addr);
void qemu_ram_free(ram_addr_t addr);
void qemu_ram_remap(ram_addr_t addr, ram_addr_t length);
ram_addr_t qemu_ram_addr_from_host_nofail(void *ptr);

static inline int cpu_physical_memory_get_dirty(ram_addr_t addr,
                                                int dirty_flags)
{
    return ram_list.phys_dirty[addr >> TARGET_PAGE_BITS] & dirty_flags;
}

static inline int cpu_physical_memory_get_dirty_flags(ram_addr_t addr)
{
    return ram_list.phys_dirty[addr >> TARGET_PAGE_BITS];
}

/* read dirty bit (return 0 or 1) */
static inline int cpu_physical_memory_is_dirty(ram_addr_t addr)
{
    return ram_list.phys_dirty[addr >> TARGET_PAGE_BITS] == 0xff;
}

static inline void cpu_physical_memory_set_dirty(ram_addr_t addr)
{
    ram_list.phys_dirty[addr >> TARGET_PAGE_BITS] = 0xff;
}

static inline int cpu_physical_memory_set_dirty_flags(ram_addr_t addr,
                                                      int dirty_flags)
{
    return ram_list.phys_dirty[addr >> TARGET_PAGE_BITS] |= dirty_flags;
}

static inline void cpu_physical_memory_mask_dirty_range(ram_addr_t start,
                                                        int length,
                                                        int dirty_flags)
{
    int i, mask, len;
    uint8_t *p;

    len = length >> TARGET_PAGE_BITS;
    mask = ~dirty_flags;
    p = ram_list.phys_dirty + (start >> TARGET_PAGE_BITS);
    for (i = 0; i < len; i++) {
        p[i] &= mask;
    }
}

int cpu_physical_memory_set_dirty_tracking(int enable);

int cpu_physical_memory_get_dirty_tracking(void);

int cpu_physical_sync_dirty_bitmap(hwaddr start_addr,
                                   hwaddr end_addr);

void cpu_physical_memory_reset_dirty(ram_addr_t start, ram_addr_t end,
                                     int dirty_flags);

#endif
#endif