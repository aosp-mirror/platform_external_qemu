/*
 *  i386 helpers
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */
#include <math.h>

#define CPU_NO_GLOBAL_REGS
#include "cpu.h"
#include "qemu/aes.h"
#include "qemu/host-utils.h"
#include "exec/cpu-defs.h"
#include "helper.h"

#if !defined(CONFIG_USER_ONLY)
#include "exec/softmmu_exec.h"
#endif /* !defined(CONFIG_USER_ONLY) */

/* check if Port I/O is allowed in TSS */
static inline void check_io(CPUX86State *env, int addr, int size)
{
    int io_offset, val, mask;

    /* TSS must be a valid 32 bit one */
    if (!(env->tr.flags & DESC_P_MASK) ||
        ((env->tr.flags >> DESC_TYPE_SHIFT) & 0xf) != 9 ||
        env->tr.limit < 103)
        goto fail;
    io_offset = cpu_lduw_kernel(env, env->tr.base + 0x66);
    io_offset += (addr >> 3);
    /* Note: the check needs two bytes */
    if ((io_offset + 1) > env->tr.limit)
        goto fail;
    val = cpu_lduw_kernel(env, env->tr.base + io_offset);
    val >>= (addr & 7);
    mask = (1 << size) - 1;
    /* all bits must be zero to allow the I/O */
    if ((val & mask) != 0) {
    fail:
        raise_exception_err(env, EXCP0D_GPF, 0);
    }
}

void helper_check_iob(CPUX86State *env, uint32_t t0)
{
    check_io(env, t0, 1);
}

void helper_check_iow(CPUX86State *env, uint32_t t0)
{
    check_io(env, t0, 2);
}

void helper_check_iol(CPUX86State *env, uint32_t t0)
{
    check_io(env, t0, 4);
}

void helper_outb(uint32_t port, uint32_t data)
{
    cpu_outb(port, data & 0xff);
}

target_ulong helper_inb(uint32_t port)
{
    return cpu_inb(port);
}

void helper_outw(uint32_t port, uint32_t data)
{
    cpu_outw(port, data & 0xffff);
}

target_ulong helper_inw(uint32_t port)
{
    return cpu_inw(port);
}

void helper_outl(uint32_t port, uint32_t data)
{
    cpu_outl(port, data);
}

target_ulong helper_inl(uint32_t port)
{
    return cpu_inl(port);
}

void helper_into(CPUX86State *env, int next_eip_addend)
{
    int eflags;
    eflags = helper_cc_compute_all(env, CC_OP);
    if (eflags & CC_O) {
        raise_interrupt(env, EXCP04_INTO, 1, 0, next_eip_addend);
    }
}

void helper_single_step(CPUX86State *env)
{
#ifndef CONFIG_USER_ONLY
    check_hw_breakpoints(env, 1);
    env->dr[6] |= DR6_BS;
#endif
    raise_exception(env, EXCP01_DB);
}

void helper_cpuid(CPUX86State *env)
{
    uint32_t eax, ebx, ecx, edx;

    helper_svm_check_intercept_param(env, SVM_EXIT_CPUID, 0);

    cpu_x86_cpuid(env, (uint32_t)EAX, (uint32_t)ECX, &eax, &ebx, &ecx, &edx);
    EAX = eax;
    EBX = ebx;
    ECX = ecx;
    EDX = edx;
}

#if defined(CONFIG_USER_ONLY)
target_ulong helper_read_crN(CPUX86State *env, int reg)
{
    return 0;
}

void helper_write_crN(CPUX86State *env, int reg, target_ulong t0)
{
}

void helper_movl_drN_T0(CPUX86State *env, int reg, target_ulong t0)
{
}
#else
target_ulong helper_read_crN(CPUX86State *env, int reg)
{
    target_ulong val;

    helper_svm_check_intercept_param(env, SVM_EXIT_READ_CR0 + reg, 0);
    switch(reg) {
    default:
        val = env->cr[reg];
        break;
    case 8:
        if (!(env->hflags2 & HF2_VINTR_MASK)) {
            val = cpu_get_apic_tpr(env);
        } else {
            val = env->v_tpr;
        }
        break;
    }
    return val;
}

void helper_write_crN(CPUX86State *env, int reg, target_ulong t0)
{
    helper_svm_check_intercept_param(env, SVM_EXIT_WRITE_CR0 + reg, 0);
    switch(reg) {
    case 0:
        cpu_x86_update_cr0(env, t0);
        break;
    case 3:
        cpu_x86_update_cr3(env, t0);
        break;
    case 4:
        cpu_x86_update_cr4(env, t0);
        break;
    case 8:
        if (!(env->hflags2 & HF2_VINTR_MASK)) {
            cpu_set_apic_tpr(env, t0);
        }
        env->v_tpr = t0 & 0x0f;
        break;
    default:
        env->cr[reg] = t0;
        break;
    }
}

void helper_movl_drN_T0(CPUX86State *env, int reg, target_ulong t0)
{
    int i;

    if (reg < 4) {
        hw_breakpoint_remove(env, reg);
        env->dr[reg] = t0;
        hw_breakpoint_insert(env, reg);
    } else if (reg == 7) {
        for (i = 0; i < 4; i++)
            hw_breakpoint_remove(env, i);
        env->dr[7] = t0;
        for (i = 0; i < 4; i++)
            hw_breakpoint_insert(env, i);
    } else
        env->dr[reg] = t0;
}
#endif

