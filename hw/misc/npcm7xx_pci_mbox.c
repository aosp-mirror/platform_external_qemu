/*
 * Nuvoton NPCM7xx PCI Mailbox Module
 *
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/qdev-clock.h"
#include "hw/qdev-properties-system.h"
#include "hw/misc/npcm7xx_pci_mbox.h"
#include "hw/registerfields.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qemu/bitops.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "qemu/units.h"
#include "trace.h"

REG32(NPCM7XX_PCI_MBOX_BMBXSTAT, 0x00);
REG32(NPCM7XX_PCI_MBOX_BMBXCTL, 0x04);
REG32(NPCM7XX_PCI_MBOX_BMBXCMD, 0x08);


#define NPCM7XX_PCI_MBOX_NR_CI 8
#define NPCM7XX_PCI_MBOX_CI_MASK MAKE_64BIT_MASK(0, NPCM7XX_PCI_MBOX_NR_CI)

static void npcm7xx_pci_mbox_update_irq(NPCM7xxPCIMBoxState *s)
{
    /* We should send an interrupt when one of the CIE and CIF are both 1. */
    if (s->regs[R_NPCM7XX_PCI_MBOX_BMBXSTAT] &
        s->regs[R_NPCM7XX_PCI_MBOX_BMBXCTL] &
        NPCM7XX_PCI_MBOX_CI_MASK) {
        qemu_irq_raise(s->irq);
        trace_npcm7xx_pci_mbox_irq(1);
    } else {
        qemu_irq_lower(s->irq);
        trace_npcm7xx_pci_mbox_irq(0);
    }
}

static uint64_t npcm7xx_pci_mbox_read(void *opaque, hwaddr offset, unsigned size)
{
    NPCM7xxPCIMBoxState *s = NPCM7XX_PCI_MBOX(opaque);
    uint16_t value = 0;

    if (offset / sizeof(uint32_t) >= NPCM7XX_PCI_MBOX_NR_REGS) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: offset 0x%04" HWADDR_PRIx " out of range\n",
                      __func__, offset);
        return 0;
    }

    value = s->regs[offset / sizeof(uint32_t)];
    trace_npcm7xx_pci_mbox_read(DEVICE(s)->canonical_path, offset, value, size);
    return value;
}

static void npcm7xx_pci_mbox_write(void *opaque, hwaddr offset,
                              uint64_t v, unsigned size)
{
    NPCM7xxPCIMBoxState *s = NPCM7XX_PCI_MBOX(opaque);

    trace_npcm7xx_pci_mbox_write(DEVICE(s)->canonical_path, offset, v, size);
    switch (offset) {
    case A_NPCM7XX_PCI_MBOX_BMBXSTAT:
        /* Clear bits that are 1. */
        s->regs[R_NPCM7XX_PCI_MBOX_BMBXSTAT] &= ~v;
        break;

    case A_NPCM7XX_PCI_MBOX_BMBXCTL:
        s->regs[R_NPCM7XX_PCI_MBOX_BMBXCTL] = v;
        break;

    case A_NPCM7XX_PCI_MBOX_BMBXCMD:
        /* Set the bits that are 1. */
        s->regs[R_NPCM7XX_PCI_MBOX_BMBXCMD] |= v;
        /* TODO: Set interrupt to host. */
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: offset 0x%04" HWADDR_PRIx " out of range\n",
                      __func__, offset);
    }
    npcm7xx_pci_mbox_update_irq(s);
}

static const struct MemoryRegionOps npcm7xx_pci_mbox_ops = {
    .read       = npcm7xx_pci_mbox_read,
    .write      = npcm7xx_pci_mbox_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid      = {
        .min_access_size        = 4,
        .max_access_size        = 4,
        .unaligned              = false,
    },
};

static void npcm7xx_pci_mbox_enter_reset(Object *obj, ResetType type)
{
    NPCM7xxPCIMBoxState *s = NPCM7XX_PCI_MBOX(obj);

    memset(s->regs, 0, 4 * NPCM7XX_PCI_MBOX_NR_REGS);
}

static void npcm7xx_pci_mbox_hold_reset(Object *obj)
{
    NPCM7xxPCIMBoxState *s = NPCM7XX_PCI_MBOX(obj);

    qemu_irq_lower(s->irq);
}

static void npcm7xx_pci_mbox_init(Object *obj)
{
    NPCM7xxPCIMBoxState *s = NPCM7XX_PCI_MBOX(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_ram_device_ptr(&s->ram, obj, "pci-mbox-ram",
                                      NPCM7XX_PCI_MBOX_RAM_SIZE, s->content);
    memory_region_init_io(&s->iomem, obj, &npcm7xx_pci_mbox_ops, s,
                          "pci-mbox-iomem", 4 * KiB);
    sysbus_init_mmio(sbd, &s->ram);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
}

static const VMStateDescription vmstate_npcm7xx_pci_mbox = {
    .name = "npcm7xx-pci-mbox-module",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, NPCM7xxPCIMBoxState,
                             NPCM7XX_PCI_MBOX_NR_REGS),
        VMSTATE_END_OF_LIST(),
    },
};

static void npcm7xx_pci_mbox_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "NPCM7xx PCI Mailbox Controller";
    dc->vmsd = &vmstate_npcm7xx_pci_mbox;
    rc->phases.enter = npcm7xx_pci_mbox_enter_reset;
    rc->phases.hold = npcm7xx_pci_mbox_hold_reset;
}

static const TypeInfo npcm7xx_pci_mbox_info = {
    .name               = TYPE_NPCM7XX_PCI_MBOX,
    .parent             = TYPE_SYS_BUS_DEVICE,
    .instance_size      = sizeof(NPCM7xxPCIMBoxState),
    .class_init         = npcm7xx_pci_mbox_class_init,
    .instance_init      = npcm7xx_pci_mbox_init,
};

static void npcm7xx_pci_mbox_register_type(void)
{
    type_register_static(&npcm7xx_pci_mbox_info);
}
type_init(npcm7xx_pci_mbox_register_type);
