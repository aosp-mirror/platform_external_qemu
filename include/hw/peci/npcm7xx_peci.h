/*
 * Nuvoton NPCM7xx PECI Module
 *
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef NPCM7XX_PECI_H
#define NPCM7XX_PECI_H

#include "exec/memory.h"
#include "hw/irq.h"
#include "hw/peci/peci.h"
#include "hw/sysbus.h"

typedef struct NPCM7xxPECIState {
    SysBusDevice parent;

    MemoryRegion iomem;

    PECIBus      *bus;
    qemu_irq      irq;

    PECICmd       pcmd;

    /* Registers */
    uint8_t       status;
    uint8_t       ctl2;
    uint8_t       pddr;
} NPCM7xxPECIState;

#define TYPE_NPCM7XX_PECI "npcm7xx-peci"
#define NPCM7XX_PECI(obj)                                               \
    OBJECT_CHECK(NPCM7xxPECIState, (obj), TYPE_NPCM7XX_PECI)

#endif /* NPCM7XX_PECI_H */
