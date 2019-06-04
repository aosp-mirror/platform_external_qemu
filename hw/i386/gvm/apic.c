/*
 * GVM in-kernel APIC support
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
#include "qemu-common.h"
#include "cpu.h"
#include "hw/i386/apic_internal.h"
#include "hw/pci/msi.h"
#include "sysemu/gvm.h"
#include "sysemu/gvm-interface.h"

static inline void gvm_apic_set_reg(struct gvm_lapic_state *kapic,
                                    int reg_id, uint32_t val)
{
    *((uint32_t *)(kapic->regs + (reg_id << 4))) = val;
}

static inline uint32_t gvm_apic_get_reg(struct gvm_lapic_state *kapic,
                                        int reg_id)
{
    return *((uint32_t *)(kapic->regs + (reg_id << 4)));
}

void gvm_put_apic_state(DeviceState *dev, struct gvm_lapic_state *kapic)
{
    APICCommonState *s = APIC_COMMON(dev);
    int i;

    memset(kapic, 0, sizeof(*kapic));
    gvm_apic_set_reg(kapic, 0x2, s->id << 24);
    gvm_apic_set_reg(kapic, 0x8, s->tpr);
    gvm_apic_set_reg(kapic, 0xd, s->log_dest << 24);
    gvm_apic_set_reg(kapic, 0xe, s->dest_mode << 28 | 0x0fffffff);
    gvm_apic_set_reg(kapic, 0xf, s->spurious_vec);
    for (i = 0; i < 8; i++) {
        gvm_apic_set_reg(kapic, 0x10 + i, s->isr[i]);
        gvm_apic_set_reg(kapic, 0x18 + i, s->tmr[i]);
        gvm_apic_set_reg(kapic, 0x20 + i, s->irr[i]);
    }
    gvm_apic_set_reg(kapic, 0x28, s->esr);
    gvm_apic_set_reg(kapic, 0x30, s->icr[0]);
    gvm_apic_set_reg(kapic, 0x31, s->icr[1]);
    for (i = 0; i < APIC_LVT_NB; i++) {
        gvm_apic_set_reg(kapic, 0x32 + i, s->lvt[i]);
    }
    gvm_apic_set_reg(kapic, 0x38, s->initial_count);
    gvm_apic_set_reg(kapic, 0x3e, s->divide_conf);
}

void gvm_get_apic_state(DeviceState *dev, struct gvm_lapic_state *kapic)
{
    APICCommonState *s = APIC_COMMON(dev);
    int i, v;

    s->id = gvm_apic_get_reg(kapic, 0x2) >> 24;
    s->tpr = gvm_apic_get_reg(kapic, 0x8);
    s->arb_id = gvm_apic_get_reg(kapic, 0x9);
    s->log_dest = gvm_apic_get_reg(kapic, 0xd) >> 24;
    s->dest_mode = gvm_apic_get_reg(kapic, 0xe) >> 28;
    s->spurious_vec = gvm_apic_get_reg(kapic, 0xf);
    for (i = 0; i < 8; i++) {
        s->isr[i] = gvm_apic_get_reg(kapic, 0x10 + i);
        s->tmr[i] = gvm_apic_get_reg(kapic, 0x18 + i);
        s->irr[i] = gvm_apic_get_reg(kapic, 0x20 + i);
    }
    s->esr = gvm_apic_get_reg(kapic, 0x28);
    s->icr[0] = gvm_apic_get_reg(kapic, 0x30);
    s->icr[1] = gvm_apic_get_reg(kapic, 0x31);
    for (i = 0; i < APIC_LVT_NB; i++) {
        s->lvt[i] = gvm_apic_get_reg(kapic, 0x32 + i);
    }
    s->initial_count = gvm_apic_get_reg(kapic, 0x38);
    s->divide_conf = gvm_apic_get_reg(kapic, 0x3e);

    v = (s->divide_conf & 3) | ((s->divide_conf >> 1) & 4);
    s->count_shift = (v + 1) & 7;

    s->initial_count_load_time = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    apic_next_timer(s, s->initial_count_load_time);
}

static void gvm_apic_set_base(APICCommonState *s, uint64_t val)
{
    s->apicbase = val;
}

static void gvm_apic_set_tpr(APICCommonState *s, uint8_t val)
{
    s->tpr = (val & 0x0f) << 4;
}

static uint8_t gvm_apic_get_tpr(APICCommonState *s)
{
    return s->tpr >> 4;
}

static void gvm_apic_enable_tpr_reporting(APICCommonState *s, bool enable)
{
    struct gvm_tpr_access_ctl ctl = {
        .enabled = enable
    };

    gvm_vcpu_ioctl(CPU(s->cpu), GVM_TPR_ACCESS_REPORTING,
            &ctl, sizeof(ctl), NULL, 0);
}

static void gvm_apic_vapic_base_update(APICCommonState *s)
{
    struct gvm_vapic_addr vapid_addr = {
        .vapic_addr = s->vapic_paddr,
    };
    int ret;

    ret = gvm_vcpu_ioctl(CPU(s->cpu), GVM_SET_VAPIC_ADDR,
            &vapid_addr, sizeof(vapid_addr), NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "GVM: setting VAPIC address failed (%s)\n",
                strerror(-ret));
        abort();
    }
}

static void do_inject_external_nmi(CPUState *cpu, run_on_cpu_data data)
{
    APICCommonState *s = data.host_ptr;
    uint32_t lvt;
    int ret;

    //XXX
    //cpu_synchronize_state(cpu);

    lvt = s->lvt[APIC_LVT_LINT1];
    if (!(lvt & APIC_LVT_MASKED) && ((lvt >> 8) & 7) == APIC_DM_NMI) {
        ret = gvm_vcpu_ioctl(cpu, GVM_NMI, NULL, 0, NULL, 0);
        if (ret < 0) {
            fprintf(stderr, "GVM: injection failed, NMI lost (%s)\n",
                    strerror(-ret));
        }
    }
}

static void gvm_apic_external_nmi(APICCommonState *s)
{
    run_on_cpu(CPU(s->cpu), do_inject_external_nmi, RUN_ON_CPU_HOST_PTR(s));
}

static uint64_t gvm_apic_mem_read(void *opaque, hwaddr addr,
                                  unsigned size)
{
    return ~(uint64_t)0;
}

static void gvm_apic_mem_write(void *opaque, hwaddr addr,
                               uint64_t data, unsigned size)
{
    MSIMessage msg = { .address = addr, .data = data };
    int ret;

    ret = gvm_irqchip_send_msi(gvm_state, msg);
    if (ret < 0) {
        fprintf(stderr, "GVM: injection failed, MSI lost (%s)\n",
                strerror(-ret));
    }
}

static const MemoryRegionOps gvm_apic_io_ops = {
    .read = gvm_apic_mem_read,
    .write = gvm_apic_mem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void gvm_apic_reset(APICCommonState *s)
{
    /* Not used by GVM, which uses the CPU mp_state instead.  */
    s->wait_for_sipi = 0;
}

