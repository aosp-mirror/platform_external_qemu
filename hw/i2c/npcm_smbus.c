/*
 * Nuvoton NPCM7xx SMBus Module.
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

#include "qemu/osdep.h"

#include "hw/i2c/npcm_smbus.h"
#include "migration/vmstate.h"
#include "qemu/bitops.h"
#include "qemu/guest-random.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/units.h"

#include "trace.h"

enum NPCMSMBusCommonRegister {
    NPCM_SMB_SDA     = 0x0,
    NPCM_SMB_ST      = 0x2,
    NPCM_SMB_CST     = 0x4,
    NPCM_SMB_CTL1    = 0x6,
    NPCM_SMB_ADDR1   = 0x8,
    NPCM_SMB_CTL2    = 0xa,
    NPCM_SMB_ADDR2   = 0xc,
    NPCM_SMB_CTL3    = 0xe,
    NPCM_SMB_CST2    = 0x18,
    NPCM_SMB_CST3    = 0x19,
    NPCM_SMB_VER     = 0x1f,
};

enum NPCMSMBusBank0Register {
    NPCM_SMB_ADDR3   = 0x10,
    NPCM_SMB_ADDR7   = 0x11,
    NPCM_SMB_ADDR4   = 0x12,
    NPCM_SMB_ADDR8   = 0x13,
    NPCM_SMB_ADDR5   = 0x14,
    NPCM_SMB_ADDR9   = 0x15,
    NPCM_SMB_ADDR6   = 0x16,
    NPCM_SMB_ADDR10  = 0x17,
    NPCM_SMB_CTL4    = 0x1a,
    NPCM_SMB_CTL5    = 0x1b,
    NPCM_SMB_SCLLT   = 0x1c,
    NPCM_SMB_FIF_CTL = 0x1d,
    NPCM_SMB_SCLHT   = 0x1e,
};

enum NPCMSMBusBank1Register {
    NPCM_SMB_FIF_CTS  = 0x10,
    NPCM_SMB_FAIR_PER = 0x11,
    NPCM_SMB_TXF_CTL  = 0x12,
    NPCM_SMB_FRTO     = 0x13,
    NPCM_SMB_T_OUT    = 0x14,
    NPCM_SMB_TXF_STS  = 0x1a,
    NPCM_SMB_RXF_STS  = 0x1c,
    NPCM_SMB_RXF_CTL  = 0x1e,
};

/* ST fields */
#define NPCM_SMBST_STP           BIT(7)
#define NPCM_SMBST_SDAST         BIT(6)
#define NPCM_SMBST_BER           BIT(5)
#define NPCM_SMBST_NEGACK        BIT(4)
#define NPCM_SMBST_STASTR        BIT(3)
#define NPCM_SMBST_NMATCH        BIT(2)
#define NPCM_SMBST_MODE          BIT(1)
#define NPCM_SMBST_XMIT          BIT(0)

/* CST fields */
#define NPCM_SMBCST_ARPMATCH        BIT(7)
#define NPCM_SMBCST_MATCHAF         BIT(6)
#define NPCM_SMBCST_TGSCL           BIT(5)
#define NPCM_SMBCST_TSDA            BIT(4)
#define NPCM_SMBCST_GCMATCH         BIT(3)
#define NPCM_SMBCST_MATCH           BIT(2)
#define NPCM_SMBCST_BB              BIT(1)
#define NPCM_SMBCST_BUSY            BIT(0)

/* CST2 fields */
#define NPCM_SMBCST2_INTSTS         BIT(7)
#define NPCM_SMBCST2_MATCH7F        BIT(6)
#define NPCM_SMBCST2_MATCH6F        BIT(5)
#define NPCM_SMBCST2_MATCH5F        BIT(4)
#define NPCM_SMBCST2_MATCH4F        BIT(3)
#define NPCM_SMBCST2_MATCH3F        BIT(2)
#define NPCM_SMBCST2_MATCH2F        BIT(1)
#define NPCM_SMBCST2_MATCH1F        BIT(0)

/* CST3 fields */
#define NPCM_SMBCST3_EO_BUSY        BIT(7)
#define NPCM_SMBCST3_MATCH10F       BIT(2)
#define NPCM_SMBCST3_MATCH9F        BIT(1)
#define NPCM_SMBCST3_MATCH8F        BIT(0)

/* CTL1 fields */
#define NPCM_SMBCTL1_STASTRE     BIT(7)
#define NPCM_SMBCTL1_NMINTE      BIT(6)
#define NPCM_SMBCTL1_GCMEN       BIT(5)
#define NPCM_SMBCTL1_ACK         BIT(4)
#define NPCM_SMBCTL1_EOBINTE     BIT(3)
#define NPCM_SMBCTL1_INTEN       BIT(2)
#define NPCM_SMBCTL1_STOP        BIT(1)
#define NPCM_SMBCTL1_START       BIT(0)

/* CTL2 fields */
#define NPCM_SMBCTL2_SCLFRQ(rv)  extract8((rv), 1, 6)
#define NPCM_SMBCTL2_ENABLE      BIT(0)

/* CTL3 fields */
#define NPCM_SMBCTL3_SCL_LVL     BIT(7)
#define NPCM_SMBCTL3_SDA_LVL     BIT(6)
#define NPCM_SMBCTL3_BNK_SEL     BIT(5)
#define NPCM_SMBCTL3_400K_MODE   BIT(4)
#define NPCM_SMBCTL3_IDL_START   BIT(3)
#define NPCM_SMBCTL3_ARPMEN      BIT(2)
#define NPCM_SMBCTL3_SCLFRQ(rv)  extract8((rv), 0, 2)

/* CTL4 fields */
#define NPCM8XX_SMBCTL4_LVL_WE      BIT(7)
#define NPCM_SMBCTL4_HLDT        extrace8((rv), 0, 6)

/* ADDR fields */
#define NPCM_ADDR_EN             BIT(7)
#define NPCM_ADDR_A(rv)          extract8((rv), 0, 6)

