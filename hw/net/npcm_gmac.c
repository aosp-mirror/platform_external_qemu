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
 *
 * Unsupported/unimplemented features:
 * - MII is not implemented, MII_ADDR.BUSY and MII_DATA always return zero
 * - Precision timestamp (PTP) is not implemented.
 */

#include "qemu/osdep.h"

#include "hw/registerfields.h"
#include "hw/net/mii.h"
#include "hw/net/npcm_gmac.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "sysemu/dma.h"
#include "trace.h"

REG32(NPCM_DMA_BUS_MODE, 0x1000)
REG32(NPCM_DMA_XMT_POLL_DEMAND, 0x1004)
REG32(NPCM_DMA_RCV_POLL_DEMAND, 0x1008)
REG32(NPCM_DMA_RX_BASE_ADDR, 0x100c)
REG32(NPCM_DMA_TX_BASE_ADDR, 0x1010)
REG32(NPCM_DMA_STATUS, 0x1014)
REG32(NPCM_DMA_CONTROL, 0x1018)
REG32(NPCM_DMA_INTR_ENA, 0x101c)
REG32(NPCM_DMA_MISSED_FRAME_CTR, 0x1020)
REG32(NPCM_DMA_HOST_TX_DESC, 0x1048)
REG32(NPCM_DMA_HOST_RX_DESC, 0x104c)
REG32(NPCM_DMA_CUR_TX_BUF_ADDR, 0x1050)
REG32(NPCM_DMA_CUR_RX_BUF_ADDR, 0x1054)
REG32(NPCM_DMA_HW_FEATURE, 0x1058)

REG32(NPCM_GMAC_MAC_CONFIG, 0x0)
REG32(NPCM_GMAC_FRAME_FILTER, 0x4)
REG32(NPCM_GMAC_HASH_HIGH, 0x8)
REG32(NPCM_GMAC_HASH_LOW, 0xc)
REG32(NPCM_GMAC_MII_ADDR, 0x10)
REG32(NPCM_GMAC_MII_DATA, 0x14)
REG32(NPCM_GMAC_FLOW_CTRL, 0x18)
REG32(NPCM_GMAC_VLAN_FLAG, 0x1c)
REG32(NPCM_GMAC_VERSION, 0x20)
REG32(NPCM_GMAC_WAKEUP_FILTER, 0x28)
REG32(NPCM_GMAC_PMT, 0x2c)
REG32(NPCM_GMAC_LPI_CTRL, 0x30)
REG32(NPCM_GMAC_TIMER_CTRL, 0x34)
REG32(NPCM_GMAC_INT_STATUS, 0x38)
REG32(NPCM_GMAC_INT_MASK, 0x3c)
REG32(NPCM_GMAC_MAC0_ADDR_HI, 0x40)
REG32(NPCM_GMAC_MAC0_ADDR_LO, 0x44)
REG32(NPCM_GMAC_MAC1_ADDR_HI, 0x48)
REG32(NPCM_GMAC_MAC1_ADDR_LO, 0x4c)
REG32(NPCM_GMAC_MAC2_ADDR_HI, 0x50)
REG32(NPCM_GMAC_MAC2_ADDR_LO, 0x54)
REG32(NPCM_GMAC_MAC3_ADDR_HI, 0x58)
REG32(NPCM_GMAC_MAC3_ADDR_LO, 0x5c)
REG32(NPCM_GMAC_RGMII_STATUS, 0xd8)
REG32(NPCM_GMAC_WATCHDOG, 0xdc)
REG32(NPCM_GMAC_PTP_TCR, 0x700)
REG32(NPCM_GMAC_PTP_SSIR, 0x704)
REG32(NPCM_GMAC_PTP_STSR, 0x708)
REG32(NPCM_GMAC_PTP_STNSR, 0x70c)
REG32(NPCM_GMAC_PTP_STSUR, 0x710)
REG32(NPCM_GMAC_PTP_STNSUR, 0x714)
REG32(NPCM_GMAC_PTP_TAR, 0x718)
REG32(NPCM_GMAC_PTP_TTSR, 0x71c)

/* Register Fields */
#define NPCM_GMAC_MII_ADDR_BUSY             BIT(0)
#define NPCM_GMAC_MII_ADDR_WRITE            BIT(1)
#define NPCM_GMAC_MII_ADDR_GR(rv)           extract16((rv), 6, 5)
#define NPCM_GMAC_MII_ADDR_PA(rv)           extract16((rv), 11, 5)

