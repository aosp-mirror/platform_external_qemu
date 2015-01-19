/*
 *  MIPS emulation helpers for qemu.
 *
 *  Copyright (c) 2004-2005 Jocelyn Mayer
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
#include "qemu/osdep.h"

#include "cpu.h"
#include "sysemu/kvm.h"
#include "exec/exec-all.h"
#include "exec/cpu_ldst.h"
#include "exec/log.h"

enum {
    TLBRET_XI = -6,
    TLBRET_RI = -5,
    TLBRET_DIRTY = -4,
    TLBRET_INVALID = -3,
    TLBRET_NOMATCH = -2,
    TLBRET_BADADDR = -1,
    TLBRET_MATCH = 0
};

#if !defined(CONFIG_USER_ONLY)

/* no MMU emulation */
int no_mmu_map_address (CPUMIPSState *env, hwaddr *physical, int *prot,
                        target_ulong address, int rw, int access_type)
{
    *physical = address;
    *prot = PAGE_READ | PAGE_WRITE;
    return TLBRET_MATCH;
}

/* fixed mapping MMU emulation */
int fixed_mmu_map_address (CPUMIPSState *env, hwaddr *physical, int *prot,
                           target_ulong address, int rw, int access_type)
{
    if (address <= (int32_t)0x7FFFFFFFUL) {
        if (!(env->CP0_Status & (1 << CP0St_ERL)))
            *physical = address + 0x40000000UL;
        else
            *physical = address;
    } else if (address <= (int32_t)0xBFFFFFFFUL)
        *physical = address & 0x1FFFFFFF;
    else
        *physical = address;

    *prot = PAGE_READ | PAGE_WRITE;
    return TLBRET_MATCH;
}

/* MIPS32/MIPS64 R4000-style MMU emulation */
int r4k_map_address (CPUMIPSState *env, hwaddr *physical, int *prot,
                     target_ulong address, int rw, int access_type)
{
    uint16_t ASID = env->CP0_EntryHi & env->CP0_EntryHi_ASID_mask;
    int i;

    for (i = 0; i < env->tlb->tlb_in_use; i++) {
        r4k_tlb_t *tlb = &env->tlb->mmu.r4k.tlb[i];
        /* 1k pages are not supported. */
        target_ulong mask = tlb->PageMask | ~(TARGET_PAGE_MASK << 1);
        target_ulong tag = address & ~mask;
        target_ulong VPN = tlb->VPN & ~mask;
#if defined(TARGET_MIPS64)
        tag &= env->SEGMask;
#endif

        /* Check ASID, virtual page number & size */
        if ((tlb->G == 1 || tlb->ASID == ASID) && VPN == tag && !tlb->EHINV) {
            /* TLB match */
            int n = !!(address & mask & ~(mask >> 1));
            /* Check access rights */
            if (!(n ? tlb->V1 : tlb->V0)) {
                return TLBRET_INVALID;
            }
            if (rw == MMU_INST_FETCH && (n ? tlb->XI1 : tlb->XI0)) {
                return TLBRET_XI;
            }
            if (rw == MMU_DATA_LOAD && (n ? tlb->RI1 : tlb->RI0)) {
                return TLBRET_RI;
            }
            if (rw != MMU_DATA_STORE || (n ? tlb->D1 : tlb->D0)) {
                *physical = tlb->PFN[n] | (address & (mask >> 1));
                *prot = PAGE_READ;
                if (n ? tlb->D1 : tlb->D0)
                    *prot |= PAGE_WRITE;
                return TLBRET_MATCH;
            }
            return TLBRET_DIRTY;
        }
    }
    return TLBRET_NOMATCH;
}

static int get_physical_address (CPUMIPSState *env, hwaddr *physical,
                                int *prot, target_ulong real_address,
                                int rw, int access_type)
{
    /* User mode can only access useg/xuseg */
    int user_mode = (env->hflags & MIPS_HFLAG_MODE) == MIPS_HFLAG_UM;
    int supervisor_mode = (env->hflags & MIPS_HFLAG_MODE) == MIPS_HFLAG_SM;
    int kernel_mode = !user_mode && !supervisor_mode;
#if defined(TARGET_MIPS64)
    int UX = (env->CP0_Status & (1 << CP0St_UX)) != 0;
    int SX = (env->CP0_Status & (1 << CP0St_SX)) != 0;
    int KX = (env->CP0_Status & (1 << CP0St_KX)) != 0;
#endif
    int ret = TLBRET_MATCH;
    /* effective address (modified for KVM T&E kernel segments) */
    target_ulong address = real_address;

#define USEG_LIMIT      0x7FFFFFFFUL
#define KSEG0_BASE      0x80000000UL
#define KSEG1_BASE      0xA0000000UL
#define KSEG2_BASE      0xC0000000UL
#define KSEG3_BASE      0xE0000000UL

#define KVM_KSEG0_BASE  0x40000000UL
#define KVM_KSEG2_BASE  0x60000000UL

    if (kvm_enabled()) {
        /* KVM T&E adds guest kernel segments in useg */
        if (real_address >= KVM_KSEG0_BASE) {
            if (real_address < KVM_KSEG2_BASE) {
                /* kseg0 */
                address += KSEG0_BASE - KVM_KSEG0_BASE;
            } else if (real_address <= USEG_LIMIT) {
                /* kseg2/3 */
                address += KSEG2_BASE - KVM_KSEG2_BASE;
            }
        }
    }

    if (address <= USEG_LIMIT) {
        /* useg */
        if (env->CP0_Status & (1 << CP0St_ERL)) {
            *physical = address & 0xFFFFFFFF;
            *prot = PAGE_READ | PAGE_WRITE;
        } else {
            ret = env->tlb->map_address(env, physical, prot, real_address, rw, access_type);
        }
#if defined(TARGET_MIPS64)
    } else if (address < 0x4000000000000000ULL) {
        /* xuseg */
        if (UX && address <= (0x3FFFFFFFFFFFFFFFULL & env->SEGMask)) {
            ret = env->tlb->map_address(env, physical, prot, real_address, rw, access_type);
        } else {
            ret = TLBRET_BADADDR;
        }
    } else if (address < 0x8000000000000000ULL) {
        /* xsseg */
        if ((supervisor_mode || kernel_mode) &&
            SX && address <= (0x7FFFFFFFFFFFFFFFULL & env->SEGMask)) {
            ret = env->tlb->map_address(env, physical, prot, real_address, rw, access_type);
        } else {
            ret = TLBRET_BADADDR;
        }
    } else if (address < 0xC000000000000000ULL) {
        /* xkphys */
        if (kernel_mode && KX &&
            (address & 0x07FFFFFFFFFFFFFFULL) <= env->PAMask) {
            *physical = address & env->PAMask;
            *prot = PAGE_READ | PAGE_WRITE;
        } else {
            ret = TLBRET_BADADDR;
        }
    } else if (address < 0xFFFFFFFF80000000ULL) {
        /* xkseg */
        if (kernel_mode && KX &&
            address <= (0xFFFFFFFF7FFFFFFFULL & env->SEGMask)) {
            ret = env->tlb->map_address(env, physical, prot, real_address, rw, access_type);
        } else {
            ret = TLBRET_BADADDR;
        }
#endif
    } else if (address < (int32_t)KSEG1_BASE) {
        /* kseg0 */
        if (kernel_mode) {
            *physical = address - (int32_t)KSEG0_BASE;
            *prot = PAGE_READ | PAGE_WRITE;
        } else {
            ret = TLBRET_BADADDR;
        }
    } else if (address < (int32_t)KSEG2_BASE) {
        /* kseg1 */
        if (kernel_mode) {
            *physical = address - (int32_t)KSEG1_BASE;
            *prot = PAGE_READ | PAGE_WRITE;
        } else {
            ret = TLBRET_BADADDR;
        }
    } else if (address < (int32_t)KSEG3_BASE) {
        /* sseg (kseg2) */
        if (supervisor_mode || kernel_mode) {
            ret = env->tlb->map_address(env, physical, prot, real_address, rw, access_type);
        } else {
            ret = TLBRET_BADADDR;
        }
    } else {
        /* kseg3 */
        /* XXX: debug segment is not emulated */
        if (kernel_mode) {
            ret = env->tlb->map_address(env, physical, prot, real_address, rw, access_type);
        } else {
            ret = TLBRET_BADADDR;
        }
    }
    return ret;
}