/* FIFO Mode Register Fields */
/* FIF_CTL fields */
#define NPCM_SMBFIF_CTL_FIFO_EN          BIT(4)
#define NPCM_SMBFIF_CTL_FAIR_RDY_IE      BIT(2)
#define NPCM_SMBFIF_CTL_FAIR_RDY         BIT(1)
#define NPCM_SMBFIF_CTL_FAIR_BUSY        BIT(0)
/* FIF_CTS fields */
#define NPCM_SMBFIF_CTS_STR              BIT(7)
#define NPCM_SMBFIF_CTS_CLR_FIFO         BIT(6)
#define NPCM_SMBFIF_CTS_RFTE_IE          BIT(3)
#define NPCM_SMBFIF_CTS_RXF_TXE          BIT(1)
/* TXF_CTL fields */
#define NPCM_SMBTXF_CTL_THR_TXIE         BIT(6)
/* T_OUT fields */
#define NPCM_SMBT_OUT_ST                 BIT(7)
#define NPCM_SMBT_OUT_IE                 BIT(6)
#define NPCM_SMBT_OUT_CLKDIV(rv)         extract8((rv), 0, 6)
/* TXF_STS fields */
#define NPCM_SMBTXF_STS_TX_THST          BIT(6)
/* RXF_STS fields */
#define NPCM_SMBRXF_STS_RX_THST          BIT(6)
/* RXF_CTL fields */
#define NPCM_SMBRXF_CTL_THR_RXIE         BIT(6)
#define NPCM7XX_SMBRXF_CTL_LAST          BIT(5)
#define NPCM8XX_SMBRXF_CTL_LAST          BIT(7)

#define NPCM_SMBUS_FIFO_BYTES(rv, size)  ((rv) & (((size) << 1) - 1))

#define KEEP_OLD_BIT(o, n, b)       (((n) & (~(b))) | ((o) & (b)))
#define WRITE_ONE_CLEAR(o, n, b)    ((n) & (b) ? (o) & (~(b)) : (o))

#define NPCM_SMBUS_ENABLED(s)    ((s)->ctl2 & NPCM_SMBCTL2_ENABLE)
#define NPCM_SMBUS_FIFO_ENABLED(s) ((s)->fif_ctl & \
                                       NPCM_SMBFIF_CTL_FIFO_EN)

/* VERSION fields values, read-only. */
#define NPCM_SMBUS_VERSION_NUMBER 1
#define NPCM_SMBUS_VERSION_FIFO_SUPPORTED 1

/* Reset values */
#define NPCM_SMB_ST_INIT_VAL     0x00
#define NPCM_SMB_CST_INIT_VAL    0x10
#define NPCM_SMB_CST2_INIT_VAL   0x00
#define NPCM_SMB_CST3_INIT_VAL   0x00
#define NPCM_SMB_CTL1_INIT_VAL   0x00
#define NPCM_SMB_CTL2_INIT_VAL   0x00
#define NPCM_SMB_CTL3_INIT_VAL   0xc0
#define NPCM_SMB_CTL4_INIT_VAL   0x07
#define NPCM_SMB_CTL5_INIT_VAL   0x00
#define NPCM_SMB_ADDR_INIT_VAL   0x00
#define NPCM_SMB_SCLLT_INIT_VAL  0x00
#define NPCM_SMB_SCLHT_INIT_VAL  0x00
#define NPCM_SMB_FIF_CTL_INIT_VAL 0x00
#define NPCM_SMB_FIF_CTS_INIT_VAL 0x00
#define NPCM_SMB_FAIR_PER_INIT_VAL 0x00
#define NPCM_SMB_TXF_CTL_INIT_VAL 0x00
#define NPCM8XX_SMB_FRTO_INIT_VAL  0x02
#define NPCM_SMB_T_OUT_INIT_VAL 0x3f
#define NPCM_SMB_TXF_STS_INIT_VAL 0x00
#define NPCM_SMB_RXF_STS_INIT_VAL 0x00
#define NPCM_SMB_RXF_CTL_INIT_VAL 0x01

static const uint8_t npcm_smbus_matchf_mask[] = {
    NPCM_SMBCST2_MATCH1F,
    NPCM_SMBCST2_MATCH2F,
    NPCM_SMBCST2_MATCH3F,
    NPCM_SMBCST2_MATCH4F,
    NPCM_SMBCST2_MATCH5F,
    NPCM_SMBCST2_MATCH6F,
    NPCM_SMBCST2_MATCH7F,
    NPCM_SMBCST3_MATCH8F,
    NPCM_SMBCST3_MATCH9F,
    NPCM_SMBCST3_MATCH10F,
};

QEMU_BUILD_BUG_ON(sizeof(npcm_smbus_matchf_mask) != NPCM_SMBUS_NR_ADDRS);

static void npcm_smbus_set_matchf(NPCMSMBusState *s, int index, bool set)
{
    uint8_t *reg;

    if (index > 5) {
        reg = &s->cst3;
    } else {
        reg = &s->cst2;
    }

    if (set) {
        *reg |= npcm_smbus_matchf_mask[index];
    } else {
        *reg &= ~npcm_smbus_matchf_mask[index];
    }
}

static uint8_t npcm_smbus_get_version(void)
{
    return NPCM_SMBUS_VERSION_FIFO_SUPPORTED << 7 |
           NPCM_SMBUS_VERSION_NUMBER;
}

static void npcm_smbus_update_irq(NPCMSMBusState *s)
{
    int level;

    if (s->ctl1 & NPCM_SMBCTL1_INTEN) {
        level = !!((s->ctl1 & NPCM_SMBCTL1_NMINTE &&
                    s->st & NPCM_SMBST_NMATCH) ||
                   (s->st & NPCM_SMBST_BER) ||
                   (s->st & NPCM_SMBST_NEGACK) ||
                   (s->st & NPCM_SMBST_SDAST) ||
                   (s->st & NPCM_SMBST_STP) ||
                   (s->ctl1 & NPCM_SMBCTL1_STASTRE &&
                    s->st & NPCM_SMBST_SDAST) ||
                   (s->ctl1 & NPCM_SMBCTL1_EOBINTE &&
                    s->cst3 & NPCM_SMBCST3_EO_BUSY) ||
                   (s->rxf_ctl & NPCM_SMBRXF_CTL_THR_RXIE &&
                    s->rxf_sts & NPCM_SMBRXF_STS_RX_THST) ||
                   (s->txf_ctl & NPCM_SMBTXF_CTL_THR_TXIE &&
                    s->txf_sts & NPCM_SMBTXF_STS_TX_THST) ||
                   (s->fif_cts & NPCM_SMBFIF_CTS_RFTE_IE &&
                    s->fif_cts & NPCM_SMBFIF_CTS_RXF_TXE));

        if (level) {
            s->cst2 |= NPCM_SMBCST2_INTSTS;
        } else {
            s->cst2 &= ~NPCM_SMBCST2_INTSTS;
        }
        qemu_set_irq(s->irq, level);
    }
}

