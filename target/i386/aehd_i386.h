/*
 * QEMU AEHD support
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

#ifndef QEMU_AEHD_I386_H
#define QEMU_AEHD_I386_H

#ifdef CONFIG_AEHD

#include "sysemu/aehd-interface.h"
#include "sysemu/aehd.h"

#endif

#define aehd_apic_in_kernel() (aehd_irqchip_in_kernel())

#ifdef CONFIG_AEHD

#define aehd_pic_in_kernel()  \
    (aehd_irqchip_in_kernel())
#define aehd_ioapic_in_kernel() \
    (aehd_irqchip_in_kernel())

#else

#define aehd_pic_in_kernel()      0
#define aehd_ioapic_in_kernel()   0

#endif  /* CONFIG_AEHD */

void aehd_synchronize_all_tsc(void);
void aehd_arch_reset_vcpu(X86CPU *cs);
void aehd_arch_do_init_vcpu(X86CPU *cs);

#endif /*QEMU_AEHD_I386_H */
