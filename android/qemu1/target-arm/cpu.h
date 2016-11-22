/*
 * ARM virtual CPU header
 *
 *  Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CPU_ARM_H
#define CPU_ARM_H

#include "config.h"

#include "kvm-consts.h"

#if defined(TARGET_AARCH64)
  /* AArch64 definitions */
#  define TARGET_LONG_BITS 64
#  define ELF_MACHINE EM_AARCH64
#else
#  define TARGET_LONG_BITS 32
#  define ELF_MACHINE EM_ARM
#endif

// TODO(digit): Remove this line.
#define CPUOldState struct CPUARMState

#define CPUArchState struct CPUARMState

#include "qemu-common.h"
#include "exec/cpu-defs.h"

#include "fpu/softfloat.h"

#define TARGET_HAS_ICE 1

#define EXCP_UDEF            1   /* undefined instruction */
#define EXCP_SWI             2   /* software interrupt */
#define EXCP_PREFETCH_ABORT  3
#define EXCP_DATA_ABORT      4
#define EXCP_IRQ             5
#define EXCP_FIQ             6
#define EXCP_BKPT            7
#define EXCP_EXCEPTION_EXIT  8   /* Return from v7M exception.  */
#define EXCP_KERNEL_TRAP     9   /* Jumped to kernel code page.  */
#define EXCP_STREX          10
#define EXCP_SMC            11   /* secure monitor call */

#define ARMV7M_EXCP_RESET   1
#define ARMV7M_EXCP_NMI     2
#define ARMV7M_EXCP_HARD    3
#define ARMV7M_EXCP_MEM     4
#define ARMV7M_EXCP_BUS     5
#define ARMV7M_EXCP_USAGE   6
#define ARMV7M_EXCP_SVC     11
#define ARMV7M_EXCP_DEBUG   12
#define ARMV7M_EXCP_PENDSV  14
#define ARMV7M_EXCP_SYSTICK 15

/* ARM-specific interrupt pending bits.  */
#define CPU_INTERRUPT_FIQ   CPU_INTERRUPT_TGT_EXT_1

/* Meanings of the ARMCPU object's two inbound GPIO lines */
#define ARM_CPU_IRQ 0
#define ARM_CPU_FIQ 1

typedef void ARMWriteCPFunc(void *opaque, int cp_info,
                            int srcreg, int operand, uint32_t value,
                            void *retaddr);
typedef uint32_t ARMReadCPFunc(void *opaque, int cp_info,
                               int dstreg, int operand,
                               void *retaddr);

struct arm_boot_info;

#define NB_MMU_MODES 2

/* We currently assume float and double are IEEE single and double
   precision respectively.
   Doing runtime conversions is tricky because VFP registers may contain
   integer values (eg. as the result of a FTOSI instruction).
   s<2n> maps to the least significant half of d<n>
   s<2n+1> maps to the most significant half of d<n>
 */

/* CPU state for each instance of a generic timer (in cp15 c14) */
typedef struct ARMGenericTimer {
    uint64_t cval; /* Timer CompareValue register */
    uint32_t ctl; /* Timer Control register */
} ARMGenericTimer;

#define GTIMER_PHYS 0
#define GTIMER_VIRT 1
#define NUM_GTIMERS 2

/* Scale factor for generic timers, ie number of ns per tick.
 * This gives a 62.5MHz timer.
 */
#define GTIMER_SCALE 16

