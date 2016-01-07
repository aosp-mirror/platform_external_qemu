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

#ifndef _HAX_SLOT_H
#define _HAX_SLOT_H

#include <inttypes.h>

/**
 * hax_slot_init_registry: initializes the registry of memory slots.
 *
 * Should be called during HAX initialization, before any call to
 * hax_slot_register().
 */
void hax_slot_init_registry(void);

/**
 * hax_slot_free_registry: destroys the registry of memory slots.
 *
 * Should be called during HAX cleanup to free up resources used by the
 * registry of memory slots.
 */
void hax_slot_free_registry(void);

/**
 * hax_slot_register: registers a memory slot, updating HAX memory mappings if
 * necessary.
 *
 * Must be called after hax_slot_init_registry(). Can be called multiple times
 * to create new memory mappings or update existing ones. This function is smart
 * enough to avoid asking the HAXM driver to do the same mapping twice for any
 * guest physical page.
 *
 * Aborts QEMU on error.
 *
 * @start_pa: a guest physical address marking the start of the slot to
 *            register; must be page-aligned
 * @size: size of the slot to register; must be page-aligned and positive
 * @host_va: a host virtual address to which @start_pa should be mapped
 * @flags: parameters for the mapping, passed verbatim to the HAXM driver if
 *         necessary; must be non-negative
 */
void hax_slot_register(uint64_t start_pa, uint32_t size, uint64_t host_va,
                       int flags);

#endif
