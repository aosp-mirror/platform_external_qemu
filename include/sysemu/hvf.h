/*
 * QEMU Hypervisor.framework (HVF) support
 *
 * Copyright Google Inc., 2017
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

/* header to be included in non-HVF-specific code */
#ifndef _HVF_H
#define _HVF_H

#include "config-host.h"
#include "qemu/osdep.h"
#include "qemu-common.h"
#ifdef CONFIG_HVF
#include "hw/hw.h"
#include "qemu/bitops.h"
#include "exec/memory.h"
#endif

/* Returns 1 if HVF is available and enabled, 0 otherwise. */
int hvf_enabled(void);

/* Disable HVF if |disable| is 1, otherwise, enable it iff it is supported by the host CPU.
 * Use hvf_enabled() after this to get the result. */
void hvf_disable(int disable);

/* Returns non-0 if the host CPU supports the VMX "unrestricted guest" feature which
 * allows the virtual CPU to directly run in "real mode". If true, this allows QEMU to run
 * several vCPU threads in parallel (see cpus.c). Otherwise, only a a single TCG thread
 * can run, and it will call HVF to run the current instructions, except in case of
 * "real mode" (paging disabled, typically at boot time), or MMIO operations. */
// int hvf_ug_platform(void); does not apply to HVF; assume we must be in UG mode

int hvf_sync_vcpus(void);

int hvf_init_vcpu(CPUState *cpu);
int hvf_vcpu_exec(CPUState *cpu);
int hvf_smp_cpu_exec(CPUState *cpu);
void hvf_cpu_synchronize_state(CPUState *cpu);
void hvf_cpu_synchronize_post_reset(CPUState *cpu);
void hvf_cpu_synchronize_post_init(CPUState *cpu);

int hvf_vcpu_destroy(CPUState *cpu);
void hvf_raise_event(CPUState *cpu);
// void hvf_reset_vcpu_state(void *opaque);

void* hvf_gpa2hva(uint64_t gpa, bool* found);
int hvf_hva2gpa(void* hva, uint64_t length, int max,
                uint64_t* gpa, uint64_t* size);

int hvf_map_safe(void* hva, uint64_t gpa, uint64_t size, uint64_t flags);
int hvf_unmap_safe(uint64_t gpa, uint64_t size);
int hvf_protect_safe(uint64_t gpa, uint64_t size, uint64_t flags);
int hvf_remap_safe(void* hva, uint64_t gpa, uint64_t size, uint64_t flags);

void hvf_irq_deactivated(int cpu, int irq);

void hvf_exit_vcpu(CPUState *cpu);

#endif /* _HVF_H */
