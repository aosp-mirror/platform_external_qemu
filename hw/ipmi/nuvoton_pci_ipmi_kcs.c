/*
 * Nuvoton Nuvoton PCI IPMI KCS implementation
 *
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qapi/error.h"
#include "qom/object.h"
#include "migration/vmstate.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_device.h"
#include "hw/isa/isa.h"
#include "hw/sysbus.h"
#include "hw/ipmi/ipmi_kcs.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"

/* By default the VID for the device belongs to Winbond, not Nuvoton */
#define PCI_VENDOR_ID_WINBOND 0x1050
#define PCI_DEVICE_ID_WINBOND_BPCI 0x750

#define TYPE_NPCM7XX_PCI_IPMI "nuvoton-pci-ipmi"
OBJECT_DECLARE_SIMPLE_TYPE(NuvotonPCIIPMIState, NPCM7XX_PCI_IPMI)

#define TYPE_NPCM7XX_IPMI_KCS "nuvoton-ipmi-kcs"
OBJECT_DECLARE_SIMPLE_TYPE(NuvotonIPMIKCSState, NPCM7XX_IPMI_KCS)

typedef struct NuvotonIPMIKCSState NuvotonIPMIKCSState;
typedef struct NuvotonPCIIPMIState NuvotonPCIIPMIState;

struct NuvotonPCIIPMIState {
    PCIDevice pcidev;
    NuvotonIPMIKCSState *ipmi_kcs_state;
};

struct NuvotonIPMIKCSState {
    ISADevice isadev;
    IPMIKCS kcs;
    qemu_irq irq;
    uint32_t uuid;

    struct {
        int32_t isairq;
        uint32_t io_base;
    } cfg;
};

static void nuvoton_ipmi_kcs_get_fwinfo(IPMIInterface *ii, IPMIFwInfo *info)
{
    NuvotonIPMIKCSState *s = NPCM7XX_IPMI_KCS(ii);

    ipmi_kcs_get_fwinfo(&s->kcs, info);
    info->interrupt_number = s->cfg.isairq;
    info->uuid = s->uuid;
}

static void nuvoton_ipmi_kcs_raise_irq(IPMIKCS *ik)
{
    NuvotonIPMIKCSState *s = ik->opaque;

    qemu_irq_raise(s->irq);
}

static void nuvoton_ipmi_kcs_lower_irq(IPMIKCS *ik)
{
    NuvotonIPMIKCSState *s = ik->opaque;

    qemu_irq_lower(s->irq);
}

static void *nuvoton_ipmi_kcs_get_backend_data(IPMIInterface *ii)
{
    NuvotonIPMIKCSState *s = NPCM7XX_IPMI_KCS(ii);;

    return &s->kcs;
}

static void nuvoton_pci_ipmi_realize(PCIDevice *pd, Error **errp)
{
    NuvotonPCIIPMIState *s = NPCM7XX_PCI_IPMI(pd);

    if (!s->ipmi_kcs_state) {
        error_setg(errp, "%s: IPMI KCS portion of device is not connected\n",
                   object_get_canonical_path(OBJECT(pd)));
    }

    pci_config_set_prog_interface(pd->config, 0x01);
    pci_config_set_interrupt_pin(pd->config, 0x01);
}

static void nuvoton_ipmi_kcs_realize(DeviceState *dev, Error **errp)
{
    Error *err = NULL;
    NuvotonIPMIKCSState *s = NPCM7XX_IPMI_KCS(dev);
    ISADevice *isadev = ISA_DEVICE(dev);
    IPMIInterface *ii = IPMI_INTERFACE(dev);
    IPMIInterfaceClass *iic = IPMI_INTERFACE_GET_CLASS(ii);
    IPMICore *ic;

    if (!s->kcs.bmc) {
        error_setg(errp, "%s: IPMI device requires a bmc attribute to be set",
                   object_get_canonical_path(OBJECT(dev)));
        return;
    }

    iic->init(ii, 8, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }

    ic = IPMI_CORE(s->kcs.bmc);
    ic->intf = ii;

    s->uuid = ipmi_next_uuid();
    s->kcs.io_base = s->cfg.io_base;
    s->kcs.opaque = s;

    if (s->cfg.isairq > 0) {
        s->irq = isa_get_irq(isadev, s->cfg.isairq);
        s->kcs.use_irq = 1;
        s->kcs.raise_irq = nuvoton_ipmi_kcs_raise_irq;
        s->kcs.lower_irq = nuvoton_ipmi_kcs_lower_irq;
    }

    qdev_set_legacy_instance_id(dev, s->kcs.io_base, s->kcs.io_length);
    isa_register_ioport(isadev, &s->kcs.io, s->kcs.io_base);
}