#define NPCM_GMAC_INT_MASK_LPIIM            BIT(10)
#define NPCM_GMAC_INT_MASK_PMTM             BIT(3)
#define NPCM_GMAC_INT_MASK_RGIM             BIT(0)

#define NPCM_DMA_BUS_MODE_SWR               BIT(0)

static const uint32_t npcm_gmac_cold_reset_values[NPCM_GMAC_NR_REGS] = {
    /* Reduce version to 3.2 so that the kernel can enable interrupt. */
    [R_NPCM_GMAC_VERSION]         = 0x00001032,
    [R_NPCM_GMAC_TIMER_CTRL]      = 0x03e80000,
    [R_NPCM_GMAC_MAC0_ADDR_HI]    = 0x8000ffff,
    [R_NPCM_GMAC_MAC0_ADDR_LO]    = 0xffffffff,
    [R_NPCM_GMAC_MAC1_ADDR_HI]    = 0x0000ffff,
    [R_NPCM_GMAC_MAC1_ADDR_LO]    = 0xffffffff,
    [R_NPCM_GMAC_MAC2_ADDR_HI]    = 0x0000ffff,
    [R_NPCM_GMAC_MAC2_ADDR_LO]    = 0xffffffff,
    [R_NPCM_GMAC_MAC3_ADDR_HI]    = 0x0000ffff,
    [R_NPCM_GMAC_MAC3_ADDR_LO]    = 0xffffffff,
    [R_NPCM_GMAC_PTP_TCR]         = 0x00002000,
    [R_NPCM_DMA_BUS_MODE]         = 0x00020101,
    [R_NPCM_DMA_HW_FEATURE]       = 0x100d4f37,
};

static const uint16_t phy_reg_init[] = {
    [MII_BMCR]      = MII_BMCR_AUTOEN | MII_BMCR_FD | MII_BMCR_SPEED1000,
    [MII_BMSR]      = MII_BMSR_100TX_FD | MII_BMSR_100TX_HD | MII_BMSR_10T_FD |
                      MII_BMSR_10T_HD | MII_BMSR_EXTSTAT | MII_BMSR_AUTONEG |
                      MII_BMSR_LINK_ST | MII_BMSR_EXTCAP,
    [MII_PHYID1]    = 0x0362,
    [MII_PHYID2]    = 0x5e6a,
    [MII_ANAR]      = MII_ANAR_TXFD | MII_ANAR_TX | MII_ANAR_10FD |
                      MII_ANAR_10 | MII_ANAR_CSMACD,
    [MII_ANLPAR]    = MII_ANLPAR_ACK | MII_ANLPAR_PAUSE |
                      MII_ANLPAR_TXFD | MII_ANLPAR_TX | MII_ANLPAR_10FD |
                      MII_ANLPAR_10 | MII_ANLPAR_CSMACD,
    [MII_ANER]      = 0x64 | MII_ANER_NWAY,
    [MII_ANNP]      = 0x2001,
    [MII_CTRL1000]  = MII_CTRL1000_FULL,
    [MII_STAT1000]  = MII_STAT1000_FULL,
    [MII_EXTSTAT]   = 0x3000, /* 1000BASTE_T full-duplex capable */
};

static void npcm_gmac_soft_reset(NPCMGMACState *gmac)
{
    memcpy(gmac->regs, npcm_gmac_cold_reset_values,
           NPCM_GMAC_NR_REGS * sizeof(uint32_t));
    /* Clear reset bits */
    gmac->regs[R_NPCM_DMA_BUS_MODE] &= ~NPCM_DMA_BUS_MODE_SWR;
}

static void gmac_phy_set_link(NPCMGMACState *s, bool active)
{
    /* Autonegotiation status mirrors link status.  */
    if (active) {
        s->phy_regs[0][MII_BMSR] |= (MII_BMSR_LINK_ST | MII_BMSR_AN_COMP);
    } else {
        s->phy_regs[0][MII_BMSR] &= ~(MII_BMSR_LINK_ST | MII_BMSR_AN_COMP);
    }
}

static bool gmac_can_receive(NetClientState *nc)
{
    return true;
}

/*
 * Function that updates the GMAC IRQ
 * It find the logical OR of the enabled bits for NIS (if enabled)
 * It find the logical OR of the enabled bits for AIS (if enabled)
 */