static void gvm_apic_realize(DeviceState *dev, Error **errp)
{
    APICCommonState *s = APIC_COMMON(dev);

    memory_region_init_io(&s->io_memory, OBJECT(s), &gvm_apic_io_ops, s,
                          "gvm-apic-msi", APIC_SPACE_SIZE);

    msi_nonbroken = true;
}

static void gvm_apic_unrealize(DeviceState *dev, Error **errp)
{
}

static void gvm_apic_class_init(ObjectClass *klass, void *data)
{
    APICCommonClass *k = APIC_COMMON_CLASS(klass);

    k->realize = gvm_apic_realize;
    k->unrealize = gvm_apic_unrealize;
    k->reset = gvm_apic_reset;
    k->set_base = gvm_apic_set_base;
    k->set_tpr = gvm_apic_set_tpr;
    k->get_tpr = gvm_apic_get_tpr;
    k->enable_tpr_reporting = gvm_apic_enable_tpr_reporting;
    k->vapic_base_update = gvm_apic_vapic_base_update;
    k->external_nmi = gvm_apic_external_nmi;
}

static const TypeInfo gvm_apic_info = {
    .name = "gvm-apic",
    .parent = TYPE_APIC_COMMON,
    .instance_size = sizeof(APICCommonState),
    .class_init = gvm_apic_class_init,
};

static void gvm_apic_register_types(void)
{
    type_register_static(&gvm_apic_info);
}

type_init(gvm_apic_register_types)