void cpu_mips_tlb_flush(CPUMIPSState *env, int flush_global)
{
    MIPSCPU *cpu = mips_env_get_cpu(env);

    /* Flush qemu's TLB and discard all shadowed entries.  */
    tlb_flush(CPU(cpu), flush_global);
    env->tlb->tlb_in_use = env->tlb->nb_tlb;
}

/* Called for updates to CP0_Status.  */
void sync_c0_status(CPUMIPSState *env, CPUMIPSState *cpu, int tc)
{
    int32_t tcstatus, *tcst;
    uint32_t v = cpu->CP0_Status;
    uint32_t cu, mx, asid, ksu;
    uint32_t mask = ((1 << CP0TCSt_TCU3)
                       | (1 << CP0TCSt_TCU2)
                       | (1 << CP0TCSt_TCU1)
                       | (1 << CP0TCSt_TCU0)
                       | (1 << CP0TCSt_TMX)
                       | (3 << CP0TCSt_TKSU)
                       | (0xff << CP0TCSt_TASID));

    cu = (v >> CP0St_CU0) & 0xf;
    mx = (v >> CP0St_MX) & 0x1;
    ksu = (v >> CP0St_KSU) & 0x3;
    asid = env->CP0_EntryHi & env->CP0_EntryHi_ASID_mask;

    tcstatus = cu << CP0TCSt_TCU0;
    tcstatus |= mx << CP0TCSt_TMX;
    tcstatus |= ksu << CP0TCSt_TKSU;
    tcstatus |= asid;

    if (tc == cpu->current_tc) {
        tcst = &cpu->active_tc.CP0_TCStatus;
    } else {
        tcst = &cpu->tcs[tc].CP0_TCStatus;
    }

    *tcst &= ~mask;
    *tcst |= tcstatus;
    compute_hflags(cpu);
}

void cpu_mips_store_status(CPUMIPSState *env, target_ulong val)
{
    uint32_t mask = env->CP0_Status_rw_bitmask;
    target_ulong old = env->CP0_Status;

    if (env->insn_flags & ISA_MIPS32R6) {
        bool has_supervisor = extract32(mask, CP0St_KSU, 2) == 0x3;
#if defined(TARGET_MIPS64)
        uint32_t ksux = (1 << CP0St_KX) & val;
        ksux |= (ksux >> 1) & val; /* KX = 0 forces SX to be 0 */
        ksux |= (ksux >> 1) & val; /* SX = 0 forces UX to be 0 */
        val = (val & ~(7 << CP0St_UX)) | ksux;
#endif
        if (has_supervisor && extract32(val, CP0St_KSU, 2) == 0x3) {
            mask &= ~(3 << CP0St_KSU);
        }
        mask &= ~(((1 << CP0St_SR) | (1 << CP0St_NMI)) & val);
    }

    env->CP0_Status = (old & ~mask) | (val & mask);
#if defined(TARGET_MIPS64)
    if ((env->CP0_Status ^ old) & (old & (7 << CP0St_UX))) {
        /* Access to at least one of the 64-bit segments has been disabled */
        cpu_mips_tlb_flush(env, 1);
    }
#endif
    if (env->CP0_Config3 & (1 << CP0C3_MT)) {
        sync_c0_status(env, env, env->current_tc);
    } else {
        compute_hflags(env);
    }
}

void cpu_mips_store_cause(CPUMIPSState *env, target_ulong val)
{
    uint32_t mask = 0x00C00300;
    uint32_t old = env->CP0_Cause;
    int i;

    if (env->insn_flags & ISA_MIPS32R2) {
        mask |= 1 << CP0Ca_DC;
    }
    if (env->insn_flags & ISA_MIPS32R6) {
        mask &= ~((1 << CP0Ca_WP) & val);
    }

    env->CP0_Cause = (env->CP0_Cause & ~mask) | (val & mask);

    if ((old ^ env->CP0_Cause) & (1 << CP0Ca_DC)) {
        if (env->CP0_Cause & (1 << CP0Ca_DC)) {
            cpu_mips_stop_count(env);
        } else {
            cpu_mips_start_count(env);
        }
    }

    /* Set/reset software interrupts */
    for (i = 0 ; i < 2 ; i++) {
        if ((old ^ env->CP0_Cause) & (1 << (CP0Ca_IP + i))) {
            cpu_mips_soft_irq(env, i, env->CP0_Cause & (1 << (CP0Ca_IP + i)));
        }
    }
}
#endif

static void raise_mmu_exception(CPUMIPSState *env, target_ulong address,
                                int rw, int tlb_error)
{
    CPUState *cs = CPU(mips_env_get_cpu(env));
    int exception = 0, error_code = 0;

    if (rw == MMU_INST_FETCH) {
        error_code |= EXCP_INST_NOTAVAIL;
    }

    switch (tlb_error) {
    default:
    case TLBRET_BADADDR:
        /* Reference to kernel address from user mode or supervisor mode */
        /* Reference to supervisor address from user mode */
        if (rw == MMU_DATA_STORE) {
            exception = EXCP_AdES;
        } else {
            exception = EXCP_AdEL;
        }
        break;
    case TLBRET_NOMATCH:
        /* No TLB match for a mapped address */
        if (rw == MMU_DATA_STORE) {
            exception = EXCP_TLBS;
        } else {
            exception = EXCP_TLBL;
        }
        error_code |= EXCP_TLB_NOMATCH;
        break;
    case TLBRET_INVALID:
        /* TLB match with no valid bit */
        if (rw == MMU_DATA_STORE) {
            exception = EXCP_TLBS;
        } else {
            exception = EXCP_TLBL;
        }
        break;
    case TLBRET_DIRTY:
        /* TLB match but 'D' bit is cleared */
        exception = EXCP_LTLBL;
        break;
    case TLBRET_XI:
        /* Execute-Inhibit Exception */
        if (env->CP0_PageGrain & (1 << CP0PG_IEC)) {
            exception = EXCP_TLBXI;
        } else {
            exception = EXCP_TLBL;
        }
        break;
    case TLBRET_RI:
        /* Read-Inhibit Exception */
        if (env->CP0_PageGrain & (1 << CP0PG_IEC)) {
            exception = EXCP_TLBRI;
        } else {
            exception = EXCP_TLBL;
        }
        break;
    }
    /* Raise exception */
    env->CP0_BadVAddr = address;
    env->CP0_Context = (env->CP0_Context & ~0x007fffff) |
                       ((address >> 9) & 0x007ffff0);
    env->CP0_EntryHi = (env->CP0_EntryHi & env->CP0_EntryHi_ASID_mask) |
                       (env->CP0_EntryHi & (1 << CP0EnHi_EHINV)) |
                       (address & (TARGET_PAGE_MASK << 1));
#if defined(TARGET_MIPS64)
    env->CP0_EntryHi &= env->SEGMask;
    env->CP0_XContext =
        /* PTEBase */   (env->CP0_XContext & ((~0ULL) << (env->SEGBITS - 7))) |
        /* R */         (extract64(address, 62, 2) << (env->SEGBITS - 9)) |
        /* BadVPN2 */   (extract64(address, 13, env->SEGBITS - 13) << 4);
#endif
    cs->exception_index = exception;
    env->error_code = error_code;
}