static void gmac_update_irq(NPCMGMACState *gmac)
{
    /*
     * Check if the normal interrupts summery is enabled
     * if so, add the bits for the summary that are enabled
     */
    if (gmac->regs[R_NPCM_DMA_INTR_ENA] & gmac->regs[R_NPCM_DMA_STATUS] &
        (NPCM_DMA_INTR_ENAB_NIE_BITS))
    {
        gmac->regs[R_NPCM_DMA_STATUS] |=  NPCM_DMA_STATUS_NIS;
    }
    /*
     * Check if the abnormal interrupts summery is enabled
     * if so, add the bits for the summary that are enabled
     */
    if (gmac->regs[R_NPCM_DMA_INTR_ENA] & gmac->regs[R_NPCM_DMA_STATUS] &
        (NPCM_DMA_INTR_ENAB_AIE_BITS))
    {
        gmac->regs[R_NPCM_DMA_STATUS] |=  NPCM_DMA_STATUS_AIS;
    }

    /* Get the logical OR of both normal and abnormal interrupts */
    int level = !!((gmac->regs[R_NPCM_DMA_STATUS] &
                    gmac->regs[R_NPCM_DMA_INTR_ENA] &
                    NPCM_DMA_STATUS_NIS) |
                   (gmac->regs[R_NPCM_DMA_STATUS] &
                   gmac->regs[R_NPCM_DMA_INTR_ENA] &
                   NPCM_DMA_STATUS_AIS));

    /* Set the IRQ */
    trace_npcm_gmac_update_irq(DEVICE(gmac)->canonical_path,
                               gmac->regs[R_NPCM_DMA_STATUS],
                               gmac->regs[R_NPCM_DMA_INTR_ENA],
                               level);
    qemu_set_irq(gmac->irq, level);
}

static ssize_t gmac_receive(NetClientState *nc, const uint8_t *buf, size_t len)
{
    /* Placeholder */
    return 0;
}
static void gmac_cleanup(NetClientState *nc)
{
    /* Nothing to do yet. */
}

static void gmac_set_link(NetClientState *nc)
{
    NPCMGMACState *s = qemu_get_nic_opaque(nc);

    trace_npcm_gmac_set_link(!nc->link_down);
    gmac_phy_set_link(s, !nc->link_down);
}

static void npcm_gmac_mdio_access(NPCMGMACState *gmac, uint16_t v)
{
    bool busy = v & NPCM_GMAC_MII_ADDR_BUSY;
    uint8_t is_write;
    uint8_t pa, gr;
    uint16_t data;

    if (busy) {
        is_write = v & NPCM_GMAC_MII_ADDR_WRITE;
        pa = NPCM_GMAC_MII_ADDR_PA(v);
        gr = NPCM_GMAC_MII_ADDR_GR(v);
        /* Both pa and gr are 5 bits, so they are less than 32. */
        g_assert(pa < NPCM_GMAC_MAX_PHYS);
        g_assert(gr < NPCM_GMAC_MAX_PHY_REGS);


        if (v & NPCM_GMAC_MII_ADDR_WRITE) {
            data = gmac->regs[R_NPCM_GMAC_MII_DATA];
            /* Clear reset bit for BMCR register */
            switch (gr) {
            case MII_BMCR:
                data &= ~MII_BMCR_RESET;
                /* Autonegotiation is a W1C bit*/
                if (data & MII_BMCR_ANRESTART) {
                    /* Tells autonegotiation to not restart again */
                    data &= ~MII_BMCR_ANRESTART;
                }
                if ((data & MII_BMCR_AUTOEN) &&
                    !(gmac->phy_regs[pa][MII_BMSR] & MII_BMSR_AN_COMP)) {
                    /* sets autonegotiation as complete */
                    gmac->phy_regs[pa][MII_BMSR] |= MII_BMSR_AN_COMP;
                    /* Resolve AN automatically->need to set this */
                    gmac->phy_regs[0][MII_ANLPAR] = 0x0000;
                }
            }
            gmac->phy_regs[pa][gr] = data;
        } else {
            data = gmac->phy_regs[pa][gr];
            gmac->regs[R_NPCM_GMAC_MII_DATA] = data;
        }
        trace_npcm_gmac_mdio_access(DEVICE(gmac)->canonical_path, is_write, pa,
                                        gr, data);
    }
    gmac->regs[R_NPCM_GMAC_MII_ADDR] = v & ~NPCM_GMAC_MII_ADDR_BUSY;
}

static uint64_t npcm_gmac_read(void *opaque, hwaddr offset, unsigned size)
{
    NPCMGMACState *gmac = opaque;
    uint32_t v = 0;

    switch (offset) {
    /* Write only registers */
    case A_NPCM_DMA_XMT_POLL_DEMAND:
    case A_NPCM_DMA_RCV_POLL_DEMAND:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Read of write-only reg: offset: 0x%04" HWADDR_PRIx
                      "\n", DEVICE(gmac)->canonical_path, offset);
        break;

    default:
        v = gmac->regs[offset / sizeof(uint32_t)];
    }

    trace_npcm_gmac_reg_read(DEVICE(gmac)->canonical_path, offset, v);
    return v;
}

