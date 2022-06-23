/*
 * Nuvoton NPCM7xx KCS Module
 *
 * Copyright 2021 Google LLC
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
 * TODO: Add a do_hw_op function that passes host reset signal to the BPC.
 */

#include "qemu/osdep.h"
#include "hw/ipmi/npcm7xx_kcs.h"
#include "hw/irq.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qemu/bitops.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "trace.h"

/* Registers in each KCS channel. */
typedef enum NPCM7xxKCSRegister {
    NPCM7XX_KCSST,
    NPCM7XX_KCSDO,
    NPCM7XX_KCSDI,
    NPCM7XX_KCSDOC,
    NPCM7XX_KCSDOM,
    NPCM7XX_KCSDIC,
    NPCM7XX_KCSCTL,
    NPCM7XX_KCSIC,
    NPCM7XX_KCSIE,
    NPCM7XX_KCS_REGS_END,
} NPCM7xxKCSRegister;

#define NPCM7XX_BPC_BASE 0x42

/* Registers for BIOS post code. */
typedef enum NPCM7xxBPCRegister {
    NPCM7XX_BPCFA2L,
    NPCM7XX_BPCFA2M,
    NPCM7XX_BPCFEN,
    NPCM7XX_BPCFSTAT,
    NPCM7XX_BPCFDATA,
    NPCM7XX_BPCFMSTAT,
    NPCM7XX_HIGLUE, /* not a BPC register, but takes up reg space. */
    NPCM7XX_BPCFA1L,
    NPCM7XX_BPCFA1M,
    NPCM7XX_BPCSDR,
    NPCM7XX_BPC_REGS_END,
} NPCM7xxBPCRegister;

/* BPC registers */
#define NPCM7XX_BPCFEN_ADDR1EN      BIT(7)
#define NPCM7XX_BPCFEN_ADDR2EN      BIT(6)
#define NPCM7XX_BPCFEN_HRIE         BIT(4)
#define NPCM7XX_BPCFEN_FRIE         BIT(3)
#define NPCM7XX_BPCFEN_DWCAPTURE    BIT(2)
#define NPCM7XX_BPCFEN_RI2IE        BIT(1)
#define NPCM7XX_BPCFEN_RI1IE        BIT(0)

#define NPCM7XX_BPCFSTAT_FIFO_V     BIT(7)
#define NPCM7XX_BPCFSTAT_FIFO_OF    BIT(5)
#define NPCM7XX_BPCFSTAT_AD         BIT(0)

#define NPCM7XX_BPCFMSTAT_HR_CHG    BIT(6)
#define NPCM7XX_BPCFMSTAT_HR_STAT   BIT(5)
#define NPCM7XX_BPCFMSTAT_VDD_STAT  BIT(3)
#define NPCM7XX_BPCFMSTAT_RI2_STS   BIT(1)
#define NPCM7XX_BPCFMSTAT_RI1_STS   BIT(0)

#define NPCM7XX_BPCFA1L_DEFAULT     0x80

static const hwaddr npcm7xx_kcs_channel_base[] = {
    0x0c, 0x1e, 0x30, 0x42
};

#define NPCM7XX_KCS_REG_DIFF    2

/* Register field definitions. */
#define NPCM7XX_CTL_OBEIE       BIT(1)
#define NPCM7XX_CTL_IBFIE       BIT(0)

/* IPMI 2.0 Table 9.1 status register bits */
#define KCS_ST_STATE(rv)    extract8(rv, 6, 2)
#define KCS_ST_CMD          BIT(3)
#define KCS_ST_SMS_ATN      BIT(2)
#define KCS_ST_IBF          BIT(1)
#define KCS_ST_OBF          BIT(0)

/* IPMI 2.0 Table 9.2 state bits */
enum KCSState {
    IDLE_STATE,
    READ_STATE,
    WRITE_STATE,
    ERROR_STATE,
};