void helper_lmsw(CPUX86State *env, target_ulong t0)
{
    /* only 4 lower bits of CR0 are modified. PE cannot be set to zero
       if already set to one. */
    t0 = (env->cr[0] & ~0xe) | (t0 & 0xf);
    helper_write_crN(env, 0, t0);
}

void helper_invlpg(CPUX86State *env, target_ulong addr)
{
    helper_svm_check_intercept_param(env, SVM_EXIT_INVLPG, 0);
    tlb_flush_page(env, addr);
}

void helper_rdtsc(CPUX86State *env)
{
    uint64_t val;

    if ((env->cr[4] & CR4_TSD_MASK) && ((env->hflags & HF_CPL_MASK) != 0)) {
        raise_exception(env, EXCP0D_GPF);
    }
    helper_svm_check_intercept_param(env, SVM_EXIT_RDTSC, 0);

    val = cpu_get_tsc(env) + env->tsc_offset;
    EAX = (uint32_t)(val);
    EDX = (uint32_t)(val >> 32);
}

void helper_rdpmc(CPUX86State *env)
{
    if ((env->cr[4] & CR4_PCE_MASK) && ((env->hflags & HF_CPL_MASK) != 0)) {
        raise_exception(env, EXCP0D_GPF);
    }
    helper_svm_check_intercept_param(env, SVM_EXIT_RDPMC, 0);

    /* currently unimplemented */
    raise_exception_err(env, EXCP06_ILLOP, 0);
}

#if defined(CONFIG_USER_ONLY)
void helper_wrmsr(CPUX86State *env)
{
}

void helper_rdmsr(CPUX86State *env)
{
}
#else
void helper_wrmsr(CPUX86State *env)
{
    uint64_t val;

    helper_svm_check_intercept_param(env, SVM_EXIT_MSR, 1);

    val = ((uint32_t)EAX) | ((uint64_t)((uint32_t)EDX) << 32);

    switch((uint32_t)ECX) {
    case MSR_IA32_SYSENTER_CS:
        env->sysenter_cs = val & 0xffff;
        break;
    case MSR_IA32_SYSENTER_ESP:
        env->sysenter_esp = val;
        break;
    case MSR_IA32_SYSENTER_EIP:
        env->sysenter_eip = val;
        break;
    case MSR_IA32_APICBASE:
        cpu_set_apic_base(env, val);
        break;
    case MSR_EFER:
        {
            uint64_t update_mask;
            update_mask = 0;
            if (env->cpuid_ext2_features & CPUID_EXT2_SYSCALL)
                update_mask |= MSR_EFER_SCE;
            if (env->cpuid_ext2_features & CPUID_EXT2_LM)
                update_mask |= MSR_EFER_LME;
            if (env->cpuid_ext2_features & CPUID_EXT2_FFXSR)
                update_mask |= MSR_EFER_FFXSR;
            if (env->cpuid_ext2_features & CPUID_EXT2_NX)
                update_mask |= MSR_EFER_NXE;
            if (env->cpuid_ext3_features & CPUID_EXT3_SVM)
                update_mask |= MSR_EFER_SVME;
            if (env->cpuid_ext2_features & CPUID_EXT2_FFXSR)
                update_mask |= MSR_EFER_FFXSR;
            cpu_load_efer(env, (env->efer & ~update_mask) |
                          (val & update_mask));
        }
        break;
    case MSR_STAR:
        env->star = val;
        break;
    case MSR_PAT:
        env->pat = val;
        break;
    case MSR_VM_HSAVE_PA:
        env->vm_hsave = val;
        break;
#ifdef TARGET_X86_64
    case MSR_LSTAR:
        env->lstar = val;
        break;
    case MSR_CSTAR:
        env->cstar = val;
        break;
    case MSR_FMASK:
        env->fmask = val;
        break;
    case MSR_FSBASE:
        env->segs[R_FS].base = val;
        break;
    case MSR_GSBASE:
        env->segs[R_GS].base = val;
        break;
    case MSR_KERNELGSBASE:
        env->kernelgsbase = val;
        break;
#endif
    case MSR_MTRRphysBase(0):
    case MSR_MTRRphysBase(1):
    case MSR_MTRRphysBase(2):
    case MSR_MTRRphysBase(3):
    case MSR_MTRRphysBase(4):
    case MSR_MTRRphysBase(5):
    case MSR_MTRRphysBase(6):
    case MSR_MTRRphysBase(7):
        env->mtrr_var[((uint32_t)ECX - MSR_MTRRphysBase(0)) / 2].base = val;
        break;
    case MSR_MTRRphysMask(0):
    case MSR_MTRRphysMask(1):
    case MSR_MTRRphysMask(2):
    case MSR_MTRRphysMask(3):
    case MSR_MTRRphysMask(4):
    case MSR_MTRRphysMask(5):
    case MSR_MTRRphysMask(6):
    case MSR_MTRRphysMask(7):
        env->mtrr_var[((uint32_t)ECX - MSR_MTRRphysMask(0)) / 2].mask = val;
        break;
    case MSR_MTRRfix64K_00000:
        env->mtrr_fixed[(uint32_t)ECX - MSR_MTRRfix64K_00000] = val;
        break;
    case MSR_MTRRfix16K_80000:
    case MSR_MTRRfix16K_A0000:
        env->mtrr_fixed[(uint32_t)ECX - MSR_MTRRfix16K_80000 + 1] = val;
        break;
    case MSR_MTRRfix4K_C0000:
    case MSR_MTRRfix4K_C8000:
    case MSR_MTRRfix4K_D0000:
    case MSR_MTRRfix4K_D8000:
    case MSR_MTRRfix4K_E0000:
    case MSR_MTRRfix4K_E8000:
    case MSR_MTRRfix4K_F0000:
    case MSR_MTRRfix4K_F8000:
        env->mtrr_fixed[(uint32_t)ECX - MSR_MTRRfix4K_C0000 + 3] = val;
        break;
    case MSR_MTRRdefType:
        env->mtrr_deftype = val;
        break;
    case MSR_MCG_STATUS:
        env->mcg_status = val;
        break;
    case MSR_MCG_CTL:
        if ((env->mcg_cap & MCG_CTL_P)
            && (val == 0 || val == ~(uint64_t)0))
            env->mcg_ctl = val;
        break;
    default:
        if ((uint32_t)ECX >= MSR_MC0_CTL
            && (uint32_t)ECX < MSR_MC0_CTL + (4 * env->mcg_cap & 0xff)) {
            uint32_t offset = (uint32_t)ECX - MSR_MC0_CTL;
            if ((offset & 0x3) != 0
                || (val == 0 || val == ~(uint64_t)0))
                env->mce_banks[offset] = val;
            break;
        }
        /* XXX: exception ? */
        break;
    }
}

