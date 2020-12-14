// Copyright 2020 The Android Open Source Project
//
// QEMU Hypervisor.framework support on Apple Silicon
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once

#include "sysemu/hvf.h"
#include "cpu.h"

typedef __uint128_t uint128_t;
_Static_assert(sizeof(__uint128_t) == 16, "uint128_t not 16 bytes");

#define HVF_MAX_VCPU 0x10
#define MAX_VM_ID 0x40
#define MAX_VCPU_ID 0x40

extern struct hvf_state hvf_global;

struct hvf_vm {
    int id;
    struct hvf_vcpu_state *vcpus[HVF_MAX_VCPU];
};

struct hvf_state {
    uint32_t version;
    struct hvf_vm *vm;
    uint64_t mem_quota;
};

#ifdef NEED_CPU_H
/* Functions exported to host specific mode */

/* Host specific functions */
int hvf_inject_interrupt(CPUArchState * env, int vector);
int hvf_vcpu_run(struct hvf_vcpu_state *vcpu);
#endif

void hvf_vcpu_set_irq(CPUState* cpu, int irq, int level);
void hvf_irq_deactivated(int cpu, int irq);

#define LIST_HVF_FEATURE_REGISTERS(f) \
    f(HV_FEATURE_REG_ID_AA64DFR0_EL1) \
    f(HV_FEATURE_REG_ID_AA64DFR1_EL1) \
    f(HV_FEATURE_REG_ID_AA64ISAR0_EL1) \
    f(HV_FEATURE_REG_ID_AA64ISAR1_EL1) \
    f(HV_FEATURE_REG_ID_AA64MMFR0_EL1) \
    f(HV_FEATURE_REG_ID_AA64MMFR1_EL1) \
    f(HV_FEATURE_REG_ID_AA64MMFR2_EL1) \
    f(HV_FEATURE_REG_ID_AA64PFR0_EL1) \
    f(HV_FEATURE_REG_ID_AA64PFR1_EL1) \

#define LIST_HVF_GENERAL_REGISTERS(f) \
    f(HV_REG_CPSR) \
    f(HV_REG_LR) \
    f(HV_REG_PC) \
    f(HV_REG_FP) \
    f(HV_REG_FPCR) \
    f(HV_REG_FPSR) \
    f(HV_REG_X0) \
    f(HV_REG_X1) \
    f(HV_REG_X2) \
    f(HV_REG_X3) \
    f(HV_REG_X4) \
    f(HV_REG_X5) \
    f(HV_REG_X6) \
    f(HV_REG_X7) \
    f(HV_REG_X8) \
    f(HV_REG_X9) \
    f(HV_REG_X10) \
    f(HV_REG_X11) \
    f(HV_REG_X12) \
    f(HV_REG_X13) \
    f(HV_REG_X14) \
    f(HV_REG_X15) \
    f(HV_REG_X16) \
    f(HV_REG_X17) \
    f(HV_REG_X18) \
    f(HV_REG_X19) \
    f(HV_REG_X20) \
    f(HV_REG_X21) \
    f(HV_REG_X22) \
    f(HV_REG_X23) \
    f(HV_REG_X24) \
    f(HV_REG_X25) \
    f(HV_REG_X26) \
    f(HV_REG_X27) \
    f(HV_REG_X28) \
    f(HV_REG_X29) \
    f(HV_REG_X30) \

