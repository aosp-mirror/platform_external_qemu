/*
 * QEMU AEHD stub
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
#include "sysemu/aehd.h"

#ifndef CONFIG_USER_ONLY
#include "hw/pci/msi.h"
#endif

AEHDState *aehd_state;
bool aehd_kernel_irqchip;
bool aehd_allowed;

int aehd_destroy_vcpu(CPUState *cpu)
{
    return -ENOSYS;
}

int aehd_init_vcpu(CPUState *cpu)
{
    return -ENOSYS;
}

void aehd_flush_coalesced_mmio_buffer(void)
{
}

void aehd_cpu_synchronize_state(CPUState *cpu)
{
}

void aehd_cpu_synchronize_post_reset(CPUState *cpu)
{
}

void aehd_cpu_synchronize_post_init(CPUState *cpu)
{
}

void aehd_cpu_synchronize_post_loadvm(CPUState *cpu)
{
}

int aehd_cpu_exec(CPUState *cpu)
{
    abort();
}

int aehd_update_guest_debug(CPUState *cpu, unsigned long reinject_trap)
{
    return -ENOSYS;
}

int aehd_insert_breakpoint(CPUState *cpu, target_ulong addr,
                          target_ulong len, int type)
{
    return -EINVAL;
}

int aehd_remove_breakpoint(CPUState *cpu, target_ulong addr,
                          target_ulong len, int type)
{
    return -EINVAL;
}

void aehd_remove_all_breakpoints(CPUState *cpu)
{
}

#ifndef _WIN32
int aehd_set_signal_mask(CPUState *cpu, const sigset_t *sigset)
{
    abort();
}
#endif

int aehd_on_sigbus_vcpu(CPUState *cpu, int code, void *addr)
{
    return 1;
}

int aehd_on_sigbus(int code, void *addr)
{
    return 1;
}

#ifndef CONFIG_USER_ONLY
int aehd_irqchip_add_msi_route(AEHDState *s, int vector, PCIDevice *dev)
{
    return -ENOSYS;
}

void aehd_init_irq_routing(AEHDState *s)
{
}

void aehd_irqchip_release_virq(AEHDState *s, int virq)
{
}

int aehd_irqchip_update_msi_route(AEHDState *s, int virq, MSIMessage msg,
                                 PCIDevice *dev)
{
    return -ENOSYS;
}

void aehd_irqchip_commit_routes(AEHDState *s)
{
}

int aehd_irqchip_add_adapter_route(AEHDState *s, AdapterInfo *adapter)
{
    return -ENOSYS;
}

bool aehd_has_free_slot(MachineState *ms)
{
    return false;
}

void* aehd_gpa2hva(uint64_t gpa, bool* found)
{
    *found = false;
    return NULL;
}

int aehd_hva2gpa(void* hva, uint64_t length, int array_size,
                uint64_t* gpa, uint64_t* size)
{
    return 0;
}

#endif