static void npcm_smbus_stop_target_device_transfer(NPCMSMBusState *s)
{
    i2c_end_transfer(s->bus);
    s->st |= NPCM_SMBST_STP;
    s->st &= ~NPCM_SMBST_SDAST;
    npcm_smbus_update_irq(s);
}

static void npcm_smbus_nack(NPCMSMBusState *s)
{
    s->st &= ~NPCM_SMBST_SDAST;
    s->st |= NPCM_SMBST_NEGACK;
    s->status = NPCM_SMBUS_STATUS_NEGACK;
}

static void npcm_smbus_clear_buffer(NPCMSMBusState *s)
{
    s->fif_cts &= ~NPCM_SMBFIF_CTS_RXF_TXE;
    s->txf_sts = 0;
    s->rxf_sts = 0;
}

static void npcm_smbus_send_byte(NPCMSMBusState *s, uint8_t value)
{
    NPCMSMBusClass *c = NPCM_SMBUS_GET_CLASS(s);
    int rv = i2c_send(s->bus, value);
    uint8_t fifo_size = c->fifo_size;

    if (rv) {
        if (s->target_device_mode_active) {
            npcm_smbus_stop_target_device_transfer(s);
        } else {
           npcm_smbus_nack(s);
        }
    } else {
        s->st |= NPCM_SMBST_SDAST;
        if (NPCM_SMBUS_FIFO_ENABLED(s)) {
            s->fif_cts |= NPCM_SMBFIF_CTS_RXF_TXE;
            if (NPCM_SMBUS_FIFO_BYTES(s->txf_sts, fifo_size) ==
                NPCM_SMBUS_FIFO_BYTES(s->txf_ctl, fifo_size)) {
                s->txf_sts = NPCM_SMBTXF_STS_TX_THST;
            } else {
                s->txf_sts = 0;
            }
        }
    }
    trace_npcm_smbus_send_byte((DEVICE(s)->canonical_path), value, !rv);
    npcm_smbus_update_irq(s);
}

static void npcm_smbus_recv_byte(NPCMSMBusState *s)
{
    if (s->target_device_mode_active) {
        if (s->target_device_send_bytes > 0) {
            s->target_device_send_bytes--;
            s->sda = i2c_recv(s->bus);
            s->st |= NPCM_SMBST_SDAST;
            trace_npcm_smbus_recv_byte((DEVICE(s)->canonical_path), s->sda);
            npcm_smbus_update_irq(s);
        }
    } else {
        s->sda = i2c_recv(s->bus);
        s->st |= NPCM_SMBST_SDAST;
        if (s->st & NPCM_SMBCTL1_ACK) {
            trace_npcm_smbus_nack(DEVICE(s)->canonical_path);
            i2c_nack(s->bus);
            s->st &= ~NPCM_SMBCTL1_ACK;
        }
        trace_npcm_smbus_recv_byte((DEVICE(s)->canonical_path), s->sda);
        npcm_smbus_update_irq(s);
    }
}

static void npcm_smbus_recv_fifo(NPCMSMBusState *s)
{
    NPCMSMBusClass *c = NPCM_SMBUS_GET_CLASS(s);
    uint8_t fifo_size = c->fifo_size;
    uint8_t expected_bytes = NPCM_SMBUS_FIFO_BYTES(s->rxf_ctl, fifo_size);
    uint8_t received_bytes = NPCM_SMBUS_FIFO_BYTES(s->rxf_sts, fifo_size);
    uint8_t pos;

    if (received_bytes == expected_bytes) {
        return;
    }

    while (received_bytes < expected_bytes &&
           received_bytes < fifo_size) {
        pos = (s->rx_cur + received_bytes) % fifo_size;
        if (s->target_device_mode_active && s->target_device_send_bytes == 0) {
            break;
        }
        s->rx_fifo[pos] = i2c_recv(s->bus);
        trace_npcm_smbus_recv_byte((DEVICE(s)->canonical_path),
                                      s->rx_fifo[pos]);
        ++received_bytes;
        if (s->target_device_mode_active && s->target_device_send_bytes > 0) {
            s->target_device_send_bytes--;
        }
    }

    trace_npcm_smbus_recv_fifo((DEVICE(s)->canonical_path),
                                  received_bytes, expected_bytes);
    s->rxf_sts = received_bytes;

    if (s->target_device_mode_active && s->target_device_send_bytes == 0 &&
       received_bytes == 0) {
        npcm_smbus_stop_target_device_transfer(s);
    }

    if (unlikely(received_bytes < expected_bytes &&
                 !s->target_device_mode_active)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: invalid rx_thr value: 0x%02x\n",
                      DEVICE(s)->canonical_path, expected_bytes);
        return;
    }

    if (received_bytes >= expected_bytes) {
        s->rxf_sts |= NPCM_SMBRXF_STS_RX_THST;
    }
    if (s->rxf_ctl & c->rxf_ctl_last &&
        !s->target_device_mode_active) {
        trace_npcm_smbus_nack(DEVICE(s)->canonical_path);
        i2c_nack(s->bus);
        s->rxf_ctl &= ~c->rxf_ctl_last;
    }
    if (received_bytes == fifo_size) {
        if (!(s->st & NPCM_SMBST_STP)) {
            s->st |= NPCM_SMBST_SDAST;
        }
        s->fif_cts |= NPCM_SMBFIF_CTS_RXF_TXE;
    } else if (!(s->rxf_ctl & NPCM_SMBRXF_CTL_THR_RXIE)
               && !(s->st & NPCM_SMBST_STP)) {
        s->st |= NPCM_SMBST_SDAST;
    } else {
        s->st &= ~NPCM_SMBST_SDAST;
    }
    npcm_smbus_update_irq(s);
}

static void npcm_smbus_read_byte_fifo(NPCMSMBusState *s)
{
    NPCMSMBusClass *c = NPCM_SMBUS_GET_CLASS(s);
    uint8_t fifo_size = c->fifo_size;
    uint8_t received_bytes = NPCM_SMBUS_FIFO_BYTES(s->rxf_sts, fifo_size);

    if (received_bytes == 0) {
        npcm_smbus_recv_fifo(s);
        return;
    }

    s->sda = s->rx_fifo[s->rx_cur];
    s->rx_cur = (s->rx_cur + 1u) % fifo_size;
    --s->rxf_sts;
    if (s->rxf_sts == 0 && s->target_device_send_bytes == 0 &&
        s->target_device_mode_active) {
        npcm_smbus_stop_target_device_transfer(s);
    }

    npcm_smbus_update_irq(s);
}

