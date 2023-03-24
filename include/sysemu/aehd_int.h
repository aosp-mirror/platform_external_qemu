/*
 * Internal definitions for a target's AEHD support
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef QEMU_AEHD_INT_H
#define QEMU_AEHD_INT_H

#include "sysemu/sysemu.h"
#include "sysemu/accel.h"
#include "sysemu/aehd.h"

typedef struct AEHDSlot
{
    hwaddr start_addr;
    ram_addr_t memory_size;
    void *ram;
    int slot;
    int flags;
} AEHDSlot;

typedef struct AEHDMemoryListener {
    MemoryListener listener;
    AEHDSlot *slots;
    int as_id;
} AEHDMemoryListener;

#define TYPE_AEHD_ACCEL ACCEL_CLASS_NAME("aehd")

#define AEHD_STATE(obj) \
    OBJECT_CHECK(AEHDState, (obj), TYPE_AEHD_ACCEL)

void aehd_memory_listener_register(AEHDState *s, AEHDMemoryListener *kml,
                                  AddressSpace *as, int as_id);

#endif
