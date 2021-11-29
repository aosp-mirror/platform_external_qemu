/*
 * PECI Client device
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/peci/peci.h"
#include "hw/qdev-core.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qemu/module.h"
#include "qemu/log.h"
#include "qom/object.h"

/*
 * A PECI client represents an Intel socket and the peripherals attached to it
 * that are accessible over the PECI bus.
 */

#define PECI_CLIENT_DEFAULT_TEMP 30

static void peci_client_update_temps(PECIClientDevice *client)
{
    uint8_t temp_cpu = 0;
    for (size_t i = 0; i < client->cpus; i++) {
        if (temp_cpu < client->core_temp[i]) {
            temp_cpu = client->core_temp[i];
        }
    }
    client->core_temp_max = -1 * (client->tjmax - temp_cpu);

    uint8_t temp_dimm = 0;
    for (size_t i = 0; i < client->dimms; i++) {
        if (temp_dimm < client->dimm_temp[i]) {
            temp_dimm = client->dimm_temp[i];
        }
    }
    client->dimm_temp_max = temp_dimm;
}

PECIClientDevice *peci_get_client(PECIBus *bus, uint8_t addr)
{
    PECIClientDevice *client;
    BusChild *var;

    QTAILQ_FOREACH(var, &bus->qbus.children, sibling) {
        DeviceState *dev = var->child;
        client = PECI_CLIENT(dev);

        if (client->address == addr) {
            return client;
        }
    }
    return 0;
}


PECIClientDevice *peci_add_client(PECIBus *bus,
                                  uint8_t address,
                                  PECIClientProperties *props)
{
    DeviceState *dev = qdev_new("peci-client");
    PECIClientDevice *client;

    /* Only 8 addresses supported as of rev 4.1, 0x30 to 0x37 */
    if (address < PECI_BASE_ADDR || address > PECI_BASE_ADDR + 7) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: failed to add client at 0x%02x",
                      __func__, address);
        return 0;
    }

    client = PECI_CLIENT(dev);
    client->address = address;

    /* these fields are optional, get set to max if unset or invalid */
    if (!props->cpus || props->cpus > PECI_NUM_CPUS_MAX) {
        props->cpus = PECI_NUM_CPUS_MAX;
    }
    if (!props->dimms || props->dimms > PECI_NUM_DIMMS_MAX) {
        props->dimms = PECI_NUM_DIMMS_MAX;
    }
    if (!props->cpu_family) {
        props->cpu_family = FAM6_ICELAKE_X;
    }

    client->cpus = props->cpus;
    client->dimms = props->dimms;
    client->cpu_id = props->cpu_family;

    /* TODO: find real revisions and TJ max for supported families */
    client->tjmax = 90;
    client->tcontrol = -5;
    client->dimms_per_channel = 2;

    switch (props->cpu_family) {
    case FAM6_HASWELL_X:
        client->revision = 0x31;
        client->dimms_per_channel = 3;
        break;
    case FAM6_BROADWELL_X:
        client->revision = 0x32;
        client->dimms_per_channel = 3;
        break;
    case FAM6_SKYLAKE_X:
    case FAM6_SKYLAKE_XD:
        client->revision = 0x36;
        break;
    case FAM6_ICELAKE_X:
        client->revision = 0x40;
        client->dimms_per_channel = 2;
        break;
    default:
        qemu_log_mask(LOG_UNIMP, "%s: unsupported cpu: 0x%x",
                      __func__, props->cpu_family);
        client->revision = 0x31;
        break;
    }

    qdev_realize_and_unref(&client->qdev, &bus->qbus, &error_abort);
    bus->num_clients += 1;
    return client;
}

static void peci_client_get(Object *obj, Visitor *v, const char *name,
                                   void *opaque, Error **errp)
{
    /* use millidegrees convention */
    uint32_t value = *(uint8_t *)opaque * 1000;
    visit_type_uint32(v, name, &value, errp);
}

static void peci_client_set(Object *obj, Visitor *v, const char *name,
                                   void *opaque, Error **errp)
{
    PECIClientDevice *client = PECI_CLIENT(obj);
    uint8_t *internal = opaque;
    uint32_t value;

    if (!visit_type_uint32(v, name, &value, errp)) {
        return;
    }

    g_assert(value <= 255000);

    *internal = value / 1000;
    peci_client_update_temps(client);
}

static void peci_client_reset(Object *obj, ResetType type)
{
    PECIClientDevice *client = PECI_CLIENT(obj);
    client->core_temp_max = 0;
    client->dimm_temp_max = 0;
}

static const VMStateDescription vmstate_peci_client = {
    .name = TYPE_PECI_CLIENT,
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8(address, PECIClientDevice),
        VMSTATE_UINT8(device_info, PECIClientDevice),
        VMSTATE_UINT8(revision, PECIClientDevice),
        VMSTATE_UINT32(cpu_id, PECIClientDevice),
        VMSTATE_UINT8(cpus, PECIClientDevice),
        VMSTATE_UINT8(tjmax, PECIClientDevice),
        VMSTATE_INT8(tcontrol, PECIClientDevice),
        VMSTATE_UINT8_ARRAY(core_temp, PECIClientDevice, PECI_NUM_CPUS_MAX),
        VMSTATE_UINT8(dimms, PECIClientDevice),
        VMSTATE_UINT8(dimms_per_channel, PECIClientDevice),
        VMSTATE_UINT8_ARRAY(dimm_temp, PECIClientDevice , PECI_NUM_DIMMS_MAX),
        VMSTATE_END_OF_LIST()
    },
};

static void peci_client_realize(DeviceState *dev, Error **errp)
{
    PECIClientDevice *client = PECI_CLIENT(dev);
    for (size_t i = 0; i < client->cpus; i++) {
        client->core_temp[i] = PECI_CLIENT_DEFAULT_TEMP;
        object_property_add(OBJECT(client), "cpu_temp[*]", "uint8",
                            peci_client_get,
                            peci_client_set, NULL, &client->core_temp[i]);
    }

    for (size_t i = 0; i < client->dimms; i++) {
        client->dimm_temp[i] = PECI_CLIENT_DEFAULT_TEMP;
        object_property_add(OBJECT(client), "dimm_temp[*]", "uint8",
                            peci_client_get,
                            peci_client_set, NULL, &client->dimm_temp[i]);
    }
}

static void peci_client_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    dc->desc = "PECI Client Device";
    dc->bus_type = TYPE_PECI_BUS;
    dc->realize = peci_client_realize;
    dc->vmsd = &vmstate_peci_client;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    rc->phases.enter = peci_client_reset;
}

static const TypeInfo peci_client_info = {
    .name = TYPE_PECI_CLIENT,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(PECIClientDevice),
    .class_init = peci_client_class_init,
};

static void peci_client_register_types(void)
{
    type_register_static(&peci_client_info);
}

type_init(peci_client_register_types)