static void npcm_gmac_write(void *opaque, hwaddr offset,
                              uint64_t v, unsigned size)
{
    NPCMGMACState *gmac = opaque;
    uint32_t prev;

    trace_npcm_gmac_reg_write(DEVICE(gmac)->canonical_path, offset, v);

    switch (offset) {
    /* Read only registers */
    case A_NPCM_GMAC_VERSION:
    case A_NPCM_GMAC_INT_STATUS:
    case A_NPCM_GMAC_RGMII_STATUS:
    case A_NPCM_GMAC_PTP_STSR:
    case A_NPCM_GMAC_PTP_STNSR:
    case A_NPCM_DMA_MISSED_FRAME_CTR:
    case A_NPCM_DMA_HOST_TX_DESC:
    case A_NPCM_DMA_HOST_RX_DESC:
    case A_NPCM_DMA_CUR_TX_BUF_ADDR:
    case A_NPCM_DMA_CUR_RX_BUF_ADDR:
    case A_NPCM_DMA_HW_FEATURE:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Write of read-only reg: offset: 0x%04" HWADDR_PRIx
                      ", value: 0x%04" PRIx64 "\n",
                      DEVICE(gmac)->canonical_path, offset, v);
        break;

    case A_NPCM_GMAC_MAC_CONFIG:
        prev = gmac->regs[offset / sizeof(uint32_t)];
        gmac->regs[offset / sizeof(uint32_t)] = v;

        /* If transmit is being enabled for first time, update desc addr */
        if (~(prev & NPCM_GMAC_MAC_CONFIG_TX_EN) &
             (v & NPCM_GMAC_MAC_CONFIG_TX_EN)) {
            gmac->regs[R_NPCM_DMA_HOST_TX_DESC] =
                gmac->regs[R_NPCM_DMA_TX_BASE_ADDR];
        }

        /* If receive is being enabled for first time, update desc addr */
        if (~(prev & NPCM_GMAC_MAC_CONFIG_RX_EN) &
             (v & NPCM_GMAC_MAC_CONFIG_RX_EN)) {
            gmac->regs[R_NPCM_DMA_HOST_RX_DESC] =
                gmac->regs[R_NPCM_DMA_RX_BASE_ADDR];
        }
        break;

    case A_NPCM_GMAC_MII_ADDR:
        npcm_gmac_mdio_access(gmac, v);
        break;

    case A_NPCM_GMAC_MAC0_ADDR_HI:
        gmac->regs[offset / sizeof(uint32_t)] = v;
        gmac->conf.macaddr.a[0] = v >> 8;
        gmac->conf.macaddr.a[1] = v >> 0;
        break;

    case A_NPCM_GMAC_MAC0_ADDR_LO:
        gmac->regs[offset / sizeof(uint32_t)] = v;
        gmac->conf.macaddr.a[2] = v >> 24;
        gmac->conf.macaddr.a[3] = v >> 16;
        gmac->conf.macaddr.a[4] = v >> 8;
        gmac->conf.macaddr.a[5] = v >> 0;
        break;

    case A_NPCM_GMAC_MAC1_ADDR_HI:
    case A_NPCM_GMAC_MAC1_ADDR_LO:
    case A_NPCM_GMAC_MAC2_ADDR_HI:
    case A_NPCM_GMAC_MAC2_ADDR_LO:
    case A_NPCM_GMAC_MAC3_ADDR_HI:
    case A_NPCM_GMAC_MAC3_ADDR_LO:
        gmac->regs[offset / sizeof(uint32_t)] = v;
        qemu_log_mask(LOG_UNIMP,
                      "%s: Only MAC Address 0 is supported. This request "
                      "is ignored.\n", DEVICE(gmac)->canonical_path);
        break;

    case A_NPCM_DMA_BUS_MODE:
        gmac->regs[offset / sizeof(uint32_t)] = v;
        if (v & NPCM_DMA_BUS_MODE_SWR) {
            npcm_gmac_soft_reset(gmac);
        }
        break;

    case A_NPCM_DMA_RCV_POLL_DEMAND:
        /* We dont actually care about the value */
        break;

    case A_NPCM_DMA_STATUS:
        /* Check that RO bits are not written to */
        if (NPCM_DMA_STATUS_RO_MASK(v)) {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "%s: Write of read-only bits of reg: offset: 0x%04"
                           HWADDR_PRIx ", value: 0x%04" PRIx64 "\n",
                           DEVICE(gmac)->canonical_path, offset, v);
        } else {
            /* for W1c bits, implement W1C */
            gmac->regs[offset / sizeof(uint32_t)] &=
                ~NPCM_DMA_STATUS_W1C_MASK(v);
            if (v & NPCM_DMA_STATUS_NIS_BITS) {
                gmac->regs[offset / sizeof(uint32_t)] &= ~NPCM_DMA_STATUS_NIS;
            }
            if (v & NPCM_DMA_STATUS_AIS_BITS) {
                gmac->regs[offset / sizeof(uint32_t)] &= ~NPCM_DMA_STATUS_AIS;
            }
        }
        break;

    default:
        gmac->regs[offset / sizeof(uint32_t)] = v;
        break;
    }

    gmac_update_irq(gmac);
}