static void npcm_smbus_start(NPCMSMBusState *s)
{
    /*
     * We can start the bus if one of these is true:
     * 1. The bus is idle (so we can request it)
     * 2. We are the occupier (it's a repeated start condition.)
     */
    int available = !i2c_bus_busy(s->bus) ||
                    s->status != NPCM_SMBUS_STATUS_IDLE;

    if (available) {
        s->st |= NPCM_SMBST_MODE | NPCM_SMBST_XMIT | NPCM_SMBST_SDAST;
        s->cst |= NPCM_SMBCST_BUSY;
        if (NPCM_SMBUS_FIFO_ENABLED(s)) {
            s->fif_cts |= NPCM_SMBFIF_CTS_RXF_TXE;
        }
    } else {
        s->st &= ~NPCM_SMBST_MODE;
        s->cst &= ~NPCM_SMBCST_BUSY;
        s->st |= NPCM_SMBST_BER;
    }

    trace_npcm_smbus_start(DEVICE(s)->canonical_path, available);
    s->cst |= NPCM_SMBCST_BB;
    s->status = NPCM_SMBUS_STATUS_IDLE;
    npcm_smbus_update_irq(s);
}

static void npcm_smbus_send_address(NPCMSMBusState *s, uint8_t value)
{
    int recv;
    int rv;

    recv = value & BIT(0);
    rv = i2c_start_transfer(s->bus, value >> 1, recv);
    trace_npcm_smbus_send_address(DEVICE(s)->canonical_path,
                                     value >> 1, recv, !rv);
    if (rv) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: requesting i2c bus for 0x%02x failed: %d\n",
                      DEVICE(s)->canonical_path, value, rv);
        /* Failed to start transfer. NACK to reject.*/
        if (recv) {
            s->st &= ~NPCM_SMBST_XMIT;
        } else {
            s->st |= NPCM_SMBST_XMIT;
        }
        npcm_smbus_nack(s);
        npcm_smbus_update_irq(s);
        return;
    }

    s->st &= ~NPCM_SMBST_NEGACK;
    if (recv) {
        s->status = NPCM_SMBUS_STATUS_RECEIVING;
        s->st &= ~NPCM_SMBST_XMIT;
    } else {
        s->status = NPCM_SMBUS_STATUS_SENDING;
        s->st |= NPCM_SMBST_XMIT;
    }

    if ((s->ctl1 & NPCM_SMBCTL1_STASTRE) && !s->target_device_mode_active) {
        s->st |= NPCM_SMBST_STASTR;
        if (!recv) {
            s->st |= NPCM_SMBST_SDAST;
        }
    } else if (recv) {
        s->st |= NPCM_SMBST_SDAST;
        if (NPCM_SMBUS_FIFO_ENABLED(s)) {
            npcm_smbus_recv_fifo(s);
        } else {
            npcm_smbus_recv_byte(s);
        }
    } else if (NPCM_SMBUS_FIFO_ENABLED(s)) {
        s->st |= NPCM_SMBST_SDAST;
        s->fif_cts |= NPCM_SMBFIF_CTS_RXF_TXE;
    }
    npcm_smbus_update_irq(s);
}

static int npcm_smbus_device_initiated_transfer(I2CBus *bus, uint8_t address,
                                                   int send_length)
{
    NPCMSMBusState *s = NPCM_SMBUS(bus->qbus.parent);
    int ret = 0;
    int recv = send_length > 0;
    int i;
    qemu_mutex_lock(&s->lock);

    if (s->st & NPCM_SMBST_SDAST) {
        ret = 0;
    } else {
        for (i  = 0; i < NPCM_SMBST_NMATCH; i++) {
            if ((address == NPCM_ADDR_A(s->addr[i]) &&
                (s->addr[i] & NPCM_ADDR_EN)) &&
                 !s->target_device_mode_active &&
                 !s->target_device_mode_initiated) {
                s->st |= NPCM_SMBST_NMATCH;
                s->cst |= NPCM_SMBCST_MATCH;
                npcm_smbus_set_matchf(s, i, true);

                s->target_device_send_bytes = send_length;

                if (recv) {
                    /*
                     * For receive mode, we don't start the i2c transaction
                     * until after the NMATCH bit is cleared to avoid clashing
                     * with the previous transaction.
                     */
                    s->target_device_mode_active = false;
                    s->target_device_mode_initiated = true;
                    s->target_device_address = recv | (address << 1);

                    npcm_smbus_clear_buffer(s);
                    s->st &= ~NPCM_SMBST_NEGACK;
                    if (recv) {
                        s->status = NPCM_SMBUS_STATUS_RECEIVING;
                        s->st &= ~NPCM_SMBST_XMIT;
                    } else {
                        s->status = NPCM_SMBUS_STATUS_SENDING;
                        s->st |= NPCM_SMBST_XMIT;
                        s->st |= NPCM_SMBST_SDAST;
                    }
                    npcm_smbus_update_irq(s);
                } else {
                    s->target_device_mode_active = true;
                    s->target_device_mode_initiated = false;
                    npcm_smbus_send_address(s, recv | (address << 1));
                }
                ret = 1;
                break;
            }
        }
    }

    qemu_mutex_unlock(&s->lock);
    return ret;
}

static void npcm_smbus_execute_stop(NPCMSMBusState *s)
{
    i2c_end_transfer(s->bus);
    s->st = 0;
    s->cst = 0;
    s->status = NPCM_SMBUS_STATUS_IDLE;
    s->cst3 |= NPCM_SMBCST3_EO_BUSY;
    trace_npcm_smbus_stop(DEVICE(s)->canonical_path);
    npcm_smbus_update_irq(s);
}

static void npcm_smbus_stop(NPCMSMBusState *s)
{
    if (s->st & NPCM_SMBST_MODE) {
        switch (s->status) {
        case NPCM_SMBUS_STATUS_RECEIVING:
        case NPCM_SMBUS_STATUS_STOPPING_LAST_RECEIVE:
            s->status = NPCM_SMBUS_STATUS_STOPPING_LAST_RECEIVE;
            break;

        case NPCM_SMBUS_STATUS_NEGACK:
            s->status = NPCM_SMBUS_STATUS_STOPPING_NEGACK;
            break;

        default:
            npcm_smbus_execute_stop(s);
            break;
        }
    }
}