/* IPMI 2.0 Table 9.3 control codes */
#define KCS_CMD_GET_STATUS_ABORT    0x60
#define KCS_CMD_WRITE_START         0x61
#define KCS_CMD_WRITE_END           0x62
#define KCS_CMD_READ                0x68

/* Host Side Operations. */

static uint8_t npcm7xx_kcs_host_read_byte(NPCM7xxKCSChannel *c)
{
    uint8_t v;

    v = c->dbbout;
    c->status &= ~KCS_ST_OBF;
    if (c->ctl & NPCM7XX_CTL_OBEIE) {
        qemu_irq_raise(c->owner->irq);
    }

    trace_npcm7xx_kcs_host_read_byte(DEVICE(c)->canonical_path, v);
    return v;
}

static void npcm7xx_kcs_host_write_byte(NPCM7xxKCSChannel *c, uint8_t value,
        bool is_cmd)
{
    trace_npcm7xx_kcs_host_write_byte(DEVICE(c)->canonical_path, value);
    c->dbbin = value;
    c->status |= KCS_ST_IBF;
    if (is_cmd) {
        c->status |= KCS_ST_CMD;
    } else {
        c->status &= ~KCS_ST_CMD;
    }
    if (c->ctl & NPCM7XX_CTL_IBFIE) {
        qemu_irq_raise(c->owner->irq);
    }
}

static void npcm7xx_kcs_handle_event(NPCM7xxKCSChannel *c)
{
    uint8_t v;
    IPMICoreClass *hk;

    trace_npcm7xx_kcs_handle_event(DEVICE(c)->canonical_path,
                                   KCS_ST_STATE(c->status));
    switch (KCS_ST_STATE(c->status)) {
    case IDLE_STATE:
        if (c->status & KCS_ST_OBF) {
            /* Read a dummy byte. */
            npcm7xx_kcs_host_read_byte(c);
            if (c->outlen > 0) {
                /* Send to ipmi host when msg ends. */
                if (c->host) {
                    hk = IPMI_CORE_GET_CLASS(c->host);
                    hk->handle_command(c->host, c->outmsg, c->outlen,
                            MAX_IPMI_MSG_SIZE, c->last_msg_id);
                }
                /* The last byte has been read. return to empty state. */
                c->outlen = 0;
            }
        }
        if (c->inlen > 0) {
            /* Send to bmc the next request */
            npcm7xx_kcs_host_write_byte(c, KCS_CMD_WRITE_START, true);
            c->last_byte_not_ready = true;
        }
        break;

    case READ_STATE:
        if (c->status & KCS_ST_OBF) {
            /* Read in a byte from BMC */
            v = npcm7xx_kcs_host_read_byte(c);
            if (c->outlen < MAX_IPMI_MSG_SIZE) {
                c->outmsg[c->outlen++] = v;
            }
            npcm7xx_kcs_host_write_byte(c, KCS_CMD_READ, false);
        }
        break;

    case WRITE_STATE:
        if (c->status & KCS_ST_IBF) {
            /* The guest hasn't read the byte yet. We'll wait. */
            break;
        }
        /* Clear OBF. */
        c->status &= ~KCS_ST_OBF;
        /* If it's the last byte written, we need to first send a command. */
        if (c->last_byte_not_ready && c->inpos == c->inlen - 1) {
            npcm7xx_kcs_host_write_byte(c, KCS_CMD_WRITE_END, true);
            c->last_byte_not_ready = false;
        } else {
            npcm7xx_kcs_host_write_byte(c, c->inmsg[c->inpos++], false);
            if (!c->last_byte_not_ready) {
                /* The last byte has been sent. */
                c->inlen = 0;
                c->inpos = 0;
            }
        }
        break;

    case ERROR_STATE:
        if (c->status & KCS_ST_OBF) {
            /* Read in error byte from BMC */
            npcm7xx_kcs_host_read_byte(c);
        }
        /* Force abort */
        c->outlen = 0;
        c->inlen = 0;
        c->inpos = 0;
        c->status = 0;
        break;

    default:
        g_assert_not_reached();
    }
}

