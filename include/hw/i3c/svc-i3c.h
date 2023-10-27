/*
 * Silvaco I3C Controller
 *
 * Copyright (C) 2023 Google, LLC
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#ifndef SVC_I3C_H
#define SVC_I3C_H

#include "hw/i3c/i3c.h"
#include "hw/sysbus.h"
#include "hw/irq.h"

#define TYPE_SVC_I3C "svc.i3c"
OBJECT_DECLARE_SIMPLE_TYPE(SVCI3C, SVC_I3C)

#define SVC_I3C_NR_REGS (0x100 >> 2)

typedef struct SVCI3C {
    SysBusDevice parent;

    MemoryRegion mr;
    I3CBus *bus;

    struct {
        uint8_t id;
    } cfg;

    uint32_t regs[SVC_I3C_NR_REGS];
    qemu_irq irq;
} SVCI3C;

#endif /* SVC_I3C_H */
