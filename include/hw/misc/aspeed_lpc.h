/*
 *  ASPEED LPC Controller
 *
 *  Copyright (C) 2017-2018 IBM Corp.
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#ifndef ASPEED_LPC_H
#define ASPEED_LPC_H

#include "hw/sysbus.h"
#include "hw/ipmi/ipmi.h"

#define TYPE_ASPEED_LPC "aspeed.lpc"
#define TYPE_ASPEED_KCS_CHANNEL "aspeed.kcs.channel"
#define ASPEED_LPC(obj) OBJECT_CHECK(AspeedLPCState, (obj), TYPE_ASPEED_LPC)
#define ASPEED_KCS_CHANNEL(obj) OBJECT_CHECK(AspeedKCSChannel, (obj), \
                                             TYPE_ASPEED_KCS_CHANNEL)

#define ASPEED_LPC_NR_REGS      (0x260 >> 2)

enum aspeed_lpc_subdevice {
    aspeed_lpc_kcs_1 = 0,
    aspeed_lpc_kcs_2,
    aspeed_lpc_kcs_3,
    aspeed_lpc_kcs_4,
    aspeed_lpc_ibt,
};

#define ASPEED_LPC_NR_SUBDEVS   5
#define APSEED_KCS_NR_CHANNELS  4
typedef struct AspeedLPCState AspeedLPCState;

typedef struct AspeedKCSChannel {
    DeviceState             parent;
    IPMICore                *host;
    AspeedLPCState          *owner;
    uint8_t                 channel_id;
    /*
     * Core side registers are defined in aspeed_lpc.c file
     */
    /* Host side buffers. */
    uint8_t                 inmsg[MAX_IPMI_MSG_SIZE];
    uint32_t                inpos;
    uint32_t                inlen;
    uint8_t                 outmsg[MAX_IPMI_MSG_SIZE];
    uint32_t                outpos;
    uint32_t                outlen;

    /* Flags. */
    bool                    last_byte_not_ready;
    uint8_t                 last_msg_id;
} AspeedKCSChannel;


/* LPC state share by LPC and KCS */
struct AspeedLPCState {
    /* <private> */
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion iomem;
    qemu_irq irq;

    qemu_irq subdevice_irqs[ASPEED_LPC_NR_SUBDEVS];
    uint32_t subdevice_irqs_pending;

    uint32_t regs[ASPEED_LPC_NR_REGS];
    uint32_t hicr7;

    /* KCS host end buffers */
    AspeedKCSChannel channels[APSEED_KCS_NR_CHANNELS];
};

#endif /* ASPEED_LPC_H */