/* Received a request from the host and send it to BMC core. */
static void npcm7xx_kcs_handle_req(IPMIInterface *ii, uint8_t msg_id,
                                unsigned char *req, unsigned req_len)
{
    IPMIInterfaceClass *iic = IPMI_INTERFACE_GET_CLASS(ii);
    NPCM7xxKCSChannel *c = iic->get_backend_data(ii);

    /* Drop the request if the last request is still not handled. */
    if (c->inlen > 0) {
        return;
    }
    if (req_len == 0) {
        return;
    }
    if (req_len > MAX_IPMI_MSG_SIZE) {
        /* Truncate the extra bytes that don't fit. */
        req_len = MAX_IPMI_MSG_SIZE;
    }
    memcpy(c->inmsg, req, req_len);
    c->inpos = 0;
    c->inlen = req_len;

    c->last_msg_id = msg_id;

    npcm7xx_kcs_handle_event(c);
}

/* Core Side Operations. */
/* Return the channel index for addr. If the addr is out of range, return -1. */
static int npcm7xx_kcs_channel_index(hwaddr addr)
{
    int index;

    if (unlikely(addr < npcm7xx_kcs_channel_base[0])) {
        return -1;
    }
    if (unlikely(addr >= npcm7xx_kcs_channel_base[NPCM7XX_KCS_NR_CHANNELS])) {
        return -1;
    }
    if (unlikely(addr % NPCM7XX_KCS_REG_DIFF)) {
        return -1;
    }

    for (index = 0; index < NPCM7XX_KCS_NR_CHANNELS; ++index) {
        if (addr < npcm7xx_kcs_channel_base[index + 1]) {
            return index;
        }
    }

    g_assert_not_reached();
}

static NPCM7xxKCSRegister npcm7xx_kcs_reg_index(hwaddr addr, int channel)
{
    NPCM7xxKCSRegister reg;

    reg = (addr - npcm7xx_kcs_channel_base[channel]) / NPCM7XX_KCS_REG_DIFF;
    return reg;
}

static uint8_t npcm7xx_kcs_read_byte(NPCM7xxKCSChannel *c,
        NPCM7xxKCSRegister reg)
{
    uint8_t v = 0;

    v = c->dbbin;

    c->status &= ~KCS_ST_IBF;
    if (c->ctl & NPCM7XX_CTL_IBFIE) {
        qemu_irq_lower(c->owner->irq);
    }

    if (reg == NPCM7XX_KCSDIC) {
        qemu_log_mask(LOG_UNIMP,
                "%s: Host nSCIPME interrupt is not implemented.\n",
                __func__);
    }

    npcm7xx_kcs_handle_event(c);
    return v;
}

static void npcm7xx_kcs_write_byte(NPCM7xxKCSChannel *c,
        NPCM7xxKCSRegister reg, uint8_t value)
{
    c->dbbout = value;

    c->status |= KCS_ST_OBF;
    if (c->ctl & NPCM7XX_CTL_OBEIE) {
        qemu_irq_lower(c->owner->irq);
    }

    if (reg == NPCM7XX_KCSDOC) {
        qemu_log_mask(LOG_UNIMP,
                "%s: Host nSCIPME interrupt is not implemented.\n",
                __func__);
    } else if (reg == NPCM7XX_KCSDOM) {
        qemu_log_mask(LOG_UNIMP,
                "%s: Host nSMI interrupt is not implemented.\n",
                __func__);
    }

    npcm7xx_kcs_handle_event(c);
}

static uint8_t npcm7xx_kcs_read_status(NPCM7xxKCSChannel *c)
{
    uint8_t status = c->status;

    return status;
}

