/*
 * Nuvoton NPCM7xx KCS Module
 *
 * Copyright 2020 Google LLC
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
#ifndef NPCM7XX_KCS_H
#define NPCM7XX_KCS_H

#include "exec/memory.h"
#include "hw/ipmi/ipmi.h"
#include "hw/irq.h"
#include "hw/sysbus.h"

#define NPCM7XX_KCS_NR_CHANNELS             3
/*
 * Number of registers in each KCS channel. Don't change this without
 * incrementing the version_id in the vmstate.
 */
#define NPCM7XX_KCS_CHANNEL_NR_REGS         9

typedef struct NPCM7xxKCSState NPCM7xxKCSState;

/**
 * struct NPCM7xxKCSChannel - KCS Channel that can be read or written by the
 * host.
 * @parent: Parent device.
 * @owner: The KCS module that manages this KCS channel.
 * @status: The status of this KCS module.
 * @dbbout: The output buffer to the host.
 * @dbbin: The input buffer from the host.
 * @ctl: The control register.
 * @ic: The host interrupt control register. Not implemented.
 * @ie: The host interrupt enable register. Not implemented.
 * @inmsg: The input message from the host. To be put in dbbin.
 * @inpos: The current position of input message.
 * @inlen: The length of input message.
 * @outmsg: The input message from the host. To be put in dbbout.
 * @outpos: The current position of output message.
 * @outlen: The length of output message.
 * @last_byte_not_ready: The last byte in inmsg is not ready to be sent.
 * @last_msg_id: The message id of last incoming request from host.
 */
typedef struct NPCM7xxKCSChannel {
    DeviceState             parent;

    NPCM7xxKCSState         *owner;
    IPMICore                *host;
    /* Core side registers. */
    uint8_t                 status;
    uint8_t                 dbbout;
    uint8_t                 dbbin;
    uint8_t                 ctl;
    uint8_t                 ic;
    uint8_t                 ie;

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
} NPCM7xxKCSChannel;

/**
 * struct NPCM7xxKCSState - Keyboard Control Style (KCS) Module device state.
 * @parent: System bus device.
 * @iomem: Memory region through which registers are accessed.
 * @irq: GIC interrupt line to fire on reading or writing the buffer.
 * @channels: The KCS channels this module manages.
 */
struct NPCM7xxKCSState {
    SysBusDevice            parent;

    MemoryRegion            iomem;

    qemu_irq                irq;
    NPCM7xxKCSChannel       channels[NPCM7XX_KCS_NR_CHANNELS];
};

#define TYPE_NPCM7XX_KCS_CHANNEL "npcm7xx-kcs-channel"
#define NPCM7XX_KCS_CHANNEL(obj)                                      \
    OBJECT_CHECK(NPCM7xxKCSChannel, (obj), TYPE_NPCM7XX_KCS_CHANNEL)

#define TYPE_NPCM7XX_KCS "npcm7xx-kcs"
#define NPCM7XX_KCS(obj)                                              \
    OBJECT_CHECK(NPCM7xxKCSState, (obj), TYPE_NPCM7XX_KCS)

#endif /* NPCM7XX_KCS_H */