static void nuvoton_ipmi_kcs_init(Object *obj)
{
    NuvotonIPMIKCSState *s = NPCM7XX_IPMI_KCS(obj);

    ipmi_bmc_find_and_link(obj, (Object **) &s->kcs.bmc);
}

static const VMStateDescription vmstate_nuvoton_ipmi_kcs = {
    .name = TYPE_IPMI_INTERFACE_PREFIX "nuvoton-ipmi-kcs",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT(kcs, NuvotonIPMIKCSState, 1, vmstate_IPMIKCS, IPMIKCS),
        VMSTATE_UINT32(uuid, NuvotonIPMIKCSState),
        VMSTATE_END_OF_LIST()
    }
};

static Property nuvoton_pci_ipmi_properties[] = {
    DEFINE_PROP_LINK("ipmi-kcs", NuvotonPCIIPMIState, ipmi_kcs_state,
                     TYPE_NPCM7XX_IPMI_KCS, NuvotonIPMIKCSState *),
    DEFINE_PROP_END_OF_LIST(),
};

static void nuvoton_pci_ipmi_class_init(ObjectClass *oc, void *data)
{
    PCIDeviceClass *pdc = PCI_DEVICE_CLASS(oc);
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->vmsd = &vmstate_nuvoton_ipmi_kcs;
    device_class_set_props(dc, nuvoton_pci_ipmi_properties);

    pdc->vendor_id = PCI_VENDOR_ID_WINBOND;
    pdc->device_id = PCI_DEVICE_ID_WINBOND_BPCI;
    pdc->revision = 1;
    pdc->class_id = PCI_CLASS_SERIAL_IPMI;
    pdc->realize = nuvoton_pci_ipmi_realize;
}

static const VMStateDescription vmstate_nuvoton_pci_ipmi = {
    .name = "nuvoton-pci-ipmi",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_PCI_DEVICE(pcidev, NuvotonPCIIPMIState),
        VMSTATE_END_OF_LIST()
    }
};

static Property nuvoton_ipmi_kcs_properties[] = {
    DEFINE_PROP_UINT32("portio", NuvotonIPMIKCSState, cfg.io_base, 0x4e),
    DEFINE_PROP_INT32("isa-irq", NuvotonIPMIKCSState, cfg.isairq, 10),
    DEFINE_PROP_END_OF_LIST(),
};

static void nuvoton_ipmi_kcs_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    IPMIInterfaceClass *iic = IPMI_INTERFACE_CLASS(oc);

    dc->vmsd = &vmstate_nuvoton_pci_ipmi;
    dc->realize = nuvoton_ipmi_kcs_realize;
    device_class_set_props(dc, nuvoton_ipmi_kcs_properties);

    iic->get_backend_data = nuvoton_ipmi_kcs_get_backend_data;
    iic->get_fwinfo = nuvoton_ipmi_kcs_get_fwinfo;
    ipmi_kcs_class_init(iic);
}

static const TypeInfo nuvoton_ipmi_kcs_info = {
    .name          = TYPE_NPCM7XX_IPMI_KCS,
    .parent        = TYPE_ISA_DEVICE,
    .instance_size = sizeof(NuvotonIPMIKCSState),
    .instance_init = nuvoton_ipmi_kcs_init,
    .class_init    = nuvoton_ipmi_kcs_class_init,
    .interfaces    = (InterfaceInfo[]) {
        { TYPE_IPMI_INTERFACE },
        { },
    },
};

static const TypeInfo nuvoton_pci_ipmi_info = {
    .name          = TYPE_NPCM7XX_PCI_IPMI,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(NuvotonPCIIPMIState),
    .class_init    = nuvoton_pci_ipmi_class_init,
    .interfaces    = (InterfaceInfo[]) {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    },
};

static void nuvoton_pci_ipmi_kcs_register_types(void)
{
    type_register_static(&nuvoton_pci_ipmi_info);
    type_register_static(&nuvoton_ipmi_kcs_info);
}

type_init(nuvoton_pci_ipmi_kcs_register_types);
