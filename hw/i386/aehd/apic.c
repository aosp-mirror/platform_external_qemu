/*
 * AEHD in-kernel APIC support
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
#include "sysemu/aehd.h"
#include "sysemu/aehd-interface.h"

static inline void aehd_apic_set_reg(struct aehd_lapic_state *aapic,
                                    int reg_id, uint32_t val)
{
    *((uint32_t *)(aapic->regs + (reg_id << 4))) = val;
}

static inline uint32_t aehd_apic_get_reg(struct aehd_lapic_state *aapic,
                                        int reg_id)
{
    return *((uint32_t *)(aapic->regs + (reg_id << 4)));
}

void aehd_put_apic_state(DeviceState *dev, struct aehd_lapic_state *aapic)
{
    APICCommonState *s = APIC_COMMON(dev);
    int i;

    memset(aapic, 0, sizeof(*aapic));
    aehd_apic_set_reg(aapic, 0x2, s->id << 24);
    aehd_apic_set_reg(aapic, 0x8, s->tpr);
    aehd_apic_set_reg(aapic, 0xd, s->log_dest << 24);
    aehd_apic_set_reg(aapic, 0xe, s->dest_mode << 28 | 0x0fffffff);
    aehd_apic_set_reg(aapic, 0xf, s->spurious_vec);
    for (i = 0; i < 8; i++) {
        aehd_apic_set_reg(aapic, 0x10 + i, s->isr[i]);
        aehd_apic_set_reg(aapic, 0x18 + i, s->tmr[i]);
        aehd_apic_set_reg(aapic, 0x20 + i, s->irr[i]);
    }
    aehd_apic_set_reg(aapic, 0x28, s->esr);
    aehd_apic_set_reg(aapic, 0x30, s->icr[0]);
    aehd_apic_set_reg(aapic, 0x31, s->icr[1]);
    for (i = 0; i < APIC_LVT_NB; i++) {
        aehd_apic_set_reg(aapic, 0x32 + i, s->lvt[i]);
    }
    aehd_apic_set_reg(aapic, 0x38, s->initial_count);
    aehd_apic_set_reg(aapic, 0x3e, s->divide_conf);
}

void aehd_get_apic_state(DeviceState *dev, struct aehd_lapic_state *aapic)
{
    APICCommonState *s = APIC_COMMON(dev);
    int i, v;

    s->id = aehd_apic_get_reg(aapic, 0x2) >> 24;
    s->tpr = aehd_apic_get_reg(aapic, 0x8);
    s->arb_id = aehd_apic_get_reg(aapic, 0x9);
    s->log_dest = aehd_apic_get_reg(aapic, 0xd) >> 24;
    s->dest_mode = aehd_apic_get_reg(aapic, 0xe) >> 28;
    s->spurious_vec = aehd_apic_get_reg(aapic, 0xf);
    for (i = 0; i < 8; i++) {
        s->isr[i] = aehd_apic_get_reg(aapic, 0x10 + i);
        s->tmr[i] = aehd_apic_get_reg(aapic, 0x18 + i);
        s->irr[i] = aehd_apic_get_reg(aapic, 0x20 + i);
    }
    s->esr = aehd_apic_get_reg(aapic, 0x28);
    s->icr[0] = aehd_apic_get_reg(aapic, 0x30);
    s->icr[1] = aehd_apic_get_reg(aapic, 0x31);
    for (i = 0; i < APIC_LVT_NB; i++) {
        s->lvt[i] = aehd_apic_get_reg(aapic, 0x32 + i);
    }
    s->initial_count = aehd_apic_get_reg(aapic, 0x38);
    s->divide_conf = aehd_apic_get_reg(aapic, 0x3e);

    v = (s->divide_conf & 3) | ((s->divide_conf >> 1) & 4);
    s->count_shift = (v + 1) & 7;

    s->initial_count_load_time = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    apic_next_timer(s, s->initial_count_load_time);
}

static void aehd_apic_set_base(APICCommonState *s, uint64_t val)
{
    s->apicbase = val;
}

static void aehd_apic_set_tpr(APICCommonState *s, uint8_t val)
{
    s->tpr = (val & 0x0f) << 4;
}

static uint8_t aehd_apic_get_tpr(APICCommonState *s)
{
    return s->tpr >> 4;
}

static void aehd_apic_enable_tpr_reporting(APICCommonState *s, bool enable)
{
    struct aehd_tpr_access_ctl ctl = {
        .enabled = enable
    };

    aehd_vcpu_ioctl(CPU(s->cpu), AEHD_TPR_ACCESS_REPORTING,
            &ctl, sizeof(ctl), NULL, 0);
}

static void aehd_apic_vapic_base_update(APICCommonState *s)
{
    struct aehd_vapic_addr vapid_addr = {
        .vapic_addr = s->vapic_paddr,
    };
    int ret;

    ret = aehd_vcpu_ioctl(CPU(s->cpu), AEHD_SET_VAPIC_ADDR,
            &vapid_addr, sizeof(vapid_addr), NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "AEHD: setting VAPIC address failed (%s)\n",
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
        ret = aehd_vcpu_ioctl(cpu, AEHD_NMI, NULL, 0, NULL, 0);
        if (ret < 0) {
            fprintf(stderr, "AEHD: injection failed, NMI lost (%s)\n",
                    strerror(-ret));
        }
    }
}

static void aehd_apic_external_nmi(APICCommonState *s)
{
    run_on_cpu(CPU(s->cpu), do_inject_external_nmi, RUN_ON_CPU_HOST_PTR(s));
}

static uint64_t aehd_apic_mem_read(void *opaque, hwaddr addr,
                                  unsigned size)
{
    return ~(uint64_t)0;
}

static void aehd_apic_mem_write(void *opaque, hwaddr addr,
                               uint64_t data, unsigned size)
{
    MSIMessage msg = { .address = addr, .data = data };
    int ret;

    ret = aehd_irqchip_send_msi(aehd_state, msg);
    if (ret < 0) {
        fprintf(stderr, "AEHD: injection failed, MSI lost (%s)\n",
                strerror(-ret));
    }
}

static const MemoryRegionOps aehd_apic_io_ops = {
    .read = aehd_apic_mem_read,
    .write = aehd_apic_mem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void aehd_apic_reset(APICCommonState *s)
{
    /* Not used by AEHD, which uses the CPU mp_state instead.  */
    s->wait_for_sipi = 0;
}

