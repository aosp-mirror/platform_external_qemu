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
#ifndef NPCM7XX_PCI_MBOX_H
#define NPCM7XX_PCI_MBOX_H

#include "chardev/char-fe.h"
#include "exec/memory.h"
#include "hw/clock.h"
#include "hw/irq.h"
#include "hw/pci/pci.h"
#include "hw/sysbus.h"
#include "qom/object.h"

#define NPCM7XX_PCI_MBOX_RAM_SIZE 0x4000

#define NPCM7XX_PCI_VENDOR_ID   0x1050
#define NPCM7XX_PCI_DEVICE_ID   0x0750
#define NPCM7XX_PCI_REVISION    0
#define NPCM7XX_PCI_CLASS_CODE  0xff

typedef enum NPCM7xxPCIMBoxHostState {
    NPCM7XX_PCI_MBOX_STATE_IDLE,
    NPCM7XX_PCI_MBOX_STATE_OFFSET,
    NPCM7XX_PCI_MBOX_STATE_SIZE,
    NPCM7XX_PCI_MBOX_STATE_DATA,
} NPCM7xxPCIMBoxHostState ;

/*
 * Maximum amount of control registers in PCI Mailbox module. Do not increase
 * this value without bumping vm version.
 */
#define NPCM7XX_PCI_MBOX_NR_REGS 3

/**
 * struct NPCM7xxPciMboxState - PCI Mailbox Device
 * @parent: System bus device.
 * @ram: the mailbox RAM memory space
 * @iomem: Memory region through which registers are accessed.
 * @content: The content of the PCI mailbox, initialized to 0.
 * @regs: The MMIO registers.
 * @chr: The chardev backend used to communicate with core CPU.
 * @offset: The offset to start transfer.
 */
typedef struct NPCM7xxPCIMBoxState {
    SysBusDevice parent;

    MemoryRegion ram;
    MemoryRegion iomem;

    qemu_irq irq;
    uint8_t content[NPCM7XX_PCI_MBOX_RAM_SIZE];
    uint32_t regs[NPCM7XX_PCI_MBOX_NR_REGS];
    CharBackend chr;

    /* aux data for receiving host commands. */
    NPCM7xxPCIMBoxHostState state;
    uint8_t op;
    hwaddr offset;
    uint8_t size;
    uint64_t data;
    int receive_count;
} NPCM7xxPCIMBoxState;

#define TYPE_NPCM7XX_PCI_MBOX "npcm7xx-pci-mbox"
#define NPCM7XX_PCI_MBOX(obj) \
    OBJECT_CHECK(NPCM7xxPCIMBoxState, (obj), TYPE_NPCM7XX_PCI_MBOX)

#endif /* NPCM7XX_PCI_MBOX_H */