void helper_rdmsr(CPUX86State *env)
{
    uint64_t val;

    helper_svm_check_intercept_param(env, SVM_EXIT_MSR, 0);

    switch((uint32_t)ECX) {
    case MSR_IA32_SYSENTER_CS:
        val = env->sysenter_cs;
        break;
    case MSR_IA32_SYSENTER_ESP:
        val = env->sysenter_esp;
        break;
    case MSR_IA32_SYSENTER_EIP:
        val = env->sysenter_eip;
        break;
    case MSR_IA32_APICBASE:
        val = cpu_get_apic_base(env);
        break;
    case MSR_EFER:
        val = env->efer;
        break;
    case MSR_STAR:
        val = env->star;
        break;
    case MSR_PAT:
        val = env->pat;
        break;
    case MSR_VM_HSAVE_PA:
        val = env->vm_hsave;
        break;
    case MSR_IA32_PERF_STATUS:
        /* tsc_increment_by_tick */
        val = 1000ULL;
        /* CPU multiplier */
        val |= (((uint64_t)4ULL) << 40);
        break;
#ifdef TARGET_X86_64
    case MSR_LSTAR:
        val = env->lstar;
        break;
    case MSR_CSTAR:
        val = env->cstar;
        break;
    case MSR_FMASK:
        val = env->fmask;
        break;
    case MSR_FSBASE:
        val = env->segs[R_FS].base;
        break;
    case MSR_GSBASE:
        val = env->segs[R_GS].base;
        break;
    case MSR_KERNELGSBASE:
        val = env->kernelgsbase;
        break;
#endif
    case MSR_MTRRphysBase(0):
    case MSR_MTRRphysBase(1):
    case MSR_MTRRphysBase(2):
    case MSR_MTRRphysBase(3):
    case MSR_MTRRphysBase(4):
    case MSR_MTRRphysBase(5):
    case MSR_MTRRphysBase(6):
    case MSR_MTRRphysBase(7):
        val = env->mtrr_var[((uint32_t)ECX - MSR_MTRRphysBase(0)) / 2].base;
        break;
    case MSR_MTRRphysMask(0):
    case MSR_MTRRphysMask(1):
    case MSR_MTRRphysMask(2):
    case MSR_MTRRphysMask(3):
    case MSR_MTRRphysMask(4):
    case MSR_MTRRphysMask(5):
    case MSR_MTRRphysMask(6):
    case MSR_MTRRphysMask(7):
        val = env->mtrr_var[((uint32_t)ECX - MSR_MTRRphysMask(0)) / 2].mask;
        break;
    case MSR_MTRRfix64K_00000:
        val = env->mtrr_fixed[0];
        break;
    case MSR_MTRRfix16K_80000:
    case MSR_MTRRfix16K_A0000:
        val = env->mtrr_fixed[(uint32_t)ECX - MSR_MTRRfix16K_80000 + 1];
        break;
    case MSR_MTRRfix4K_C0000:
    case MSR_MTRRfix4K_C8000:
    case MSR_MTRRfix4K_D0000:
    case MSR_MTRRfix4K_D8000:
    case MSR_MTRRfix4K_E0000:
    case MSR_MTRRfix4K_E8000:
    case MSR_MTRRfix4K_F0000:
    case MSR_MTRRfix4K_F8000:
        val = env->mtrr_fixed[(uint32_t)ECX - MSR_MTRRfix4K_C0000 + 3];
        break;
    case MSR_MTRRdefType:
        val = env->mtrr_deftype;
        break;
    case MSR_MTRRcap:
        if (env->cpuid_features & CPUID_MTRR)
            val = MSR_MTRRcap_VCNT | MSR_MTRRcap_FIXRANGE_SUPPORT | MSR_MTRRcap_WC_SUPPORTED;
        else
            /* XXX: exception ? */
            val = 0;
        break;
    case MSR_MCG_CAP:
        val = env->mcg_cap;
        break;
    case MSR_MCG_CTL:
        if (env->mcg_cap & MCG_CTL_P)
            val = env->mcg_ctl;
        else
            val = 0;
        break;
    case MSR_MCG_STATUS:
        val = env->mcg_status;
        break;
    default:
        if ((uint32_t)ECX >= MSR_MC0_CTL
            && (uint32_t)ECX < MSR_MC0_CTL + (4 * env->mcg_cap & 0xff)) {
            uint32_t offset = (uint32_t)ECX - MSR_MC0_CTL;
            val = env->mce_banks[offset];
            break;
        }
        /* XXX: exception ? */
        val = 0;
        break;
    }
    EAX = (uint32_t)(val);
    EDX = (uint32_t)(val >> 32);
}
#endif

