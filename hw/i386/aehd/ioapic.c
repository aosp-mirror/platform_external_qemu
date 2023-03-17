/*
 * AEHD in-kernel IOPIC support
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
#include "sysemu/aehd.h"
#include "sysemu/aehd-interface.h"

/* PC Utility function */
void aehd_pc_setup_irq_routing(bool pci_enabled)
{
    AEHDState *s = aehd_state;
    int i;

    for (i = 0; i < 8; ++i) {
        if (i == 2) {
            continue;
        }
        aehd_irqchip_add_irq_route(s, i, AEHD_IRQCHIP_PIC_MASTER, i);
    }
    for (i = 8; i < 16; ++i) {
        aehd_irqchip_add_irq_route(s, i, AEHD_IRQCHIP_PIC_SLAVE, i - 8);
    }
    if (pci_enabled) {
        for (i = 0; i < 24; ++i) {
            if (i == 0) {
                aehd_irqchip_add_irq_route(s, i, AEHD_IRQCHIP_IOAPIC, 2);
            } else if (i != 2) {
                aehd_irqchip_add_irq_route(s, i, AEHD_IRQCHIP_IOAPIC, i);
            }
        }
    }
    aehd_irqchip_commit_routes(s);
}

void aehd_pc_gsi_handler(void *opaque, int n, int level)
{
    GSIState *s = opaque;

    if (n < ISA_NUM_IRQS) {
        /* Kernel will forward to both PIC and IOAPIC */
        qemu_set_irq(s->i8259_irq[n], level);
    } else {
        qemu_set_irq(s->ioapic_irq[n], level);
    }
}

typedef struct AEHDIOAPICState AEHDIOAPICState;

struct AEHDIOAPICState {
    IOAPICCommonState ioapic;
    uint32_t aehd_gsi_base;
};

static void aehd_ioapic_get(IOAPICCommonState *s)
{
    struct aehd_irqchip chip;
    struct aehd_ioapic_state *aiopic;
    int ret, i;

    chip.chip_id = AEHD_IRQCHIP_IOAPIC;
    ret = aehd_vm_ioctl(aehd_state, AEHD_GET_IRQCHIP,
            &chip, sizeof(chip), &chip, sizeof(chip));
    if (ret < 0) {
        fprintf(stderr, "AEHD_GET_IRQCHIP failed: %s\n", strerror(ret));
        abort();
    }

    aiopic = &chip.chip.ioapic;

    s->id = aiopic->id;
    s->ioregsel = aiopic->ioregsel;
    s->irr = aiopic->irr;
    for (i = 0; i < IOAPIC_NUM_PINS; i++) {
        s->ioredtbl[i] = aiopic->redirtbl[i].bits;
    }
}

static void aehd_ioapic_put(IOAPICCommonState *s)
{
    struct aehd_irqchip chip;
    struct aehd_ioapic_state *aiopic;
    int ret, i;

    chip.chip_id = AEHD_IRQCHIP_IOAPIC;
    aiopic = &chip.chip.ioapic;

    aiopic->id = s->id;
    aiopic->ioregsel = s->ioregsel;
    aiopic->base_address = s->busdev.mmio[0].addr;
    aiopic->irr = s->irr;
    for (i = 0; i < IOAPIC_NUM_PINS; i++) {
        aiopic->redirtbl[i].bits = s->ioredtbl[i];
    }

    ret = aehd_vm_ioctl(aehd_state, AEHD_SET_IRQCHIP,
            &chip, sizeof(chip), NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "AEHD_GET_IRQCHIP failed: %s\n", strerror(ret));
        abort();
    }
}

void aehd_ioapic_dump_state(Monitor *mon, const QDict *qdict)
{
    IOAPICCommonState s;

    aehd_ioapic_get(&s);

    ioapic_print_redtbl(mon, &s);
}

static void aehd_ioapic_reset(DeviceState *dev)
{
    IOAPICCommonState *s = IOAPIC_COMMON(dev);

    ioapic_reset_common(dev);
    aehd_ioapic_put(s);
}

static void aehd_ioapic_set_irq(void *opaque, int irq, int level)
{
    AEHDIOAPICState *s = opaque;
    int delivered;

    delivered = aehd_set_irq(aehd_state, s->aehd_gsi_base + irq, level);
    apic_report_irq_delivered(delivered);
}

static void aehd_ioapic_realize(DeviceState *dev, Error **errp)
{
    IOAPICCommonState *s = IOAPIC_COMMON(dev);

    memory_region_init_reservation(&s->io_memory, NULL, "aehd-ioapic", 0x1000);

    qdev_init_gpio_in(dev, aehd_ioapic_set_irq, IOAPIC_NUM_PINS);
}

static Property aehd_ioapic_properties[] = {
    DEFINE_PROP_UINT32("gsi_base", AEHDIOAPICState, aehd_gsi_base, 0),
    DEFINE_PROP_END_OF_LIST()
};

static void aehd_ioapic_class_init(ObjectClass *klass, void *data)
{
    IOAPICCommonClass *k = IOAPIC_COMMON_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    k->realize   = aehd_ioapic_realize;
    k->pre_save  = aehd_ioapic_get;
    k->post_load = aehd_ioapic_put;
    dc->reset    = aehd_ioapic_reset;
    dc->props    = aehd_ioapic_properties;
}

static const TypeInfo aehd_ioapic_info = {
    .name  = "aehd-ioapic",
    .parent = TYPE_IOAPIC_COMMON,
    .instance_size = sizeof(AEHDIOAPICState),
    .class_init = aehd_ioapic_class_init,
};

static void aehd_ioapic_register_types(void)
{
    type_register_static(&aehd_ioapic_info);
}

type_init(aehd_ioapic_register_types)
