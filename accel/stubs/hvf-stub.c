/*
 * QEMU HVF support
 *
 * Copyright (c) 2017, Google
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "sysemu/hvf.h"

int hvf_enabled(void)
{
   return 0;
}

void* hvf_gpa2hva(uint64_t gpa, bool* found) {
    *found = false;
    return 0;
}

int hvf_hva2gpa(void* hva, uint64_t length, int max,
                uint64_t* gpa, uint64_t* size) {
    return 0;
}

int hvf_map_safe(void* hva, uint64_t gpa, uint64_t size, uint64_t flags) {
    return -1;
}

int hvf_unmap_safe(uint64_t gpa, uint64_t size) {
    return -1;
}

int hvf_protect_safe(uint64_t gpa, uint64_t size, uint64_t flags) {
    return -1;
}

int hvf_remap_safe(void* hva, uint64_t gpa, uint64_t size, uint64_t flags) {
    return -1;
}

void hvf_irq_deactivated(int cpunum, int irqnum) {
    (void)cpunum;
    (void)irqnum;
}

void hvf_cpu_synchronize_state(CPUState *cpu) {}

void hvf_exit_vcpu(CPUState* cpu) { (void)cpu; }
