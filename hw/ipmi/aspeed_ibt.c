/*
 * ASPEED iBT Device
 *
 * Copyright (c) 2016-2021 Cédric Le Goater, IBM Corporation.
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/ipmi/aspeed_ibt.h"
#include "hw/ipmi/ipmi_extern.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qemu/bitops.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "sysemu/qtest.h"
#include "sysemu/sysemu.h"
#include "trace.h"

static void aspeed_ibt_dump_msg(const char *func, unsigned char *msg,
                                unsigned int len)
{
    if (trace_event_get_state_backends(TRACE_ASPEED_IBT_CHR_DUMP_MSG)) {
        int size = len * 3 + 1;
        char tmp[size];
        int i, n = 0;

        for (i = 0; i < len; i++) {
            n += snprintf(tmp + n, size - n, "%02x:", msg[i]);
        }
        tmp[size - 1] = 0;

        trace_aspeed_ibt_chr_dump_msg(func, tmp, len);
    }
}

/* convert and send IPMI message from BMC to remote host */
static void aspeed_ibt_b2h_write(AspeedIBTState *ibt)
{
    IPMICoreClass *hk = IPMI_CORE_GET_CLASS(ibt->handler);

    aspeed_ibt_dump_msg(__func__, ibt->send_msg, ibt->send_msg_len);
    /* separate seq from raw contents */
    ibt->msg_id = ibt->send_msg[2];
    ibt->send_msg[2] = ibt->send_msg[1];
    /* length and seq not included in vm message */
    hk->handle_command(ibt->handler, ibt->send_msg + 2,
                        ibt->send_msg_len - 2, MAX_IPMI_MSG_SIZE,
                        ibt->msg_id);
}


/* BMC firmware writing to memory region */
static void aspeed_ibt_write(void *opaque, hwaddr offset, uint64_t data,
                             uint32_t size)
{
    AspeedIBTState *ibt = ASPEED_IBT(opaque);


    trace_aspeed_ibt_write(offset, data);
    switch (offset) {
    case BT_CTRL:
        /* CLR_WR_PTR: cleared before a message is written */
        if (data & BT_CTRL_CLR_WR_PTR) {
            memset(ibt->send_msg, 0, sizeof(ibt->send_msg));
            ibt->send_msg_len = 0;
            trace_aspeed_ibt_event("CLR_WR_PTR");
        }

        /* CLR_RD_PTR: cleared before a message is read */
        else if (data & BT_CTRL_CLR_RD_PTR) {
            ibt->recv_msg_index = -1;
            trace_aspeed_ibt_event("CLR_RD_PTR");
        }

        /*
         * H2B_ATN: raised by host to end message, cleared by BMC
         * before reading message
         */
        else if (data & BT_CTRL_H2B_ATN) {
            ibt->regs[TO_REG(BT_CTRL)] &= ~BT_CTRL_H2B_ATN;
            trace_aspeed_ibt_event("H2B_ATN");
        }

        /* B_BUSY: raised and cleared by BMC when message is read */
        else if (data & BT_CTRL_B_BUSY) {
            ibt->regs[TO_REG(BT_CTRL)] ^= BT_CTRL_B_BUSY;
            trace_aspeed_ibt_event("B_BUSY");
        }

        /*
         * B2H_ATN: raised by BMC and cleared by host
         *
         * Also simulate the host busy bit which is set while the host
         * is reading the message from the BMC
         */
        else if (data & BT_CTRL_B2H_ATN) {
            trace_aspeed_ibt_event("B2H_ATN");
            ibt->regs[TO_REG(BT_CTRL)] |= (BT_CTRL_B2H_ATN | BT_CTRL_H_BUSY);

            aspeed_ibt_b2h_write(ibt);

            ibt->regs[TO_REG(BT_CTRL)] &= ~(BT_CTRL_B2H_ATN | BT_CTRL_H_BUSY);
        }

        /* Anything else is unexpected */
        else {
            qemu_log_mask(LOG_GUEST_ERROR, "%s: unexpected CTRL setting\n",
                          __func__);
        }

        /* Message was read by BMC. we can reset the receive state */
        if (!(ibt->regs[TO_REG(BT_CTRL)] & BT_CTRL_B_BUSY)) {
            trace_aspeed_ibt_event("B_BUSY cleared");
            ibt->recv_msg_len = 0;
        }
        break;

    case BT_BMC2HOST:
        if (ibt->send_msg_len < sizeof(ibt->send_msg)) {
            trace_aspeed_ibt_event("BMC2HOST");
            ibt->send_msg[ibt->send_msg_len++] = data & 0xff;
        }
        break;

    case BT_CR0: /* TODO: iBT config */
    case BT_CR1: /* interrupt enable */
    case BT_CR3: /* unused */
    case BT_INTMASK:
        ibt->regs[TO_REG(offset)] = (uint32_t) data;
        break;

    case BT_CR2: /* interrupt status. writing 1 clears. */
        ibt->regs[TO_REG(offset)] ^= (uint32_t) data;
        qemu_irq_lower(ibt->irq);
        break;

    default:
        qemu_log_mask(LOG_UNIMP, "%s: not implemented 0x%" HWADDR_PRIx "\n",
                      __func__, offset);
        break;
    }
}