static void npcm7xx_kcs_write_status(NPCM7xxKCSChannel *c, uint8_t value)
{
    static const uint8_t mask =
        KCS_ST_CMD | KCS_ST_IBF | KCS_ST_OBF;

    if (value & mask) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: read-only bits in 0x%02x ignored\n",
                      __func__, value);
        value &= ~mask;
    }

    c->status = (c->status & mask) | value;
}

static void npcm7xx_kcs_write_ctl(NPCM7xxKCSChannel *c, uint8_t value)
{
    if (value & ~(NPCM7XX_CTL_OBEIE | NPCM7XX_CTL_IBFIE)) {
        qemu_log_mask(LOG_UNIMP, "%s: Host interrupts are not implemented.\n",
                __func__);
    }

    c->ctl = value;
}

static void npcm7xx_bpc_update_interrupt(NPCM7xxBPC *b)
{
    int level = 0;

    /* TODO: Interrupt for host reset signal. */
    if (b->regs[NPCM7XX_BPCFEN] & NPCM7XX_BPCFEN_FRIE) {
        if (b->regs[NPCM7XX_BPCFSTAT] & NPCM7XX_BPCFSTAT_FIFO_V) {
            level = 1;
        }
    }
    qemu_set_irq(b->owner->irq, level);
}

static void npcm7xx_bpc_add_data(NPCM7xxBPC *b, uint8_t data, uint8_t addr)
{
    /* Ignore the data if FIFO is full */
    if (fifo8_is_full(&b->data_fifo)) {
        b->regs[NPCM7XX_BPCFSTAT] |= NPCM7XX_BPCFSTAT_FIFO_OF;
        return;
    }

    if (b->regs[NPCM7XX_BPCFSTAT] & NPCM7XX_BPCFSTAT_FIFO_V) {
        fifo8_push(&b->data_fifo, data);
        fifo8_push(&b->addr_fifo, addr);
    } else {
        b->regs[NPCM7XX_BPCFDATA] = data;
        b->regs[NPCM7XX_BPCFSTAT] = NPCM7XX_BPCFSTAT_FIFO_V | addr;
    }
}

static uint8_t npcm7xx_bpc_read_fstat(NPCM7xxBPC *b)
{
    uint8_t v = b->regs[NPCM7XX_BPCFSTAT];

    /* Clear FIFO_OF when it's read */
    b->regs[NPCM7XX_BPCFSTAT] &= ~NPCM7XX_BPCFSTAT_FIFO_OF;
    return v;
}

static uint8_t npcm7xx_bpc_read_fdata(NPCM7xxBPC *b)
{
    uint8_t v = b->regs[NPCM7XX_BPCFDATA];

    if (fifo8_is_empty(&b->data_fifo)) {
        /* Set empty state */
        b->regs[NPCM7XX_BPCFSTAT] &= ~NPCM7XX_BPCFSTAT_FIFO_V;
    } else {
        /* Pop the next data in the buffer. */
        b->regs[NPCM7XX_BPCFDATA] = fifo8_pop(&b->data_fifo);
        b->regs[NPCM7XX_BPCFSTAT] =
            NPCM7XX_BPCFSTAT_FIFO_V | fifo8_pop(&b->addr_fifo);
    }
    npcm7xx_bpc_update_interrupt(b);
    return v;
}

static uint8_t npcm7xx_bpc_read(NPCM7xxBPC *b, hwaddr addr)
{
    uint8_t v = 0;
    hwaddr reg;

    if (addr % 2 != 0) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: unaligned access for 0x%02" HWADDR_PRIx,
                      __func__, addr);
        return 0;
    }

    reg = addr / 2;
    if (reg >= NPCM7XX_BPC_REGS_END) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: invalid access at 0x%02" HWADDR_PRIx,
                      __func__, addr);
        return 0;
    }

    switch (reg) {
    case NPCM7XX_BPCFSTAT:
        v = npcm7xx_bpc_read_fstat(b);
        break;

    case NPCM7XX_BPCFDATA:
        v = npcm7xx_bpc_read_fdata(b);
        break;

    default:
        v = b->regs[reg];
        break;
    }
    trace_npcm7xx_bpc_read(DEVICE(b)->canonical_path, reg, v);
    return v;
}