#if !defined(CONFIG_USER_ONLY)

#define MUST_HAVE_FASTTLB 1

#define KERNEL_64_BIT           (1 << 0)
#define KERNEL_PGD_C0_CONTEXT   (1 << 1)
#define KERNEL_HUGE_TLB         (1 << 2)
#define KERNEL_RIXI             (1 << 3)

/* TLB maintenance PTE software flags.
 *
 * Low bits are: CCC D V G RI XI [S H] A W R M(=F) P
 * TLB refill will do a ROTR 7/9 (in case of cpu_has_rixi),
 * or SRL/DSRL 7/9 to strip low bits.
 * PFN size in high bits is 49 or 51 bit --> 512TB or 4*512TB for 4KB pages
 *
 * take a look at <KERNEL>/arch/mips/include/asm/pgtable-bits.h
 */
#define PTE_PAGE_PRESENT_SHIFT      (0)
#define PTE_PAGE_PRESENT            (1 << PTE_PAGE_PRESENT_SHIFT)

#define PTE_PAGE_MODIFIED_SHIFT     (PTE_PAGE_PRESENT_SHIFT + 1)
#define PTE_PAGE_MODIFIED           (1 << PTE_PAGE_MODIFIED_SHIFT)

#define PTE_PAGE_FILE               (PTE_PAGE_MODIFIED)

#define PTE_PAGE_READ_SHIFT         (PTE_PAGE_MODIFIED_SHIFT + 1)
#define PTE_PAGE_READ               (1 << PTE_PAGE_READ_SHIFT)

#define PTE_PAGE_WRITE_SHIFT        (PTE_PAGE_READ_SHIFT + 1)
#define PTE_PAGE_WRITE              (1 << PTE_PAGE_WRITE_SHIFT)

#define PTE_PAGE_ACCESSED_SHIFT     (PTE_PAGE_WRITE_SHIFT + 1)
#define PTE_PAGE_ACCESSED           (1 << PTE_PAGE_ACCESSED_SHIFT)

/* Huge TLB support maintenance bits */
#define PTE_PAGE_HUGE_SHIFT         (PTE_PAGE_ACCESSED_SHIFT + 1)
#define PTE_PAGE_HUGE               (1 << PTE_PAGE_HUGE_SHIFT)

#define PTE_PAGE_SPLITTING_SHIFT    (PTE_PAGE_HUGE_SHIFT + 1)
#define PTE_PAGE_SPLITTING          (1 << PTE_PAGE_SPLITTING_SHIFT)

/*
 * Get the pgd_current from TLB refill handler
 * The kernel refill handler is generated by
 * function build_r4000_tlb_refill_handler.
 */
typedef void (*pagetable_walk_t)(CPUState *cs,
                                  target_ulong pgd_addr, target_ulong vaddr,
                                  target_ulong *entrylo0, target_ulong *entrylo1,
                                  target_ulong *sw_pte_lo0, target_ulong *sw_pte_lo1);
static struct {
    enum {PROBE, USEFASTTLB, USESLOWTLB} state;
    uint32_t config;
    pagetable_walk_t pagetable_walk;
    target_ulong pgd_current_p;
    target_ulong swapper_pg_dir;
    int softshift;
} linux_pte_info = {0};

static inline void pagetable_walk32(CPUState *cs,
                                  target_ulong pgd_addr, target_ulong vaddr,
                                  target_ulong *entrylo0, target_ulong *entrylo1,
                                  target_ulong *sw_pte_lo0, target_ulong *sw_pte_lo1)
{
    target_ulong ptw_phys, pt_addr, index;

#if defined(TARGET_MIPS64)
    /* workaround when running a 32bit
     * emulation with the 64bit target emulator
     */
    vaddr = (uint32_t)vaddr;
#endif

    ptw_phys = pgd_addr & 0x1fffffffUL; /* Assume pgd is in KSEG0/KSEG1 */
    /* 32bit PTE lookup */
    index = (vaddr >> 22) << 2; /* Use bits 31..22 to index pgd */
    ptw_phys += index;

    pt_addr = ldl_phys(cs->as, ptw_phys);

    ptw_phys = pt_addr & 0x1fffffffUL; /* Assume pgt is in KSEG0/KSEG1 */
    index = ((vaddr >> 13) & 0x1ff) << 3; /* Use bits 21..13 to index pgt */
    ptw_phys += index;

    /* Get the entrylo values from pgt */
    *entrylo0 = ldl_phys(cs->as, ptw_phys);
    if (sw_pte_lo0) {
        *sw_pte_lo0 = *entrylo0 & ((target_ulong)(1 << linux_pte_info.softshift) - 1);
    }
    *entrylo0 >>= linux_pte_info.softshift;

    *entrylo1 = ldl_phys(cs->as, ptw_phys + 4);
    if (sw_pte_lo1) {
        *sw_pte_lo1 = *entrylo1 & ((target_ulong)(1 << linux_pte_info.softshift) - 1);
    }
    *entrylo1 >>= linux_pte_info.softshift;
}

