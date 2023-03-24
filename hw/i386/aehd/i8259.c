/*
 * AEHD in-kernel PIC (i8259) support
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
#include "hw/isa/i8259_internal.h"
#include "hw/i386/apic_internal.h"
#include "sysemu/aehd.h"
#include "sysemu/aehd-interface.h"

#define TYPE_AEHD_I8259 "aehd-i8259"
#define AEHD_PIC_CLASS(class) \
    OBJECT_CLASS_CHECK(AEHDPICClass, (class), TYPE_AEHD_I8259)
#define AEHD_PIC_GET_CLASS(obj) \
    OBJECT_GET_CLASS(AEHDPICClass, (obj), TYPE_AEHD_I8259)

/**
 * AEHDPICClass:
 * @parent_realize: The parent's realizefn.
 */
typedef struct AEHDPICClass {
    PICCommonClass parent_class;

    DeviceRealize parent_realize;
} AEHDPICClass;

static void aehd_pic_get(PICCommonState *s)
{
    struct aehd_irqchip chip;
    struct aehd_pic_state *aepic;
    int ret;

    chip.chip_id = s->master ? AEHD_IRQCHIP_PIC_MASTER : AEHD_IRQCHIP_PIC_SLAVE;
    ret = aehd_vm_ioctl(aehd_state, AEHD_GET_IRQCHIP,
            &chip, sizeof(chip), &chip, sizeof(chip));
    if (ret < 0) {
        fprintf(stderr, "AEHD_GET_IRQCHIP failed: %s\n", strerror(ret));
        abort();
    }

    aepic = &chip.chip.pic;

    s->last_irr = aepic->last_irr;
    s->irr = aepic->irr;
    s->imr = aepic->imr;
    s->isr = aepic->isr;
    s->priority_add = aepic->priority_add;
    s->irq_base = aepic->irq_base;
    s->read_reg_select = aepic->read_reg_select;
    s->poll = aepic->poll;
    s->special_mask = aepic->special_mask;
    s->init_state = aepic->init_state;
    s->auto_eoi = aepic->auto_eoi;
    s->rotate_on_auto_eoi = aepic->rotate_on_auto_eoi;
    s->special_fully_nested_mode = aepic->special_fully_nested_mode;
    s->init4 = aepic->init4;
    s->elcr = aepic->elcr;
    s->elcr_mask = aepic->elcr_mask;
}

static void aehd_pic_put(PICCommonState *s)
{
    struct aehd_irqchip chip;
    struct aehd_pic_state *aepic;
    int ret;

    chip.chip_id = s->master ? AEHD_IRQCHIP_PIC_MASTER : AEHD_IRQCHIP_PIC_SLAVE;

    aepic = &chip.chip.pic;

    aepic->last_irr = s->last_irr;
    aepic->irr = s->irr;
    aepic->imr = s->imr;
    aepic->isr = s->isr;
    aepic->priority_add = s->priority_add;
    aepic->irq_base = s->irq_base;
    aepic->read_reg_select = s->read_reg_select;
    aepic->poll = s->poll;
    aepic->special_mask = s->special_mask;
    aepic->init_state = s->init_state;
    aepic->auto_eoi = s->auto_eoi;
    aepic->rotate_on_auto_eoi = s->rotate_on_auto_eoi;
    aepic->special_fully_nested_mode = s->special_fully_nested_mode;
    aepic->init4 = s->init4;
    aepic->elcr = s->elcr;
    aepic->elcr_mask = s->elcr_mask;

    ret = aehd_vm_ioctl(aehd_state, AEHD_SET_IRQCHIP,
            &chip, sizeof(chip), NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "AEHD_GET_IRQCHIP failed: %s\n", strerror(ret));
        abort();
    }
}

static void aehd_pic_reset(DeviceState *dev)
{
    PICCommonState *s = PIC_COMMON(dev);

    s->elcr = 0;
    pic_reset_common(s);

    aehd_pic_put(s);
}

static void aehd_pic_set_irq(void *opaque, int irq, int level)
{
    int delivered;

    delivered = aehd_set_irq(aehd_state, irq, level);
    apic_report_irq_delivered(delivered);
}

static void aehd_pic_realize(DeviceState *dev, Error **errp)
{
    PICCommonState *s = PIC_COMMON(dev);
    AEHDPICClass *kpc = AEHD_PIC_GET_CLASS(dev);

    memory_region_init_reservation(&s->base_io, NULL, "aehd-pic", 2);
    memory_region_init_reservation(&s->elcr_io, NULL, "aehd-elcr", 1);

    kpc->parent_realize(dev, errp);
}

qemu_irq *aehd_i8259_init(ISABus *bus)
{
    i8259_init_chip(TYPE_AEHD_I8259, bus, true);
    i8259_init_chip(TYPE_AEHD_I8259, bus, false);

    return qemu_allocate_irqs(aehd_pic_set_irq, NULL, ISA_NUM_IRQS);
}

static void aehd_i8259_class_init(ObjectClass *klass, void *data)
{
    AEHDPICClass *kpc = AEHD_PIC_CLASS(klass);
    PICCommonClass *k = PIC_COMMON_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset     = aehd_pic_reset;
    kpc->parent_realize = dc->realize;
    dc->realize   = aehd_pic_realize;
    k->pre_save   = aehd_pic_get;
    k->post_load  = aehd_pic_put;
}

static const TypeInfo aehd_i8259_info = {
    .name = TYPE_AEHD_I8259,
    .parent = TYPE_PIC_COMMON,
    .instance_size = sizeof(PICCommonState),
    .class_init = aehd_i8259_class_init,
    .class_size = sizeof(AEHDPICClass),
};

static void aehd_pic_register_types(void)
{
    type_register_static(&aehd_i8259_info);
}

type_init(aehd_pic_register_types)