/* BMC firmware reading from memory region */
static uint64_t aspeed_ibt_read(void *opaque, hwaddr offset, unsigned size)
{
    AspeedIBTState *ibt = ASPEED_IBT(opaque);
    uint64_t val = 0;

    switch (offset) {
    case BT_HOST2BMC: /* shares offset with B2H */
        trace_aspeed_ibt_event("HOST2BMC");
        /*
         * The IPMI BT interface requires the first byte to be the
         * length of the message
         */
        if (ibt->recv_msg_index == -1) {
            val = ibt->recv_msg_len;
            ibt->recv_msg_index++;
        } else if (ibt->recv_msg_index < ibt->recv_msg_len) {
            val = ibt->recv_msg[ibt->recv_msg_index++];
        }
        break;

    case BT_CR0:
    case BT_CR1:
    case BT_CR2:
    case BT_CR3:
    case BT_CTRL:
    case BT_INTMASK:
        val = ibt->regs[TO_REG(offset)];
        break;

    default:
        qemu_log_mask(LOG_UNIMP, "%s: not implemented 0x%" HWADDR_PRIx "\n",
                      __func__, offset);
        return 0;
    }

    trace_aspeed_ibt_read(offset, val);
    return val;
}

/* send a request from a host to the BMC core */
static void aspeed_ibt_handle_msg(IPMIInterface *ii, uint8_t msg_id,
                                 unsigned char *req, unsigned int req_len)
{
    AspeedIBTState *ibt = ASPEED_IBT(ii);

    if (req_len == 0) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: zero length request", __func__);
        return;
    }

    if (req_len > ASPEED_IBT_BUFFER_SIZE - 1) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: request of %d bytes is too long",
                      __func__, req_len);
        req_len = ASPEED_IBT_BUFFER_SIZE - 1;
    }

    /* include length and reuse msg_id as seq in message */
    ibt->recv_msg[0] = req[0];
    ibt->recv_msg[1] = msg_id;
    memcpy(ibt->recv_msg + 2, req + 1, req_len - 1);
    ibt->recv_msg_len = req_len + 1;


    ibt->regs[TO_REG(BT_CTRL)] |= BT_CTRL_H2B_ATN;
    aspeed_ibt_dump_msg(__func__, ibt->recv_msg, ibt->recv_msg_len);

}

static void *aspeed_ibt_backend_data(IPMIInterface *ii)
{
    return ii;
}

static void aspeed_ibt_set_ipmi_handler(IPMIInterface *ii, IPMICore *h)
{
    AspeedIBTState *ibt = ASPEED_IBT(ii);
    ibt->handler = h;
}

static const MemoryRegionOps aspeed_ibt_ops = {
    .read = aspeed_ibt_read,
    .write = aspeed_ibt_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};

static void aspeed_ibt_enter_reset(Object *obj, ResetType type)
{
    AspeedIBTState *ibt = ASPEED_IBT(obj);

    memset(ibt->regs, 0, sizeof(ibt->regs));

    memset(ibt->recv_msg, 0, sizeof(ibt->recv_msg));
    ibt->recv_msg_len = 0;
    ibt->recv_msg_index = -1;

    memset(ibt->send_msg, 0, sizeof(ibt->send_msg));
    ibt->send_msg_len = 0;
}


static void aspeed_ibt_realize(DeviceState *dev, Error **errp)
{
    AspeedIBTState *ibt = ASPEED_IBT(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    IPMIInterface *ii = IPMI_INTERFACE(dev);

    sysbus_init_irq(sbd, &ibt->irq);
    memory_region_init_io(&ibt->iomem, OBJECT(ibt), &aspeed_ibt_ops, ibt,
                          TYPE_ASPEED_IBT, ASPEED_IBT_IO_REGION_SIZE);
    sysbus_init_mmio(sbd, &ibt->iomem);


    if (ibt->handler) {
        ibt->handler->intf = ii;
    }
}

static const VMStateDescription vmstate_aspeed_ibt = {
    .name = TYPE_ASPEED_IBT,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, AspeedIBTState, ASPEED_IBT_NR_REGS),
        VMSTATE_END_OF_LIST()
    }
};

static void aspeed_ibt_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    IPMIInterfaceClass *iic = IPMI_INTERFACE_CLASS(klass);

    dc->desc = "ASPEED IPMI BT Host Controller";
    dc->vmsd = &vmstate_aspeed_ibt;
    dc->realize = aspeed_ibt_realize;
    rc->phases.enter = aspeed_ibt_enter_reset;

    iic->handle_msg = aspeed_ibt_handle_msg;
    iic->get_backend_data = aspeed_ibt_backend_data;
    iic->set_ipmi_handler = aspeed_ibt_set_ipmi_handler;
}

static const TypeInfo aspeed_ibt_info[] = {
    {
        .name = TYPE_ASPEED_IBT,
        .parent = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(AspeedIBTState),
        .class_init = aspeed_ibt_class_init,
        .interfaces = (InterfaceInfo[]) {
            { TYPE_IPMI_INTERFACE },
            { },
        },
    },
};

DEFINE_TYPES(aspeed_ibt_info);
