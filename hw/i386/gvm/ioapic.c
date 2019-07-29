/*
 * GVM in-kernel IOPIC support
 *
 * Copyright (c) 2011 Siemens AG
 *
 * Authors:
 *  Jan Kiszka          <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL version 2.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "monitor/monitor.h"
#include "hw/i386/pc.h"
#include "hw/i386/ioapic_internal.h"
#include "hw/i386/apic_internal.h"
#include "sysemu/gvm.h"
#include "sysemu/gvm-interface.h"

/* PC Utility function */
void gvm_pc_setup_irq_routing(bool pci_enabled)
{
    GVMState *s = gvm_state;
    int i;

    for (i = 0; i < 8; ++i) {
        if (i == 2) {
            continue;
        }
        gvm_irqchip_add_irq_route(s, i, GVM_IRQCHIP_PIC_MASTER, i);
    }
    for (i = 8; i < 16; ++i) {
        gvm_irqchip_add_irq_route(s, i, GVM_IRQCHIP_PIC_SLAVE, i - 8);
    }
    if (pci_enabled) {
        for (i = 0; i < 24; ++i) {
            if (i == 0) {
                gvm_irqchip_add_irq_route(s, i, GVM_IRQCHIP_IOAPIC, 2);
            } else if (i != 2) {
                gvm_irqchip_add_irq_route(s, i, GVM_IRQCHIP_IOAPIC, i);
            }
        }
    }
    gvm_irqchip_commit_routes(s);
}

void gvm_pc_gsi_handler(void *opaque, int n, int level)
{
    GSIState *s = opaque;

    if (n < ISA_NUM_IRQS) {
        /* Kernel will forward to both PIC and IOAPIC */
        qemu_set_irq(s->i8259_irq[n], level);
    } else {
        qemu_set_irq(s->ioapic_irq[n], level);
    }
}

typedef struct GVMIOAPICState GVMIOAPICState;

struct GVMIOAPICState {
    IOAPICCommonState ioapic;
    uint32_t gvm_gsi_base;
};

static void gvm_ioapic_get(IOAPICCommonState *s)
{
    struct gvm_irqchip chip;
    struct gvm_ioapic_state *kioapic;
    int ret, i;

    chip.chip_id = GVM_IRQCHIP_IOAPIC;
    ret = gvm_vm_ioctl(gvm_state, GVM_GET_IRQCHIP,
            &chip, sizeof(chip), NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "GVM_GET_IRQCHIP failed: %s\n", strerror(ret));
        abort();
    }

    kioapic = &chip.chip.ioapic;

    s->id = kioapic->id;
    s->ioregsel = kioapic->ioregsel;
    s->irr = kioapic->irr;
    for (i = 0; i < IOAPIC_NUM_PINS; i++) {
        s->ioredtbl[i] = kioapic->redirtbl[i].bits;
    }
}

static void gvm_ioapic_put(IOAPICCommonState *s)
{
    struct gvm_irqchip chip;
    struct gvm_ioapic_state *kioapic;
    int ret, i;

    chip.chip_id = GVM_IRQCHIP_IOAPIC;
    kioapic = &chip.chip.ioapic;

    kioapic->id = s->id;
    kioapic->ioregsel = s->ioregsel;
    kioapic->base_address = s->busdev.mmio[0].addr;
    kioapic->irr = s->irr;
    for (i = 0; i < IOAPIC_NUM_PINS; i++) {
        kioapic->redirtbl[i].bits = s->ioredtbl[i];
    }

    ret = gvm_vm_ioctl(gvm_state, GVM_SET_IRQCHIP,
            &chip, sizeof(chip), NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "GVM_GET_IRQCHIP failed: %s\n", strerror(ret));
        abort();
    }
}

void gvm_ioapic_dump_state(Monitor *mon, const QDict *qdict)
{
    IOAPICCommonState s;

    gvm_ioapic_get(&s);

    ioapic_print_redtbl(mon, &s);
}

static void gvm_ioapic_reset(DeviceState *dev)
{
    IOAPICCommonState *s = IOAPIC_COMMON(dev);

    ioapic_reset_common(dev);
    gvm_ioapic_put(s);
}

static void gvm_ioapic_set_irq(void *opaque, int irq, int level)
{
    GVMIOAPICState *s = opaque;
    int delivered;

    delivered = gvm_set_irq(gvm_state, s->gvm_gsi_base + irq, level);
    apic_report_irq_delivered(delivered);
}

static void gvm_ioapic_realize(DeviceState *dev, Error **errp)
{
    IOAPICCommonState *s = IOAPIC_COMMON(dev);

    memory_region_init_reservation(&s->io_memory, NULL, "gvm-ioapic", 0x1000);

    qdev_init_gpio_in(dev, gvm_ioapic_set_irq, IOAPIC_NUM_PINS);
}

static Property gvm_ioapic_properties[] = {
    DEFINE_PROP_UINT32("gsi_base", GVMIOAPICState, gvm_gsi_base, 0),
    DEFINE_PROP_END_OF_LIST()
};

static void gvm_ioapic_class_init(ObjectClass *klass, void *data)
{
    IOAPICCommonClass *k = IOAPIC_COMMON_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    k->realize   = gvm_ioapic_realize;
    k->pre_save  = gvm_ioapic_get;
    k->post_load = gvm_ioapic_put;
    dc->reset    = gvm_ioapic_reset;
    dc->props    = gvm_ioapic_properties;
}

static const TypeInfo gvm_ioapic_info = {
    .name  = "gvm-ioapic",
    .parent = TYPE_IOAPIC_COMMON,
    .instance_size = sizeof(GVMIOAPICState),
    .class_init = gvm_ioapic_class_init,
};

static void gvm_ioapic_register_types(void)
{
    type_register_static(&gvm_ioapic_info);
}

type_init(gvm_ioapic_register_types)