typedef struct CPUARMState {
    /* Regs for current mode.  */
    uint32_t regs[16];
    /* Frequently accessed CPSR bits are stored separately for efficiently.
       This contains all the other bits.  Use cpsr_{read,write} to access
       the whole CPSR.  */
    uint32_t uncached_cpsr;
    uint32_t spsr;

    /* Banked registers.  */
    uint32_t banked_spsr[7];
    uint32_t banked_r13[7];
    uint32_t banked_r14[7];

    /* These hold r8-r12.  */
    uint32_t usr_regs[5];
    uint32_t fiq_regs[5];

    /* cpsr flag cache for faster execution */
    uint32_t CF; /* 0 or 1 */
    uint32_t VF; /* V is the bit 31. All other bits are undefined */
    uint32_t NF; /* N is bit 31. All other bits are undefined.  */
    uint32_t ZF; /* Z set if zero.  */
    uint32_t QF; /* 0 or 1 */
    uint32_t GE; /* cpsr[19:16] */
    uint32_t thumb; /* cpsr[5]. 0 = arm mode, 1 = thumb mode. */
    uint32_t condexec_bits; /* IT bits.  cpsr[15:10,26:25].  */

    /* System control coprocessor (cp15) */
    struct {
        uint32_t c0_cpuid;
        uint32_t c0_cachetype;
        uint32_t c0_ccsid[16]; /* Cache size.  */
        uint32_t c0_clid; /* Cache level.  */
        uint32_t c0_cssel; /* Cache size selection.  */
        uint32_t c0_c1[8]; /* Feature registers.  */
        uint32_t c0_c2[8]; /* Instruction set registers.  */
        uint32_t c1_sys; /* System control register.  */
        uint32_t c1_coproc; /* Coprocessor access register.  */
        uint32_t c1_xscaleauxcr; /* XScale auxiliary control register.  */
        uint32_t c1_secfg; /* Secure configuration register. */
        uint32_t c1_sedbg; /* Secure debug enable register. */
        uint32_t c1_nseac; /* Non-secure access control register. */
        uint32_t c2_base0; /* MMU translation table base 0.  */
        uint32_t c2_base1; /* MMU translation table base 1.  */
        uint32_t c2_control; /* MMU translation table base control.  */
        uint32_t c2_mask; /* MMU translation table base selection mask.  */
        uint32_t c2_base_mask; /* MMU translation table base 0 mask. */
        uint32_t c2_data; /* MPU data cachable bits.  */
        uint32_t c2_insn; /* MPU instruction cachable bits.  */
        uint32_t c3; /* MMU domain access control register
                        MPU write buffer control.  */
        uint32_t c5_insn; /* Fault status registers.  */
        uint32_t c5_data;
        uint32_t c6_region[8]; /* MPU base/size registers.  */
        uint32_t c6_insn; /* Fault address registers.  */
        uint32_t c6_data;
        uint32_t c7_par;  /* Translation result. */
        uint32_t c9_insn; /* Cache lockdown registers.  */
        uint32_t c9_data;
        uint32_t c9_pmcr; /* performance monitor control register */
        uint32_t c9_pmcnten; /* perf monitor counter enables */
        uint32_t c9_pmovsr; /* perf monitor overflow status */
        uint32_t c9_pmxevtyper; /* perf monitor event type */
        uint32_t c9_pmuserenr; /* perf monitor user enable */
        uint32_t c9_pminten; /* perf monitor interrupt enables */
        uint32_t c12_vbar; /* secure/nonsecure vector base address register. */
        uint32_t c12_mvbar; /* monitor vector base address register. */
        uint32_t c13_fcse; /* FCSE PID.  */
        uint32_t c13_context; /* Context ID.  */
        uint32_t c13_tls1; /* User RW Thread register.  */
        uint32_t c13_tls2; /* User RO Thread register.  */
        uint32_t c13_tls3; /* Privileged Thread register.  */
        uint32_t c15_cpar; /* XScale Coprocessor Access Register */
        uint32_t c15_ticonfig; /* TI925T configuration byte.  */
        uint32_t c15_i_max; /* Maximum D-cache dirty line index.  */
        uint32_t c15_i_min; /* Minimum D-cache dirty line index.  */
        uint32_t c15_threadid; /* TI debugger thread-ID.  */
    } cp15;

    struct {
        uint32_t other_sp;
        uint32_t vecbase;
        uint32_t basepri;
        uint32_t control;
        int current_sp;
        int exception;
        int pending_exception;
    } v7m;

    /* Minimal set of debug coprocessor state (cp14) */
    uint32_t cp14_dbgdidr;

    /* Thumb-2 EE state.  */
    uint32_t teecr;
    uint32_t teehbr;

    /* Internal CPU feature flags.  */
    uint32_t features;

    /* VFP coprocessor state.  */
    struct {
        float64 regs[32];

        uint32_t xregs[16];
        /* We store these fpcsr fields separately for convenience.  */
        int vec_len;
        int vec_stride;

        /* scratch space when Tn are not sufficient.  */
        uint32_t scratch[8];

        /* fp_status is the "normal" fp status. standard_fp_status retains
         * values corresponding to the ARM "Standard FPSCR Value", ie
         * default-NaN, flush-to-zero, round-to-nearest and is used by
         * any operations (generally Neon) which the architecture defines
         * as controlled by the standard FPSCR value rather than the FPSCR.
         *
         * To avoid having to transfer exception bits around, we simply
         * say that the FPSCR cumulative exception flags are the logical
         * OR of the flags in the two fp statuses. This relies on the
         * only thing which needs to read the exception flags being
         * an explicit FPSCR read.
         */
        float_status fp_status;
        float_status standard_fp_status;
    } vfp;
    uint32_t exclusive_addr;
    uint32_t exclusive_val;
    uint32_t exclusive_high;
#if defined(CONFIG_USER_ONLY)
    uint32_t exclusive_test;
    uint32_t exclusive_info;
#endif

    /* iwMMXt coprocessor state.  */
    struct {
        uint64_t regs[16];
        uint64_t val;

        uint32_t cregs[16];
    } iwmmxt;

#if defined(CONFIG_USER_ONLY)
    /* For usermode syscall translation.  */
    int eabi;
#endif

    CPU_COMMON

    /* These fields after the common ones so they are preserved on reset.  */

    /* Coprocessor IO used by peripherals */
    struct {
        ARMReadCPFunc *cp_read;
        ARMWriteCPFunc *cp_write;
        void *opaque;
    } cp[15];
    void *nvic;
    const struct arm_boot_info *boot_info;
} CPUARMState;

#include "cpu-qom.h"

