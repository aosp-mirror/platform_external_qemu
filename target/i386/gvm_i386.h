/*
 * QEMU GVM support
 *
 * Copyright IBM, Corp. 2008
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * Copyright (c) 2017 Intel Corporation
 *  Written by:
 *  Haitao Shan <hshan@google.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef QEMU_GVM_I386_H
#define QEMU_GVM_I386_H

#include "sysemu/gvm-interface.h"
#include "sysemu/gvm.h"

#define gvm_apic_in_kernel() (gvm_irqchip_in_kernel())

#ifdef CONFIG_GVM

#define gvm_pic_in_kernel()  \
    (gvm_irqchip_in_kernel())
#define gvm_ioapic_in_kernel() \
    (gvm_irqchip_in_kernel())

#else

#define gvm_pic_in_kernel()      0
#define gvm_ioapic_in_kernel()   0

#endif  /* CONFIG_GVM */

void gvm_synchronize_all_tsc(void);
void gvm_arch_reset_vcpu(X86CPU *cs);
void gvm_arch_do_init_vcpu(X86CPU *cs);

#endif /*QEMU_GVM_I386_H */
