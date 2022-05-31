/*
 * Nuvoton NPCM7xx/8xx GMAC Module
 *
 * Copyright 2022 Google LLC
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

#ifndef NPCM_GMAC_H
#define NPCM_GMAC_H

#include "hw/irq.h"
#include "hw/sysbus.h"
#include "hw/net/npcm_pcs.h"
#include "net/net.h"

#define NPCM_GMAC_NR_REGS (0x1060 / sizeof(uint32_t))

#define NPCM_GMAC_MAX_PHYS 32
#define NPCM_GMAC_MAX_PHY_REGS 32

typedef struct NPCMGMACState {
    SysBusDevice parent;

    MemoryRegion iomem;
    qemu_irq irq;

    NICState *nic;
    NICConf conf;

    NPCMPCSState *pcs;
    uint32_t regs[NPCM_GMAC_NR_REGS];
    uint32_t phy_regs[NPCM_GMAC_MAX_PHYS][NPCM_GMAC_MAX_PHY_REGS];
} NPCMGMACState;

#define TYPE_NPCM_GMAC "npcm-gmac"
OBJECT_DECLARE_SIMPLE_TYPE(NPCMGMACState, NPCM_GMAC)

#endif /* NPCM_GMAC_H */
