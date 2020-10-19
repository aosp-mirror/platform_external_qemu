/*
 * IPMI Host external connection
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

/*
 * This is designed to connect to a host QEMU VM that runs ipmi_bmc_extern.
 */

#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "chardev/char-fe.h"
#include "hw/ipmi/ipmi.h"
#include "hw/ipmi/ipmi_extern.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "migration/vmstate.h"
#include "qom/object.h"

#define TYPE_IPMI_HOST_EXTERN "ipmi-host-extern"
OBJECT_DECLARE_SIMPLE_TYPE(IPMIHostExtern, IPMI_HOST_EXTERN)

typedef struct IPMIHostExtern {
    IPMICore parent;

    IPMIExtern conn;

    IPMIInterface *responder;

    uint8_t capability;
} IPMIHostExtern;

/*
 * Handle a command (typically IPMI response) from IPMI interface
 * and send it out to the external host.
 */
static void ipmi_host_extern_handle_command(IPMICore *h, uint8_t *cmd,
        unsigned cmd_len, unsigned max_cmd_len, uint8_t msg_id)
{
    IPMIHostExtern *ihe = IPMI_HOST_EXTERN(h);

    ipmi_extern_handle_command(&ihe->conn, cmd, cmd_len, max_cmd_len, msg_id);
}

/* This function handles a control command from the host. */
static void ipmi_host_extern_handle_hw_op(IPMICore *ic, uint8_t cmd,
                                          uint8_t operand)
{
    IPMIHostExtern *ihe = IPMI_HOST_EXTERN(ic);

    switch (cmd) {
    case VM_CMD_VERSION:
        /* The host informs us the protocol version. */
        if (unlikely(operand != VM_PROTOCOL_VERSION)) {
            ipmi_debug("Host protocol version %u is different from our version"
                    " %u\n", operand, VM_PROTOCOL_VERSION);
        }
        break;

    case VM_CMD_RESET:
        /* The host tells us a reset has happened. */
        break;

    case VM_CMD_CAPABILITIES:
        /* The host tells us its capability. */
        ihe->capability = operand;
        break;

    default:
        /* The host shouldn't send us this command. Just ignore if they do. */
        ipmi_debug("Host cmd type %02x is invalid.\n", cmd);
        break;
    }
}

static void ipmi_host_extern_realize(DeviceState *dev, Error **errp)
{
    IPMIHostExtern *ihe = IPMI_HOST_EXTERN(dev);
    IPMIInterfaceClass *rk;

    if (ihe->responder == NULL) {
        error_setg(errp, "IPMI host extern requires responder attribute");
        return;
    }
    rk = IPMI_INTERFACE_GET_CLASS(ihe->responder);
    rk->set_ipmi_handler(ihe->responder, IPMI_CORE(ihe));
    ihe->conn.core->intf = ihe->responder;

    if (!qdev_realize(DEVICE(&ihe->conn), NULL, errp)) {
        return;
    }
}

static void ipmi_host_extern_init(Object *obj)
{
    IPMICore *ic = IPMI_CORE(obj);
    IPMIHostExtern *ihe = IPMI_HOST_EXTERN(obj);

    object_initialize_child(obj, "extern", &ihe->conn,
                            TYPE_IPMI_EXTERN);
    ihe->conn.core = ic;
    ihe->conn.bmc_side = true;
}

static Property ipmi_host_extern_properties[] = {
    DEFINE_PROP_CHR("chardev", IPMIHostExtern, conn.chr),
    DEFINE_PROP_LINK("responder", IPMIHostExtern, responder,
                     TYPE_IPMI_INTERFACE, IPMIInterface *),
    DEFINE_PROP_END_OF_LIST(),
};

static void ipmi_host_extern_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    IPMICoreClass *ck = IPMI_CORE_CLASS(oc);

    device_class_set_props(dc, ipmi_host_extern_properties);

    ck->handle_command = ipmi_host_extern_handle_command;
    ck->handle_hw_op = ipmi_host_extern_handle_hw_op;
    dc->hotpluggable = false;
    dc->realize = ipmi_host_extern_realize;
}

static const TypeInfo ipmi_host_extern_type = {
    .name          = TYPE_IPMI_HOST_EXTERN,
    .parent        = TYPE_IPMI_CORE,
    .instance_size = sizeof(IPMIHostExtern),
    .instance_init = ipmi_host_extern_init,
    .class_init    = ipmi_host_extern_class_init,
};

static void ipmi_host_extern_register_types(void)
{
    type_register_static(&ipmi_host_extern_type);
}

type_init(ipmi_host_extern_register_types)
