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

struct NPCMGMACRxDesc {
    uint32_t rdes0;
    uint32_t rdes1;
    uint32_t rdes2;
    uint32_t rdes3;
};

/* NPCMGMACRxDesc.flags values */
/* RDES2 and RDES3 are buffer address pointers */
/* Owner: 0 = software, 1 = gmac */
#define RX_DESC_RDES0_OWNER_MASK BIT(31)
/* Destination Address Filter Fail */
#define RX_DESC_RDES0_DEST_ADDR_FILT_FAIL_MASK BIT(30)
/* Frame length*/
#define RX_DESC_RDES0_FRAME_LEN_MASK extract32(rdes0, 16, 29)
/* Error Summary */
#define RX_DESC_RDES0_ERR_SUMM_MASK BIT(15)
/* Descriptor Error */
#define RX_DESC_RDES0_DESC_ERR_MASK BIT(14)
/* Source Address Filter Fail */
#define RX_DESC_RDES0_SRC_ADDR_FILT_FAIL_MASK BIT(13)
/* Length Error */
#define RX_DESC_RDES0_LEN_ERR_MASK BIT(12)
/* Overflow Error */
#define RX_DESC_RDES0_OVRFLW_ERR_MASK BIT(11)
/* VLAN Tag */
#define RX_DESC_RDES0_VLAN_TAG_MASK BIT(10)
/* First Descriptor */
#define RX_DESC_RDES0_FIRST_DESC_MASK BIT(9)
/* Last Descriptor */
#define RX_DESC_RDES0_LAST_DESC_MASK BIT(8)
/* IPC Checksum Error/Giant Frame */
#define RX_DESC_RDES0_IPC_CHKSM_ERR_GNT_FRM_MASK BIT(7)
/* Late Collision */
#define RX_DESC_RDES0_LT_COLL_MASK BIT(6)
/* Frame Type */
#define RX_DESC_RDES0_FRM_TYPE_MASK BIT(5)
/* Receive Watchdog Timeout */
#define RX_DESC_RDES0_REC_WTCHDG_TMT_MASK BIT(4)
/* Receive Error */
#define RX_DESC_RDES0_RCV_ERR_MASK BIT(3)
/* Dribble Bit Error */
#define RX_DESC_RDES0_DRBL_BIT_ERR_MASK BIT(2)
/* Cyclcic Redundancy Check Error */
#define RX_DESC_RDES0_CRC_ERR_MASK BIT(1)
/* Rx MAC Address/Payload Checksum Error */
#define RC_DESC_RDES0_RCE_MASK BIT(0)

/* Disable Interrupt on Completion */
#define RX_DESC_RDES1_DIS_INTR_COMP_MASK BIT(31)
/* Recieve end of ring */
#define RX_DESC_RDES1_RC_END_RING_MASK BIT(25)
/* Second Address Chained */
#define RX_DESC_RDES1_SEC_ADDR_CHND_MASK BIT(24)
/* Receive Buffer 2 Size */
#define RX_DESC_RDES1_BFFR2_SZ_MASK extract32(rdes1, 11, 21)
/* Receive Buffer 1 Size */
#define RX_DESC_RDES1_BFFR1_SZ_MASK extract32(rdes1, 0, 10)


struct NPCMGMACTxDesc {
    uint32_t tdes0;
    uint32_t tdes1;
    uint32_t tdes2;
    uint32_t tdes3;
};

/* NPCMGMACTxDesc.flags values */
/* TDES2 and TDES3 are buffer address pointers */
/* Owner: 0 = software, 1 = gmac */
#define TX_DESC_TDES0_OWNER_MASK BIT(31)
/* Tx Time Stamp Status */
#define TX_DESC_TDES0_TTSS_MASK BIT(17)
/* IP Header Error */
#define TX_DESC_TDES0_IP_HEAD_ERR_MASK BIT(16)
/* Error Summary */
#define TX_DESC_TDES0_ERR_SUMM_MASK BIT(15)
/* Jabber Timeout */
#define TX_DESC_TDES0_JBBR_TMT_MASK BIT(14)
/* Frame Flushed */
#define TX_DESC_TDES0_FRM_FLSHD_MASK BIT(13)
/* Payload Checksum Error */
#define TX_DESC_TDES0_PYLD_CHKSM_ERR_MASK BIT(12)
/* Loss of Carrier */
#define TX_DESC_TDES0_LSS_CARR_MASK BIT(11)
/* No Carrier */
#define TX_DESC_TDES0_NO_CARR_MASK BIT(10)
/* Late Collision */
#define TX_DESC_TDES0_LATE_COLL_MASK BIT(9)
/* Excessive Collision */
#define TX_DESC_TDES0_EXCS_COLL_MASK BIT(8)
/* VLAN Frame */
#define TX_DESC_TDES0_VLAN_FRM_MASK BIT(7)
/* Collision Count */
#define TX_DESC_TDES0_COLL_CNT_MASK extract32(tdes0, 3, 6)
/* Excessive Deferral */
#define TX_DESC_TDES0_EXCS_DEF_MASK BIT(2)
/* Underflow Error */
#define TX_DESC_TDES0_UNDRFLW_ERR_MASK BIT(1)
/* Deferred Bit */
#define TX_DESC_TDES0_DFRD_BIT_MASK BIT(0)

/* Interrupt of Completion */
#define TX_DESC_TDES1_INTERR_COMP_MASK BIT(31)
/* Last Segment */
#define TX_DESC_TDES1_LAST_SEG_MASK BIT(30)
/* Last Segment */
#define TX_DESC_TDES1_FIRST_SEG_MASK BIT(29)
/* Checksum Insertion Control */
#define TX_DESC_TDES1_CHKSM_INS_CTRL_MASK extract32(tdes1, 27, 28)
/* Disable Cyclic Redundancy Check */
#define TX_DESC_TDES1_DIS_CDC_MASK BIT(26)
/* Transmit End of Ring */
#define TX_DESC_TDES1_TX_END_RING_MASK BIT(25)
/* Secondary Address Chained */
#define TX_DESC_TDES1_SEC_ADDR_CHND_MASK BIT(24)
/* Transmit Buffer 2 Size */
#define TX_DESC_TDES1_BFFR2_SZ_MASK extract32(tdes1, 11, 21)
/* Transmit Buffer 1 Size */
#define TX_DESC_TDES1_BFFR1_SZ_MASK extract32(tdes1, 0, 10)

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
