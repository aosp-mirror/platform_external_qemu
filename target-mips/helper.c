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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>

#include "cpu.h"

enum {
    TLBRET_DIRTY = -4,
    TLBRET_INVALID = -3,
    TLBRET_NOMATCH = -2,
    TLBRET_BADADDR = -1,
    TLBRET_MATCH = 0
};

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
    uint8_t ASID = env->CP0_EntryHi & 0xFF;
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
        if ((tlb->G == 1 || tlb->ASID == ASID) && VPN == tag) {
            /* TLB match */
            int n = !!(address & mask & ~(mask >> 1));
            /* Check access rights */
            if (!(n ? tlb->V1 : tlb->V0))
                return TLBRET_INVALID;
            if (rw == 0 || (n ? tlb->D1 : tlb->D0)) {
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


#if !defined(CONFIG_USER_ONLY)
static int get_physical_address (CPUMIPSState *env, hwaddr *physical,
                                int *prot, target_ulong address,
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

#if 0
    qemu_log("user mode %d h %08x\n", user_mode, env->hflags);
#endif

    if (address <= (int32_t)0x7FFFFFFFUL) {
        /* useg */
        if (env->CP0_Status & (1 << CP0St_ERL)) {
            *physical = address & 0xFFFFFFFF;
            *prot = PAGE_READ | PAGE_WRITE;
        } else {
            ret = env->tlb->map_address(env, physical, prot, address, rw, access_type);
        }
#if defined(TARGET_MIPS64)
    } else if (address < 0x4000000000000000ULL) {
        /* xuseg */
        if (UX && address <= (0x3FFFFFFFFFFFFFFFULL & env->SEGMask)) {
            ret = env->tlb->map_address(env, physical, prot, address, rw, access_type);
        } else {
            ret = TLBRET_BADADDR;
        }
    } else if (address < 0x8000000000000000ULL) {
        /* xsseg */
        if ((supervisor_mode || kernel_mode) &&
            SX && address <= (0x7FFFFFFFFFFFFFFFULL & env->SEGMask)) {
            ret = env->tlb->map_address(env, physical, prot, address, rw, access_type);
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
            ret = env->tlb->map_address(env, physical, prot, address, rw, access_type);
        } else {
            ret = TLBRET_BADADDR;
        }
#endif
    } else if (address < (int32_t)0xA0000000UL) {
        /* kseg0 */
        if (kernel_mode) {
            *physical = address - (int32_t)0x80000000UL;
            *prot = PAGE_READ | PAGE_WRITE;
        } else {
            ret = TLBRET_BADADDR;
        }
    } else if (address < (int32_t)0xC0000000UL) {
        /* kseg1 */
        if (kernel_mode) {
            *physical = address - (int32_t)0xA0000000UL;
            *prot = PAGE_READ | PAGE_WRITE;
        } else {
            ret = TLBRET_BADADDR;
        }
    } else if (address < (int32_t)0xE0000000UL) {
        /* sseg (kseg2) */
        if (supervisor_mode || kernel_mode) {
            ret = env->tlb->map_address(env, physical, prot, address, rw, access_type);
        } else {
            ret = TLBRET_BADADDR;
        }
    } else {
        /* kseg3 */
        /* XXX: debug segment is not emulated */
        if (kernel_mode) {
            ret = env->tlb->map_address(env, physical, prot, address, rw, access_type);
        } else {
            ret = TLBRET_BADADDR;
        }
    }
#if 0
    qemu_log(TARGET_FMT_lx " %d %d => " TARGET_FMT_lx " %d (%d)\n",
            address, rw, access_type, *physical, *prot, ret);
#endif

    return ret;
}
#endif

static void raise_mmu_exception(CPUMIPSState *env, target_ulong address,
                                int rw, int tlb_error)
{
    int exception = 0, error_code = 0;

    switch (tlb_error) {
    default:
    case TLBRET_BADADDR:
        /* Reference to kernel address from user mode or supervisor mode */
        /* Reference to supervisor address from user mode */
        if (rw)
            exception = EXCP_AdES;
        else
            exception = EXCP_AdEL;
        break;
    case TLBRET_NOMATCH:
        /* No TLB match for a mapped address */
        if (rw)
            exception = EXCP_TLBS;
        else
            exception = EXCP_TLBL;
        error_code = 1;
        break;
    case TLBRET_INVALID:
        /* TLB match with no valid bit */
        if (rw)
            exception = EXCP_TLBS;
        else
            exception = EXCP_TLBL;
        break;
    case TLBRET_DIRTY:
        /* TLB match but 'D' bit is cleared */
        exception = EXCP_LTLBL;
        break;

    }
    /* Raise exception */
    env->CP0_BadVAddr = address;
    env->CP0_Context = (env->CP0_Context & ~0x007fffff) |
                       ((address >> 9) & 0x007ffff0);
    env->CP0_EntryHi =
        (env->CP0_EntryHi & 0xFF) | (address & (TARGET_PAGE_MASK << 1));
#if defined(TARGET_MIPS64)
    env->CP0_EntryHi &= env->SEGMask;
    env->CP0_XContext = (env->CP0_XContext & ((~0ULL) << (env->SEGBITS - 7))) |
                        ((address & 0xC00000000000ULL) >> (55 - env->SEGBITS)) |
                        ((address & ((1ULL << env->SEGBITS) - 1) & 0xFFFFFFFFFFFFE000ULL) >> 9);
#endif
    env->exception_index = exception;
    env->error_code = error_code;
}

/*
 * Get the pgd_current from TLB refill handler
 * The kernel refill handler is generated by
 * function build_r4000_tlb_refill_handler.
 */

typedef void (*pagetable_walk_t)(CPUMIPSState *env,
                                  target_ulong pgd_addr, target_ulong vaddr,
                                  target_ulong *entrylo0, target_ulong *entrylo1);
static struct {
    enum {PROBE, USEFASTTLB, USESLOWTLB} state;
    uint32_t is_64bit;
    pagetable_walk_t pagetable_walk;
    target_ulong pgd_current_p;
    int softshift;
} linux_pte_info = {0};

static inline void pagetable_walk32(CPUMIPSState *env,
                                  target_ulong pgd_addr, target_ulong vaddr,
                                  target_ulong *entrylo0, target_ulong *entrylo1)
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

    pt_addr = ldl_phys(ptw_phys);

    ptw_phys = pt_addr & 0x1fffffffUL; /* Assume pgt is in KSEG0/KSEG1 */
    index = ((vaddr >> 13) & 0x1ff) << 3; /* Use bits 21..13 to index pgt */
    ptw_phys += index;

    /* Get the entrylo values from pgt */
    *entrylo0 = ldl_phys(ptw_phys) >> linux_pte_info.softshift;
    *entrylo1 = ldl_phys(ptw_phys + 4) >> linux_pte_info.softshift;
}

static inline void pagetable_walk64(CPUMIPSState *env,
                                  target_ulong pgd_addr, target_ulong vaddr,
                                  target_ulong *entrylo0, target_ulong *entrylo1)
{
    target_ulong ptw_phys, pt_addr, index;

    pgd_addr = pgd_addr & 0x1fffffffUL;
    index = ((uint64_t)vaddr >> 0x1b) & 0x1ff8;
    pgd_addr += index;

    pgd_addr = ldl_phys(pgd_addr);

    ptw_phys = pgd_addr & 0x1fffffffUL;
    index = ((uint64_t)vaddr >> 0x12) & 0xff8;
    ptw_phys += index;

    pt_addr = ldl_phys(ptw_phys);

    ptw_phys = pt_addr & 0x1fffffffUL;
    index = (((vaddr & 0xC00000000000ULL) >> (55 - env->SEGBITS)) |
             ((vaddr & ((1ULL << env->SEGBITS) - 1) & 0xFFFFFFFFFFFFE000ULL) >> 9)) & 0xff0;
    ptw_phys += index;

    /* Get the entrylo values from pgt */
    *entrylo0 = ldl_phys(ptw_phys) >> linux_pte_info.softshift;
    *entrylo1 = ldl_phys(ptw_phys + 8) >> linux_pte_info.softshift;
}

static inline target_ulong cpu_mips_get_pgd(CPUMIPSState *env, target_long bad_vaddr)
{
    if (unlikely(linux_pte_info.state == PROBE)) {
        int i;
        uint32_t lui_ins, lw_ins, srl_ins, is_64bit;
        target_ulong address;
        uint32_t ebase;

        /*
         * The exact TLB refill code varies depeing on the kernel version
         * and configuration. Examins the TLB handler to extract
         * pgd_current_p and the shift required to convert in memory PTE
         * to TLB format
         */
        static struct {
            uint32_t is_64bit;
            struct {
                uint32_t off;
                uint32_t op;
                uint32_t mask;
            } lui, lw, srl;
        } handlers[] = {
            /* 2.6.29+ */
            {
                0,  /* 32-bit refill handler */
                {0x00, 0x3c1b0000, 0xffff0000}, /* 0x3c1b803f : lui k1,%hi(pgd_current_p) */
                {0x08, 0x8f7b0000, 0xffff0000}, /* 0x8f7b3000 : lw  k1,%lo(k1) */
                {0x34, 0x001ad182, 0xffffffff}  /* 0x001ad182 : srl k0,k0,0x6 */
            },
            /* 3.4+ */
            {
                0, /* 32-bit refill handler */
                {0x00, 0x3c1b0000, 0xffff0000}, /* 0x3c1b803f : lui k1,%hi(pgd_current_p) */
                {0x08, 0x8f7b0000, 0xffff0000}, /* 0x8f7b3000 : lw  k1,%lo(k1) */
                {0x34, 0x001ad142, 0xffffffff}  /* 0x001ad182 : srl k0,k0,0x5 */
            },
            /* 3.10+ No HugeTLB support*/
            {
                1, /* 64-bit refill handler */
                {0x04, 0x3c1b0000, 0xffff0000}, /* 0x3c1b0000 : lui k1,%hi(swapper_pg_dir) */
                {0xac, 0xdf7b0000, 0xffff0000}, /* 0xdf7b0000 : ld k1,0(k1) */
                {0xd4, 0x001ad17a, 0xffffffff}  /* 0x001ad17a : dsrl k0,k0,0x5 */
            }
        };

        ebase = env->CP0_EBase - 0x80000000;
        linux_pte_info.is_64bit = 0;

        /* Match the kernel TLB refill exception handler against known code */
        for (i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++) {
            is_64bit = handlers[i].is_64bit;
            lui_ins = ldl_phys(ebase + handlers[i].lui.off);
            lw_ins = ldl_phys(ebase + handlers[i].lw.off);
            srl_ins = ldl_phys(ebase + handlers[i].srl.off);
            if (((lui_ins & handlers[i].lui.mask) == handlers[i].lui.op) &&
                ((lw_ins & handlers[i].lw.mask) == handlers[i].lw.op) &&
                ((srl_ins & handlers[i].srl.mask) == handlers[i].srl.op))
                break;
        }
        if (i >= sizeof(handlers)/sizeof(handlers[0])) {
#if defined(MUST_HAVE_FASTTLB)
                printf("TLBMiss handler dump:\n");
            for (i = 0; i < 0x80; i+= 4)
                printf("0x%08x: 0x%08x\n", ebase + i, ldl_phys(ebase + i));
            cpu_abort(env, "TLBMiss handler signature not recognised\n");
#else
            fprintf(stderr, "TLBMiss handler signature not recognised, using slowpath\n");
            linux_pte_info.state = USESLOWTLB;
            linux_pte_info.pagetable_walk = NULL;
            goto done;
#endif
        }

        if (is_64bit) {
            linux_pte_info.pagetable_walk = &pagetable_walk64;
            linux_pte_info.is_64bit = 1;
            /* swapper_pg_dir address */
            address = (lui_ins & 0xffff) << 16;
        } else {
            address = (lui_ins & 0xffff) << 16;
            address += (((int32_t)(lw_ins & 0xffff)) << 16) >> 16;
            if (address >= 0x80000000 && address < 0xa0000000)
                address -= 0x80000000;
            else if (address >= 0xa0000000 && address <= 0xc0000000)
                address -= 0xa0000000;
            else
                cpu_abort(env, "pgd_current_p not in KSEG0/KSEG1\n");

            linux_pte_info.pagetable_walk = &pagetable_walk32;
        }

        linux_pte_info.state = USEFASTTLB;
        linux_pte_info.pgd_current_p = address;
        linux_pte_info.softshift = (srl_ins >> 6) & 0x1f;
    }

done:
    /* Get pgd_current */
    if (linux_pte_info.state == USEFASTTLB) {
        if (linux_pte_info.is_64bit) {
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
            } else if (bad_vaddr < 0) {
                /* swapper_pg_dir address */
                return linux_pte_info.pgd_current_p;
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
            return ldl_phys(linux_pte_info.pgd_current_p);
        }
    }
    return 0;
}

static inline int cpu_mips_tlb_refill(CPUMIPSState *env, target_ulong address, int rw,
                                      int mmu_idx, int is_softmmu)
{
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
    pgd_addr = cpu_mips_get_pgd(env, address);

    /* if pgd_addr is unknown return TLBRET_NOMATCH
     * to allow software handler to run
     */
    if (unlikely(pgd_addr == 0)) {
        ret = TLBRET_NOMATCH;
        goto out;
    }

    env->hflags = MIPS_HFLAG_KM;
    fault_addr = env->CP0_BadVAddr;

    linux_pte_info.pagetable_walk(env, pgd_addr, fault_addr, &entrylo0, &entrylo1);

    /* Refill the TLB */
    env->CP0_EntryLo0 = entrylo0;
    env->CP0_EntryLo1 = entrylo1;
    r4k_helper_tlbwr(env);

    /* Since we know the TLB contents, we can
     * return the TLB lookup value here.
     */

    target_ulong mask = env->CP0_PageMask | ~(TARGET_PAGE_MASK << 1);
    target_ulong lo = (address & mask & ~(mask >> 1)) ? entrylo1 : entrylo0;

    /* Is the TLB entry valid? */
    if ((lo & (1 << CP0EnLo_V)) == 0) {
        ret = TLBRET_INVALID;
        goto out;
    }

    /* Is this a read access or a write to a modifiable page? */
    if (rw == 0 || (lo & (1 << CP0EnLo_D))) {
        hwaddr physical = (lo >> CP0EnLo_PFN) << 12;
        physical |= address & (mask >> 1);
        int prot = PAGE_READ;
        if (lo & (1 << CP0EnLo_D))
            prot |= PAGE_WRITE;

        tlb_set_page(env, address & TARGET_PAGE_MASK,
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

int cpu_mips_handle_mmu_fault (CPUMIPSState *env, target_ulong address, int rw,
                               int mmu_idx)
{
#if !defined(CONFIG_USER_ONLY)
    hwaddr physical;
    int prot;
#endif
    //int exception = 0, error_code = 0;
    int access_type;
    int ret = 0;

#if 0
    log_cpu_state(env, 0);
#endif
    qemu_log("%s pc " TARGET_FMT_lx " ad " TARGET_FMT_lx " rw %d mmu_idx %d\n",
              __func__, env->active_tc.PC, address, rw, mmu_idx);

    rw &= 1;

    /* data access */
    /* XXX: put correct access by using cpu_restore_state()
       correctly */
    access_type = ACCESS_INT;
#if defined(CONFIG_USER_ONLY)
    ret = TLBRET_NOMATCH;
#else
    ret = get_physical_address(env, &physical, &prot,
                               address, rw, access_type);
    qemu_log("%s address=" TARGET_FMT_lx " ret %d physical " TARGET_FMT_plx " prot %d\n",
              __func__, address, ret, physical, prot);
    if (ret == TLBRET_MATCH) {
       tlb_set_page(env, address & TARGET_PAGE_MASK,
                    physical & TARGET_PAGE_MASK, prot | PAGE_EXEC,
                    mmu_idx, TARGET_PAGE_SIZE);
       ret = 0;
   } else if (ret == TLBRET_NOMATCH)
        ret = cpu_mips_tlb_refill(env,address,rw,mmu_idx,1);

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

    rw &= 1;

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
#endif

hwaddr cpu_get_phys_page_debug(CPUMIPSState *env, target_ulong addr)
{
#if defined(CONFIG_USER_ONLY)
    return addr;
#else
    hwaddr phys_addr;
    int prot, ret;

#if defined(TARGET_MIPS64)
    if (!(linux_pte_info.is_64bit) &&
         (linux_pte_info.state == USEFASTTLB))
        addr = ((int64_t)addr << 32) >> 32;
#endif
    ret = get_physical_address(env, &phys_addr, &prot, addr, 0, ACCESS_INT);
    if (ret != TLBRET_MATCH) {
        target_ulong pgd_addr = cpu_mips_get_pgd(env, addr);

        phys_addr = -1;
        if (likely(pgd_addr)) {
            target_ulong entrylo0, entrylo1;

            linux_pte_info.pagetable_walk(env, pgd_addr, addr, &entrylo0, &entrylo1);

            target_ulong mask = env->CP0_PageMask | ~(TARGET_PAGE_MASK << 1);
            target_ulong lo = (addr & mask & ~(mask >> 1)) ? entrylo1 : entrylo0;

            if (lo & (1 << CP0EnLo_V)) {
                phys_addr = ((lo >> CP0EnLo_PFN) << 12) | (addr & (mask >> 1));
            } else {
                qemu_log("cpu_get_phys_page_debug() invalid mapping for vaddr: 0x" TARGET_FMT_plx "\n", addr);
            }
        } else {
            qemu_log("cpu_get_phys_page_debug() fails for vaddr: 0x" TARGET_FMT_plx "\n", addr);
        }
    }
    return phys_addr;
#endif
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
};

void do_interrupt (CPUMIPSState *env)
{
#if !defined(CONFIG_USER_ONLY)
    target_ulong offset;
    int cause = -1;
    const char *name;

    if (qemu_log_enabled() && env->exception_index != EXCP_EXT_INTERRUPT) {
        if (env->exception_index < 0 || env->exception_index > EXCP_LAST)
            name = "unknown";
        else
            name = excp_names[env->exception_index];

        qemu_log("%s enter: PC " TARGET_FMT_lx " EPC " TARGET_FMT_lx " %s exception\n",
                 __func__, env->active_tc.PC, env->CP0_EPC, name);
    }
    if (env->exception_index == EXCP_EXT_INTERRUPT &&
        (env->hflags & MIPS_HFLAG_DM))
        env->exception_index = EXCP_DINT;
    offset = 0x180;
    switch (env->exception_index) {
    case EXCP_DSS:
        env->CP0_Debug |= 1 << CP0DB_DSS;
        /* Debug single step cannot be raised inside a delay slot and
           resume will always occur on the next instruction
           (but we assume the pc has always been updated during
           code translation). */
        env->CP0_DEPC = env->active_tc.PC;
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
        if (env->hflags & MIPS_HFLAG_BMASK) {
            /* If the exception was raised from a delay slot,
               come back to the jump.  */
            env->CP0_DEPC = env->active_tc.PC - 4;
            env->hflags &= ~MIPS_HFLAG_BMASK;
        } else {
            env->CP0_DEPC = env->active_tc.PC;
        }
 enter_debug_mode:
        env->hflags |= MIPS_HFLAG_DM | MIPS_HFLAG_64 | MIPS_HFLAG_CP0;
        env->hflags &= ~(MIPS_HFLAG_KSU);
        /* EJTAG probe trap enable is not implemented... */
        if (!(env->CP0_Status & (1 << CP0St_EXL)))
            env->CP0_Cause &= ~(1 << CP0Ca_BD);
        env->active_tc.PC = (int32_t)0xBFC00480;
        break;
    case EXCP_RESET:
        cpu_reset(ENV_GET_CPU(env));
        break;
    case EXCP_SRESET:
        env->CP0_Status |= (1 << CP0St_SR);
        memset(env->CP0_WatchLo, 0, sizeof(*env->CP0_WatchLo));
        goto set_error_EPC;
    case EXCP_NMI:
        env->CP0_Status |= (1 << CP0St_NMI);
 set_error_EPC:
        if (env->hflags & MIPS_HFLAG_BMASK) {
            /* If the exception was raised from a delay slot,
               come back to the jump.  */
            env->CP0_ErrorEPC = env->active_tc.PC - 4;
            env->hflags &= ~MIPS_HFLAG_BMASK;
        } else {
            env->CP0_ErrorEPC = env->active_tc.PC;
        }
        env->CP0_Status |= (1 << CP0St_ERL) | (1 << CP0St_BEV);
        env->hflags |= MIPS_HFLAG_64 | MIPS_HFLAG_CP0;
        env->hflags &= ~(MIPS_HFLAG_KSU);
        if (!(env->CP0_Status & (1 << CP0St_EXL)))
            env->CP0_Cause &= ~(1 << CP0Ca_BD);
        env->active_tc.PC = (int32_t)0xBFC00000;
        break;
    case EXCP_EXT_INTERRUPT:
        cause = 0;
        if (env->CP0_Cause & (1 << CP0Ca_IV))
            offset = 0x200;
        goto set_EPC;
    case EXCP_LTLBL:
        cause = 1;
        goto set_EPC;
    case EXCP_TLBL:
        cause = 2;
        if (env->error_code == 1 && !(env->CP0_Status & (1 << CP0St_EXL))) {
#if defined(TARGET_MIPS64)
            int R = env->CP0_BadVAddr >> 62;
            int UX = (env->CP0_Status & (1 << CP0St_UX)) != 0;
            int SX = (env->CP0_Status & (1 << CP0St_SX)) != 0;
            int KX = (env->CP0_Status & (1 << CP0St_KX)) != 0;

            if ((R == 0 && UX) || (R == 1 && SX) || (R == 3 && KX))
                offset = 0x080;
            else
#endif
                offset = 0x000;
        }
        goto set_EPC;
    case EXCP_TLBS:
        cause = 3;
        if (env->error_code == 1 && !(env->CP0_Status & (1 << CP0St_EXL))) {
#if defined(TARGET_MIPS64)
            int R = env->CP0_BadVAddr >> 62;
            int UX = (env->CP0_Status & (1 << CP0St_UX)) != 0;
            int SX = (env->CP0_Status & (1 << CP0St_SX)) != 0;
            int KX = (env->CP0_Status & (1 << CP0St_KX)) != 0;

            if ((R == 0 && UX) || (R == 1 && SX) || (R == 3 && KX))
                offset = 0x080;
            else
#endif
                offset = 0x000;
        }
        goto set_EPC;
    case EXCP_AdEL:
        cause = 4;
        goto set_EPC;
    case EXCP_AdES:
        cause = 5;
        goto set_EPC;
    case EXCP_IBE:
        cause = 6;
        goto set_EPC;
    case EXCP_DBE:
        cause = 7;
        goto set_EPC;
    case EXCP_SYSCALL:
        cause = 8;
        goto set_EPC;
    case EXCP_BREAK:
        cause = 9;
        goto set_EPC;
    case EXCP_RI:
        cause = 10;
        goto set_EPC;
    case EXCP_CpU:
        cause = 11;
        env->CP0_Cause = (env->CP0_Cause & ~(0x3 << CP0Ca_CE)) |
                         (env->error_code << CP0Ca_CE);
        goto set_EPC;
    case EXCP_OVERFLOW:
        cause = 12;
        goto set_EPC;
    case EXCP_TRAP:
        cause = 13;
        goto set_EPC;
    case EXCP_FPE:
        cause = 15;
        goto set_EPC;
    case EXCP_C2E:
        cause = 18;
        goto set_EPC;
    case EXCP_MDMX:
        cause = 22;
        goto set_EPC;
    case EXCP_DWATCH:
        cause = 23;
        /* XXX: TODO: manage defered watch exceptions */
        goto set_EPC;
    case EXCP_MCHECK:
        cause = 24;
        goto set_EPC;
    case EXCP_THREAD:
        cause = 25;
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
            if (env->hflags & MIPS_HFLAG_BMASK) {
                /* If the exception was raised from a delay slot,
                   come back to the jump.  */
                env->CP0_EPC = env->active_tc.PC - 4;
                env->CP0_Cause |= (1 << CP0Ca_BD);
            } else {
                env->CP0_EPC = env->active_tc.PC;
                env->CP0_Cause &= ~(1 << CP0Ca_BD);
            }
            env->CP0_Status |= (1 << CP0St_EXL);
            env->hflags |= MIPS_HFLAG_64 | MIPS_HFLAG_CP0;
            env->hflags &= ~(MIPS_HFLAG_KSU);
        }
        env->hflags &= ~MIPS_HFLAG_BMASK;
        if (env->CP0_Status & (1 << CP0St_BEV)) {
            env->active_tc.PC = (int32_t)0xBFC00200;
        } else {
            env->active_tc.PC = (int32_t)(env->CP0_EBase & ~0x3ff);
        }
        env->active_tc.PC += offset;
        env->CP0_Cause = (env->CP0_Cause & ~(0x1f << CP0Ca_EC)) | (cause << CP0Ca_EC);
        break;
    default:
        qemu_log("Invalid MIPS exception %d. Exiting\n", env->exception_index);
        printf("Invalid MIPS exception %d. Exiting\n", env->exception_index);
        exit(1);
    }
    if (qemu_log_enabled() && env->exception_index != EXCP_EXT_INTERRUPT) {
        qemu_log("%s: PC " TARGET_FMT_lx " EPC " TARGET_FMT_lx " cause %d\n"
                "    S %08x C %08x A " TARGET_FMT_lx " D " TARGET_FMT_lx "\n",
                __func__, env->active_tc.PC, env->CP0_EPC, cause,
                env->CP0_Status, env->CP0_Cause, env->CP0_BadVAddr,
                env->CP0_DEPC);
    }
#endif
    env->exception_index = EXCP_NONE;
}

void r4k_invalidate_tlb (CPUMIPSState *env, int idx, int use_extra)
{
    r4k_tlb_t *tlb;
    target_ulong addr;
    target_ulong end;
    uint8_t ASID = env->CP0_EntryHi & 0xFF;
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
        addr = tlb->VPN & ~mask;
#if defined(TARGET_MIPS64)
        if (addr >= (0xFFFFFFFF80000000ULL & env->SEGMask)) {
            addr |= 0x3FFFFF0000000000ULL;
        }
#endif
        end = addr | (mask >> 1);
        while (addr < end) {
            tlb_flush_page(env, addr);
            addr += TARGET_PAGE_SIZE;
        }
    }
    if (tlb->V1) {
        addr = (tlb->VPN & ~mask) | ((mask >> 1) + 1);
#if defined(TARGET_MIPS64)
        if (addr >= (0xFFFFFFFF80000000ULL & env->SEGMask)) {
            addr |= 0x3FFFFF0000000000ULL;
        }
#endif
        end = addr | mask;
        while (addr - 1 < end) {
            tlb_flush_page(env, addr);
            addr += TARGET_PAGE_SIZE;
        }
    }
}