CPUARMState *cpu_arm_init(const char *cpu_model);
void arm_translate_init(void);
int cpu_arm_exec(CPUARMState *s);
void cpu_arm_close(CPUARMState *s);
void do_interrupt(CPUARMState *);
void switch_mode(CPUARMState *, int);
uint32_t do_arm_semihosting(CPUARMState *env);

static inline bool is_a64(CPUARMState *env)
{
#ifdef CONFIG_ANDROID // TODO(digit)
    return 0;
#else
    return env->aarch64;
#endif
}

#define PSTATE_N_SHIFT 3
#define PSTATE_N  (1 << PSTATE_N_SHIFT)
#define PSTATE_Z_SHIFT 2
#define PSTATE_Z  (1 << PSTATE_Z_SHIFT)
#define PSTATE_C_SHIFT 1
#define PSTATE_C  (1 << PSTATE_C_SHIFT)
#define PSTATE_V_SHIFT 0
#define PSTATE_V  (1 << PSTATE_V_SHIFT)

/* you can call this signal handler from your SIGBUS and SIGSEGV
   signal handlers to inform the virtual CPU of exceptions. non zero
   is returned if the signal was handled by the virtual CPU.  */
int cpu_arm_signal_handler(int host_signum, void *pinfo,
                           void *puc);
int cpu_arm_handle_mmu_fault (CPUARMState *env, target_ulong address, int rw,
                              int mmu_idx);
#define cpu_handle_mmu_fault cpu_arm_handle_mmu_fault

static inline void cpu_set_tls(CPUARMState *env, target_ulong newtls)
{
  env->cp15.c13_tls2 = newtls;
}

#define CPSR_M (0x1fU)
#define CPSR_T (1U << 5)
#define CPSR_F (1U << 6)
#define CPSR_I (1U << 7)
#define CPSR_A (1U << 8)
#define CPSR_E (1U << 9)
#define CPSR_IT_2_7 (0xfc00U)
#define CPSR_GE (0xfU << 16)
#define CPSR_RESERVED (0xfU << 20)
#define CPSR_J (1U << 24)
#define CPSR_IT_0_1 (3U << 25)
#define CPSR_Q (1U << 27)
#define CPSR_V (1U << 28)
#define CPSR_C (1U << 29)
#define CPSR_Z (1U << 30)
#define CPSR_N (1U << 31)
#define CPSR_NZCV (CPSR_N | CPSR_Z | CPSR_C | CPSR_V)

#define CPSR_IT (CPSR_IT_0_1 | CPSR_IT_2_7)
#define CACHED_CPSR_BITS (CPSR_T | CPSR_GE | CPSR_IT | CPSR_Q | CPSR_NZCV)
/* Bits writable in user mode.  */
#define CPSR_USER (CPSR_NZCV | CPSR_Q | CPSR_GE)
/* Execution state bits.  MRS read as zero, MSR writes ignored.  */
#define CPSR_EXEC (CPSR_T | CPSR_IT | CPSR_J)

/* Return the current CPSR value.  */
uint32_t cpsr_read(CPUARMState *env);
/* Set the CPSR.  Note that some bits of mask must be all-set or all-clear.  */
void cpsr_write(CPUARMState *env, uint32_t val, uint32_t mask);

/* Return the current xPSR value.  */
static inline uint32_t xpsr_read(CPUARMState *env)
{
    int ZF;
    ZF = (env->ZF == 0);
    return (env->NF & 0x80000000) | (ZF << 30)
        | (env->CF << 29) | ((env->VF & 0x80000000) >> 3) | (env->QF << 27)
        | (env->thumb << 24) | ((env->condexec_bits & 3) << 25)
        | ((env->condexec_bits & 0xfc) << 8)
        | env->v7m.exception;
}

/* Set the xPSR.  Note that some bits of mask must be all-set or all-clear.  */
static inline void xpsr_write(CPUARMState *env, uint32_t val, uint32_t mask)
{
    if (mask & CPSR_NZCV) {
        env->ZF = (~val) & CPSR_Z;
        env->NF = val;
        env->CF = (val >> 29) & 1;
        env->VF = (val << 3) & 0x80000000;
    }
    if (mask & CPSR_Q)
        env->QF = ((val & CPSR_Q) != 0);
    if (mask & (1 << 24))
        env->thumb = ((val & (1 << 24)) != 0);
    if (mask & CPSR_IT_0_1) {
        env->condexec_bits &= ~3;
        env->condexec_bits |= (val >> 25) & 3;
    }
    if (mask & CPSR_IT_2_7) {
        env->condexec_bits &= 3;
        env->condexec_bits |= (val >> 8) & 0xfc;
    }
    if (mask & 0x1ff) {
        env->v7m.exception = val & 0x1ff;
    }
}

/* Return the current FPSCR value.  */
uint32_t vfp_get_fpscr(CPUARMState *env);
void vfp_set_fpscr(CPUARMState *env, uint32_t val);

