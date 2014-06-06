/* header to be included in non-HAX-specific code */
#ifndef _HAX_H
#define _HAX_H

#include "config.h"
#include "qemu-common.h"
#include "cpu.h"

extern int hax_disabled;
struct hax_vcpu_state;

#ifdef CONFIG_HAX
int hax_enabled(void);
int hax_set_ramsize(uint64_t ramsize);
int hax_init(int smp_cpus);
int hax_init_vcpu(CPUState *cpu);
/* Execute vcpu in non-root mode */
int hax_vcpu_exec(CPUState *cpu);
/* Sync vcpu state with HAX driver */
int hax_sync_vcpus(void);
void hax_vcpu_sync_state(CPUState *cpu, int modified);
int hax_populate_ram(uint64_t va, uint32_t size);
int hax_set_phys_mem(hwaddr start_addr,
                     ram_addr_t size, ram_addr_t phys_offset);
/* Check if QEMU need emulate guest execution */
int hax_vcpu_emulation_mode(CPUState *cpu);
int hax_stop_emulation(CPUState *cpu);
int hax_stop_translate(CPUState *cpu);
int hax_arch_get_registers(CPUState *cpu);
void hax_raise_event(CPUState *cpu);
void hax_reset_vcpu_state(void *opaque);

#include "target-i386/hax-interface.h"

#else
#define hax_enabled() (0)
#endif

#endif
