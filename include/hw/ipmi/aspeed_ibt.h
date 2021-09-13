/*
 * ASPEED iBT Device
 *
 * Copyright (c) 2016-2021 Cédric Le Goater, IBM Corporation.
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ASPEED_IBT_H
#define ASPEED_IBT_H

#include "hw/ipmi/ipmi.h"
#include "hw/sysbus.h"

#define TYPE_ASPEED_IBT "aspeed.ibt"
#define ASPEED_IBT(obj) OBJECT_CHECK(AspeedIBTState, (obj), TYPE_ASPEED_IBT)

#define ASPEED_IBT_NR_REGS 7

#define ASPEED_IBT_BUFFER_SIZE 64

#define ASPEED_IBT_IO_REGION_SIZE 0x1C

#define TO_REG(o) (o >> 2)

/* from linux/char/ipmi/bt-bmc. */
#define BT_CR0      0x0
#define   BT_CR0_IO_BASE        16
#define   BT_CR0_IRQ            12
#define   BT_CR0_EN_CLR_SLV_RDP 0x8
#define   BT_CR0_EN_CLR_SLV_WRP 0x4
#define   BT_CR0_ENABLE_IBT     0x1
#define BT_CR1      0x4
#define   BT_CR1_IRQ_H2B        0x01
#define   BT_CR1_IRQ_HBUSY      0x40
#define BT_CR2      0x8
#define   BT_CR2_IRQ_H2B        0x01
#define   BT_CR2_IRQ_HBUSY      0x40
#define BT_CR3      0xc
#define BT_CTRL     0x10
#define   BT_CTRL_B_BUSY        BIT(7) /* BMC is busy */
#define   BT_CTRL_H_BUSY        BIT(6) /* Host is busy */
#define   BT_CTRL_OEM0          BIT(5)
#define   BT_CTRL_SMS_ATN       BIT(4) /* SMS/EVT Attention */
#define   BT_CTRL_B2H_ATN       BIT(3) /* BMC to Host Attention */
#define   BT_CTRL_H2B_ATN       BIT(2) /* Host to BMC Attention */
#define   BT_CTRL_CLR_RD_PTR    BIT(1) /* Clear Read Ptr */
#define   BT_CTRL_CLR_WR_PTR    BIT(0) /* Clear Write Ptr */
#define BT_BMC2HOST 0x14
#define BT_HOST2BMC 0x14
#define BT_INTMASK  0x18
#define   BT_INTMASK_BMC_HWRST      BIT(7)
#define   BT_INTMASK_B2H_IRQ        BIT(1)
#define   BT_INTMASK_B2H_IRQEN      BIT(0)

typedef struct AspeedIBTState {
    SysBusDevice parent;

    IPMICore *handler;

    uint8_t msg_id;
    uint8_t recv_msg[ASPEED_IBT_BUFFER_SIZE];
    uint8_t recv_msg_len;
    int recv_msg_index;

    uint8_t send_msg[ASPEED_IBT_BUFFER_SIZE];
    uint8_t send_msg_len;

    MemoryRegion iomem;
    qemu_irq irq;

    uint32_t regs[ASPEED_IBT_NR_REGS];
} AspeedIBTState;


#endif /* ASPEED_IBT_H */
