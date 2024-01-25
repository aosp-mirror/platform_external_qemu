/* Copyright (C) 2016 The Android Open Source Project
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
#ifndef _HW_GOLDFISH_ADDRESS_SPACE_H
#define _HW_GOLDFISH_ADDRESS_SPACE_H

#include "qemu/typedefs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct GoldfishAddressSpaceOpsTag {
    int (*load)(QEMUFile *file);
    int (*save)(QEMUFile *file);
} GoldfishAddressSpaceOps;

extern void
goldfish_address_space_set_service_ops(const GoldfishAddressSpaceOps* ops);

/* Called by the host to reserve a shared region. Guest users can then
 * suballocate into this region. This saves us a lot of KVM slots.
 * Returns the relative offset to the starting phys addr in |offset|
 * and returns 0 if successful, -errno otherwise. */
extern int
goldfish_address_space_alloc_shared_host_region(
    uint64_t page_aligned_size, uint64_t* offset);

/* Called by the host to free a shared region. Only useful on teardown
 * or when loading a snapshot while the emulator is running.
 * Returns 0 if successful, -errno otherwise. */
extern int
goldfish_address_space_free_shared_host_region(uint64_t offset);

/* Versions of the above but when the state is already locked. */
extern int
goldfish_address_space_alloc_shared_host_region_locked(
    uint64_t page_aligned_size, uint64_t* offset);
extern int
goldfish_address_space_free_shared_host_region_locked(uint64_t offset);

/* Called by the host to get the starting physical address,
 * which is determined by the kernel. */
extern uint64_t
goldfish_address_space_get_phys_addr_start(void);
extern uint64_t
goldfish_address_space_get_phys_addr_start_locked(void);
extern uint32_t
goldfish_address_space_get_guest_page_size(void);

/* Version of alloc for a fixed offset */
extern int goldfish_address_space_alloc_shared_host_region_fixed_locked(uint64_t page_aligned_size, uint64_t offset);

#endif /* _HW_GOLDFISH_ADDRESS_SPACE_H */