static void npcm7xx_bpc_write(NPCM7xxBPC *b, hwaddr addr, uint8_t v)
{
    hwaddr reg;

    if (addr % 2 != 0) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: unaligned access for 0x%02" HWADDR_PRIx,
                      __func__, addr);
        return;
    }

    reg = addr / 2;
    trace_npcm7xx_bpc_write(DEVICE(b)->canonical_path, reg, v);
    if (reg >= NPCM7XX_BPC_REGS_END) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: invalid access at 0x%02" HWADDR_PRIx,
                      __func__, addr);
        return;
    }

    b->regs[reg] = v;
}

static void npcm7xx_bpc_send_data(Object *obj, Visitor *v, const char *name,
                                 void *opaque, Error **errp)
{
    NPCM7xxBPC *b = NPCM7XX_BPC(obj);
    uint32_t data;
    /* match = 0 or 1 for address match, match = 2 for no match */
    uint8_t match = 2;
    uint8_t dwcapture = b->regs[NPCM7XX_BPCFEN] & NPCM7XX_BPCFEN_DWCAPTURE;

    if (!visit_type_uint32(v, name, &data, errp)) {
        return;
    }

    trace_npcm7xx_bpc_send_data(DEVICE(b)->canonical_path, b->addr, data);
    /* Match address */
    if ((dwcapture | (b->regs[NPCM7XX_BPCFEN] & NPCM7XX_BPCFEN_ADDR1EN)) &&
        (b->addr >> 8 == b->regs[NPCM7XX_BPCFA1M]) &&
        ((b->addr & 0xff) == b->regs[NPCM7XX_BPCFA1L])) {
        match = 0;
    } else if ((b->regs[NPCM7XX_BPCFEN] & NPCM7XX_BPCFEN_ADDR2EN) &&
               (b->addr >> 8 == b->regs[NPCM7XX_BPCFA2M]) &&
               ((b->addr & 0xff) == b->regs[NPCM7XX_BPCFA2L])) {
        match = 1;
    }

    /*
     * Add data to FIFO buffer if matched.
     * In dw capture mode, only match 0 is accepted.
     */
    if (match > 1 || (dwcapture && (match == 1))) {
        return;
    }
    /*
     * In dw capture mode, put all data in the FIFO with the last one bit = 1.
     * otherwise, only intepret data as uint8_t and push it to the fifo.
     */
    if (dwcapture) {
        /*
         * At least send 1 byte even if data == 0.
         * Last byte is marked by match == 1.
         */
        g_assert(match == 0);
        while (match == 0) {
            uint8_t byte = data & 0xff;

            data >>= 8;
            match = (data == 0) ? 1 : 0;
            npcm7xx_bpc_add_data(b, byte, match);
        }
    } else {
        npcm7xx_bpc_add_data(b, data, match);
    }

    /* npcm7xx_bpc_add_data should have set valid bits for BPCSTAT register. */
    g_assert(b->regs[NPCM7XX_BPCFSTAT] & NPCM7XX_BPCFSTAT_FIFO_V);
    npcm7xx_bpc_update_interrupt(b);
}