static inline void pagetable_walk64(CPUState *cs,
                                  target_ulong pgd_addr, target_ulong vaddr,
                                  target_ulong *entrylo0, target_ulong *entrylo1,
                                  target_ulong *sw_pte_lo0, target_ulong *sw_pte_lo1)
{
    MIPSCPU *cpu = MIPS_CPU(cs);
    CPUMIPSState *env = &cpu->env;

    target_ulong ptw_phys, pt_addr, index;

    pgd_addr = pgd_addr & 0x1fffffffUL;
    index = ((uint64_t)vaddr >> 0x1b) & 0x1ff8;
    pgd_addr += index;

    pgd_addr = ldl_phys(cs->as, pgd_addr);

    ptw_phys = pgd_addr & 0x1fffffffUL;
    index = ((uint64_t)vaddr >> 0x12) & 0xff8;
    ptw_phys += index;

    pt_addr = ldl_phys(cs->as, ptw_phys);

    ptw_phys = pt_addr & 0x1fffffffUL;
    index = (((vaddr & 0xC00000000000ULL) >> (55 - env->SEGBITS)) |
             ((vaddr & ((1ULL << env->SEGBITS) - 1) & 0xFFFFFFFFFFFFE000ULL) >> 9)) & 0xff0;
    ptw_phys += index;

    if (linux_pte_info.config & KERNEL_RIXI) {
        target_ulong mask = ~(-1 << linux_pte_info.softshift);

        *entrylo0 = ldl_phys(cs->as, ptw_phys);
        if (sw_pte_lo0) {
            *sw_pte_lo0 = *entrylo0 & ((target_ulong)(1 << linux_pte_info.softshift) - 1);
        }
        *entrylo0 = ((*entrylo0 & mask) << (64 - linux_pte_info.softshift)) |
                     ((uint64_t)*entrylo0 >> linux_pte_info.softshift);
        *entrylo0 = (*entrylo0 & 0x3FFFFFFF) |
                    (*entrylo0 & ((env->CP0_PageGrain & (3ull << CP0PG_XIE)) << 32));

        *entrylo1 = ldl_phys(cs->as, ptw_phys + 8);
        if (sw_pte_lo1) {
            *sw_pte_lo1 = *entrylo1 & ((target_ulong)(1 << linux_pte_info.softshift) - 1);
        }
        *entrylo1 = ((*entrylo1 & mask) << (64 - linux_pte_info.softshift)) |
                     ((uint64_t)*entrylo1 >> linux_pte_info.softshift);
        *entrylo1 = (*entrylo1 & 0x3FFFFFFF) |
                    (*entrylo1 & ((env->CP0_PageGrain & (3ull << CP0PG_XIE)) << 32));
    } else {
        /* Get the entrylo values from pgt */
        *entrylo0 = ldl_phys(cs->as, ptw_phys);
        if (sw_pte_lo0) {
            *sw_pte_lo0 = *entrylo0 & ((target_ulong)(1 << linux_pte_info.softshift) - 1);
        }
        *entrylo0 >>= linux_pte_info.softshift;

        *entrylo1 = ldl_phys(cs->as, ptw_phys + 8);
        if (sw_pte_lo1) {
            *sw_pte_lo1 = *entrylo1 & ((target_ulong)(1 << linux_pte_info.softshift) - 1);
        }
        *entrylo1 >>= linux_pte_info.softshift;
    }
}

static inline target_ulong cpu_mips_get_pgd(CPUState *cs, target_long bad_vaddr)
{
    MIPSCPU *cpu = MIPS_CPU(cs);
    CPUMIPSState *env = &cpu->env;

    if (unlikely(linux_pte_info.state == PROBE)) {
        int i;
        uint32_t lui_ins, lw_ins, srl_ins, config;
        target_ulong address;
        uint32_t ebase;

        /*
         * The exact TLB refill code varies depeing on the kernel version
         * and configuration. Examins the TLB handler to extract
         * pgd_current_p and the shift required to convert in memory PTE
         * to TLB format
         */
        static struct {
            uint32_t config;
            struct {
                uint32_t off;
                uint32_t op;
                uint32_t mask;
            } lui, lw, srl;
        } handlers[] = {
            /* 2.6.29+ 32-bit Kernel */
            {
                0,
                {0x00, 0x3c1b0000, 0xffff0000}, /* 0x3c1b803f : lui k1,%hi(pgd_current_p) */
                {0x08, 0x8f7b0000, 0xffff0000}, /* 0x8f7b3000 : lw  k1,%lo(k1) */
                {0x34, 0x001ad002, 0xfffff83f}  /* 0x001ad002 : srl k0,k0,#shift */
            },
            /* 3.10+ 64-bit R2 Kernel */
            {
                KERNEL_64_BIT | KERNEL_PGD_C0_CONTEXT,
                {0x04, 0x3c1b0000, 0xffff0000}, /* 0x3c1b0000 : lui  k1,%hi(swapper_pg_dir) */
                {0xac, 0xdf7b0000, 0xffff0000}, /* 0xdf7b0000 : ld   k1,0(k1) */
                {0xd4, 0x001ad03a, 0xfffff83f}  /* 0x001ad03a : dsrl k0,k0,#shift */
            },
            /* 3.10+ 64-bit R6 Kernel */
            {
                KERNEL_64_BIT | KERNEL_RIXI,
                {0x8c, 0x3c1b0000, 0xffff0000}, /* 0x3c1b0000 : lui  k1,%hi(pgd_current_p) */
                {0x90, 0xdf7b0000, 0xffff0000}, /* 0xdf7b0000 : ld   k1,%lo(k1) */
                {0xcc, 0x003ad03a, 0xfffff83f}  /* 0x003ad03a : dror k0,k0,#shift */
            }
        };

        ebase = env->CP0_EBase - 0x80000000;
        linux_pte_info.config = 0;

        /* Match the kernel TLB refill exception handler against known code */
        for (i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++) {
            config = handlers[i].config;
            lui_ins = ldl_phys(cs->as, ebase + handlers[i].lui.off);
            lw_ins = ldl_phys(cs->as, ebase + handlers[i].lw.off);
            srl_ins = ldl_phys(cs->as, ebase + handlers[i].srl.off);
            if (((lui_ins & handlers[i].lui.mask) == handlers[i].lui.op) &&
                ((lw_ins & handlers[i].lw.mask) == handlers[i].lw.op) &&
                ((srl_ins & handlers[i].srl.mask) == handlers[i].srl.op))
                break;
        }
        if (i >= sizeof(handlers)/sizeof(handlers[0])) {
#if defined(MUST_HAVE_FASTTLB)
                printf("TLBMiss handler dump:\n");
            for (i = 0; i < 0x100; i+= 4)
                printf("0x%08x: 0x%08x\n", ebase + i, ldl_phys(cs->as, ebase + i));
            fprintf(stderr, "TLBMiss handler signature not recognised\n");
            exit(1);
#endif
            fprintf(stderr, "TLBMiss handler signature not recognised, using slowpath\n");
            linux_pte_info.state = USESLOWTLB;
            linux_pte_info.pagetable_walk = NULL;
            goto done;
        }

        linux_pte_info.config = config;

        if (config & KERNEL_64_BIT) {
            linux_pte_info.pagetable_walk = &pagetable_walk64;
        } else {
            linux_pte_info.pagetable_walk = &pagetable_walk32;
        }

        if (config & KERNEL_PGD_C0_CONTEXT) {
            /* swapper_pg_dir address */
            address = (lui_ins & 0xffff) << 16;
            linux_pte_info.swapper_pg_dir = address;
        } else {
            address = (lui_ins & 0xffff) << 16;
            linux_pte_info.swapper_pg_dir = address;
            address += (((int32_t)(lw_ins & 0xffff)) << 16) >> 16;
            if (address >= 0x80000000 && address < 0xa0000000)
                address -= 0x80000000;
            else if (address >= 0xa0000000 && address <= 0xc0000000)
                address -= 0xa0000000;
            else
                cpu_abort(cs, "pgd_current_p not in KSEG0/KSEG1\n");
        }

        linux_pte_info.state = USEFASTTLB;
        linux_pte_info.pgd_current_p = address;
        linux_pte_info.softshift = (srl_ins >> 6) & 0x1f;
    }

done:
    /* Get pgd_current */
    if (linux_pte_info.state == USEFASTTLB) {
        if (linux_pte_info.config & KERNEL_64_BIT) {
            target_ulong address = 0;
            /*
             * The kernel currently implicitely assumes that the
             * MIPS SEGBITS parameter for the processor is
             * (PGDIR_SHIFT+PGDIR_BITS) or less, and will never
             * allocate virtual addresses outside the maximum
             * range for SEGBITS = (PGDIR_SHIFT+PGDIR_BITS). But
             * that doesn't prevent user code from accessing the
             * higher xuseg addresses.  Here, we make sure that
             * everything but the lower xuseg addresses goes down
             * the module_alloc/vmalloc path.
             */
            address = ((uint64_t)bad_vaddr) >> 40;
            if (likely(!address)) {
                if (linux_pte_info.config & KERNEL_PGD_C0_CONTEXT) {
                    /*
                     * &pgd << 11 stored in CONTEXT [23..63].
                     */
                    address = env->CP0_Context;
                    address = ((uint64_t)address >> 23) << 23;
                    /* 1 0  1 0 1  << 6  xkphys cached */
                    address |= 0x540;
                    /* dror k1,k1,0xb */
                    address = ((uint64_t)address >> 11) |
                              (((uint64_t)address & 0x7ff) << 53);
                    return address;
                } else {
                    return ldl_phys(cs->as, linux_pte_info.pgd_current_p);
                }
            } else if (bad_vaddr < 0) {
                /* swapper_pg_dir address */
                return linux_pte_info.swapper_pg_dir;
            } else {
                /*
                 * We get here if we are an xsseg address, or if we are
                 * an xuseg address above (PGDIR_SHIFT+PGDIR_BITS) boundary.
                 *
                 * Ignoring xsseg (assume disabled so would generate
                 * (address errors?), the only remaining possibility
                 * is the upper xuseg addresses.  On processors with
                 * TLB_SEGBITS <= PGDIR_SHIFT+PGDIR_BITS, these
                 * addresses would have taken an address error. We try
                 * to mimic that here by taking a load/istream page
                 * fault.
                 */
                return 0; /* fallback to software handler and do page fault */
            }
        } else {
            return ldl_phys(cs->as, linux_pte_info.pgd_current_p);
        }
    }
    return 0;
}

