/*
 * Silvaco I3C Controller
 *
 * Copyright (C) 2023 Google, LLC
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "hw/registerfields.h"
#include "hw/qdev-properties.h"
#include "qapi/error.h"
#include "hw/i3c/i3c.h"
#include "hw/i3c/svc-i3c.h"
#include "hw/irq.h"

static uint64_t svc_i3c_read(void *opaque, hwaddr offset, unsigned size)
{
    return 0;
}

static int svc_i3c_ibi_handle(I3CBus *bus, uint8_t addr, bool is_recv)
{
    return 0;
}

static int svc_i3c_ibi_recv(I3CBus *bus, uint8_t data)
{
    return 0;
}

static int svc_i3c_ibi_finish(I3CBus *bus)
{
    return 0;
}

static void svc_i3c_enter_reset(Object *obj, ResetType type)
{
}

static void svc_i3c_write(void *opaque, hwaddr offset, uint64_t value,
                         unsigned size)
{
}

static const MemoryRegionOps svc_i3c_ops = {
    .read = svc_i3c_read,
    .write = svc_i3c_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void svc_i3c_realize(DeviceState *dev, Error **errp)
{
    SVCI3C *s = SVC_I3C(dev);
    g_autofree char *name = g_strdup_printf(TYPE_SVC_I3C ".%d",
                                            s->cfg.id);

    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq);

    memory_region_init_io(&s->mr, OBJECT(s), &svc_i3c_ops,
                          s, name, SVC_I3C_NR_REGS << 2);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mr);

    s->bus = i3c_init_bus(DEVICE(s), name);
    I3CBusClass *bc = I3C_BUS_GET_CLASS(s->bus);
    bc->ibi_handle = svc_i3c_ibi_handle;
    bc->ibi_recv = svc_i3c_ibi_recv;
    bc->ibi_finish = svc_i3c_ibi_finish;
}

static Property svc_i3c_properties[] = {
    DEFINE_PROP_UINT8("device-id", SVCI3C, cfg.id, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void svc_i3c_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    dc->desc = "Silvaco I3C Controller";
    dc->realize = svc_i3c_realize;
    device_class_set_props(dc, svc_i3c_properties);

    rc->phases.enter = svc_i3c_enter_reset;
}

static const TypeInfo svc_i3c_info = {
    .name = TYPE_SVC_I3C,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SVCI3C),
    .class_init = svc_i3c_class_init,
};

static void svc_i3c_register_types(void)
{
    type_register_static(&svc_i3c_info);
}

type_init(svc_i3c_register_types);