static void aehd_apic_realize(DeviceState *dev, Error **errp)
{
    APICCommonState *s = APIC_COMMON(dev);

    memory_region_init_io(&s->io_memory, OBJECT(s), &aehd_apic_io_ops, s,
                          "aehd-apic-msi", APIC_SPACE_SIZE);

    msi_nonbroken = true;
}

static void aehd_apic_unrealize(DeviceState *dev, Error **errp)
{
}

static void aehd_apic_class_init(ObjectClass *klass, void *data)
{
    APICCommonClass *k = APIC_COMMON_CLASS(klass);

    k->realize = aehd_apic_realize;
    k->unrealize = aehd_apic_unrealize;
    k->reset = aehd_apic_reset;
    k->set_base = aehd_apic_set_base;
    k->set_tpr = aehd_apic_set_tpr;
    k->get_tpr = aehd_apic_get_tpr;
    k->enable_tpr_reporting = aehd_apic_enable_tpr_reporting;
    k->vapic_base_update = aehd_apic_vapic_base_update;
    k->external_nmi = aehd_apic_external_nmi;
}

static const TypeInfo aehd_apic_info = {
    .name = "aehd-apic",
    .parent = TYPE_APIC_COMMON,
    .instance_size = sizeof(APICCommonState),
    .class_init = aehd_apic_class_init,
};

static void aehd_apic_register_types(void)
{
    type_register_static(&aehd_apic_info);
}

type_init(aehd_apic_register_types)