enum arm_cpu_mode {
  ARM_CPU_MODE_USR = 0x10,
  ARM_CPU_MODE_FIQ = 0x11,
  ARM_CPU_MODE_IRQ = 0x12,
  ARM_CPU_MODE_SVC = 0x13,
  ARM_CPU_MODE_SMC = 0x16,
  ARM_CPU_MODE_ABT = 0x17,
  ARM_CPU_MODE_UND = 0x1b,
  ARM_CPU_MODE_SYS = 0x1f
};

/* VFP system registers.  */
#define ARM_VFP_FPSID   0
#define ARM_VFP_FPSCR   1
#define ARM_VFP_MVFR1   6
#define ARM_VFP_MVFR0   7
#define ARM_VFP_FPEXC   8
#define ARM_VFP_FPINST  9
#define ARM_VFP_FPINST2 10

/* iwMMXt coprocessor control registers.  */
#define ARM_IWMMXT_wCID		0
#define ARM_IWMMXT_wCon		1
#define ARM_IWMMXT_wCSSF	2
#define ARM_IWMMXT_wCASF	3
#define ARM_IWMMXT_wCGR0	8
#define ARM_IWMMXT_wCGR1	9
#define ARM_IWMMXT_wCGR2	10
#define ARM_IWMMXT_wCGR3	11

/* If adding a feature bit which corresponds to a Linux ELF
 * HWCAP bit, remember to update the feature-bit-to-hwcap
 * mapping in linux-user/elfload.c:get_elf_hwcap().
 */
enum arm_features {
    ARM_FEATURE_VFP,
    ARM_FEATURE_AUXCR,  /* ARM1026 Auxiliary control register.  */
    ARM_FEATURE_XSCALE, /* Intel XScale extensions.  */
    ARM_FEATURE_IWMMXT, /* Intel iwMMXt extension.  */
    ARM_FEATURE_V6,
    ARM_FEATURE_V6K,
    ARM_FEATURE_V7,
    ARM_FEATURE_THUMB2,
    ARM_FEATURE_MPU,    /* Only has Memory Protection Unit, not full MMU.  */
    ARM_FEATURE_VFP3,
    ARM_FEATURE_VFP_FP16,
    ARM_FEATURE_NEON,
    ARM_FEATURE_DIV,
    ARM_FEATURE_M, /* Microcontroller profile.  */
    ARM_FEATURE_OMAPCP, /* OMAP specific CP15 ops handling.  */
    ARM_FEATURE_THUMB2EE,
    ARM_FEATURE_V7MP,    /* v7 Multiprocessing Extensions */
    ARM_FEATURE_V4T,
    ARM_FEATURE_V5,
    ARM_FEATURE_STRONGARM,
    ARM_FEATURE_VAPA, /* cp15 VA to PA lookups */
    ARM_FEATURE_TRUSTZONE, /* TrustZone Security Extensions. */
};

static inline int arm_feature(CPUARMState *env, int feature)
{
    return (env->features & (1ULL << feature)) != 0;
}

void arm_cpu_list(FILE *f, fprintf_function cpu_fprintf);

/* Interface between CPU and Interrupt controller.  */
void armv7m_nvic_set_pending(void *opaque, int irq);
int armv7m_nvic_acknowledge_irq(void *opaque);
void armv7m_nvic_complete_irq(void *opaque, int irq);

/* Interface for defining coprocessor registers.
 * Registers are defined in tables of arm_cp_reginfo structs
 * which are passed to define_arm_cp_regs().
 */

/* When looking up a coprocessor register we look for it
 * via an integer which encodes all of:
 *  coprocessor number
 *  Crn, Crm, opc1, opc2 fields
 *  32 or 64 bit register (ie is it accessed via MRC/MCR
 *    or via MRRC/MCRR?)
 * We allow 4 bits for opc1 because MRRC/MCRR have a 4 bit field.
 * (In this case crn and opc2 should be zero.)
 */
#define ENCODE_CP_REG(cp, is64, crn, crm, opc1, opc2)   \
    (((cp) << 16) | ((is64) << 15) | ((crn) << 11) |    \
     ((crm) << 7) | ((opc1) << 3) | (opc2))

/* Convert a full 64 bit KVM register ID to the truncated 32 bit
 * version used as a key for the coprocessor register hashtable
 */
static inline uint32_t kvm_to_cpreg_id(uint64_t kvmid)
{
    uint32_t cpregid = kvmid;
    if ((kvmid & CP_REG_SIZE_MASK) == CP_REG_SIZE_U64) {
        cpregid |= (1 << 15);
    }
    return cpregid;
}

/* Convert a truncated 32 bit hashtable key into the full
 * 64 bit KVM register ID.
 */
static inline uint64_t cpreg_to_kvm_id(uint32_t cpregid)
{
    uint64_t kvmid = cpregid & ~(1 << 15);
    if (cpregid & (1 << 15)) {
        kvmid |= CP_REG_SIZE_U64 | CP_REG_ARM;
    } else {
        kvmid |= CP_REG_SIZE_U32 | CP_REG_ARM;
    }
    return kvmid;
}