static inline int cpu_mips_tlb_refill(CPUState *cs, target_ulong address, int rw,
                                      int mmu_idx, int is_softmmu)
{
    MIPSCPU *cpu = MIPS_CPU(cs);
    CPUMIPSState *env = &cpu->env;

    int32_t saved_hflags;
    target_ulong saved_badvaddr,saved_entryhi,saved_context,saved_xcontext;
    target_ulong pgd_addr;
    target_ulong fault_addr;
    target_ulong entrylo0, entrylo1;
    int ret;

    saved_badvaddr = env->CP0_BadVAddr;
    saved_context = env->CP0_Context;
    saved_xcontext = env->CP0_XContext;
    saved_entryhi = env->CP0_EntryHi;
    saved_hflags = env->hflags;

    env->CP0_BadVAddr = address;
    env->CP0_Context = (env->CP0_Context & ~0x007fffff) |
                    ((address >> 9) &   0x007ffff0);
    env->CP0_EntryHi =
        (env->CP0_EntryHi & 0xFF) | (address & (TARGET_PAGE_MASK << 1));
#if defined(TARGET_MIPS64)
    env->CP0_EntryHi &= env->SEGMask;
    env->CP0_XContext = (env->CP0_XContext & ((~0ULL) << (env->SEGBITS - 7))) |
                        ((address & 0xC00000000000ULL) >> (55 - env->SEGBITS)) |
                        ((address & ((1ULL << env->SEGBITS) - 1) & 0xFFFFFFFFFFFFE000ULL) >> 9);
#endif

    pgd_addr = 0;
    pgd_addr = cpu_mips_get_pgd(cs, address);

    /* if pgd_addr is unknown return TLBRET_NOMATCH
     * to allow software handler to run
     */
    if (unlikely(pgd_addr == 0)) {
        ret = TLBRET_NOMATCH;
        goto out;
    }

    env->hflags = MIPS_HFLAG_KM;
    fault_addr = env->CP0_BadVAddr;

    linux_pte_info.pagetable_walk(cs, pgd_addr, fault_addr, &entrylo0, &entrylo1, NULL, NULL);

    /* Refill the TLB */
    env->CP0_EntryLo0 = entrylo0;
    env->CP0_EntryLo1 = entrylo1;
    r4k_helper_tlbwr(env);

    /* Since we know the TLB contents, we can
     * return the TLB lookup value here.
     */

    target_ulong mask = env->CP0_PageMask | ~(TARGET_PAGE_MASK << 1);
    target_ulong lo = (address & mask & ~(mask >> 1)) ? entrylo1 : entrylo0;
    uint16_t RI = (lo >> CP0EnLo_RI) & 1;
    uint16_t XI = (lo >> CP0EnLo_XI) & 1;

    if (rw == MMU_INST_FETCH && (XI)) {
        ret = TLBRET_XI;
        goto out;
    }
    if (rw == MMU_DATA_LOAD && (RI)) {
        ret = TLBRET_RI;
        goto out;
    }

    /* Is the TLB entry valid? */
    if ((lo & (1 << CP0EnLo_V)) == 0) {
        ret = TLBRET_INVALID;
        goto out;
    }

    /* Is this a read access or a write to a modifiable page? */
    if (rw != MMU_DATA_STORE || (lo & (1 << CP0EnLo_D))) {
        hwaddr physical = (lo >> CP0EnLo_PFN) << 12;
        physical |= address & (mask >> 1);
        int prot = PAGE_READ;
        if (lo & (1 << CP0EnLo_D))
            prot |= PAGE_WRITE;

        tlb_set_page(cs, address & TARGET_PAGE_MASK,
                        physical & TARGET_PAGE_MASK, prot | PAGE_EXEC,
                        mmu_idx, TARGET_PAGE_SIZE);
        ret = TLBRET_MATCH;
        goto out;
    }
    ret = TLBRET_DIRTY;

out:
    env->CP0_BadVAddr = saved_badvaddr;
    env->CP0_Context = saved_context;
    env->CP0_XContext = saved_xcontext;
    env->CP0_EntryHi = saved_entryhi;
    env->hflags = saved_hflags;
    return ret;
}

hwaddr mips_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
    MIPSCPU *cpu = MIPS_CPU(cs);
    CPUMIPSState *env = &cpu->env;
    hwaddr phys_addr;
    int prot, ret;

#if defined(TARGET_MIPS64)
    if (!(linux_pte_info.config & KERNEL_64_BIT) &&
         (linux_pte_info.state == USEFASTTLB))
        addr = ((int64_t)addr << 32) >> 32;
#endif
    ret = get_physical_address(env, &phys_addr, &prot, addr, 0, ACCESS_INT);
    if (ret != TLBRET_MATCH) {
        target_ulong pgd_addr = cpu_mips_get_pgd(cs, addr);

        phys_addr = -1;
        if (likely(pgd_addr)) {
            target_ulong entrylo0, entrylo1;
            target_ulong sw_pte_lo0, sw_pte_lo1;

            linux_pte_info.pagetable_walk(cs, pgd_addr, addr,
                                          &entrylo0, &entrylo1,
                                          &sw_pte_lo0, &sw_pte_lo1);

            target_ulong mask = env->CP0_PageMask | ~(TARGET_PAGE_MASK << 1);
            target_ulong lo = (addr & mask & ~(mask >> 1)) ? entrylo1 : entrylo0;
            target_ulong sw_pte = (addr & mask & ~(mask >> 1)) ? sw_pte_lo1 : sw_pte_lo0;

            if (sw_pte & PTE_PAGE_PRESENT) {
                phys_addr = ((lo >> CP0EnLo_PFN) << 12) | (addr & (mask >> 1));
            } else {
                qemu_log("cpu_get_phys_page_debug() invalid mapping for vaddr: 0x" TARGET_FMT_plx "\n", addr);
            }
        } else {
            qemu_log("cpu_get_phys_page_debug() fails for vaddr: 0x" TARGET_FMT_plx "\n", addr);
        }
    }
    return phys_addr;
}
#endif

