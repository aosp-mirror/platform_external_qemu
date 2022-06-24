/*
 * Guest Only PCI Device
 *
 * Copyright 2022 Google LLC
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
#include "qemu/log.h"
#include "qapi/error.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_device.h"
#include "hw/qdev-properties-system.h"
#include "migration/vmstate.h"
#include "hw/registerfields.h"
#include "hw/pci/pci_regs.h"
#include "trace.h"

#define TYPE_GUEST_ONLY_PCI "guest-only-pci"
OBJECT_DECLARE_SIMPLE_TYPE(GuestOnlyPci, GUEST_ONLY_PCI)

typedef struct GuestOnlyPci {
    PCIDevice parent;

    /* PCI config properties */
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_device_id;
    uint32_t class_revision;
} GuestOnlyPci;

static void guest_only_pci_realize(PCIDevice *p, Error **errp)
{
    GuestOnlyPci *s = GUEST_ONLY_PCI(p);

    if (s->vendor_id == 0xffff) {
        error_setg(errp, "Vendor ID invalid, it must always be supplied");
        return;
    }
    if (s->device_id == 0xffff) {
        error_setg(errp, "Device ID invalid, it must always be supplied");
        return;
    }

    pci_set_word(&p->config[PCI_VENDOR_ID], s->vendor_id);
    pci_set_word(&p->config[PCI_DEVICE_ID], s->device_id);
    pci_set_word(&p->config[PCI_SUBSYSTEM_VENDOR_ID], s->subsystem_vendor_id);
    pci_set_word(&p->config[PCI_SUBSYSTEM_ID], s->subsystem_device_id);
    pci_set_long(&p->config[PCI_CLASS_REVISION], s->class_revision);
}

static const VMStateDescription vmstate_guest_only_pci = {
    .name = "guest_only_pci",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_PCI_DEVICE(parent, GuestOnlyPci),
        VMSTATE_END_OF_LIST()
    }
};

static Property guest_only_pci_properties[] = {
    DEFINE_PROP_UINT16("vendor-id", GuestOnlyPci, vendor_id, 0xffff),
    DEFINE_PROP_UINT16("device-id", GuestOnlyPci, device_id, 0xffff),
    DEFINE_PROP_UINT16("subsystem-vendor-id", GuestOnlyPci,
                       subsystem_vendor_id, 0),
    DEFINE_PROP_UINT16("subsystem-device-id", GuestOnlyPci,
                       subsystem_device_id, 0),
    DEFINE_PROP_UINT32("class-revision", GuestOnlyPci, class_revision,
                       0xff000000 /* Unknown class */),
    DEFINE_PROP_END_OF_LIST(),
};

static void guest_only_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *pdc = PCI_DEVICE_CLASS(klass);

    dc->vmsd = &vmstate_guest_only_pci;
    device_class_set_props(dc, guest_only_pci_properties);
    pdc->realize = guest_only_pci_realize;
}

static const TypeInfo guest_only_pci_types[] = {
    {
        .name = TYPE_GUEST_ONLY_PCI,
        .parent = TYPE_PCI_DEVICE,
        .instance_size = sizeof(GuestOnlyPci),
        .class_init = guest_only_pci_class_init,
        .interfaces = (InterfaceInfo[]) {
            { INTERFACE_PCIE_DEVICE },
            { }
        }
    },
};
DEFINE_TYPES(guest_only_pci_types)
