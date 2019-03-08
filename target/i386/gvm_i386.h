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

bool gvm_allows_irq0_override(void);
bool gvm_has_smm(void);
void gvm_synchronize_all_tsc(void);
void gvm_arch_reset_vcpu(X86CPU *cs);
void gvm_arch_do_init_vcpu(X86CPU *cs);

int gvm_device_pci_assign(GVMState *s, PCIHostDeviceAddress *dev_addr,
                          uint32_t flags, uint32_t *dev_id);
int gvm_device_pci_deassign(GVMState *s, uint32_t dev_id);

int gvm_device_intx_assign(GVMState *s, uint32_t dev_id,
                           bool use_host_msi, uint32_t guest_irq);
int gvm_device_intx_set_mask(GVMState *s, uint32_t dev_id, bool masked);
int gvm_device_intx_deassign(GVMState *s, uint32_t dev_id, bool use_host_msi);

int gvm_device_msi_assign(GVMState *s, uint32_t dev_id, int virq);
int gvm_device_msi_deassign(GVMState *s, uint32_t dev_id);

bool gvm_device_msix_supported(GVMState *s);
int gvm_device_msix_init_vectors(GVMState *s, uint32_t dev_id,
                                 uint32_t nr_vectors);
int gvm_device_msix_set_vector(GVMState *s, uint32_t dev_id, uint32_t vector,
                               int virq);
int gvm_device_msix_assign(GVMState *s, uint32_t dev_id);
int gvm_device_msix_deassign(GVMState *s, uint32_t dev_id);

#endif /*QEMU_GVM_I386_H */