int mips_cpu_handle_mmu_fault(CPUState *cs, vaddr address, int rw,
                              int mmu_idx)
{
    MIPSCPU *cpu = MIPS_CPU(cs);
    CPUMIPSState *env = &cpu->env;
#if !defined(CONFIG_USER_ONLY)
    hwaddr physical;
    int prot;
    int access_type;
#endif
    int ret = 0;

#if 0
    log_cpu_state(cs, 0);
#endif
    qemu_log_mask(CPU_LOG_MMU,
              "%s pc " TARGET_FMT_lx " ad %" VADDR_PRIx " rw %d mmu_idx %d\n",
              __func__, env->active_tc.PC, address, rw, mmu_idx);

    /* data access */
#if !defined(CONFIG_USER_ONLY)
    /* XXX: put correct access by using cpu_restore_state()
       correctly */
    access_type = ACCESS_INT;
    ret = get_physical_address(env, &physical, &prot,
                               address, rw, access_type);
    qemu_log_mask(CPU_LOG_MMU,
             "%s address=%" VADDR_PRIx " ret %d physical " TARGET_FMT_plx
             " prot %d\n",
             __func__, address, ret, physical, prot);
    if (ret == TLBRET_MATCH) {
        tlb_set_page(cs, address & TARGET_PAGE_MASK,
                     physical & TARGET_PAGE_MASK, prot | PAGE_EXEC,
                     mmu_idx, TARGET_PAGE_SIZE);
        ret = 0;
    } else if (ret == TLBRET_NOMATCH)
        ret = cpu_mips_tlb_refill(cs, address, rw, mmu_idx, 1);

    if (ret < 0)
#endif
    {
        raise_mmu_exception(env, address, rw, ret);
        ret = 1;
    }

    return ret;
}

#if !defined(CONFIG_USER_ONLY)
hwaddr cpu_mips_translate_address(CPUMIPSState *env, target_ulong address, int rw)
{
    hwaddr physical;
    int prot;
    int access_type;
    int ret = 0;

    /* data access */
    access_type = ACCESS_INT;
    ret = get_physical_address(env, &physical, &prot,
                               address, rw, access_type);
    if (ret != TLBRET_MATCH) {
        raise_mmu_exception(env, address, rw, ret);
        return -1LL;
    } else {
        return physical;
    }
}

static const char * const excp_names[EXCP_LAST + 1] = {
    [EXCP_RESET] = "reset",
    [EXCP_SRESET] = "soft reset",
    [EXCP_DSS] = "debug single step",
    [EXCP_DINT] = "debug interrupt",
    [EXCP_NMI] = "non-maskable interrupt",
    [EXCP_MCHECK] = "machine check",
    [EXCP_EXT_INTERRUPT] = "interrupt",
    [EXCP_DFWATCH] = "deferred watchpoint",
    [EXCP_DIB] = "debug instruction breakpoint",
    [EXCP_IWATCH] = "instruction fetch watchpoint",
    [EXCP_AdEL] = "address error load",
    [EXCP_AdES] = "address error store",
    [EXCP_TLBF] = "TLB refill",
    [EXCP_IBE] = "instruction bus error",
    [EXCP_DBp] = "debug breakpoint",
    [EXCP_SYSCALL] = "syscall",
    [EXCP_BREAK] = "break",
    [EXCP_CpU] = "coprocessor unusable",
    [EXCP_RI] = "reserved instruction",
    [EXCP_OVERFLOW] = "arithmetic overflow",
    [EXCP_TRAP] = "trap",
    [EXCP_FPE] = "floating point",
    [EXCP_DDBS] = "debug data break store",
    [EXCP_DWATCH] = "data watchpoint",
    [EXCP_LTLBL] = "TLB modify",
    [EXCP_TLBL] = "TLB load",
    [EXCP_TLBS] = "TLB store",
    [EXCP_DBE] = "data bus error",
    [EXCP_DDBL] = "debug data break load",
    [EXCP_THREAD] = "thread",
    [EXCP_MDMX] = "MDMX",
    [EXCP_C2E] = "precise coprocessor 2",
    [EXCP_CACHE] = "cache error",
    [EXCP_TLBXI] = "TLB execute-inhibit",
    [EXCP_TLBRI] = "TLB read-inhibit",
    [EXCP_MSADIS] = "MSA disabled",
    [EXCP_MSAFPE] = "MSA floating point",
};
#endif

target_ulong exception_resume_pc (CPUMIPSState *env)
{
    target_ulong bad_pc;
    target_ulong isa_mode;

    isa_mode = !!(env->hflags & MIPS_HFLAG_M16);
    bad_pc = env->active_tc.PC | isa_mode;
    if (env->hflags & MIPS_HFLAG_BMASK) {
        /* If the exception was raised from a delay slot, come back to
           the jump.  */
        bad_pc -= (env->hflags & MIPS_HFLAG_B16 ? 2 : 4);
    }

    return bad_pc;
}

#if !defined(CONFIG_USER_ONLY)
static void set_hflags_for_handler (CPUMIPSState *env)
{
    /* Exception handlers are entered in 32-bit mode.  */
    env->hflags &= ~(MIPS_HFLAG_M16);
    /* ...except that microMIPS lets you choose.  */
    if (env->insn_flags & ASE_MICROMIPS) {
        env->hflags |= (!!(env->CP0_Config3
                           & (1 << CP0C3_ISA_ON_EXC))
                        << MIPS_HFLAG_M16_SHIFT);
    }
}

static inline void set_badinstr_registers(CPUMIPSState *env)
{
    if (env->hflags & MIPS_HFLAG_M16) {
        /* TODO: add BadInstr support for microMIPS */
        return;
    }
    if (env->CP0_Config3 & (1 << CP0C3_BI)) {
        env->CP0_BadInstr = cpu_ldl_code(env, env->active_tc.PC);
    }
    if ((env->CP0_Config3 & (1 << CP0C3_BP)) &&
        (env->hflags & MIPS_HFLAG_BMASK)) {
        env->CP0_BadInstrP = cpu_ldl_code(env, env->active_tc.PC - 4);
    }
}
#endif