static void do_hlt(CPUX86State *env)
{
    env->hflags &= ~HF_INHIBIT_IRQ_MASK; /* needed if sti is just before */
    ENV_GET_CPU(env)->halted = 1;
    env->exception_index = EXCP_HLT;
    cpu_loop_exit(env);
}

void helper_hlt(CPUX86State *env, int next_eip_addend)
{
    helper_svm_check_intercept_param(env, SVM_EXIT_HLT, 0);
    EIP += next_eip_addend;

    do_hlt(env);
}

void helper_monitor(CPUX86State *env, target_ulong ptr)
{
    if ((uint32_t)ECX != 0)
        raise_exception(env, EXCP0D_GPF);
    /* XXX: store address ? */
    helper_svm_check_intercept_param(env, SVM_EXIT_MONITOR, 0);
}

void helper_mwait(CPUX86State *env, int next_eip_addend)
{
    if ((uint32_t)ECX != 0)
        raise_exception(env, EXCP0D_GPF);
    helper_svm_check_intercept_param(env, SVM_EXIT_MWAIT, 0);
    EIP += next_eip_addend;

    /* XXX: not complete but not completely erroneous */
    CPUState *cpu = ENV_GET_CPU(env);
    if (cpu->cpu_index != 0 || QTAILQ_NEXT(cpu, node) != NULL) {
        /* more than one CPU: do not sleep because another CPU may
           wake this one */
    } else {
        do_hlt(env);
    }
}

void helper_debug(CPUX86State *env)
{
    env->exception_index = EXCP_DEBUG;
    cpu_loop_exit(env);
}

/* Secure Virtual Machine helpers */

#if defined(CONFIG_USER_ONLY)

void helper_vmrun(CPUX86State *env, int aflag, int next_eip_addend)
{
}
void helper_vmmcall(CPUX86State *env)
{
}
void helper_vmload(CPUX86State *env, int aflag)
{
}
void helper_vmsave(CPUX86State *env, int aflag)
{
}
void helper_stgi(CPUX86State *env)
{
}
void helper_clgi(CPUX86State *env)
{
}
void helper_skinit(CPUX86State *env)
{
}
void helper_invlpga(CPUX86State *env, int aflag)
{
}
void helper_vmexit(CPUX86State *env,
                   uint32_t exit_code, uint64_t exit_info_1)
{
}
void helper_svm_check_intercept_param(CPUX86State *env,
                                      uint32_t type, uint64_t param)
{
}

void svm_check_intercept(CPUX86State *env, uint32_t type)
{
}

void helper_svm_check_io(CPUX86State* env,
                         uint32_t port, uint32_t param,
                         uint32_t next_eip_addend)
{
}
#else

static inline void svm_save_seg(hwaddr addr,
                                const SegmentCache *sc)
{
    stw_phys(addr + offsetof(struct vmcb_seg, selector),
             sc->selector);
    stq_phys(addr + offsetof(struct vmcb_seg, base),
             sc->base);
    stl_phys(addr + offsetof(struct vmcb_seg, limit),
             sc->limit);
    stw_phys(addr + offsetof(struct vmcb_seg, attrib),
             ((sc->flags >> 8) & 0xff) | ((sc->flags >> 12) & 0x0f00));
}

static inline void svm_load_seg(hwaddr addr, SegmentCache *sc)
{
    unsigned int flags;

    sc->selector = lduw_phys(addr + offsetof(struct vmcb_seg, selector));
    sc->base = ldq_phys(addr + offsetof(struct vmcb_seg, base));
    sc->limit = ldl_phys(addr + offsetof(struct vmcb_seg, limit));
    flags = lduw_phys(addr + offsetof(struct vmcb_seg, attrib));
    sc->flags = ((flags & 0xff) << 8) | ((flags & 0x0f00) << 12);
}

static inline void svm_load_seg_cache(hwaddr addr,
                                      CPUX86State *env, int seg_reg)
{
    SegmentCache sc1, *sc = &sc1;
    svm_load_seg(addr, sc);
    cpu_x86_load_seg_cache(env, seg_reg, sc->selector,
                           sc->base, sc->limit, sc->flags);
}

