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
 * Copyright 2016 Google, Inc.
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef QEMU_HAX_H
#define QEMU_HAX_H

#include "qemu-common.h"

int hax_sync_vcpus(void);
int hax_init_vcpu(CPUState *cpu);
int hax_vcpu_exec(CPUState *cpu);
int hax_smp_cpu_exec(CPUState *cpu);
int hax_vcpu_emulation_mode(CPUState *cpu);
int hax_stop_emulation(CPUState *cpu);
int hax_stop_translate(CPUState *cpu);
int hax_populate_ram(uint64_t va, uint64_t size);
int hax_gpa_protect(uint64_t va, uint64_t size, uint64_t flags);
bool hax_gpa_protection_supported(void);

void hax_cpu_synchronize_state(CPUState *cpu);
void hax_cpu_synchronize_post_reset(CPUState *cpu);
void hax_cpu_synchronize_post_init(CPUState *cpu);
void hax_cpu_synchronize_pre_loadvm(CPUState *cpu);

void* hax_gpa2hva(uint64_t gpa, bool* found);
int hax_hva2gpa(void* hva, uint64_t length, int array_size,
                uint64_t* gpa, uint64_t* size);

#ifdef CONFIG_HAX

int hax_enabled(void);
int hax_ug_platform(void);
int hax_vcpu_active(CPUState* cpu);
uint64_t hax_mem_limit(void);

#include "hw/hw.h"
#include "qemu/bitops.h"
#include "exec/memory.h"
int hax_vcpu_destroy(CPUState *cpu);
void hax_raise_event(CPUState *cpu);
void hax_reset_vcpu_state(void *opaque);
#include "target/i386/hax-interface.h"
#include "target/i386/hax-i386.h"

#else /* CONFIG_HAX */

#define hax_enabled() (0)
#define hax_ug_platform() (0)
#define hax_vcpu_active(unused) (0)
#define hax_mem_limit() ((uint64_t)(-1))

#endif /* CONFIG_HAX */

#endif /* QEMU_HAX_H */
