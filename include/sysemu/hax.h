/*
 * QEMU HAXM support
 *
 * Copyright IBM, Corp. 2008
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * Copyright (c) 2011 Intel Corporation
 *  Written by:
 *  Jiang Yunhong<yunhong.jiang@intel.com>
 *  Xin Xiaohui<xiaohui.xin@intel.com>
 *  Zhang Xiantao<xiantao.zhang@intel.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

/* header to be included in non-HAX-specific code */
#ifndef _HAX_H
#define _HAX_H

#include "config-host.h"
#include "qemu-common.h"

int hax_enabled(void);
void hax_disable(int disable);
int hax_ug_platform(void);
int hax_pre_init(uint64_t ram_size);
int hax_sync_vcpus(void);

#ifdef CONFIG_HAX

#include "hw/hw.h"
#include "qemu/bitops.h"
#include "exec/memory.h"
int hax_init_vcpu(CPUState *cpu);
int hax_vcpu_exec(CPUState *cpu);
int hax_smp_cpu_exec(CPUState *cpu);
void hax_cpu_synchronize_state(CPUState *cpu);
void hax_cpu_synchronize_post_reset(CPUState *cpu);
void hax_cpu_synchronize_post_init(CPUState *cpu);
int hax_populate_ram(uint64_t va, uint32_t size);
int hax_set_phys_mem(MemoryRegionSection *section);
int hax_vcpu_emulation_mode(CPUState *cpu);
int hax_stop_emulation(CPUState *cpu);
int hax_stop_translate(CPUState *cpu);
int hax_vcpu_destroy(CPUState *cpu);
void hax_raise_event(CPUState *cpu);
void hax_reset_vcpu_state(void *opaque);
#include "target-i386/hax-interface.h"
#include "target-i386/hax-i386.h"

#endif

#endif /* _HAX_H */