void helper_vmrun(CPUX86State *env, int aflag, int next_eip_addend)
{
    target_ulong addr;
    uint32_t event_inj;
    uint32_t int_ctl;

    helper_svm_check_intercept_param(env, SVM_EXIT_VMRUN, 0);

    if (aflag == 2)
        addr = EAX;
    else
        addr = (uint32_t)EAX;

    qemu_log_mask(CPU_LOG_TB_IN_ASM, "vmrun! " TARGET_FMT_lx "\n", addr);

    env->vm_vmcb = addr;

    /* save the current CPU state in the hsave page */
    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.gdtr.base), env->gdt.base);
    stl_phys(env->vm_hsave + offsetof(struct vmcb, save.gdtr.limit), env->gdt.limit);

    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.idtr.base), env->idt.base);
    stl_phys(env->vm_hsave + offsetof(struct vmcb, save.idtr.limit), env->idt.limit);

    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.cr0), env->cr[0]);
    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.cr2), env->cr[2]);
    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.cr3), env->cr[3]);
    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.cr4), env->cr[4]);
    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.dr6), env->dr[6]);
    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.dr7), env->dr[7]);

    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.efer), env->efer);
    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.rflags), cpu_compute_eflags(env));

    svm_save_seg(env->vm_hsave + offsetof(struct vmcb, save.es),
                  &env->segs[R_ES]);
    svm_save_seg(env->vm_hsave + offsetof(struct vmcb, save.cs),
                 &env->segs[R_CS]);
    svm_save_seg(env->vm_hsave + offsetof(struct vmcb, save.ss),
                 &env->segs[R_SS]);
    svm_save_seg(env->vm_hsave + offsetof(struct vmcb, save.ds),
                 &env->segs[R_DS]);

    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.rip),
             EIP + next_eip_addend);
    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.rsp), ESP);
    stq_phys(env->vm_hsave + offsetof(struct vmcb, save.rax), EAX);

    /* load the interception bitmaps so we do not need to access the
       vmcb in svm mode */
    env->intercept            = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, control.intercept));
    env->intercept_cr_read    = lduw_phys(env->vm_vmcb + offsetof(struct vmcb, control.intercept_cr_read));
    env->intercept_cr_write   = lduw_phys(env->vm_vmcb + offsetof(struct vmcb, control.intercept_cr_write));
    env->intercept_dr_read    = lduw_phys(env->vm_vmcb + offsetof(struct vmcb, control.intercept_dr_read));
    env->intercept_dr_write   = lduw_phys(env->vm_vmcb + offsetof(struct vmcb, control.intercept_dr_write));
    env->intercept_exceptions = ldl_phys(env->vm_vmcb + offsetof(struct vmcb, control.intercept_exceptions));

    /* enable intercepts */
    env->hflags |= HF_SVMI_MASK;

    env->tsc_offset = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, control.tsc_offset));

    env->gdt.base  = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.gdtr.base));
    env->gdt.limit = ldl_phys(env->vm_vmcb + offsetof(struct vmcb, save.gdtr.limit));

    env->idt.base  = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.idtr.base));
    env->idt.limit = ldl_phys(env->vm_vmcb + offsetof(struct vmcb, save.idtr.limit));

    /* clear exit_info_2 so we behave like the real hardware */
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, control.exit_info_2), 0);

    cpu_x86_update_cr0(env, ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.cr0)));
    cpu_x86_update_cr4(env, ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.cr4)));
    cpu_x86_update_cr3(env, ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.cr3)));
    env->cr[2] = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.cr2));
    int_ctl = ldl_phys(env->vm_vmcb + offsetof(struct vmcb, control.int_ctl));
    env->hflags2 &= ~(HF2_HIF_MASK | HF2_VINTR_MASK);
    if (int_ctl & V_INTR_MASKING_MASK) {
        env->v_tpr = int_ctl & V_TPR_MASK;
        env->hflags2 |= HF2_VINTR_MASK;
        if (env->eflags & IF_MASK)
            env->hflags2 |= HF2_HIF_MASK;
    }

    cpu_load_efer(env,
                  ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.efer)));
    env->eflags = 0;
    cpu_load_eflags(env, ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.rflags)),
                ~(CC_O | CC_S | CC_Z | CC_A | CC_P | CC_C | DF_MASK));
    CC_OP = CC_OP_EFLAGS;

    svm_load_seg_cache(env->vm_vmcb + offsetof(struct vmcb, save.es),
                       env, R_ES);
    svm_load_seg_cache(env->vm_vmcb + offsetof(struct vmcb, save.cs),
                       env, R_CS);
    svm_load_seg_cache(env->vm_vmcb + offsetof(struct vmcb, save.ss),
                       env, R_SS);
    svm_load_seg_cache(env->vm_vmcb + offsetof(struct vmcb, save.ds),
                       env, R_DS);

    EIP = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.rip));
    env->eip = EIP;
    ESP = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.rsp));
    EAX = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.rax));
    env->dr[7] = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.dr7));
    env->dr[6] = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, save.dr6));
    cpu_x86_set_cpl(env, ldub_phys(env->vm_vmcb + offsetof(struct vmcb, save.cpl)));

    /* FIXME: guest state consistency checks */

    switch(ldub_phys(env->vm_vmcb + offsetof(struct vmcb, control.tlb_ctl))) {
        case TLB_CONTROL_DO_NOTHING:
            break;
        case TLB_CONTROL_FLUSH_ALL_ASID:
            /* FIXME: this is not 100% correct but should work for now */
            tlb_flush(env, 1);
        break;
    }

    env->hflags2 |= HF2_GIF_MASK;

    if (int_ctl & V_IRQ_MASK) {
        ENV_GET_CPU(env)->interrupt_request |= CPU_INTERRUPT_VIRQ;
    }

    /* maybe we need to inject an event */
    event_inj = ldl_phys(env->vm_vmcb + offsetof(struct vmcb, control.event_inj));
    if (event_inj & SVM_EVTINJ_VALID) {
        uint8_t vector = event_inj & SVM_EVTINJ_VEC_MASK;
        uint16_t valid_err = event_inj & SVM_EVTINJ_VALID_ERR;
        uint32_t event_inj_err = ldl_phys(env->vm_vmcb + offsetof(struct vmcb, control.event_inj_err));

        qemu_log_mask(CPU_LOG_TB_IN_ASM, "Injecting(%#hx): ", valid_err);
        /* FIXME: need to implement valid_err */
        switch (event_inj & SVM_EVTINJ_TYPE_MASK) {
        case SVM_EVTINJ_TYPE_INTR:
                env->exception_index = vector;
                env->error_code = event_inj_err;
                env->exception_is_int = 0;
                env->exception_next_eip = -1;
                qemu_log_mask(CPU_LOG_TB_IN_ASM, "INTR");
                /* XXX: is it always correct ? */
                do_interrupt_x86_hardirq(env, vector, 1);
                break;
        case SVM_EVTINJ_TYPE_NMI:
                env->exception_index = EXCP02_NMI;
                env->error_code = event_inj_err;
                env->exception_is_int = 0;
                env->exception_next_eip = EIP;
                qemu_log_mask(CPU_LOG_TB_IN_ASM, "NMI");
                cpu_loop_exit(env);
                break;
        case SVM_EVTINJ_TYPE_EXEPT:
                env->exception_index = vector;
                env->error_code = event_inj_err;
                env->exception_is_int = 0;
                env->exception_next_eip = -1;
                qemu_log_mask(CPU_LOG_TB_IN_ASM, "EXEPT");
                cpu_loop_exit(env);
                break;
        case SVM_EVTINJ_TYPE_SOFT:
                env->exception_index = vector;
                env->error_code = event_inj_err;
                env->exception_is_int = 1;
                env->exception_next_eip = EIP;
                qemu_log_mask(CPU_LOG_TB_IN_ASM, "SOFT");
                cpu_loop_exit(env);
                break;
        }
        qemu_log_mask(CPU_LOG_TB_IN_ASM, " %#x %#x\n", env->exception_index, env->error_code);
    }
}