#define LIST_HVF_SIMD_FP_REGISTERS(f) \
    f(HV_SIMD_FP_REG_Q0) \
    f(HV_SIMD_FP_REG_Q1) \
    f(HV_SIMD_FP_REG_Q2) \
    f(HV_SIMD_FP_REG_Q3) \
    f(HV_SIMD_FP_REG_Q4) \
    f(HV_SIMD_FP_REG_Q5) \
    f(HV_SIMD_FP_REG_Q6) \
    f(HV_SIMD_FP_REG_Q7) \
    f(HV_SIMD_FP_REG_Q8) \
    f(HV_SIMD_FP_REG_Q9) \
    f(HV_SIMD_FP_REG_Q10) \
    f(HV_SIMD_FP_REG_Q11) \
    f(HV_SIMD_FP_REG_Q12) \
    f(HV_SIMD_FP_REG_Q13) \
    f(HV_SIMD_FP_REG_Q14) \
    f(HV_SIMD_FP_REG_Q15) \
    f(HV_SIMD_FP_REG_Q16) \
    f(HV_SIMD_FP_REG_Q17) \
    f(HV_SIMD_FP_REG_Q18) \
    f(HV_SIMD_FP_REG_Q19) \
    f(HV_SIMD_FP_REG_Q20) \
    f(HV_SIMD_FP_REG_Q21) \
    f(HV_SIMD_FP_REG_Q22) \
    f(HV_SIMD_FP_REG_Q23) \
    f(HV_SIMD_FP_REG_Q24) \
    f(HV_SIMD_FP_REG_Q25) \
    f(HV_SIMD_FP_REG_Q26) \
    f(HV_SIMD_FP_REG_Q27) \
    f(HV_SIMD_FP_REG_Q28) \
    f(HV_SIMD_FP_REG_Q29) \
    f(HV_SIMD_FP_REG_Q30) \
    f(HV_SIMD_FP_REG_Q31) \

