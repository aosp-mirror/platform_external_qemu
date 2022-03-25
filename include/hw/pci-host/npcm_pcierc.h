/*
 * Nuvoton PCIe Root complex
 *
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * The PCIERC configuration registers must be initialised by the BMC kernel
 * during boot for PCIe to function
 * - A single window from the PCIe to the Memory controller
 * - 4 windows from the BMC to the PCIe.
 *     1 of these five BMC-to-PCIe windows must be allocated for configuration
 *     transactions, the rest can be used for I/0 or memory transactions
 * - All BMC-to-PCIe windows are mapped to address range
 *   0xe800_0000 to 0xefff_ffff (128MB)
 */

#ifndef NPCM_PCIERC_H
#define NPCM_PCIERC_H

#include "hw/sysbus.h"
#include "hw/pci/pci.h"
#include "hw/pci/pcie_host.h"
#include "qom/object.h"

/* PCIe Root Complex Registers */
#define LINKSTAT                        0x92
#define NPCM_PCIERC_RCCFGNUM            0x140 /* Configuration Number */
#define NPCM_PCIERC_INTEN               0x180 /* Interrupt Enable */
#define NPCM_PCIERC_INTST               0x184 /* Interrupt Status */
#define NPCM_PCIERC_IMSI_ADDR           0x190
#define NPCM_PCIERC_MSISTAT             0x194 /* MSI Status Register */
#define NPCM_PCIERC_AXI_ERROR_REPORT    0x3E0

#define TYPE_NPCM_PCIERC "npcm-pcie-root-complex"
OBJECT_DECLARE_SIMPLE_TYPE(NPCMPCIERCState, NPCM_PCIERC)

struct NPCMPCIERCState {
    PCIExpressHost parent;

    qemu_irq irq;

    /* PCIe RC registers */
    MemoryRegion mmio;
    uint32_t rccfgnum;
    uint32_t rcinten;
    uint32_t rcintstat;
    uint32_t rcimsiaddr;
    uint32_t rcmsisstat;
    uint32_t axierr;
};

#endif /* NPCM_PCIERC_H */