static uint64_t npcm7xx_kcs_read(void *opaque, hwaddr addr, unsigned int size)
{
    NPCM7xxKCSState *s = opaque;
    uint8_t value = 0;
    int channel;
    NPCM7xxKCSRegister reg;

    if (addr >= NPCM7XX_BPC_BASE) {
        return npcm7xx_bpc_read(&s->bpc, addr - NPCM7XX_BPC_BASE);
    }

    channel = npcm7xx_kcs_channel_index(addr);
    if (channel < 0 || channel >= NPCM7XX_KCS_NR_CHANNELS) {
        qemu_log_mask(LOG_UNIMP,
                      "%s: read from addr 0x%04" HWADDR_PRIx
                      " is invalid or unimplemented.\n",
                      __func__, addr);
        return value;
    }

    reg = npcm7xx_kcs_reg_index(addr, channel);
    switch (reg) {
    case NPCM7XX_KCSDI:
    case NPCM7XX_KCSDIC:
        value = npcm7xx_kcs_read_byte(&s->channels[channel], reg);
        break;
    case NPCM7XX_KCSDO:
    case NPCM7XX_KCSDOC:
    case NPCM7XX_KCSDOM:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: register @ 0x%04" HWADDR_PRIx " is write-only\n",
                      __func__, addr);
        break;
    case NPCM7XX_KCSST:
        value = npcm7xx_kcs_read_status(&s->channels[channel]);
        break;
    case NPCM7XX_KCSCTL:
        value = s->channels[channel].ctl;
        break;
    case NPCM7XX_KCSIC:
        value = s->channels[channel].ic;
        break;
    case NPCM7XX_KCSIE:
        value = s->channels[channel].ie;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: read from invalid addr 0x%04" HWADDR_PRIx "\n",
                      __func__, addr);
    }

    trace_npcm7xx_kcs_read(DEVICE(s)->canonical_path, channel, reg, value);
    return value;
}

static void npcm7xx_kcs_write(void *opaque, hwaddr addr, uint64_t v,
                                    unsigned int size)
{
    NPCM7xxKCSState *s = opaque;
    int channel;
    NPCM7xxKCSRegister reg;

    if (addr >= NPCM7XX_BPC_BASE) {
        npcm7xx_bpc_write(&s->bpc, addr - NPCM7XX_BPC_BASE, v);
        return;
    }

    channel = npcm7xx_kcs_channel_index(addr);
    if (channel < 0 || channel >= NPCM7XX_KCS_NR_CHANNELS) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: write to addr 0x%04" HWADDR_PRIx
                      " is invalid or unimplemented.\n",
                      __func__, addr);
        return;
    }

    reg = npcm7xx_kcs_reg_index(addr, channel);
    trace_npcm7xx_kcs_write(DEVICE(s)->canonical_path, channel, reg, (uint8_t)v);
    switch (reg) {
    case NPCM7XX_KCSDI:
    case NPCM7XX_KCSDIC:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: register @ 0x%04" HWADDR_PRIx " is read-only\n",
                      __func__, addr);
        break;
    case NPCM7XX_KCSDO:
    case NPCM7XX_KCSDOC:
    case NPCM7XX_KCSDOM:
        npcm7xx_kcs_write_byte(&s->channels[channel], reg, v);
        break;
    case NPCM7XX_KCSST:
        npcm7xx_kcs_write_status(&s->channels[channel], v);
        break;
    case NPCM7XX_KCSCTL:
        npcm7xx_kcs_write_ctl(&s->channels[channel], v);
        break;
    case NPCM7XX_KCSIC:
        qemu_log_mask(LOG_UNIMP, "%s: Host interrupts are not implemented.\n",
                __func__);
        break;
    case NPCM7XX_KCSIE:
        qemu_log_mask(LOG_UNIMP, "%s: Host interrupts are not implemented.\n",
                __func__);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: read from invalid addr 0x%04" HWADDR_PRIx "\n",
                      __func__, addr);
    }
}

static const MemoryRegionOps npcm7xx_kcs_ops = {
    .read = npcm7xx_kcs_read,
    .write = npcm7xx_kcs_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1, /* KCS registers are 8-bits. */
        .max_access_size = 1, /* KCS registers are 8-bits. */
        .unaligned = false,
    },
};

