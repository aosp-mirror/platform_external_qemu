/*
 * QEMU Windows Hypervisor Platform accelerator (WHPX) stub
 *
 * Copyright Microsoft Corp. 2017
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "cpu.h"
#include "sysemu/whpx.h"

#include <stdio.h>

int whpx_init_vcpu(CPUState *cpu)
{
    return -1;
}

int whpx_vcpu_exec(CPUState *cpu)
{
    return -1;
}

void whpx_destroy_vcpu(CPUState *cpu)
{
}

void whpx_vcpu_kick(CPUState *cpu)
{
}

void whpx_cpu_synchronize_state(CPUState *cpu)
{
}

void whpx_cpu_synchronize_post_reset(CPUState *cpu)
{
}

void whpx_cpu_synchronize_post_init(CPUState *cpu)
{
}

void whpx_cpu_synchronize_pre_loadvm(CPUState *cpu)
{
}

void* whpx_gpa2hva(uint64_t gpa, bool* found)
{
    fprintf(stderr, "%s: error; using stub!\n", __func__);
    *found = false;
    return NULL;
}
