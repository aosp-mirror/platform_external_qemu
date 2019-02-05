/*
 * QEMU KVM ARM specific function stubs
 *
 * Copyright Linaro Limited 2013
 *
 * Author: Peter Maydell <peter.maydell@linaro.org>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "cpu.h"
#include "kvm_arm.h"
#include "sysemu/kvm.h"

void kvm_cpu_synchronize_pre_loadvm(CPUState *cpu)
{
}

bool write_kvmstate_to_list(ARMCPU *cpu)
{
    abort();
}

bool write_list_to_kvmstate(ARMCPU *cpu, int level)
{
    abort();
}

void* kvm_gpa2hva(uint64_t gpa, bool* found) {
    (void)gpa;
    *found = false;
    return NULL;
}