#define LIST_HVF_SYS_REGISTERS(f) \
    f(HV_SYS_REG_APDAKEYHI_EL1) \
    f(HV_SYS_REG_APDAKEYLO_EL1) \
    f(HV_SYS_REG_APDBKEYHI_EL1) \
    f(HV_SYS_REG_APDBKEYLO_EL1) \
    f(HV_SYS_REG_APGAKEYHI_EL1) \
    f(HV_SYS_REG_APGAKEYLO_EL1) \
    f(HV_SYS_REG_APIAKEYHI_EL1) \
    f(HV_SYS_REG_APIAKEYLO_EL1) \
    f(HV_SYS_REG_APIBKEYHI_EL1) \
    f(HV_SYS_REG_APIBKEYLO_EL1) \
    f(HV_SYS_REG_DBGBCR0_EL1) \
    f(HV_SYS_REG_DBGBCR10_EL1) \
    f(HV_SYS_REG_DBGBCR11_EL1) \
    f(HV_SYS_REG_DBGBCR12_EL1) \
    f(HV_SYS_REG_DBGBCR13_EL1) \
    f(HV_SYS_REG_DBGBCR14_EL1) \
    f(HV_SYS_REG_DBGBCR15_EL1) \
    f(HV_SYS_REG_DBGBCR1_EL1) \
    f(HV_SYS_REG_DBGBCR2_EL1) \
    f(HV_SYS_REG_DBGBCR3_EL1) \
    f(HV_SYS_REG_DBGBCR4_EL1) \
    f(HV_SYS_REG_DBGBCR5_EL1) \
    f(HV_SYS_REG_DBGBCR6_EL1) \
    f(HV_SYS_REG_DBGBCR7_EL1) \
    f(HV_SYS_REG_DBGBCR8_EL1) \
    f(HV_SYS_REG_DBGBCR9_EL1) \
    f(HV_SYS_REG_DBGBVR0_EL1) \
    f(HV_SYS_REG_DBGBVR10_EL1) \
    f(HV_SYS_REG_DBGBVR11_EL1) \
    f(HV_SYS_REG_DBGBVR12_EL1) \
    f(HV_SYS_REG_DBGBVR13_EL1) \
    f(HV_SYS_REG_DBGBVR14_EL1) \
    f(HV_SYS_REG_DBGBVR15_EL1) \
    f(HV_SYS_REG_DBGBVR1_EL1) \
    f(HV_SYS_REG_DBGBVR2_EL1) \
    f(HV_SYS_REG_DBGBVR3_EL1) \
    f(HV_SYS_REG_DBGBVR4_EL1) \
    f(HV_SYS_REG_DBGBVR5_EL1) \
    f(HV_SYS_REG_DBGBVR6_EL1) \
    f(HV_SYS_REG_DBGBVR7_EL1) \
    f(HV_SYS_REG_DBGBVR8_EL1) \
    f(HV_SYS_REG_DBGBVR9_EL1) \
    f(HV_SYS_REG_DBGWCR0_EL1) \
    f(HV_SYS_REG_DBGWCR10_EL1) \
    f(HV_SYS_REG_DBGWCR11_EL1) \
    f(HV_SYS_REG_DBGWCR12_EL1) \
    f(HV_SYS_REG_DBGWCR13_EL1) \
    f(HV_SYS_REG_DBGWCR14_EL1) \
    f(HV_SYS_REG_DBGWCR15_EL1) \
    f(HV_SYS_REG_DBGWCR1_EL1) \
    f(HV_SYS_REG_DBGWCR2_EL1) \
    f(HV_SYS_REG_DBGWCR3_EL1) \
    f(HV_SYS_REG_DBGWCR4_EL1) \
    f(HV_SYS_REG_DBGWCR5_EL1) \
    f(HV_SYS_REG_DBGWCR6_EL1) \
    f(HV_SYS_REG_DBGWCR7_EL1) \
    f(HV_SYS_REG_DBGWCR8_EL1) \
    f(HV_SYS_REG_DBGWCR9_EL1) \
    f(HV_SYS_REG_DBGWVR0_EL1) \
    f(HV_SYS_REG_DBGWVR10_EL1) \
    f(HV_SYS_REG_DBGWVR11_EL1) \
    f(HV_SYS_REG_DBGWVR12_EL1) \
    f(HV_SYS_REG_DBGWVR13_EL1) \
    f(HV_SYS_REG_DBGWVR14_EL1) \
    f(HV_SYS_REG_DBGWVR15_EL1) \
    f(HV_SYS_REG_DBGWVR1_EL1) \
    f(HV_SYS_REG_DBGWVR2_EL1) \
    f(HV_SYS_REG_DBGWVR3_EL1) \
    f(HV_SYS_REG_DBGWVR4_EL1) \
    f(HV_SYS_REG_DBGWVR5_EL1) \
    f(HV_SYS_REG_DBGWVR6_EL1) \
    f(HV_SYS_REG_DBGWVR7_EL1) \
    f(HV_SYS_REG_DBGWVR8_EL1) \
    f(HV_SYS_REG_DBGWVR9_EL1) \
    f(HV_SYS_REG_MDCCINT_EL1) \
    f(HV_SYS_REG_AFSR0_EL1) \
    f(HV_SYS_REG_AFSR1_EL1) \
    f(HV_SYS_REG_AMAIR_EL1) \
    f(HV_SYS_REG_CNTKCTL_EL1) \
    f(HV_SYS_REG_CNTV_CVAL_EL0) \
    f(HV_SYS_REG_CONTEXTIDR_EL1) \
    f(HV_SYS_REG_CPACR_EL1) \
    f(HV_SYS_REG_CSSELR_EL1) \
    f(HV_SYS_REG_ELR_EL1) \
    f(HV_SYS_REG_ESR_EL1) \
    f(HV_SYS_REG_FAR_EL1) \
    f(HV_SYS_REG_MAIR_EL1) \
    f(HV_SYS_REG_MDSCR_EL1) \
    f(HV_SYS_REG_MIDR_EL1) \
    f(HV_SYS_REG_MPIDR_EL1) \
    f(HV_SYS_REG_PAR_EL1) \
    f(HV_SYS_REG_SCTLR_EL1) \
    f(HV_SYS_REG_SPSR_EL1) \
    f(HV_SYS_REG_SP_EL0) \
    f(HV_SYS_REG_SP_EL1) \
    f(HV_SYS_REG_TCR_EL1) \
    f(HV_SYS_REG_TPIDRRO_EL0) \
    f(HV_SYS_REG_TPIDR_EL0) \
    f(HV_SYS_REG_TPIDR_EL1) \
    f(HV_SYS_REG_TTBR0_EL1) \
    f(HV_SYS_REG_TTBR1_EL1) \
    f(HV_SYS_REG_VBAR_EL1) \