/* ARMCPRegInfo type field bits. If the SPECIAL bit is set this is a
 * special-behaviour cp reg and bits [15..8] indicate what behaviour
 * it has. Otherwise it is a simple cp reg, where CONST indicates that
 * TCG can assume the value to be constant (ie load at translate time)
 * and 64BIT indicates a 64 bit wide coprocessor register. SUPPRESS_TB_END
 * indicates that the TB should not be ended after a write to this register
 * (the default is that the TB ends after cp writes). OVERRIDE permits
 * a register definition to override a previous definition for the
 * same (cp, is64, crn, crm, opc1, opc2) tuple: either the new or the
 * old must have the OVERRIDE bit set.
 * NO_MIGRATE indicates that this register should be ignored for migration;
 * (eg because any state is accessed via some other coprocessor register).
 * IO indicates that this register does I/O and therefore its accesses
 * need to be surrounded by gen_io_start()/gen_io_end(). In particular,
 * registers which implement clocks or timers require this.
 */
#define ARM_CP_SPECIAL 1
#define ARM_CP_CONST 2
#define ARM_CP_64BIT 4
#define ARM_CP_SUPPRESS_TB_END 8
#define ARM_CP_OVERRIDE 16
#define ARM_CP_NO_MIGRATE 32
#define ARM_CP_IO 64
#define ARM_CP_NOP (ARM_CP_SPECIAL | (1 << 8))
#define ARM_CP_WFI (ARM_CP_SPECIAL | (2 << 8))
#define ARM_LAST_SPECIAL ARM_CP_WFI
/* Used only as a terminator for ARMCPRegInfo lists */
#define ARM_CP_SENTINEL 0xffff
/* Mask of only the flag bits in a type field */
#define ARM_CP_FLAG_MASK 0x7f

/* Return true if cptype is a valid type field. This is used to try to
 * catch errors where the sentinel has been accidentally left off the end
 * of a list of registers.
 */
static inline bool cptype_valid(int cptype)
{
    return ((cptype & ~ARM_CP_FLAG_MASK) == 0)
        || ((cptype & ARM_CP_SPECIAL) &&
            ((cptype & ~ARM_CP_FLAG_MASK) <= ARM_LAST_SPECIAL));
}

/* Access rights:
 * We define bits for Read and Write access for what rev C of the v7-AR ARM ARM
 * defines as PL0 (user), PL1 (fiq/irq/svc/abt/und/sys, ie privileged), and
 * PL2 (hyp). The other level which has Read and Write bits is Secure PL1
 * (ie any of the privileged modes in Secure state, or Monitor mode).
 * If a register is accessible in one privilege level it's always accessible
 * in higher privilege levels too. Since "Secure PL1" also follows this rule
 * (ie anything visible in PL2 is visible in S-PL1, some things are only
 * visible in S-PL1) but "Secure PL1" is a bit of a mouthful, we bend the
 * terminology a little and call this PL3.
 *
 * If access permissions for a register are more complex than can be
 * described with these bits, then use a laxer set of restrictions, and
 * do the more restrictive/complex check inside a helper function.
 */
#define PL3_R 0x80
#define PL3_W 0x40
#define PL2_R (0x20 | PL3_R)
#define PL2_W (0x10 | PL3_W)
#define PL1_R (0x08 | PL2_R)
#define PL1_W (0x04 | PL2_W)
#define PL0_R (0x02 | PL1_R)
#define PL0_W (0x01 | PL1_W)

#define PL3_RW (PL3_R | PL3_W)
#define PL2_RW (PL2_R | PL2_W)
#define PL1_RW (PL1_R | PL1_W)
#define PL0_RW (PL0_R | PL0_W)

static inline int arm_current_pl(CPUARMState *env)
{
    if ((env->uncached_cpsr & 0x1f) == ARM_CPU_MODE_USR) {
        return 0;
    }
    /* We don't currently implement the Virtualization or TrustZone
     * extensions, so PL2 and PL3 don't exist for us.
     */
    return 1;
}

typedef struct ARMCPRegInfo ARMCPRegInfo;

/* Access functions for coprocessor registers. These should return
 * 0 on success, or one of the EXCP_* constants if access should cause
 * an exception (in which case *value is not written).
 */
typedef int CPReadFn(CPUARMState *env, const ARMCPRegInfo *opaque,
                     uint64_t *value);
typedef int CPWriteFn(CPUARMState *env, const ARMCPRegInfo *opaque,
                      uint64_t value);
/* Hook function for register reset */
typedef void CPResetFn(CPUARMState *env, const ARMCPRegInfo *opaque);

#define CP_ANY 0xff