void mips_cpu_do_interrupt(CPUState *cs)
{
#if !defined(CONFIG_USER_ONLY)
    MIPSCPU *cpu = MIPS_CPU(cs);
    CPUMIPSState *env = &cpu->env;
    bool update_badinstr = 0;
    target_ulong offset;
    int cause = -1;
    const char *name;

    if (qemu_loglevel_mask(CPU_LOG_INT)
        && cs->exception_index != EXCP_EXT_INTERRUPT) {
        if (cs->exception_index < 0 || cs->exception_index > EXCP_LAST) {
            name = "unknown";
        } else {
            name = excp_names[cs->exception_index];
        }

        qemu_log("%s enter: PC " TARGET_FMT_lx " EPC " TARGET_FMT_lx
                 " %s exception\n",
                 __func__, env->active_tc.PC, env->CP0_EPC, name);
    }
    if (cs->exception_index == EXCP_EXT_INTERRUPT &&
        (env->hflags & MIPS_HFLAG_DM)) {
        cs->exception_index = EXCP_DINT;
    }
    offset = 0x180;
    switch (cs->exception_index) {
    case EXCP_DSS:
        env->CP0_Debug |= 1 << CP0DB_DSS;
        /* Debug single step cannot be raised inside a delay slot and
           resume will always occur on the next instruction
           (but we assume the pc has always been updated during
           code translation). */
        env->CP0_DEPC = env->active_tc.PC | !!(env->hflags & MIPS_HFLAG_M16);
        goto enter_debug_mode;
    case EXCP_DINT:
        env->CP0_Debug |= 1 << CP0DB_DINT;
        goto set_DEPC;
    case EXCP_DIB:
        env->CP0_Debug |= 1 << CP0DB_DIB;
        goto set_DEPC;
    case EXCP_DBp:
        env->CP0_Debug |= 1 << CP0DB_DBp;
        goto set_DEPC;
    case EXCP_DDBS:
        env->CP0_Debug |= 1 << CP0DB_DDBS;
        goto set_DEPC;
    case EXCP_DDBL:
        env->CP0_Debug |= 1 << CP0DB_DDBL;
    set_DEPC:
        env->CP0_DEPC = exception_resume_pc(env);
        env->hflags &= ~MIPS_HFLAG_BMASK;
 enter_debug_mode:
        if (env->insn_flags & ISA_MIPS3) {
            env->hflags |= MIPS_HFLAG_64;
            if (!(env->insn_flags & ISA_MIPS64R6) ||
                env->CP0_Status & (1 << CP0St_KX)) {
                env->hflags &= ~MIPS_HFLAG_AWRAP;
            }
        }
        env->hflags |= MIPS_HFLAG_DM | MIPS_HFLAG_CP0;
        env->hflags &= ~(MIPS_HFLAG_KSU);
        /* EJTAG probe trap enable is not implemented... */
        if (!(env->CP0_Status & (1 << CP0St_EXL)))
            env->CP0_Cause &= ~(1U << CP0Ca_BD);
        env->active_tc.PC = env->exception_base + 0x480;
        set_hflags_for_handler(env);
        break;
    case EXCP_RESET:
        cpu_reset(CPU(cpu));
        break;
    case EXCP_SRESET:
        env->CP0_Status |= (1 << CP0St_SR);
        memset(env->CP0_WatchLo, 0, sizeof(env->CP0_WatchLo));
        goto set_error_EPC;
    case EXCP_NMI:
        env->CP0_Status |= (1 << CP0St_NMI);
 set_error_EPC:
        env->CP0_ErrorEPC = exception_resume_pc(env);
        env->hflags &= ~MIPS_HFLAG_BMASK;
        env->CP0_Status |= (1 << CP0St_ERL) | (1 << CP0St_BEV);
        if (env->insn_flags & ISA_MIPS3) {
            env->hflags |= MIPS_HFLAG_64;
            if (!(env->insn_flags & ISA_MIPS64R6) ||
                env->CP0_Status & (1 << CP0St_KX)) {
                env->hflags &= ~MIPS_HFLAG_AWRAP;
            }
        }
        env->hflags |= MIPS_HFLAG_CP0;
        env->hflags &= ~(MIPS_HFLAG_KSU);
        if (!(env->CP0_Status & (1 << CP0St_EXL)))
            env->CP0_Cause &= ~(1U << CP0Ca_BD);
        env->active_tc.PC = env->exception_base;
        set_hflags_for_handler(env);
        break;
    case EXCP_EXT_INTERRUPT:
        cause = 0;
        if (env->CP0_Cause & (1 << CP0Ca_IV)) {
            uint32_t spacing = (env->CP0_IntCtl >> CP0IntCtl_VS) & 0x1f;

            if ((env->CP0_Status & (1 << CP0St_BEV)) || spacing == 0) {
                offset = 0x200;
            } else {
                uint32_t vector = 0;
                uint32_t pending = (env->CP0_Cause & CP0Ca_IP_mask) >> CP0Ca_IP;

                if (env->CP0_Config3 & (1 << CP0C3_VEIC)) {
                    /* For VEIC mode, the external interrupt controller feeds
                     * the vector through the CP0Cause IP lines.  */
                    vector = pending;
                } else {
                    /* Vectored Interrupts
                     * Mask with Status.IM7-IM0 to get enabled interrupts. */
                    pending &= (env->CP0_Status >> CP0St_IM) & 0xff;
                    /* Find the highest-priority interrupt. */
                    while (pending >>= 1) {
                        vector++;
                    }
                }
                offset = 0x200 + (vector * (spacing << 5));
            }
        }
        goto set_EPC;
    case EXCP_LTLBL:
        cause = 1;
        update_badinstr = !(env->error_code & EXCP_INST_NOTAVAIL);
        goto set_EPC;
    case EXCP_TLBL:
        cause = 2;
        update_badinstr = !(env->error_code & EXCP_INST_NOTAVAIL);
        if ((env->error_code & EXCP_TLB_NOMATCH) &&
            !(env->CP0_Status & (1 << CP0St_EXL))) {
#if defined(TARGET_MIPS64)
            int R = env->CP0_BadVAddr >> 62;
            int UX = (env->CP0_Status & (1 << CP0St_UX)) != 0;
            int SX = (env->CP0_Status & (1 << CP0St_SX)) != 0;
            int KX = (env->CP0_Status & (1 << CP0St_KX)) != 0;

            if (((R == 0 && UX) || (R == 1 && SX) || (R == 3 && KX)) &&
                (!(env->insn_flags & (INSN_LOONGSON2E | INSN_LOONGSON2F))))
                offset = 0x080;
            else
#endif
                offset = 0x000;
        }
        goto set_EPC;
    case EXCP_TLBS:
        cause = 3;
        update_badinstr = 1;
        if ((env->error_code & EXCP_TLB_NOMATCH) &&
            !(env->CP0_Status & (1 << CP0St_EXL))) {
#if defined(TARGET_MIPS64)
            int R = env->CP0_BadVAddr >> 62;
            int UX = (env->CP0_Status & (1 << CP0St_UX)) != 0;
            int SX = (env->CP0_Status & (1 << CP0St_SX)) != 0;
            int KX = (env->CP0_Status & (1 << CP0St_KX)) != 0;

            if (((R == 0 && UX) || (R == 1 && SX) || (R == 3 && KX)) &&
                (!(env->insn_flags & (INSN_LOONGSON2E | INSN_LOONGSON2F))))
                offset = 0x080;
            else
#endif
                offset = 0x000;
        }
        goto set_EPC;
    case EXCP_AdEL:
        cause = 4;
        update_badinstr = !(env->error_code & EXCP_INST_NOTAVAIL);
        goto set_EPC;
    case EXCP_AdES:
        cause = 5;
        update_badinstr = 1;
        goto set_EPC;
    case EXCP_IBE:
        cause = 6;
        goto set_EPC;
    case EXCP_DBE:
        cause = 7;
        goto set_EPC;
    case EXCP_SYSCALL:
        cause = 8;
        update_badinstr = 1;
        goto set_EPC;
    case EXCP_BREAK:
        cause = 9;
        update_badinstr = 1;
        goto set_EPC;
    case EXCP_RI:
        cause = 10;
        update_badinstr = 1;
        goto set_EPC;
    case EXCP_CpU:
        cause = 11;
        update_badinstr = 1;
        env->CP0_Cause = (env->CP0_Cause & ~(0x3 << CP0Ca_CE)) |
                         (env->error_code << CP0Ca_CE);
        goto set_EPC;
    case EXCP_OVERFLOW:
        cause = 12;
        update_badinstr = 1;
        goto set_EPC;
    case EXCP_TRAP:
        cause = 13;
        update_badinstr = 1;
        goto set_EPC;
    case EXCP_MSAFPE:
        cause = 14;
        update_badinstr = 1;
        goto set_EPC;
    case EXCP_FPE:
        cause = 15;
        update_badinstr = 1;
        goto set_EPC;
    case EXCP_C2E:
        cause = 18;
        goto set_EPC;
    case EXCP_TLBRI:
        cause = 19;
        update_badinstr = 1;
        goto set_EPC;
    case EXCP_TLBXI:
        cause = 20;
        goto set_EPC;
    case EXCP_MSADIS:
        cause = 21;
        update_badinstr = 1;
        goto set_EPC;
    case EXCP_MDMX:
        cause = 22;
        goto set_EPC;
    case EXCP_DWATCH:
        cause = 23;
        /* XXX: TODO: manage deferred watch exceptions */
        goto set_EPC;
    case EXCP_MCHECK:
        cause = 24;
        goto set_EPC;
    case EXCP_THREAD:
        cause = 25;
        goto set_EPC;
    case EXCP_DSPDIS:
        cause = 26;
        goto set_EPC;
    case EXCP_CACHE:
        cause = 30;
        if (env->CP0_Status & (1 << CP0St_BEV)) {
            offset = 0x100;
        } else {
            offset = 0x20000100;
        }
 set_EPC:
        if (!(env->CP0_Status & (1 << CP0St_EXL))) {
            env->CP0_EPC = exception_resume_pc(env);
            if (update_badinstr) {
                set_badinstr_registers(env);
            }
            if (env->hflags & MIPS_HFLAG_BMASK) {
                env->CP0_Cause |= (1U << CP0Ca_BD);
            } else {
                env->CP0_Cause &= ~(1U << CP0Ca_BD);
            }
            env->CP0_Status |= (1 << CP0St_EXL);
            if (env->insn_flags & ISA_MIPS3) {
                env->hflags |= MIPS_HFLAG_64;
                if (!(env->insn_flags & ISA_MIPS64R6) ||
                    env->CP0_Status & (1 << CP0St_KX)) {
                    env->hflags &= ~MIPS_HFLAG_AWRAP;
                }
            }
            env->hflags |= MIPS_HFLAG_CP0;
            env->hflags &= ~(MIPS_HFLAG_KSU);
        }
        env->hflags &= ~MIPS_HFLAG_BMASK;
        if (env->CP0_Status & (1 << CP0St_BEV)) {
            env->active_tc.PC = env->exception_base + 0x200;
        } else {
            env->active_tc.PC = (int32_t)(env->CP0_EBase & ~0x3ff);
        }
        env->active_tc.PC += offset;
        set_hflags_for_handler(env);
        env->CP0_Cause = (env->CP0_Cause & ~(0x1f << CP0Ca_EC)) | (cause << CP0Ca_EC);
        break;
    default:
        abort();
    }
    if (qemu_loglevel_mask(CPU_LOG_INT)
        && cs->exception_index != EXCP_EXT_INTERRUPT) {
        qemu_log("%s: PC " TARGET_FMT_lx " EPC " TARGET_FMT_lx " cause %d\n"
                 "    S %08x C %08x A " TARGET_FMT_lx " D " TARGET_FMT_lx "\n",
                 __func__, env->active_tc.PC, env->CP0_EPC, cause,
                 env->CP0_Status, env->CP0_Cause, env->CP0_BadVAddr,
                 env->CP0_DEPC);
    }
#endif
    cs->exception_index = EXCP_NONE;
}

