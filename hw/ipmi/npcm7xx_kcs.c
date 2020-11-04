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
 */

#include "qemu/osdep.h"
#include "hw/ipmi/npcm7xx_kcs.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
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

static uint64_t npcm7xx_kcs_read(void *opaque, hwaddr addr, unsigned int size)
{
    NPCM7xxKCSState *s = opaque;
    uint8_t value = 0;
    int channel;
    NPCM7xxKCSRegister reg;

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

static const TypeInfo npcm7xx_kcs_channel_info = {
    .name               = TYPE_NPCM7XX_KCS_CHANNEL,
    .parent             = TYPE_DEVICE,
    .instance_size      = sizeof(NPCM7xxKCSChannel),
    .class_init         = npcm7xx_kcs_channel_class_init,
    .interfaces         = (InterfaceInfo[]) {
        { TYPE_IPMI_INTERFACE },
        { },
    },
};

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
    }
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
}

static void npcm7xx_kcs_init(Object *obj)
{
    NPCM7xxKCSState *s = NPCM7XX_KCS(obj);
    int i;

    for (i = 0; i < NPCM7XX_KCS_NR_CHANNELS; i++) {
        object_initialize_child(obj, g_strdup_printf("channels[%d]", i),
                &s->channels[i], TYPE_NPCM7XX_KCS_CHANNEL);
    }
}

static void npcm7xx_kcs_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "NPCM7xx Timer Controller";
    dc->vmsd = &vmstate_npcm7xx_kcs;
    dc->realize = npcm7xx_kcs_realize;
    rc->phases.enter = npcm7xx_kcs_enter_reset;
    rc->phases.hold = npcm7xx_kcs_hold_reset;
}

static const TypeInfo npcm7xx_kcs_info = {
    .name               = TYPE_NPCM7XX_KCS,
    .parent             = TYPE_SYS_BUS_DEVICE,
    .instance_size      = sizeof(NPCM7xxKCSState),
    .class_init         = npcm7xx_kcs_class_init,
    .instance_init      = npcm7xx_kcs_init,
};

static void npcm7xx_kcs_register_type(void)
{
    type_register_static(&npcm7xx_kcs_channel_info);
    type_register_static(&npcm7xx_kcs_info);
}
type_init(npcm7xx_kcs_register_type);