static uint8_t npcm_smbus_read_sda(NPCMSMBusState *s)
{
    NPCMSMBusClass *c = NPCM_SMBUS_GET_CLASS(s);
    uint8_t value = s->sda;
    uint8_t fifo_size = c->fifo_size;

    switch (s->status) {
    case NPCM_SMBUS_STATUS_STOPPING_LAST_RECEIVE:
        if (NPCM_SMBUS_FIFO_ENABLED(s)) {
            if (NPCM_SMBUS_FIFO_BYTES(s->rxf_sts, fifo_size) <= 1) {
                npcm_smbus_execute_stop(s);
            }
            if (NPCM_SMBUS_FIFO_BYTES(s->rxf_sts, fifo_size) == 0) {
                qemu_log_mask(LOG_GUEST_ERROR,
                              "%s: read to SDA with an empty rx-fifo buffer, "
                              "result undefined: %u\n",
                              DEVICE(s)->canonical_path, s->sda);
                break;
            }
            npcm_smbus_read_byte_fifo(s);
            value = s->sda;
        } else {
            npcm_smbus_execute_stop(s);
        }
        break;

    case NPCM_SMBUS_STATUS_RECEIVING:
        if (NPCM_SMBUS_FIFO_ENABLED(s)) {
            npcm_smbus_read_byte_fifo(s);
            value = s->sda;
        } else {
            npcm_smbus_recv_byte(s);
        }
        break;

    default:
        /* Do nothing */
        break;
    }

    return value;
}

static void npcm_smbus_write_sda(NPCMSMBusState *s, uint8_t value)
{
    s->sda = value;
    if (s->st & NPCM_SMBST_MODE) {
        switch (s->status) {
        case NPCM_SMBUS_STATUS_IDLE:
            npcm_smbus_send_address(s, value);
            break;
        case NPCM_SMBUS_STATUS_SENDING:
            npcm_smbus_send_byte(s, value);
            break;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "%s: write to SDA in invalid status %d: %u\n",
                          DEVICE(s)->canonical_path, s->status, value);
            break;
        }
    } else if (s->target_device_mode_active) {
        npcm_smbus_send_byte(s, value);
    }
}

static void npcm_smbus_write_st(NPCMSMBusState *s, uint8_t value)
{
    int i;

    if (value & NPCM_SMBST_STP) {
        s->target_device_send_bytes = 0;
        npcm_smbus_clear_buffer(s);

        s->st = 0;
        s->cst = 0;

        s->status = NPCM_SMBUS_STATUS_IDLE;
        s->cst3 |= NPCM_SMBCST3_EO_BUSY;
        s->target_device_mode_active = false;
        s->target_device_mode_initiated = false;
    }

    if (value & NPCM_SMBST_NMATCH) {
        for (i = 0; i < NPCM_SMBUS_NR_ADDRS; i++) {
            npcm_smbus_set_matchf(s, i, false);
        }
        if (s->target_device_mode_initiated) {
            s->target_device_mode_initiated = false;
            s->target_device_mode_active = true;
            npcm_smbus_send_address(s, s->target_device_address);
        }
    }

    s->st = WRITE_ONE_CLEAR(s->st, value, NPCM_SMBST_STP);
    s->st = WRITE_ONE_CLEAR(s->st, value, NPCM_SMBST_BER);
    s->st = WRITE_ONE_CLEAR(s->st, value, NPCM_SMBST_STASTR);
    s->st = WRITE_ONE_CLEAR(s->st, value, NPCM_SMBST_NMATCH);

    if (value & NPCM_SMBST_NEGACK) {
        s->st &= ~NPCM_SMBST_NEGACK;
        if (s->status == NPCM_SMBUS_STATUS_STOPPING_NEGACK) {
            npcm_smbus_execute_stop(s);
        }
    }

    if (value & NPCM_SMBST_STASTR &&
        s->status == NPCM_SMBUS_STATUS_RECEIVING) {
        if (NPCM_SMBUS_FIFO_ENABLED(s)) {
            npcm_smbus_recv_fifo(s);
        } else {
            npcm_smbus_recv_byte(s);
        }
    }

    npcm_smbus_update_irq(s);
}

static void npcm_smbus_write_cst(NPCMSMBusState *s, uint8_t value)
{
    uint8_t new_value = s->cst;

    s->cst = WRITE_ONE_CLEAR(new_value, value, NPCM_SMBCST_BB);
    npcm_smbus_update_irq(s);
}

static void npcm_smbus_write_cst3(NPCMSMBusState *s, uint8_t value)
{
    s->cst3 = WRITE_ONE_CLEAR(s->cst3, value, NPCM_SMBCST3_EO_BUSY);
    npcm_smbus_update_irq(s);
}

static void npcm_smbus_write_ctl1(NPCMSMBusState *s, uint8_t value)
{
    s->ctl1 = KEEP_OLD_BIT(s->ctl1, value,
            NPCM_SMBCTL1_START | NPCM_SMBCTL1_STOP | NPCM_SMBCTL1_ACK);

    if (value & NPCM_SMBCTL1_START) {
        npcm_smbus_start(s);
    }

    if (value & NPCM_SMBCTL1_STOP) {
        npcm_smbus_stop(s);
    }

    npcm_smbus_update_irq(s);
}

static void npcm_smbus_write_ctl2(NPCMSMBusState *s, uint8_t value)
{
    s->ctl2 = value;

    if (!NPCM_SMBUS_ENABLED(s)) {
        /*
         * When disabling the SMBus module, the SDA and SCL lines go high,
         * which can look like an I2C stop.
         */
        npcm_smbus_execute_stop(s);
        /* Disable this SMBus module. */
        s->ctl1 = 0;
        s->st = 0;
        s->cst3 = s->cst3 & (~NPCM_SMBCST3_EO_BUSY);
        s->cst = 0;
        npcm_smbus_clear_buffer(s);
    }
}

static void npcm_smbus_write_ctl3(NPCMSMBusState *s, uint8_t value)
{
    NPCMSMBusClass *c = NPCM_SMBUS_GET_CLASS(s);
    uint8_t old_ctl3 = s->ctl3;
    bool can_write_lvl;

    /* In NPCM8XX, CTL4_LVL_WE controls whether we can write to LVL regs. */
    can_write_lvl = c->has_lvl_we && (s->ctl4 & NPCM8XX_SMBCTL4_LVL_WE);

    if (!can_write_lvl) {
        /* Write to SDA and SCL bits are ignored. */
        s->ctl3 = KEEP_OLD_BIT(old_ctl3, value,
                               NPCM_SMBCTL3_SCL_LVL | NPCM_SMBCTL3_SDA_LVL);
    }
}