bool mips_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    if (interrupt_request & CPU_INTERRUPT_HARD) {
        MIPSCPU *cpu = MIPS_CPU(cs);
        CPUMIPSState *env = &cpu->env;

        if (cpu_mips_hw_interrupts_enabled(env) &&
            cpu_mips_hw_interrupts_pending(env)) {
            /* Raise it */
            cs->exception_index = EXCP_EXT_INTERRUPT;
            env->error_code = 0;
            mips_cpu_do_interrupt(cs);
            return true;
        }
    }
    return false;
}

#if !defined(CONFIG_USER_ONLY)
void r4k_invalidate_tlb (CPUMIPSState *env, int idx, int use_extra)
{
    MIPSCPU *cpu = mips_env_get_cpu(env);
    CPUState *cs;
    r4k_tlb_t *tlb;
    target_ulong addr;
    target_ulong end;
    uint16_t ASID = env->CP0_EntryHi & env->CP0_EntryHi_ASID_mask;
    target_ulong mask;

    tlb = &env->tlb->mmu.r4k.tlb[idx];
    /* The qemu TLB is flushed when the ASID changes, so no need to
       flush these entries again.  */
    if (tlb->G == 0 && tlb->ASID != ASID) {
        return;
    }

    if (use_extra && env->tlb->tlb_in_use < MIPS_TLB_MAX) {
        /* For tlbwr, we can shadow the discarded entry into
           a new (fake) TLB entry, as long as the guest can not
           tell that it's there.  */
        env->tlb->mmu.r4k.tlb[env->tlb->tlb_in_use] = *tlb;
        env->tlb->tlb_in_use++;
        return;
    }

    /* 1k pages are not supported. */
    mask = tlb->PageMask | ~(TARGET_PAGE_MASK << 1);
    if (tlb->V0) {
        cs = CPU(cpu);
        addr = tlb->VPN & ~mask;
#if defined(TARGET_MIPS64)
        if (addr >= (0xFFFFFFFF80000000ULL & env->SEGMask)) {
            addr |= 0x3FFFFF0000000000ULL;
        }
#endif
        end = addr | (mask >> 1);
        while (addr < end) {
            tlb_flush_page(cs, addr);
            addr += TARGET_PAGE_SIZE;
        }
    }
    if (tlb->V1) {
        cs = CPU(cpu);
        addr = (tlb->VPN & ~mask) | ((mask >> 1) + 1);
#if defined(TARGET_MIPS64)
        if (addr >= (0xFFFFFFFF80000000ULL & env->SEGMask)) {
            addr |= 0x3FFFFF0000000000ULL;
        }
#endif
        end = addr | mask;
        while (addr - 1 < end) {
            tlb_flush_page(cs, addr);
            addr += TARGET_PAGE_SIZE;
        }
    }
}
#endif

void QEMU_NORETURN do_raise_exception_err(CPUMIPSState *env,
                                          uint32_t exception,
                                          int error_code,
                                          uintptr_t pc)
{
    CPUState *cs = CPU(mips_env_get_cpu(env));

    if (exception < EXCP_SC) {
        qemu_log_mask(CPU_LOG_INT, "%s: %d %d\n",
                      __func__, exception, error_code);
    }
    cs->exception_index = exception;
    env->error_code = error_code;

    cpu_loop_exit_restore(cs, pc);
}
