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
 * - A single window from PCIe to the Memory controller
 * - 4 windows from the BMC to PCIe.
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
#include "hw/pci/pcie_port.h"
#include "qom/object.h"

/* PCIe Root Complex Registers */
#define NPCM_PCIE_LINK_CTRL             0x90
#define NPCM_PCIERC_RCCFGNUM            0x140 /* Configuration Number */
#define     NPCM_PCIE_RCCFGNUM_BUS(a)   (((a) >> 8) & 0xFF)
#define     NPCM_PCIE_RCCFGNUM_DEVFN(a) ((a) & 0xFF)
#define NPCM_PCIERC_INTEN               0x180 /* Interrupt Enable */
#define NPCM_PCIERC_INTST               0x184 /* Interrupt Status */
#define NPCM_PCIERC_IMSI_ADDR           0x190
#define NPCM_PCIERC_MSISTAT             0x194 /* MSI Status Register */
#define NPCM_PCIERC_AXI_ERROR_REPORT    0x3E0

/* PCIe-to-AXI Window 0 and 1 Registers */
/* Source address low */
#define NPCM_PCIERC_PAnSAL(n)          (0x600 + (0x100 * (n)))
/* Source address high */
#define NPCM_PCIERC_PAnSAH(n)          (0x604 + (0x100 * (n)))
/* Translation address low */
#define NPCM_PCIERC_PAnTAL(n)          (0x608 + (0x100 * (n)))
/* Translation address high */
#define NPCM_PCIERC_PAnTAH(n)          (0x60C + (0x100 * (n)))
/* Translation parameters */
#define NPCM_PCIERC_PAnTP(n)           (0x610 + (0x100 * (n)))
/* Get window number from address */
#define NPCM_PCIERC_PA_WINDOW(addr)    (((addr) - 0x600) / 0x100)
#define NPCM_PCIERC_PA_OFFSET_MASK      0xff

/* AXI-to-PCIe Window 1 to 5 Registers, n in range [0,4] */
/* Source address low */
#define NPCM_PCIERC_APnSAL(n)          (0x820 + (0x20 * (n)))
/* Source address high */
#define NPCM_PCIERC_APnSAH(n)          (0x824 + (0x20 * (n)))
/* Translation address low */
#define NPCM_PCIERC_APnTAL(n)          (0x828 + (0x20 * (n)))
/* Translation address high */
#define NPCM_PCIERC_APnTAH(n)          (0x82C + (0x20 * (n)))
/* Translation parameters */
#define NPCM_PCIERC_APnTP(n)           (0x830 + (0x20 * (n)))
/* Get window number from address */
#define NPCM_PCIERC_AP_WINDOW(addr)    (((addr) - 0x820) / 0x20)
#define NPCM_PCIERC_AP_OFFSET_MASK      0x1f

/* Translation window parameters */
#define NPCM_PCIERC_TRSL_ID(p)              ((p) & 0x7)
#define     NPCM_PCIERC_TRSL_ID_TX_RX       0
#define     NPCM_PCIERC_TRSL_ID_CONFIG      1
#define NPCM_PCIERC_TRSF_PARAM(p)           (((p) >> 16) & 0xFFF)
#define     NPCM_PCIERC_TRSF_PARAM_MEMORY   0
#define     NPCM_PCIERC_TRSF_PARAM_CONFIG   1
#define     NPCM_PCIERC_TRSF_PARAM_IO       2

#define NPCM_PCIERC_SAL_OFFSET              0x0
#define     NPCM_PCIERC_SAL_EN              1
#define     NPCM_PCIERC_SAL_SIZE(addr)     (2ull << (((addr) >> 1) & 0x1F))
#define NPCM_PCIERC_SAH_OFFSET              0x4
#define NPCM_PCIERC_TAL_OFFSET              0x8
#define NPCM_PCIERC_TAH_OFFSET              0xC
#define NPCM_PCIERC_PARAM_OFFSET            0x10

#define NPCM_PCIERC_NUM_PA_WINDOWS          2
#define NPCM_PCIERC_NUM_AP_WINDOWS          5

#define NPCM_PCIERC_MSI_NR                  32
#define NPCM_PCIERC_MSI_OFFSET              0x50
/* PCIe extended config space offsets */
#define NPCM_PCIE_HEADER_OFFSET             0x80
#define NPCM_PCIE_AER_OFFSET                0x100

#define TYPE_NPCM_PCIERC "npcm-pcie-root-complex"
OBJECT_DECLARE_SIMPLE_TYPE(NPCMPCIERCState, NPCM_PCIERC)

#define NPCM_PCIE_HOLE       (0xe8000000)
#define NPCM_PCIE_HOLE_END   (0xe8000000 + (128 * MiB))

typedef enum {
    AXI2PCIE = 1,
    PCIE2AXI
} NPCMPCIEWindowType;

/* Nuvoton PCIe translation Window */
typedef struct NPCMPCIEWindow {
    uint32_t sal;            /* source address low */
    uint32_t sah;            /* source address high */
    uint32_t tal;            /* translation address low */
    uint32_t tah;            /* translation address high */
    uint32_t params;         /* translation window parameters */

    MemoryRegion mem;        /* QEMU memory subregion per window */
    NPCMPCIEWindowType type; /* translation direction */
    uint8_t set_fields;
    uint8_t id;
} NPCMPCIEWindow;

#define TYPE_NPCM_PCIE_ROOT_PORT "npcm-pcie-root-port"
OBJECT_DECLARE_SIMPLE_TYPE(NPCMPCIERootPort, NPCM_PCIE_ROOT_PORT)

struct NPCMPCIERootPort {
    PCIESlot parent;
};

struct NPCMPCIERCState {
    PCIExpressHost parent;

    qemu_irq irq;

    /* PCIe RC registers */
    MemoryRegion rc_regs;
    uint32_t rccfgnum;
    uint32_t rcinten;
    uint32_t rcintstat;
    uint32_t rcimsiaddr;
    uint32_t rcmsisstat;
    uint32_t axierr;

    /* Address translation state */
    AddressSpace pcie_space;
    MemoryRegion pcie_memory;
    MemoryRegion pcie_io; /* unused - but required for IO space PCI */
    MemoryRegion rp_config;
    NPCMPCIERootPort port;
    /* PCIe to AXI Windows */
    NPCMPCIEWindow pcie2axi[NPCM_PCIERC_NUM_PA_WINDOWS];

    /* AXI to PCIe Windows */
    NPCMPCIEWindow axi2pcie[NPCM_PCIERC_NUM_AP_WINDOWS];
};

#endif /* NPCM_PCIERC_H */