static void npcm_smbus_write_fif_ctl(NPCMSMBusState *s, uint8_t value)
{
    uint8_t new_ctl = value;

    new_ctl = KEEP_OLD_BIT(s->fif_ctl, new_ctl, NPCM_SMBFIF_CTL_FAIR_RDY);
    new_ctl = WRITE_ONE_CLEAR(new_ctl, value, NPCM_SMBFIF_CTL_FAIR_RDY);
    new_ctl = KEEP_OLD_BIT(s->fif_ctl, new_ctl, NPCM_SMBFIF_CTL_FAIR_BUSY);
    s->fif_ctl = new_ctl;
}

static void npcm_smbus_write_fif_cts(NPCMSMBusState *s, uint8_t value)
{
    s->fif_cts = WRITE_ONE_CLEAR(s->fif_cts, value, NPCM_SMBFIF_CTS_STR);
    s->fif_cts = WRITE_ONE_CLEAR(s->fif_cts, value, NPCM_SMBFIF_CTS_RXF_TXE);
    s->fif_cts = KEEP_OLD_BIT(value, s->fif_cts, NPCM_SMBFIF_CTS_RFTE_IE);

    if (value & NPCM_SMBFIF_CTS_CLR_FIFO) {
        npcm_smbus_clear_buffer(s);
    }
}

static void npcm_smbus_write_txf_ctl(NPCMSMBusState *s, uint8_t value)
{
    s->txf_ctl = value;
}

static void npcm_smbus_write_t_out(NPCMSMBusState *s, uint8_t value)
{
    uint8_t new_t_out = value;

    if ((value & NPCM_SMBT_OUT_ST) || (!(s->t_out & NPCM_SMBT_OUT_ST))) {
        new_t_out &= ~NPCM_SMBT_OUT_ST;
    } else {
        new_t_out |= NPCM_SMBT_OUT_ST;
    }

    s->t_out = new_t_out;
}

static void npcm_smbus_write_txf_sts(NPCMSMBusState *s, uint8_t value)
{
    s->txf_sts = WRITE_ONE_CLEAR(s->txf_sts, value, NPCM_SMBTXF_STS_TX_THST);
}

static void npcm_smbus_write_rxf_sts(NPCMSMBusState *s, uint8_t value)
{
    if (value & NPCM_SMBRXF_STS_RX_THST) {
        s->rxf_sts &= ~NPCM_SMBRXF_STS_RX_THST;
    }
}

static void npcm_smbus_write_rxf_ctl(NPCMSMBusState *s, uint8_t value)
{
    NPCMSMBusClass *c = NPCM_SMBUS_GET_CLASS(s);
    uint8_t new_ctl = value;

    if (!(value & c->rxf_ctl_last)) {
        new_ctl = KEEP_OLD_BIT(s->rxf_ctl, new_ctl, c->rxf_ctl_last);
    }
    s->rxf_ctl = new_ctl;
    if (s->status == NPCM_SMBUS_STATUS_RECEIVING &&
        !s->target_device_mode_initiated) {
        npcm_smbus_recv_fifo(s);
    }
}

static uint64_t npcm_smbus_read(void *opaque, hwaddr offset, unsigned size)
{
    NPCMSMBusState *s = opaque;
    uint64_t value = 0;
    uint8_t bank = s->ctl3 & NPCM_SMBCTL3_BNK_SEL;

    /* The order of the registers are their order in memory. */
    qemu_mutex_lock(&s->lock);

    switch (offset) {
    case NPCM_SMB_SDA:
        value = npcm_smbus_read_sda(s);
        break;

    case NPCM_SMB_ST:
        value = s->st;
        break;

    case NPCM_SMB_CST:
        value = s->cst;
        break;

    case NPCM_SMB_CTL1:
        value = s->ctl1;
        break;

    case NPCM_SMB_ADDR1:
        value = s->addr[0];
        break;

    case NPCM_SMB_CTL2:
        value = s->ctl2;
        break;

    case NPCM_SMB_ADDR2:
        value = s->addr[1];
        break;

    case NPCM_SMB_CTL3:
        value = s->ctl3;
        break;

    case NPCM_SMB_CST2:
        value = s->cst2;
        break;

    case NPCM_SMB_CST3:
        value = s->cst3;
        break;

    case NPCM_SMB_VER:
        value = npcm_smbus_get_version();
        break;

    /* This register is either invalid or banked at this point. */
    default:
        if (bank) {
            /* Bank 1 */
            switch (offset) {
            case NPCM_SMB_FIF_CTS:
                value = s->fif_cts;
                break;

            case NPCM_SMB_FAIR_PER:
                value = s->fair_per;
                break;

            case NPCM_SMB_TXF_CTL:
                value = s->txf_ctl;
                break;

            case NPCM_SMB_FRTO:
                value = s->frto;
                break;

            case NPCM_SMB_T_OUT:
                value = s->t_out;
                break;

            case NPCM_SMB_TXF_STS:
                value = s->txf_sts;
                break;

            case NPCM_SMB_RXF_STS:
                value = s->rxf_sts;
                break;

            case NPCM_SMB_RXF_CTL:
                value = s->rxf_ctl;
                break;

            default:
                qemu_log_mask(LOG_GUEST_ERROR,
                        "%s: read from invalid offset 0x%" HWADDR_PRIx "\n",
                        DEVICE(s)->canonical_path, offset);
                break;
            }
        } else {
            /* Bank 0 */
            switch (offset) {
            case NPCM_SMB_ADDR3:
                value = s->addr[2];
                break;

            case NPCM_SMB_ADDR7:
                value = s->addr[6];
                break;

            case NPCM_SMB_ADDR4:
                value = s->addr[3];
                break;

            case NPCM_SMB_ADDR8:
                value = s->addr[7];
                break;

            case NPCM_SMB_ADDR5:
                value = s->addr[4];
                break;

            case NPCM_SMB_ADDR9:
                value = s->addr[8];
                break;

            case NPCM_SMB_ADDR6:
                value = s->addr[5];
                break;

            case NPCM_SMB_ADDR10:
                value = s->addr[9];
                break;

            case NPCM_SMB_CTL4:
                value = s->ctl4;
                break;

            case NPCM_SMB_CTL5:
                value = s->ctl5;
                break;

            case NPCM_SMB_SCLLT:
                value = s->scllt;
                break;

            case NPCM_SMB_FIF_CTL:
                value = s->fif_ctl;
                break;

            case NPCM_SMB_SCLHT:
                value = s->sclht;
                break;

            default:
                qemu_log_mask(LOG_GUEST_ERROR,
                        "%s: read from invalid offset 0x%" HWADDR_PRIx "\n",
                        DEVICE(s)->canonical_path, offset);
                break;
            }
        }
        break;
    }

    trace_npcm_smbus_read(DEVICE(s)->canonical_path, offset, value, size);

    qemu_mutex_unlock(&s->lock);

    return value;
}

