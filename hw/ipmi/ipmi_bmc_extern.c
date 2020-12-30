/*
 * IPMI BMC external connection
 *
 * Copyright (c) 2015 Corey Minyard, MontaVista Software, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * This is designed to connect with OpenIPMI's lanserv serial interface
 * using the "VM" connection type.  See that for details.
 */

#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "qemu/timer.h"
#include "chardev/char-fe.h"
#include "hw/ipmi/ipmi.h"
#include "hw/ipmi/ipmi_extern.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "migration/vmstate.h"
#include "qom/object.h"

#define TYPE_IPMI_BMC_EXTERN "ipmi-bmc-extern"
OBJECT_DECLARE_SIMPLE_TYPE(IPMIBmcExtern, IPMI_BMC_EXTERN)

struct IPMIBmcExtern {
    IPMIBmc parent;

    IPMIExtern conn;

    /* A reset event is pending to be sent upstream. */
    bool send_reset;
};

static void continue_send_bmc(IPMIBmcExtern *ibe)
{
    if (continue_send(&ibe->conn)) {
        if (ibe->conn.connected && ibe->send_reset) {
            /* Send the reset */
            ibe->conn.outbuf[0] = VM_CMD_RESET;
            ibe->conn.outbuf[1] = VM_CMD_CHAR;
            ibe->conn.outlen = 2;
            ibe->conn.outpos = 0;
            ibe->send_reset = false;
            ibe->conn.sending_cmd = true;
            continue_send(&ibe->conn);
        }
    }
}

static void ipmi_bmc_handle_hw_op(IPMICore *ic, unsigned char hw_op,
                                  uint8_t operand)
{
    IPMIInterface *s = ic->intf;
    IPMIInterfaceClass *k = IPMI_INTERFACE_GET_CLASS(s);

    switch (hw_op) {
    case VM_CMD_VERSION:
        /* We only support one version at this time. */
        break;

    case VM_CMD_NOATTN:
        k->set_atn(s, 0, 0);
        break;

    case VM_CMD_ATTN:
        k->set_atn(s, 1, 0);
        break;

    case VM_CMD_ATTN_IRQ:
        k->set_atn(s, 1, 1);
        break;

    case VM_CMD_POWEROFF:
        k->do_hw_op(s, IPMI_POWEROFF_CHASSIS, 0);
        break;

    case VM_CMD_RESET:
        k->do_hw_op(s, IPMI_RESET_CHASSIS, 0);
        break;

    case VM_CMD_ENABLE_IRQ:
        k->set_irq_enable(s, 1);
        break;

    case VM_CMD_DISABLE_IRQ:
        k->set_irq_enable(s, 0);
        break;

    case VM_CMD_SEND_NMI:
        k->do_hw_op(s, IPMI_SEND_NMI, 0);
        break;

    case VM_CMD_GRACEFUL_SHUTDOWN:
        k->do_hw_op(s, IPMI_SHUTDOWN_VIA_ACPI_OVERTEMP, 0);
        break;
    }
}

static void ipmi_bmc_extern_handle_command(IPMIBmc *b,
                                       uint8_t *cmd, unsigned int cmd_len,
                                       unsigned int max_cmd_len,
                                       uint8_t msg_id)
{
    IPMIBmcExtern *ibe = IPMI_BMC_EXTERN(b);

    ipmi_extern_handle_command(&ibe->conn, cmd, cmd_len, max_cmd_len, msg_id);
}

static void ipmi_bmc_extern_realize(DeviceState *dev, Error **errp)
{
    IPMIBmcExtern *ibe = IPMI_BMC_EXTERN(dev);

    if (!qdev_realize(DEVICE(&ibe->conn), NULL, errp)) {
        return;
    }
}

static void ipmi_bmc_extern_handle_reset(IPMIBmc *b)
{
    IPMIBmcExtern *ibe = IPMI_BMC_EXTERN(b);

    ibe->send_reset = true;
    continue_send_bmc(ibe);
}

static void ipmi_bmc_extern_init(Object *obj)
{
    IPMICore *ic = IPMI_CORE(obj);
    IPMIBmcExtern *ibe = IPMI_BMC_EXTERN(obj);

    object_initialize_child(obj, "extern", &ibe->conn,
                            TYPE_IPMI_EXTERN);
    ibe->conn.core = ic;
}

static Property ipmi_bmc_extern_properties[] = {
    DEFINE_PROP_CHR("chardev", IPMIBmcExtern, conn.chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void ipmi_bmc_extern_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    IPMIBmcClass *bk = IPMI_BMC_CLASS(oc);
    IPMICoreClass *ck = IPMI_CORE_CLASS(oc);

    bk->handle_command = ipmi_bmc_extern_handle_command;
    bk->handle_reset = ipmi_bmc_extern_handle_reset;
    ck->handle_hw_op = ipmi_bmc_handle_hw_op;
    dc->hotpluggable = false;
    dc->realize = ipmi_bmc_extern_realize;
    device_class_set_props(dc, ipmi_bmc_extern_properties);
}

static const TypeInfo ipmi_bmc_extern_type = {
    .name          = TYPE_IPMI_BMC_EXTERN,
    .parent        = TYPE_IPMI_BMC,
    .instance_size = sizeof(IPMIBmcExtern),
    .instance_init = ipmi_bmc_extern_init,
    .class_init    = ipmi_bmc_extern_class_init,
};

static void ipmi_bmc_extern_register_types(void)
{
    type_register_static(&ipmi_bmc_extern_type);
}

type_init(ipmi_bmc_extern_register_types)
