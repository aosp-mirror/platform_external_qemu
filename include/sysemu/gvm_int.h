/*
 * Internal definitions for a target's GVM support
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef QEMU_GVM_INT_H
#define QEMU_GVM_INT_H

#include "sysemu/sysemu.h"
#include "sysemu/accel.h"
#include "sysemu/gvm.h"

typedef struct GVMSlot
{
    hwaddr start_addr;
    ram_addr_t memory_size;
    void *ram;
    int slot;
    int flags;
} GVMSlot;

typedef struct GVMMemoryListener {
    MemoryListener listener;
    GVMSlot *slots;
    int as_id;
} GVMMemoryListener;

#define TYPE_GVM_ACCEL ACCEL_CLASS_NAME("gvm")

#define GVM_STATE(obj) \
    OBJECT_CHECK(GVMState, (obj), TYPE_GVM_ACCEL)

void gvm_memory_listener_register(GVMState *s, GVMMemoryListener *kml,
                                  AddressSpace *as, int as_id);

#endif