static void npcm_smbus_write(void *opaque, hwaddr offset, uint64_t value,
                              unsigned size)
{
    NPCMSMBusState *s = opaque;
    uint8_t bank = s->ctl3 & NPCM_SMBCTL3_BNK_SEL;

    qemu_mutex_lock(&s->lock);

    trace_npcm_smbus_write(DEVICE(s)->canonical_path, offset, value, size);

    /* The order of the registers are their order in memory. */
    switch (offset) {
    case NPCM_SMB_SDA:
        npcm_smbus_write_sda(s, value);
        break;

    case NPCM_SMB_ST:
        npcm_smbus_write_st(s, value);
        break;

    case NPCM_SMB_CST:
        npcm_smbus_write_cst(s, value);
        break;

    case NPCM_SMB_CTL1:
        npcm_smbus_write_ctl1(s, value);
        break;

    case NPCM_SMB_ADDR1:
        s->addr[0] = value;
        break;

    case NPCM_SMB_CTL2:
        npcm_smbus_write_ctl2(s, value);
        break;

    case NPCM_SMB_ADDR2:
        s->addr[1] = value;
        break;

    case NPCM_SMB_CTL3:
        npcm_smbus_write_ctl3(s, value);
        break;

    case NPCM_SMB_CST2:
        qemu_log_mask(LOG_GUEST_ERROR,
                "%s: write to read-only reg: offset 0x%" HWADDR_PRIx "\n",
                DEVICE(s)->canonical_path, offset);
        break;

    case NPCM_SMB_CST3:
        npcm_smbus_write_cst3(s, value);
        break;

    case NPCM_SMB_VER:
        qemu_log_mask(LOG_GUEST_ERROR,
                "%s: write to read-only reg: offset 0x%" HWADDR_PRIx "\n",
                DEVICE(s)->canonical_path, offset);
        break;

    /* This register is either invalid or banked at this point. */
    default:
        if (bank) {
            /* Bank 1 */
            switch (offset) {
            case NPCM_SMB_FIF_CTS:
                npcm_smbus_write_fif_cts(s, value);
                break;

            case NPCM_SMB_FAIR_PER:
                s->fair_per = value;
                break;

            case NPCM_SMB_TXF_CTL:
                npcm_smbus_write_txf_ctl(s, value);
                break;

            case NPCM_SMB_FRTO:
                /* TODO: Implement frame timeout for NPCM8XX */
                s->frto = value;
                break;

            case NPCM_SMB_T_OUT:
                npcm_smbus_write_t_out(s, value);
                break;

            case NPCM_SMB_TXF_STS:
                npcm_smbus_write_txf_sts(s, value);
                break;

            case NPCM_SMB_RXF_STS:
                npcm_smbus_write_rxf_sts(s, value);
                break;

            case NPCM_SMB_RXF_CTL:
                npcm_smbus_write_rxf_ctl(s, value);
                break;

            default:
                qemu_log_mask(LOG_GUEST_ERROR,
                        "%s: write to invalid offset 0x%" HWADDR_PRIx "\n",
                        DEVICE(s)->canonical_path, offset);
                break;
            }
        } else {
            /* Bank 0 */
            switch (offset) {
            case NPCM_SMB_ADDR3:
                s->addr[2] = value;
                break;

            case NPCM_SMB_ADDR7:
                s->addr[6] = value;
                break;

            case NPCM_SMB_ADDR4:
                s->addr[3] = value;
                break;

            case NPCM_SMB_ADDR8:
                s->addr[7] = value;
                break;

            case NPCM_SMB_ADDR5:
                s->addr[4] = value;
                break;

            case NPCM_SMB_ADDR9:
                s->addr[8] = value;
                break;

            case NPCM_SMB_ADDR6:
                s->addr[5] = value;
                break;

            case NPCM_SMB_ADDR10:
                s->addr[9] = value;
                break;

            case NPCM_SMB_CTL4:
                s->ctl4 = value;
                break;

            case NPCM_SMB_CTL5:
                s->ctl5 = value;
                break;

            case NPCM_SMB_SCLLT:
                s->scllt = value;
                break;

            case NPCM_SMB_FIF_CTL:
                npcm_smbus_write_fif_ctl(s, value);
                break;

            case NPCM_SMB_SCLHT:
                s->sclht = value;
                break;

            default:
                qemu_log_mask(LOG_GUEST_ERROR,
                        "%s: write to invalid offset 0x%" HWADDR_PRIx "\n",
                        DEVICE(s)->canonical_path, offset);
                break;
            }
        }
        break;
    }

    qemu_mutex_unlock(&s->lock);
}

static const MemoryRegionOps npcm_smbus_ops = {
    .read = npcm_smbus_read,
    .write = npcm_smbus_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 1,
        .unaligned = false,
    },
};

