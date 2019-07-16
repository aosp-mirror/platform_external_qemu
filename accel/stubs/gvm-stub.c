/*
 * QEMU GVM stub
 *
 * Copyright Red Hat, Inc. 2010
 *
 * Author: Paolo Bonzini     <pbonzini@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "cpu.h"
#include "sysemu/gvm.h"

#ifndef CONFIG_USER_ONLY
#include "hw/pci/msi.h"
#endif

GVMState *gvm_state;
bool gvm_kernel_irqchip;
bool gvm_async_interrupts_allowed;
bool gvm_gsi_routing_allowed;
bool gvm_gsi_direct_mapping;
bool gvm_allowed;
bool gvm_readonly_mem_allowed;

int gvm_destroy_vcpu(CPUState *cpu)
{
    return -ENOSYS;
}

int gvm_init_vcpu(CPUState *cpu)
{
    return -ENOSYS;
}

void gvm_flush_coalesced_mmio_buffer(void)
{
}

void gvm_cpu_synchronize_state(CPUState *cpu)
{
}

void gvm_cpu_synchronize_post_reset(CPUState *cpu)
{
}

void gvm_cpu_synchronize_post_init(CPUState *cpu)
{
}

int gvm_cpu_exec(CPUState *cpu)
{
    abort();
}

int gvm_has_sync_mmu(void)
{
    return 0;
}

int gvm_has_many_ioeventfds(void)
{
    return 0;
}

void gvm_setup_guest_memory(void *start, size_t size)
{
}

int gvm_update_guest_debug(CPUState *cpu, unsigned long reinject_trap)
{
    return -ENOSYS;
}

int gvm_insert_breakpoint(CPUState *cpu, target_ulong addr,
                          target_ulong len, int type)
{
    return -EINVAL;
}

int gvm_remove_breakpoint(CPUState *cpu, target_ulong addr,
                          target_ulong len, int type)
{
    return -EINVAL;
}

void gvm_remove_all_breakpoints(CPUState *cpu)
{
}

#ifndef _WIN32
int gvm_set_signal_mask(CPUState *cpu, const sigset_t *sigset)
{
    abort();
}
#endif

int gvm_on_sigbus_vcpu(CPUState *cpu, int code, void *addr)
{
    return 1;
}

int gvm_on_sigbus(int code, void *addr)
{
    return 1;
}

#ifndef CONFIG_USER_ONLY
int gvm_irqchip_add_msi_route(GVMState *s, int vector, PCIDevice *dev)
{
    return -ENOSYS;
}

void gvm_init_irq_routing(GVMState *s)
{
}

void gvm_irqchip_release_virq(GVMState *s, int virq)
{
}

int gvm_irqchip_update_msi_route(GVMState *s, int virq, MSIMessage msg,
                                 PCIDevice *dev)
{
    return -ENOSYS;
}

void gvm_irqchip_commit_routes(GVMState *s)
{
}

int gvm_irqchip_add_adapter_route(GVMState *s, AdapterInfo *adapter)
{
    return -ENOSYS;
}

int gvm_irqchip_add_irqfd_notifier_gsi(GVMState *s, EventNotifier *n,
                                       EventNotifier *rn, int virq)
{
    return -ENOSYS;
}

int gvm_irqchip_remove_irqfd_notifier_gsi(GVMState *s, EventNotifier *n,
                                          int virq)
{
    return -ENOSYS;
}

bool gvm_has_free_slot(MachineState *ms)
{
    return false;
}
#endif
