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

typedef struct arm64_reg {
    union {
        // The 64 bit Xn register.
        struct {
            uint64_t xn;
        };
        // The lower 32-bits Wn register.
        struct {
            uint32_t wn;
        };
        // Special registers.
        struct { uint64_t xzr; };
        struct { uint32_t wzr; };
        struct { uint64_t sp; };
        struct { uint32_t wsp; };
        struct { uint64_t pc; };
        struct { uint32_t spsr; };

        struct {
            uint32_t spsr_mlo : 4; // 3:0
            uint32_t spsr_m : 1; // 4
            uint32_t spsr_unused0 : 1; // 5
            uint32_t spsr_f : 1; // 6
            uint32_t spsr_i : 1; // 7
            uint32_t spsr_a : 1; // 8
            uint32_t spsr_d : 1; // 9
            uint32_t spsr_unused1 : 10; // 19:10
            uint32_t spsr_il : 1; // 20
            uint32_t spsr_ss : 1; // 21
            uint32_t spsr_unused2 : 6; // 27:22
            uint32_t spsr_v : 1; // 28
            uint32_t spsr_c : 1; // 29
            uint32_t spsr_z : 1; // 30
            uint32_t spsr_n : 1; // 31
        };

        struct {
            uint32_t sctlrel1_m : 1; // 0
            uint32_t sctlrel1_a : 1; // 1
            uint32_t sctlrel1_c : 1; // 2
            uint32_t sctlrel1_sa : 1; // 3
            uint32_t sctlrel1_s0 : 1; // 4
            uint32_t sctlrel1_cp15ben : 1; // 5
            uint32_t sctlrel1_unused0 : 1; // 6
            uint32_t sctlrel1_itd : 1; // 7
            uint32_t sctlrel1_sed : 1; // 8
            uint32_t sctlrel1_uma : 1; // 9
            uint32_t sctlrel1_unused1 : 2; // 11:10
            uint32_t sctlrel1_i : 1; // 12
            uint32_t sctlrel1_unused2 : 1; // 13
            uint32_t sctlrel1_dze : 1; // 14
            uint32_t sctlrel1_uct : 1; // 15
            uint32_t sctlrel1_ntwi : 1; // 16
            uint32_t sctlrel1_unused3 : 1; // 17
            uint32_t sctlrel1_ntwe : 1; // 18
            uint32_t sctlrel1_wxn : 1; // 19
            uint32_t sctlrel1_unused4 : 4; // 23:20
            uint32_t sctlrel1_eoe : 1; // 24
            uint32_t sctlrel1_ee : 1; // 25
            uint32_t sctlrel1_uci : 1; // 26
            uint32_t sctlrel1_unused5 : 31; // 31:27
        };

        struct {
            uint32_t sctlrel23_m : 1; // 0
            uint32_t sctlrel23_a : 1; // 1
            uint32_t sctlrel23_c : 1; // 2
            uint32_t sctlrel23_sa : 1; // 3
            uint32_t sctlrel23_unused0 : 8; // 11:4
            uint32_t sctlrel23_i : 1; // 12
            uint32_t sctlrel23_unused1 : 1; // 18:13
            uint32_t sctlrel23_wxn : 1; // 19
            uint32_t sctlrel23_unused2 : 5; // 24:20
            uint32_t sctlrel23_ee : 1; // 25
            uint32_t sctlrel23_unused3 : 6; // 31:26
        };

    };
};

typedef struct arm128_reg {
    union {
        struct {
            uint128_t v;
        };
        struct {
            uint128_t q;
        };
        struct {
            uint64_t d[2];
        };
        struct {
            uint32_t s[4];
        };
        struct {
            uint16_t h[8];
        };
        struct {
            uint8_t b[16];
        };
    };
};

struct hvf_arm64_state {
    // Xn/Wn regs
    struct arm64_reg xregs[32];
    // Zero register
    struct arm64_reg zr;
    // Stack pointer (split by exception level)
    struct arm64_reg sp[4];
    // Program counter
    struct arm64_reg pc;
    // System registers
    struct arm64_reg actr_el[4];
    struct arm64_reg ccsidr_el[2];
    struct arm64_reg clidr_el[4];
    struct arm64_reg cntfrq_el[1];
    struct arm64_reg cntpct_el[1];
    struct arm64_reg cntkctl_el[2];
    struct arm64_reg cntp_cval_el[1];
    struct arm64_reg cpacr_el[2];
    struct arm64_reg csselr_el[2];
    struct arm64_reg cntp_ctl_el[1];
    struct arm64_reg ctr_el[1];
    struct arm64_reg dczid_el[1];
    struct arm64_reg elr_el[4];
    struct arm64_reg esr_el[4];
    struct arm64_reg far_el[4];
    struct arm64_reg fpcr;
    struct arm64_reg fpsr;
    struct arm64_reg hcr_el[3];
    struct arm64_reg mair_el[4];
    struct arm64_reg midr_el[2];
    struct arm64_reg mpidr_el[2];
    struct arm64_reg scr_el[4];
    struct arm64_reg sctlr_el[4];
    struct arm64_reg spsr_el[8]; // abt: 4 fiq: 5 irq: 6 und: 7
    struct arm64_reg tcr_el[4];
    struct arm64_reg tpidr_el[4];
    struct arm64_reg tpidrro_el[1];
    struct arm64_reg ttbr0_el[4];
    struct arm64_reg ttbr1_el[2];
    struct arm64_reg vbar_el[4];
    struct arm64_reg vtcr_el[2];
    struct arm64_reg vttbr_el[2];

    struct arm128_reg vregs[32];

    // Exception level
    int el;

    int irq;
};