static const VMStateDescription vmstate_npcm7xx_kcs_channel = {
    .name = "npcm7xx-kcs-channel",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8(status, NPCM7xxKCSChannel),
        VMSTATE_UINT8(dbbout, NPCM7xxKCSChannel),
        VMSTATE_UINT8(dbbin, NPCM7xxKCSChannel),
        VMSTATE_UINT8(ctl, NPCM7xxKCSChannel),
        VMSTATE_UINT8(ic, NPCM7xxKCSChannel),
        VMSTATE_UINT8(ie, NPCM7xxKCSChannel),
        VMSTATE_END_OF_LIST(),
    },
};

static void npcm7xx_kcs_channel_realize(DeviceState *dev, Error **errp)
{
    IPMIInterface *ii = IPMI_INTERFACE(dev);
    NPCM7xxKCSChannel *c = NPCM7XX_KCS_CHANNEL(ii);

    if (c->host) {
        c->host->intf = ii;
    }
}

static void *npcm7xx_kcs_get_backend_data(IPMIInterface *ii)
{
    NPCM7xxKCSChannel *c = NPCM7XX_KCS_CHANNEL(ii);

    return c;
}

static void npcm7xx_kcs_set_ipmi_handler(IPMIInterface *ii, IPMICore *h)
{
    NPCM7xxKCSChannel *c = NPCM7XX_KCS_CHANNEL(ii);

    c->host = h;
}

static void npcm7xx_kcs_set_atn(struct IPMIInterface *s, int val, int irq)
{
}

static void npcm7xx_kcs_channel_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    IPMIInterfaceClass *iic = IPMI_INTERFACE_CLASS(klass);

    QEMU_BUILD_BUG_ON(NPCM7XX_KCS_REGS_END != NPCM7XX_KCS_CHANNEL_NR_REGS);

    dc->desc = "NPCM7xx KCS Channel";
    dc->vmsd = &vmstate_npcm7xx_kcs_channel;
    dc->realize = npcm7xx_kcs_channel_realize;

    iic->get_backend_data = npcm7xx_kcs_get_backend_data;
    iic->handle_msg = npcm7xx_kcs_handle_req;
    iic->set_ipmi_handler = npcm7xx_kcs_set_ipmi_handler;
    iic->set_atn = npcm7xx_kcs_set_atn;
}

static const VMStateDescription vmstate_npcm7xx_bpc = {
    .name = "npcm7xx-bpc",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8_ARRAY(regs, NPCM7xxBPC, NPCM7XX_BPC_NR_REGS),
        VMSTATE_FIFO8(data_fifo, NPCM7xxBPC),
        VMSTATE_FIFO8(addr_fifo, NPCM7xxBPC),
        VMSTATE_UINT16(addr, NPCM7xxBPC),
        VMSTATE_END_OF_LIST(),
    },
};

static void npcm7xx_bpc_reset(NPCM7xxBPC *b)
{
    for (int i = 0; i < NPCM7XX_BPC_NR_REGS; ++i) {
        b->regs[i] = 0;
    }
    b->regs[NPCM7XX_BPCFA1L] = NPCM7XX_BPCFA1L_DEFAULT;
    fifo8_reset(&b->data_fifo);
    fifo8_reset(&b->addr_fifo);
}

static void npcm7xx_bpc_init(Object *obj)
{
    NPCM7xxBPC *b = NPCM7XX_BPC(obj);

    fifo8_create(&b->data_fifo, NPCM7XX_BPC_FIFO_SIZE);
    fifo8_create(&b->addr_fifo, NPCM7XX_BPC_FIFO_SIZE);
    object_property_add_uint16_ptr(obj, "addr", &b->addr, OBJ_PROP_FLAG_WRITE);
    object_property_add(obj, "data", "uint32", NULL, npcm7xx_bpc_send_data,
                        NULL, NULL);
}

static void npcm7xx_bpc_finalize(Object *obj)
{
    NPCM7xxBPC *b = NPCM7XX_BPC(obj);

    fifo8_destroy(&b->data_fifo);
    fifo8_destroy(&b->addr_fifo);
}

static void npcm7xx_bpc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "NPCM7xx BPC Module";
    dc->vmsd = &vmstate_npcm7xx_bpc;
}