/* Definition of an ARM coprocessor register */
struct ARMCPRegInfo {
    /* Name of register (useful mainly for debugging, need not be unique) */
    const char *name;
    /* Location of register: coprocessor number and (crn,crm,opc1,opc2)
     * tuple. Any of crm, opc1 and opc2 may be CP_ANY to indicate a
     * 'wildcard' field -- any value of that field in the MRC/MCR insn
     * will be decoded to this register. The register read and write
     * callbacks will be passed an ARMCPRegInfo with the crn/crm/opc1/opc2
     * used by the program, so it is possible to register a wildcard and
     * then behave differently on read/write if necessary.
     * For 64 bit registers, only crm and opc1 are relevant; crn and opc2
     * must both be zero.
     */
    uint8_t cp;
    uint8_t crn;
    uint8_t crm;
    uint8_t opc1;
    uint8_t opc2;
    /* Register type: ARM_CP_* bits/values */
    int type;
    /* Access rights: PL*_[RW] */
    int access;
    /* The opaque pointer passed to define_arm_cp_regs_with_opaque() when
     * this register was defined: can be used to hand data through to the
     * register read/write functions, since they are passed the ARMCPRegInfo*.
     */
    void *opaque;
    /* Value of this register, if it is ARM_CP_CONST. Otherwise, if
     * fieldoffset is non-zero, the reset value of the register.
     */
    uint64_t resetvalue;
    /* Offset of the field in CPUARMState for this register. This is not
     * needed if either:
     *  1. type is ARM_CP_CONST or one of the ARM_CP_SPECIALs
     *  2. both readfn and writefn are specified
     */
    ptrdiff_t fieldoffset; /* offsetof(CPUARMState, field) */
    /* Function for handling reads of this register. If NULL, then reads
     * will be done by loading from the offset into CPUARMState specified
     * by fieldoffset.
     */
    CPReadFn *readfn;
    /* Function for handling writes of this register. If NULL, then writes
     * will be done by writing to the offset into CPUARMState specified
     * by fieldoffset.
     */
    CPWriteFn *writefn;
    /* Function for doing a "raw" read; used when we need to copy
     * coprocessor state to the kernel for KVM or out for
     * migration. This only needs to be provided if there is also a
     * readfn and it makes an access permission check.
     */
    CPReadFn *raw_readfn;
    /* Function for doing a "raw" write; used when we need to copy KVM
     * kernel coprocessor state into userspace, or for inbound
     * migration. This only needs to be provided if there is also a
     * writefn and it makes an access permission check or masks out
     * "unwritable" bits or has write-one-to-clear or similar behaviour.
     */
    CPWriteFn *raw_writefn;
    /* Function for resetting the register. If NULL, then reset will be done
     * by writing resetvalue to the field specified in fieldoffset. If
     * fieldoffset is 0 then no reset will be done.
     */
    CPResetFn *resetfn;
};

/* Macros which are lvalues for the field in CPUARMState for the
 * ARMCPRegInfo *ri.
 */
#define CPREG_FIELD32(env, ri) \
    (*(uint32_t *)((char *)(env) + (ri)->fieldoffset))
#define CPREG_FIELD64(env, ri) \
    (*(uint64_t *)((char *)(env) + (ri)->fieldoffset))

#define REGINFO_SENTINEL { .type = ARM_CP_SENTINEL }

#ifndef CONFIG_ANDROID  // TODO(digit): Implement ARMCPU
void define_arm_cp_regs_with_opaque(ARMCPU *cpu,
                                    const ARMCPRegInfo *regs, void *opaque);
void define_one_arm_cp_reg_with_opaque(ARMCPU *cpu,
                                       const ARMCPRegInfo *regs, void *opaque);
static inline void define_arm_cp_regs(ARMCPU *cpu, const ARMCPRegInfo *regs)
{
    define_arm_cp_regs_with_opaque(cpu, regs, 0);
}
static inline void define_one_arm_cp_reg(ARMCPU *cpu, const ARMCPRegInfo *regs)
{
    define_one_arm_cp_reg_with_opaque(cpu, regs, 0);
}
const ARMCPRegInfo *get_arm_cp_reginfo(ARMCPU *cpu, uint32_t encoded_cp);
#endif  // !CONFIG_ANDROID

/* CPWriteFn that can be used to implement writes-ignored behaviour */
int arm_cp_write_ignore(CPUARMState *env, const ARMCPRegInfo *ri,
                        uint64_t value);
/* CPReadFn that can be used for read-as-zero behaviour */
int arm_cp_read_zero(CPUARMState *env, const ARMCPRegInfo *ri, uint64_t *value);

static inline bool cp_access_ok(CPUARMState *env,
                                const ARMCPRegInfo *ri, int isread)
{
    return (ri->access >> ((arm_current_pl(env) * 2) + isread)) & 1;
}

#ifndef CONFIG_ANDROID  // TODO(digit): Implement ARMCPU
/**
 * write_list_to_cpustate
 * @cpu: ARMCPU
 *
 * For each register listed in the ARMCPU cpreg_indexes list, write
 * its value from the cpreg_values list into the ARMCPUARMState structure.
 * This updates TCG's working data structures from KVM data or
 * from incoming migration state.
 *
 * Returns: true if all register values were updated correctly,
 * false if some register was unknown or could not be written.
 * Note that we do not stop early on failure -- we will attempt
 * writing all registers in the list.
 */
bool write_list_to_cpustate(ARMCPU *cpu);