void helper_vmmcall(CPUX86State *env)
{
    helper_svm_check_intercept_param(env, SVM_EXIT_VMMCALL, 0);
    raise_exception(env, EXCP06_ILLOP);
}

void helper_vmload(CPUX86State *env, int aflag)
{
    target_ulong addr;
    helper_svm_check_intercept_param(env, SVM_EXIT_VMLOAD, 0);

    if (aflag == 2)
        addr = EAX;
    else
        addr = (uint32_t)EAX;

    qemu_log_mask(CPU_LOG_TB_IN_ASM, "vmload! " TARGET_FMT_lx "\nFS: %016" PRIx64 " | " TARGET_FMT_lx "\n",
                addr, ldq_phys(addr + offsetof(struct vmcb, save.fs.base)),
                env->segs[R_FS].base);

    svm_load_seg_cache(addr + offsetof(struct vmcb, save.fs),
                       env, R_FS);
    svm_load_seg_cache(addr + offsetof(struct vmcb, save.gs),
                       env, R_GS);
    svm_load_seg(addr + offsetof(struct vmcb, save.tr),
                 &env->tr);
    svm_load_seg(addr + offsetof(struct vmcb, save.ldtr),
                 &env->ldt);

#ifdef TARGET_X86_64
    env->kernelgsbase = ldq_phys(addr + offsetof(struct vmcb, save.kernel_gs_base));
    env->lstar = ldq_phys(addr + offsetof(struct vmcb, save.lstar));
    env->cstar = ldq_phys(addr + offsetof(struct vmcb, save.cstar));
    env->fmask = ldq_phys(addr + offsetof(struct vmcb, save.sfmask));
#endif
    env->star = ldq_phys(addr + offsetof(struct vmcb, save.star));
    env->sysenter_cs = ldq_phys(addr + offsetof(struct vmcb, save.sysenter_cs));
    env->sysenter_esp = ldq_phys(addr + offsetof(struct vmcb, save.sysenter_esp));
    env->sysenter_eip = ldq_phys(addr + offsetof(struct vmcb, save.sysenter_eip));
}

void helper_vmsave(CPUX86State *env, int aflag)
{
    target_ulong addr;
    helper_svm_check_intercept_param(env, SVM_EXIT_VMSAVE, 0);

    if (aflag == 2)
        addr = EAX;
    else
        addr = (uint32_t)EAX;

    qemu_log_mask(CPU_LOG_TB_IN_ASM, "vmsave! " TARGET_FMT_lx "\nFS: %016" PRIx64 " | " TARGET_FMT_lx "\n",
                addr, ldq_phys(addr + offsetof(struct vmcb, save.fs.base)),
                env->segs[R_FS].base);

    svm_save_seg(addr + offsetof(struct vmcb, save.fs),
                 &env->segs[R_FS]);
    svm_save_seg(addr + offsetof(struct vmcb, save.gs),
                 &env->segs[R_GS]);
    svm_save_seg(addr + offsetof(struct vmcb, save.tr),
                 &env->tr);
    svm_save_seg(addr + offsetof(struct vmcb, save.ldtr),
                 &env->ldt);

#ifdef TARGET_X86_64
    stq_phys(addr + offsetof(struct vmcb, save.kernel_gs_base), env->kernelgsbase);
    stq_phys(addr + offsetof(struct vmcb, save.lstar), env->lstar);
    stq_phys(addr + offsetof(struct vmcb, save.cstar), env->cstar);
    stq_phys(addr + offsetof(struct vmcb, save.sfmask), env->fmask);
#endif
    stq_phys(addr + offsetof(struct vmcb, save.star), env->star);
    stq_phys(addr + offsetof(struct vmcb, save.sysenter_cs), env->sysenter_cs);
    stq_phys(addr + offsetof(struct vmcb, save.sysenter_esp), env->sysenter_esp);
    stq_phys(addr + offsetof(struct vmcb, save.sysenter_eip), env->sysenter_eip);
}

void helper_stgi(CPUX86State *env)
{
    helper_svm_check_intercept_param(env, SVM_EXIT_STGI, 0);
    env->hflags2 |= HF2_GIF_MASK;
}

void helper_clgi(CPUX86State *env)
{
    helper_svm_check_intercept_param(env, SVM_EXIT_CLGI, 0);
    env->hflags2 &= ~HF2_GIF_MASK;
}