static void npcm7xx_kcs_enter_reset(Object *obj, ResetType type)
{
    NPCM7xxKCSState *s = NPCM7XX_KCS(obj);
    int i;

    for (i = 0; i < NPCM7XX_KCS_NR_CHANNELS; i++) {
        NPCM7xxKCSChannel *c = &s->channels[i];

        c->status = 0x00;
        c->ctl = 0xc0;
        c->ic = 0x41;
        c->ie = 0x00;
        c->inpos = 0x00;
        c->inlen = 0x00;
        c->outpos = 0x00;
        c->outlen = 0x00;
        c->last_byte_not_ready = false;
    }
    npcm7xx_bpc_reset(&s->bpc);
}

static void npcm7xx_kcs_hold_reset(Object *obj)
{
    NPCM7xxKCSState *s = NPCM7XX_KCS(obj);

    qemu_irq_lower(s->irq);
}

static const VMStateDescription vmstate_npcm7xx_kcs = {
    .name = "npcm7xx-kcs",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_END_OF_LIST(),
    },
};

static void npcm7xx_kcs_realize(DeviceState *dev, Error **errp)
{
    NPCM7xxKCSState *s = NPCM7XX_KCS(dev);
    SysBusDevice *sbd = &s->parent;
    int i;

    memory_region_init_io(&s->iomem, OBJECT(s), &npcm7xx_kcs_ops, s,
                          TYPE_NPCM7XX_KCS, 4 * KiB);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    for (i = 0; i < NPCM7XX_KCS_NR_CHANNELS; i++) {
        s->channels[i].owner = s;
        if (!qdev_realize(DEVICE(&s->channels[i]), NULL, errp)) {
            return;
        }
    }
    s->bpc.owner = s;
    if (!qdev_realize(DEVICE(&s->bpc), NULL, errp)) {
        return;
    }
}

static void npcm7xx_kcs_init(Object *obj)
{
    NPCM7xxKCSState *s = NPCM7XX_KCS(obj);
    int i;

    for (i = 0; i < NPCM7XX_KCS_NR_CHANNELS; i++) {
        object_initialize_child(obj, g_strdup_printf("channels[%d]", i),
                &s->channels[i], TYPE_NPCM7XX_KCS_CHANNEL);
    }
    object_initialize_child(obj, "bpc", &s->bpc, TYPE_NPCM7XX_BPC);
}

static void npcm7xx_kcs_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "NPCM7xx KCS";
    dc->vmsd = &vmstate_npcm7xx_kcs;
    dc->realize = npcm7xx_kcs_realize;
    rc->phases.enter = npcm7xx_kcs_enter_reset;
    rc->phases.hold = npcm7xx_kcs_hold_reset;
}

static const TypeInfo npcm7xx_kcs_infos[] = {
    {
        .name               = TYPE_NPCM7XX_KCS_CHANNEL,
        .parent             = TYPE_DEVICE,
        .instance_size      = sizeof(NPCM7xxKCSChannel),
        .class_init         = npcm7xx_kcs_channel_class_init,
        .interfaces         = (InterfaceInfo[]) {
            { TYPE_IPMI_INTERFACE },
            { },
        },
    },
    {
        .name               = TYPE_NPCM7XX_BPC,
        .parent             = TYPE_DEVICE,
        .instance_size      = sizeof(NPCM7xxBPC),
        .class_init         = npcm7xx_bpc_class_init,
        .instance_init      = npcm7xx_bpc_init,
        .instance_finalize  = npcm7xx_bpc_finalize,
    },
    {
        .name               = TYPE_NPCM7XX_KCS,
        .parent             = TYPE_SYS_BUS_DEVICE,
        .instance_size      = sizeof(NPCM7xxKCSState),
        .class_init         = npcm7xx_kcs_class_init,
        .instance_init      = npcm7xx_kcs_init,
    },
};
DEFINE_TYPES(npcm7xx_kcs_infos);