/**
 * write_cpustate_to_list:
 * @cpu: ARMCPU
 *
 * For each register listed in the ARMCPU cpreg_indexes list, write
 * its value from the ARMCPUARMState structure into the cpreg_values list.
 * This is used to copy info from TCG's working data structures into
 * KVM or for outbound migration.
 *
 * Returns: true if all register values were read correctly,
 * false if some register was unknown or could not be read.
 * Note that we do not stop early on failure -- we will attempt
 * reading all registers in the list.
 */
bool write_cpustate_to_list(ARMCPU *cpu);
#endif  // !CONFIG_ANDROID

void cpu_arm_set_cp_io(CPUARMState *env, int cpnum,
                       ARMReadCPFunc *cp_read, ARMWriteCPFunc *cp_write,
                       void *opaque);

/* Does the core conform to the the "MicroController" profile. e.g. Cortex-M3.
   Note the M in older cores (eg. ARM7TDMI) stands for Multiply. These are
   conventional cores (ie. Application or Realtime profile).  */

#define IS_M(env) arm_feature(env, ARM_FEATURE_M)
#define ARM_CPUID(env) (env->cp15.c0_cpuid)

#define ARM_CPUID_ARM1026     0x4106a262
#define ARM_CPUID_ARM926      0x41069265
#define ARM_CPUID_ARM946      0x41059461
#define ARM_CPUID_TI915T      0x54029152
#define ARM_CPUID_TI925T      0x54029252
#define ARM_CPUID_SA1100      0x4401A11B
#define ARM_CPUID_SA1110      0x6901B119
#define ARM_CPUID_PXA250      0x69052100
#define ARM_CPUID_PXA255      0x69052d00
#define ARM_CPUID_PXA260      0x69052903
#define ARM_CPUID_PXA261      0x69052d05
#define ARM_CPUID_PXA262      0x69052d06
#define ARM_CPUID_PXA270      0x69054110
#define ARM_CPUID_PXA270_A0   0x69054110
#define ARM_CPUID_PXA270_A1   0x69054111
#define ARM_CPUID_PXA270_B0   0x69054112
#define ARM_CPUID_PXA270_B1   0x69054113
#define ARM_CPUID_PXA270_C0   0x69054114
#define ARM_CPUID_PXA270_C5   0x69054117
#define ARM_CPUID_ARM1136     0x4117b363
#define ARM_CPUID_ARM1136_R2  0x4107b362
#define ARM_CPUID_ARM1176     0x410fb767
#define ARM_CPUID_ARM11MPCORE 0x410fb022
#define ARM_CPUID_CORTEXA8    0x410fc080
#define ARM_CPUID_CORTEXA8_R2 0x412fc083
#define ARM_CPUID_CORTEXA9    0x410fc090
#define ARM_CPUID_CORTEXM3    0x410fc231
#define ARM_CPUID_ANY         0xffffffff

#if defined(CONFIG_USER_ONLY)
#define TARGET_PAGE_BITS 12
#else
/* The ARM MMU allows 1k pages.  */
/* ??? Linux doesn't actually use these, and they're deprecated in recent
   architecture revisions.  Maybe a configure option to disable them.  */
#define TARGET_PAGE_BITS 10
#endif

#if defined(TARGET_AARCH64)
#  define TARGET_PHYS_ADDR_SPACE_BITS 48
#  define TARGET_VIRT_ADDR_SPACE_BITS 64
#else
#  define TARGET_PHYS_ADDR_SPACE_BITS 40
#  define TARGET_VIRT_ADDR_SPACE_BITS 32
#endif

#define cpu_init cpu_arm_init
#define cpu_exec cpu_arm_exec
#define cpu_gen_code cpu_arm_gen_code
#define cpu_signal_handler cpu_arm_signal_handler
#define cpu_list arm_cpu_list

#define CPU_SAVE_VERSION 4

/* MMU modes definitions */
#define MMU_MODE0_SUFFIX _kernel
#define MMU_MODE1_SUFFIX _user
#define MMU_USER_IDX 1
static inline int cpu_mmu_index (CPUARMState *env)
{
    return (env->uncached_cpsr & CPSR_M) == ARM_CPU_MODE_USR ? 1 : 0;
}

static inline int is_cpu_user (CPUARMState *env)
{
#ifdef CONFIG_USER_ONLY
    return 1;
#else
    return (env->uncached_cpsr & CPSR_M) == ARM_CPU_MODE_USR;
#endif  // CONFIG_USER_ONLY
}

#if defined(CONFIG_USER_ONLY)
static inline void cpu_clone_regs(CPUARMState *env, target_ulong newsp)
{
    if (newsp)
        env->regs[13] = newsp;
    env->regs[0] = 0;
}
#endif

#include "exec/cpu-all.h"

/* Bit usage in the TB flags field: bit 31 indicates whether we are
 * in 32 or 64 bit mode. The meaning of the other bits depends on that.
 */
#define ARM_TBFLAG_AARCH64_STATE_SHIFT 31
#define ARM_TBFLAG_AARCH64_STATE_MASK  (1U << ARM_TBFLAG_AARCH64_STATE_SHIFT)