static void npcm_gmac_reset(DeviceState *dev)
{
    NPCMGMACState *gmac = NPCM_GMAC(dev);

    npcm_gmac_soft_reset(gmac);
    memcpy(gmac->phy_regs[0], phy_reg_init, sizeof(phy_reg_init));

    trace_npcm_gmac_reset(DEVICE(gmac)->canonical_path,
                          gmac->phy_regs[0][MII_BMSR]);
}

static NetClientInfo net_npcm_gmac_info = {
    .type = NET_CLIENT_DRIVER_NIC,
    .size = sizeof(NICState),
    .can_receive = gmac_can_receive,
    .receive = gmac_receive,
    .cleanup = gmac_cleanup,
    .link_status_changed = gmac_set_link,
};

static const struct MemoryRegionOps npcm_gmac_ops = {
    .read = npcm_gmac_read,
    .write = npcm_gmac_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
        .unaligned = false,
    },
};

static void npcm_gmac_realize(DeviceState *dev, Error **errp)
{
    NPCMGMACState *gmac = NPCM_GMAC(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    memory_region_init_io(&gmac->iomem, OBJECT(gmac), &npcm_gmac_ops, gmac,
                          TYPE_NPCM_GMAC, 8 * KiB);
    sysbus_init_mmio(sbd, &gmac->iomem);
    sysbus_init_irq(sbd, &gmac->irq);

    qemu_macaddr_default_if_unset(&gmac->conf.macaddr);

    gmac->nic = qemu_new_nic(&net_npcm_gmac_info, &gmac->conf, TYPE_NPCM_GMAC,
                             dev->id, &dev->mem_reentrancy_guard, gmac);
    qemu_format_nic_info_str(qemu_get_queue(gmac->nic), gmac->conf.macaddr.a);
    gmac->regs[R_NPCM_GMAC_MAC0_ADDR_HI] = (gmac->conf.macaddr.a[0] << 8) + \
                                            gmac->conf.macaddr.a[1];
    gmac->regs[R_NPCM_GMAC_MAC0_ADDR_LO] = (gmac->conf.macaddr.a[2] << 24) + \
                                           (gmac->conf.macaddr.a[3] << 16) + \
                                           (gmac->conf.macaddr.a[4] << 8) + \
                                            gmac->conf.macaddr.a[5];
}

static void npcm_gmac_unrealize(DeviceState *dev)
{
    NPCMGMACState *gmac = NPCM_GMAC(dev);

    qemu_del_nic(gmac->nic);
}

static const VMStateDescription vmstate_npcm_gmac = {
    .name = TYPE_NPCM_GMAC,
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, NPCMGMACState, NPCM_GMAC_NR_REGS),
        VMSTATE_END_OF_LIST(),
    },
};

static Property npcm_gmac_properties[] = {
    DEFINE_NIC_PROPERTIES(NPCMGMACState, conf),
    DEFINE_PROP_END_OF_LIST(),
};

static void npcm_gmac_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    set_bit(DEVICE_CATEGORY_NETWORK, dc->categories);
    dc->desc = "NPCM GMAC Controller";
    dc->realize = npcm_gmac_realize;
    dc->unrealize = npcm_gmac_unrealize;
    dc->reset = npcm_gmac_reset;
    dc->vmsd = &vmstate_npcm_gmac;
    device_class_set_props(dc, npcm_gmac_properties);
}

static const TypeInfo npcm_gmac_types[] = {
    {
        .name = TYPE_NPCM_GMAC,
        .parent = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(NPCMGMACState),
        .class_init = npcm_gmac_class_init,
    },
};
DEFINE_TYPES(npcm_gmac_types)