void helper_skinit(CPUX86State *env)
{
    helper_svm_check_intercept_param(env, SVM_EXIT_SKINIT, 0);
    /* XXX: not implemented */
    raise_exception(env, EXCP06_ILLOP);
}

void helper_invlpga(CPUX86State *env, int aflag)
{
    target_ulong addr;
    helper_svm_check_intercept_param(env, SVM_EXIT_INVLPGA, 0);

    if (aflag == 2)
        addr = EAX;
    else
        addr = (uint32_t)EAX;

    /* XXX: could use the ASID to see if it is needed to do the
       flush */
    tlb_flush_page(env, addr);
}

void helper_svm_check_intercept_param(CPUX86State *env,
                                      uint32_t type, uint64_t param)
{
    if (likely(!(env->hflags & HF_SVMI_MASK)))
        return;
    switch(type) {
    case SVM_EXIT_READ_CR0 ... SVM_EXIT_READ_CR0 + 8:
        if (env->intercept_cr_read & (1 << (type - SVM_EXIT_READ_CR0))) {
            helper_vmexit(env, type, param);
        }
        break;
    case SVM_EXIT_WRITE_CR0 ... SVM_EXIT_WRITE_CR0 + 8:
        if (env->intercept_cr_write & (1 << (type - SVM_EXIT_WRITE_CR0))) {
            helper_vmexit(env, type, param);
        }
        break;
    case SVM_EXIT_READ_DR0 ... SVM_EXIT_READ_DR0 + 7:
        if (env->intercept_dr_read & (1 << (type - SVM_EXIT_READ_DR0))) {
            helper_vmexit(env, type, param);
        }
        break;
    case SVM_EXIT_WRITE_DR0 ... SVM_EXIT_WRITE_DR0 + 7:
        if (env->intercept_dr_write & (1 << (type - SVM_EXIT_WRITE_DR0))) {
            helper_vmexit(env, type, param);
        }
        break;
    case SVM_EXIT_EXCP_BASE ... SVM_EXIT_EXCP_BASE + 31:
        if (env->intercept_exceptions & (1 << (type - SVM_EXIT_EXCP_BASE))) {
            helper_vmexit(env, type, param);
        }
        break;
    case SVM_EXIT_MSR:
        if (env->intercept & (1ULL << (SVM_EXIT_MSR - SVM_EXIT_INTR))) {
            /* FIXME: this should be read in at vmrun (faster this way?) */
            uint64_t addr = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, control.msrpm_base_pa));
            uint32_t t0, t1;
            switch((uint32_t)ECX) {
            case 0 ... 0x1fff:
                t0 = (ECX * 2) % 8;
                t1 = ECX / 8;
                break;
            case 0xc0000000 ... 0xc0001fff:
                t0 = (8192 + ECX - 0xc0000000) * 2;
                t1 = (t0 / 8);
                t0 %= 8;
                break;
            case 0xc0010000 ... 0xc0011fff:
                t0 = (16384 + ECX - 0xc0010000) * 2;
                t1 = (t0 / 8);
                t0 %= 8;
                break;
            default:
                helper_vmexit(env, type, param);
                t0 = 0;
                t1 = 0;
                break;
            }
            if (ldub_phys(addr + t1) & ((1 << param) << t0))
                helper_vmexit(env, type, param);
        }
        break;
    default:
        if (env->intercept & (1ULL << (type - SVM_EXIT_INTR))) {
            helper_vmexit(env, type, param);
        }
        break;
    }
}

void svm_check_intercept(CPUArchState *env, uint32_t type)
{
    helper_svm_check_intercept_param(env, type, 0);
}

void helper_svm_check_io(CPUX86State *env,
                         uint32_t port, uint32_t param,
                         uint32_t next_eip_addend)
{
    if (env->intercept & (1ULL << (SVM_EXIT_IOIO - SVM_EXIT_INTR))) {
        /* FIXME: this should be read in at vmrun (faster this way?) */
        uint64_t addr = ldq_phys(env->vm_vmcb + offsetof(struct vmcb, control.iopm_base_pa));
        uint16_t mask = (1 << ((param >> 4) & 7)) - 1;
        if(lduw_phys(addr + port / 8) & (mask << (port & 7))) {
            /* next EIP */
            stq_phys(env->vm_vmcb + offsetof(struct vmcb, control.exit_info_2),
                     env->eip + next_eip_addend);
            helper_vmexit(env, SVM_EXIT_IOIO, param | (port << 16));
        }
    }
}