static void npcm_smbus_enter_reset(Object *obj, ResetType type)
{
    NPCMSMBusState *s = NPCM_SMBUS(obj);

    s->st = NPCM_SMB_ST_INIT_VAL;
    s->cst = NPCM_SMB_CST_INIT_VAL;
    s->cst2 = NPCM_SMB_CST2_INIT_VAL;
    s->cst3 = NPCM_SMB_CST3_INIT_VAL;
    s->ctl1 = NPCM_SMB_CTL1_INIT_VAL;
    s->ctl2 = NPCM_SMB_CTL2_INIT_VAL;
    s->ctl3 = NPCM_SMB_CTL3_INIT_VAL;
    s->ctl4 = NPCM_SMB_CTL4_INIT_VAL;
    s->ctl5 = NPCM_SMB_CTL5_INIT_VAL;

    for (int i = 0; i < NPCM_SMBUS_NR_ADDRS; ++i) {
        s->addr[i] = NPCM_SMB_ADDR_INIT_VAL;
    }
    s->scllt = NPCM_SMB_SCLLT_INIT_VAL;
    s->sclht = NPCM_SMB_SCLHT_INIT_VAL;

    s->fif_ctl = NPCM_SMB_FIF_CTL_INIT_VAL;
    s->fif_cts = NPCM_SMB_FIF_CTS_INIT_VAL;
    s->fair_per = NPCM_SMB_FAIR_PER_INIT_VAL;
    s->txf_ctl = NPCM_SMB_TXF_CTL_INIT_VAL;
    s->frto = NPCM8XX_SMB_FRTO_INIT_VAL;
    s->t_out = NPCM_SMB_T_OUT_INIT_VAL;
    s->txf_sts = NPCM_SMB_TXF_STS_INIT_VAL;
    s->rxf_sts = NPCM_SMB_RXF_STS_INIT_VAL;
    s->rxf_ctl = NPCM_SMB_RXF_CTL_INIT_VAL;

    npcm_smbus_clear_buffer(s);
    s->status = NPCM_SMBUS_STATUS_IDLE;
    s->rx_cur = 0;

    s->target_device_mode_active = false;
    s->target_device_mode_initiated = false;
    s->target_device_send_bytes = 0;
    s->target_device_address = 0;
}

static void npcm_smbus_hold_reset(Object *obj)
{
    NPCMSMBusState *s = NPCM_SMBUS(obj);

    qemu_irq_lower(s->irq);
}

static void npcm_smbus_init(Object *obj)
{
    NPCMSMBusState *s = NPCM_SMBUS(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    qemu_mutex_init(&s->lock);

    sysbus_init_irq(sbd, &s->irq);
    memory_region_init_io(&s->iomem, obj, &npcm_smbus_ops, s,
                          "regs", 4 * KiB);
    sysbus_init_mmio(sbd, &s->iomem);

    s->bus = i2c_init_bus_type(TYPE_NPCM_I2C_BUS, DEVICE(s), "i2c-bus");
    s->status = NPCM_SMBUS_STATUS_IDLE;
}

static const VMStateDescription vmstate_npcm_smbus = {
    .name = "npcm-smbus",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8(sda, NPCMSMBusState),
        VMSTATE_UINT8(st, NPCMSMBusState),
        VMSTATE_UINT8(cst, NPCMSMBusState),
        VMSTATE_UINT8(cst2, NPCMSMBusState),
        VMSTATE_UINT8(cst3, NPCMSMBusState),
        VMSTATE_UINT8(ctl1, NPCMSMBusState),
        VMSTATE_UINT8(ctl2, NPCMSMBusState),
        VMSTATE_UINT8(ctl3, NPCMSMBusState),
        VMSTATE_UINT8(ctl4, NPCMSMBusState),
        VMSTATE_UINT8(ctl5, NPCMSMBusState),
        VMSTATE_UINT8_ARRAY(addr, NPCMSMBusState, NPCM_SMBUS_NR_ADDRS),
        VMSTATE_UINT8(scllt, NPCMSMBusState),
        VMSTATE_UINT8(sclht, NPCMSMBusState),
        VMSTATE_UINT8(fif_ctl, NPCMSMBusState),
        VMSTATE_UINT8(fif_cts, NPCMSMBusState),
        VMSTATE_UINT8(fair_per, NPCMSMBusState),
        VMSTATE_UINT8(txf_ctl, NPCMSMBusState),
        VMSTATE_UINT8(frto, NPCMSMBusState),
        VMSTATE_UINT8(t_out, NPCMSMBusState),
        VMSTATE_UINT8(txf_sts, NPCMSMBusState),
        VMSTATE_UINT8(rxf_sts, NPCMSMBusState),
        VMSTATE_UINT8(rxf_ctl, NPCMSMBusState),
        VMSTATE_UINT8_ARRAY(rx_fifo, NPCMSMBusState,
                            NPCM8XX_SMBUS_FIFO_SIZE),
        VMSTATE_UINT8(rx_cur, NPCMSMBusState),
        VMSTATE_END_OF_LIST(),
    },
};

static void npcm_smbus_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_npcm_smbus;
    rc->phases.enter = npcm_smbus_enter_reset;
    rc->phases.hold = npcm_smbus_hold_reset;
}

static void npcm7xx_smbus_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    NPCMSMBusClass *c = NPCM_SMBUS_CLASS(klass);

    dc->desc = "NPCM7xx System Management Bus";
    c->has_lvl_we = false;
    c->fifo_size = NPCM7XX_SMBUS_FIFO_SIZE;
    c->rxf_ctl_last = NPCM7XX_SMBRXF_CTL_LAST;
}

static void npcm8xx_smbus_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    NPCMSMBusClass *c = NPCM_SMBUS_CLASS(klass);

    dc->desc = "NPCM8xx System Management Bus";
    c->has_lvl_we = true;
    c->fifo_size = NPCM8XX_SMBUS_FIFO_SIZE;
    c->rxf_ctl_last = NPCM8XX_SMBRXF_CTL_LAST;
}

static void npcm_i2c_bus_class_init(ObjectClass *klass, void *data)
{
    I2CBusClass *ic = I2C_BUS_CLASS(klass);

    ic->device_initiated_transfer = npcm_smbus_device_initiated_transfer;
}

static const TypeInfo npcm_smbus_types[] = {
    {
        .name = TYPE_NPCM_SMBUS,
        .parent = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(NPCMSMBusState),
        .class_size = sizeof(NPCMSMBusClass),
        .class_init = npcm_smbus_class_init,
        .instance_init = npcm_smbus_init,
    },
    {
        .name = TYPE_NPCM7XX_SMBUS,
        .parent = TYPE_NPCM_SMBUS,
        .class_init = npcm7xx_smbus_class_init,
    },
    {
        .name = TYPE_NPCM8XX_SMBUS,
        .parent = TYPE_NPCM_SMBUS,
        .class_init = npcm8xx_smbus_class_init,
    },
    {
        .name = TYPE_NPCM_I2C_BUS,
        .parent = TYPE_I2C_BUS,
        .instance_size = sizeof(NPCMI2CBus),
        .class_init = npcm_i2c_bus_class_init,
    },
};
DEFINE_TYPES(npcm_smbus_types);