/* Bit usage when in AArch32 state: */
#define ARM_TBFLAG_THUMB_SHIFT      0
#define ARM_TBFLAG_THUMB_MASK       (1 << ARM_TBFLAG_THUMB_SHIFT)
#define ARM_TBFLAG_VECLEN_SHIFT     1
#define ARM_TBFLAG_VECLEN_MASK      (0x7 << ARM_TBFLAG_VECLEN_SHIFT)
#define ARM_TBFLAG_VECSTRIDE_SHIFT  4
#define ARM_TBFLAG_VECSTRIDE_MASK   (0x3 << ARM_TBFLAG_VECSTRIDE_SHIFT)
#define ARM_TBFLAG_PRIV_SHIFT       6
#define ARM_TBFLAG_PRIV_MASK        (1 << ARM_TBFLAG_PRIV_SHIFT)
#define ARM_TBFLAG_VFPEN_SHIFT      7
#define ARM_TBFLAG_VFPEN_MASK       (1 << ARM_TBFLAG_VFPEN_SHIFT)
#define ARM_TBFLAG_CONDEXEC_SHIFT   8
#define ARM_TBFLAG_CONDEXEC_MASK    (0xff << ARM_TBFLAG_CONDEXEC_SHIFT)
#define ARM_TBFLAG_BSWAP_CODE_SHIFT 16
#define ARM_TBFLAG_BSWAP_CODE_MASK  (1 << ARM_TBFLAG_BSWAP_CODE_SHIFT)

/* Bit usage when in AArch64 state: currently no bits defined */

/* some convenience accessor macros */
#define ARM_TBFLAG_AARCH64_STATE(F) \
    (((F) & ARM_TBFLAG_AARCH64_STATE_MASK) >> ARM_TBFLAG_AARCH64_STATE_SHIFT)
#define ARM_TBFLAG_THUMB(F) \
    (((F) & ARM_TBFLAG_THUMB_MASK) >> ARM_TBFLAG_THUMB_SHIFT)
#define ARM_TBFLAG_VECLEN(F) \
    (((F) & ARM_TBFLAG_VECLEN_MASK) >> ARM_TBFLAG_VECLEN_SHIFT)
#define ARM_TBFLAG_VECSTRIDE(F) \
    (((F) & ARM_TBFLAG_VECSTRIDE_MASK) >> ARM_TBFLAG_VECSTRIDE_SHIFT)
#define ARM_TBFLAG_PRIV(F) \
    (((F) & ARM_TBFLAG_PRIV_MASK) >> ARM_TBFLAG_PRIV_SHIFT)
#define ARM_TBFLAG_VFPEN(F) \
    (((F) & ARM_TBFLAG_VFPEN_MASK) >> ARM_TBFLAG_VFPEN_SHIFT)
#define ARM_TBFLAG_CONDEXEC(F) \
    (((F) & ARM_TBFLAG_CONDEXEC_MASK) >> ARM_TBFLAG_CONDEXEC_SHIFT)
#define ARM_TBFLAG_BSWAP_CODE(F) \
    (((F) & ARM_TBFLAG_BSWAP_CODE_MASK) >> ARM_TBFLAG_BSWAP_CODE_SHIFT)

static inline void cpu_get_tb_cpu_state(CPUARMState *env, target_ulong *pc,
                                        target_ulong *cs_base, int *flags)
{
    if (is_a64(env)) {
        *pc = env->regs[15];
        *flags = ARM_TBFLAG_AARCH64_STATE_MASK;
    } else {
        int privmode;
        *pc = env->regs[15];
        *flags = (env->thumb << ARM_TBFLAG_THUMB_SHIFT)
            | (env->vfp.vec_len << ARM_TBFLAG_VECLEN_SHIFT)
            | (env->vfp.vec_stride << ARM_TBFLAG_VECSTRIDE_SHIFT)
            | (env->condexec_bits << ARM_TBFLAG_CONDEXEC_SHIFT);
            // | (env->bswap_code << ARM_TBFLAG_BSWAP_CODE_SHIFT);
        if (arm_feature(env, ARM_FEATURE_M)) {
            privmode = !((env->v7m.exception == 0) && (env->v7m.control & 1));
        } else {
            privmode = (env->uncached_cpsr & CPSR_M) != ARM_CPU_MODE_USR;
        }
        if (privmode) {
            *flags |= ARM_TBFLAG_PRIV_MASK;
        }
        if (env->vfp.xregs[ARM_VFP_FPEXC] & (1 << 30)) {
            *flags |= ARM_TBFLAG_VFPEN_MASK;
        }
    }

    *cs_base = 0;
}

static inline bool cpu_has_work(CPUState *cpu)
{
    return (cpu->interrupt_request &
            (CPU_INTERRUPT_FIQ | CPU_INTERRUPT_HARD | CPU_INTERRUPT_EXITTB));
}

#include "exec/exec-all.h"

static inline void cpu_pc_from_tb(CPUARMState *env, TranslationBlock *tb)
{
    env->regs[15] = tb->pc;
}

#endif