/* Note: currently only 32 bits of exit_code are used */
void helper_vmexit(CPUX86State *env,
                   uint32_t exit_code, uint64_t exit_info_1)
{
    uint32_t int_ctl;

    qemu_log_mask(CPU_LOG_TB_IN_ASM, "vmexit(%08x, %016" PRIx64 ", %016" PRIx64 ", " TARGET_FMT_lx ")!\n",
                exit_code, exit_info_1,
                ldq_phys(env->vm_vmcb + offsetof(struct vmcb, control.exit_info_2)),
                EIP);

    if(env->hflags & HF_INHIBIT_IRQ_MASK) {
        stl_phys(env->vm_vmcb + offsetof(struct vmcb, control.int_state), SVM_INTERRUPT_SHADOW_MASK);
        env->hflags &= ~HF_INHIBIT_IRQ_MASK;
    } else {
        stl_phys(env->vm_vmcb + offsetof(struct vmcb, control.int_state), 0);
    }

    /* Save the VM state in the vmcb */
    svm_save_seg(env->vm_vmcb + offsetof(struct vmcb, save.es),
                 &env->segs[R_ES]);
    svm_save_seg(env->vm_vmcb + offsetof(struct vmcb, save.cs),
                 &env->segs[R_CS]);
    svm_save_seg(env->vm_vmcb + offsetof(struct vmcb, save.ss),
                 &env->segs[R_SS]);
    svm_save_seg(env->vm_vmcb + offsetof(struct vmcb, save.ds),
                 &env->segs[R_DS]);

    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.gdtr.base), env->gdt.base);
    stl_phys(env->vm_vmcb + offsetof(struct vmcb, save.gdtr.limit), env->gdt.limit);

    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.idtr.base), env->idt.base);
    stl_phys(env->vm_vmcb + offsetof(struct vmcb, save.idtr.limit), env->idt.limit);

    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.efer), env->efer);
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.cr0), env->cr[0]);
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.cr2), env->cr[2]);
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.cr3), env->cr[3]);
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.cr4), env->cr[4]);

    int_ctl = ldl_phys(env->vm_vmcb + offsetof(struct vmcb, control.int_ctl));
    int_ctl &= ~(V_TPR_MASK | V_IRQ_MASK);
    int_ctl |= env->v_tpr & V_TPR_MASK;
    if (ENV_GET_CPU(env)->interrupt_request & CPU_INTERRUPT_VIRQ)
        int_ctl |= V_IRQ_MASK;
    stl_phys(env->vm_vmcb + offsetof(struct vmcb, control.int_ctl), int_ctl);

    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.rflags), cpu_compute_eflags(env));
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.rip), env->eip);
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.rsp), ESP);
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.rax), EAX);
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.dr7), env->dr[7]);
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, save.dr6), env->dr[6]);
    stb_phys(env->vm_vmcb + offsetof(struct vmcb, save.cpl), env->hflags & HF_CPL_MASK);

    /* Reload the host state from vm_hsave */
    env->hflags2 &= ~(HF2_HIF_MASK | HF2_VINTR_MASK);
    env->hflags &= ~HF_SVMI_MASK;
    env->intercept = 0;
    env->intercept_exceptions = 0;
    ENV_GET_CPU(env)->interrupt_request &= ~CPU_INTERRUPT_VIRQ;
    env->tsc_offset = 0;

    env->gdt.base  = ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.gdtr.base));
    env->gdt.limit = ldl_phys(env->vm_hsave + offsetof(struct vmcb, save.gdtr.limit));

    env->idt.base  = ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.idtr.base));
    env->idt.limit = ldl_phys(env->vm_hsave + offsetof(struct vmcb, save.idtr.limit));

    cpu_x86_update_cr0(env, ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.cr0)) | CR0_PE_MASK);
    cpu_x86_update_cr4(env, ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.cr4)));
    cpu_x86_update_cr3(env, ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.cr3)));
    /* we need to set the efer after the crs so the hidden flags get
       set properly */
    cpu_load_efer(env,
                  ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.efer)));
    env->eflags = 0;
    cpu_load_eflags(env, ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.rflags)),
                ~(CC_O | CC_S | CC_Z | CC_A | CC_P | CC_C | DF_MASK));
    CC_OP = CC_OP_EFLAGS;

    svm_load_seg_cache(env->vm_hsave + offsetof(struct vmcb, save.es),
                       env, R_ES);
    svm_load_seg_cache(env->vm_hsave + offsetof(struct vmcb, save.cs),
                       env, R_CS);
    svm_load_seg_cache(env->vm_hsave + offsetof(struct vmcb, save.ss),
                       env, R_SS);
    svm_load_seg_cache(env->vm_hsave + offsetof(struct vmcb, save.ds),
                       env, R_DS);

    EIP = ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.rip));
    ESP = ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.rsp));
    EAX = ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.rax));

    env->dr[6] = ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.dr6));
    env->dr[7] = ldq_phys(env->vm_hsave + offsetof(struct vmcb, save.dr7));

    /* other setups */
    cpu_x86_set_cpl(env, 0);
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, control.exit_code), exit_code);
    stq_phys(env->vm_vmcb + offsetof(struct vmcb, control.exit_info_1), exit_info_1);

    stl_phys(env->vm_vmcb + offsetof(struct vmcb, control.exit_int_info),
             ldl_phys(env->vm_vmcb + offsetof(struct vmcb, control.event_inj)));
    stl_phys(env->vm_vmcb + offsetof(struct vmcb, control.exit_int_info_err),
             ldl_phys(env->vm_vmcb + offsetof(struct vmcb, control.event_inj_err)));

    env->hflags2 &= ~HF2_GIF_MASK;
    /* FIXME: Resets the current ASID register to zero (host ASID). */

    /* Clears the V_IRQ and V_INTR_MASKING bits inside the processor. */

    /* Clears the TSC_OFFSET inside the processor. */

    /* If the host is in PAE mode, the processor reloads the host's PDPEs
       from the page table indicated the host's CR3. If the PDPEs contain
       illegal state, the processor causes a shutdown. */

    /* Forces CR0.PE = 1, RFLAGS.VM = 0. */
    env->cr[0] |= CR0_PE_MASK;
    env->eflags &= ~VM_MASK;

    /* Disables all breakpoints in the host DR7 register. */

    /* Checks the reloaded host state for consistency. */

    /* If the host's rIP reloaded by #VMEXIT is outside the limit of the
       host's code segment or non-canonical (in the case of long mode), a
       #GP fault is delivered inside the host.) */

    /* remove any pending exception */
    env->exception_index = -1;
    env->error_code = 0;
    env->old_exception = -1;

    cpu_loop_exit(env);
}

#endif

