/*
 * QEMU GVM support
 *
 * Copyright (C) 2006-2008 Qumranet Technologies
 * Copyright IBM, Corp. 2008
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "qapi/error.h"

#include "qemu-common.h"
#include "cpu.h"
#include "gvm_i386.h"
#include "sysemu/sysemu.h"
#include "sysemu/gvm_int.h"

#include "exec/gdbstub.h"
#include "qemu/host-utils.h"
#include "qemu/config-file.h"
#include "qemu/error-report.h"
#include "hw/i386/pc.h"
#include "hw/i386/apic.h"
#include "hw/i386/apic_internal.h"
#include "hw/i386/apic-msidef.h"
#include "hw/i386/intel_iommu.h"
#include "hw/i386/x86-iommu.h"

#include "exec/ioport.h"
#include "standard-headers/asm-x86/hyperv.h"
#include "hw/pci/pci.h"
#include "hw/pci/msi.h"
#include "migration/migration.h"
#include "exec/memattrs.h"
#include "trace.h"

#ifdef DEBUG_GVM
#define DPRINTF(fmt, ...) \
    do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

#define MSR_GVM_WALL_CLOCK  0x11
#define MSR_GVM_SYSTEM_TIME 0x12

/* A 4096-byte buffer can hold the 8-byte gvm_msrs header, plus
 * 255 gvm_msr_entry structs */
#define MSR_BUF_SIZE 4096

#ifndef BUS_MCEERR_AR
#define BUS_MCEERR_AR 4
#endif
#ifndef BUS_MCEERR_AO
#define BUS_MCEERR_AO 5
#endif

const GVMCapabilityInfo gvm_arch_required_capabilities[] = {
    GVM_CAP_LAST_INFO
};

static bool has_msr_star;
static bool has_msr_hsave_pa;
static bool has_msr_tsc_aux;
static bool has_msr_tsc_adjust;
static bool has_msr_tsc_deadline;
static bool has_msr_feature_control;
#if 0
static bool has_msr_async_pf_en;
static bool has_msr_pv_eoi_en;
#endif
static bool has_msr_misc_enable;
static bool has_msr_smbase;
static bool has_msr_bndcfgs;
//static bool has_msr_gvm_steal_time;
static int lm_capable_kernel;
static bool has_msr_mtrr;
static bool has_msr_xss;

static bool has_msr_architectural_pmu;
static uint32_t num_architectural_pmu_counters;

static int has_xsave;
static int has_xcrs;

static struct gvm_cpuid2 *cpuid_cache;

bool gvm_has_smm(void)
{
    return gvm_check_extension(gvm_state, GVM_CAP_X86_SMM);
}

bool gvm_allows_irq0_override(void)
{
    return !gvm_irqchip_in_kernel() || gvm_has_gsi_routing();
}

static int gvm_get_tsc(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;
    struct {
        struct gvm_msrs info;
        struct gvm_msr_entry entries[1];
    } msr_data;
    int ret;

    if (env->tsc_valid) {
        return 0;
    }

    msr_data.info.nmsrs = 1;
    msr_data.entries[0].index = MSR_IA32_TSC;
    env->tsc_valid = !runstate_is_running();

    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_GET_MSRS,
            &msr_data, sizeof(msr_data),
            NULL, 0);
    if (ret < 0) {
        return ret;
    }

    assert(ret == 1);
    env->tsc = msr_data.entries[0].data;
    return 0;
}

static inline void do_gvm_synchronize_tsc(CPUState *cpu, run_on_cpu_data arg)
{
    gvm_get_tsc(cpu);
}

void gvm_synchronize_all_tsc(void)
{
    CPUState *cpu;

    if (gvm_enabled()) {
        CPU_FOREACH(cpu) {
            run_on_cpu(cpu, do_gvm_synchronize_tsc, RUN_ON_CPU_NULL);
        }
    }
}

static struct gvm_cpuid2 *try_get_cpuid(GVMState *s, int max)
{
    struct gvm_cpuid2 *cpuid;
    int r, size;

    size = sizeof(*cpuid) + max * sizeof(*cpuid->entries);
    cpuid = g_malloc0(size);
    cpuid->nent = max;
    r = gvm_ioctl(s, GVM_GET_SUPPORTED_CPUID,
            cpuid, size, cpuid, size);
    if (r == 0 && cpuid->nent >= max) {
        r = -E2BIG;
    }
    if (r < 0) {
        if (r == -E2BIG) {
            g_free(cpuid);
            return NULL;
        } else {
            fprintf(stderr, "GVM_GET_SUPPORTED_CPUID failed: %s\n",
                    strerror(-r));
            exit(1);
        }
    }
    return cpuid;
}

/* Run GVM_GET_SUPPORTED_CPUID ioctl(), allocating a buffer large enough
 * for all entries.
 */
static struct gvm_cpuid2 *get_supported_cpuid(GVMState *s)
{
    struct gvm_cpuid2 *cpuid;
    int max = 1;

    if (cpuid_cache != NULL) {
        return cpuid_cache;
    }
    while ((cpuid = try_get_cpuid(s, max)) == NULL) {
        max *= 2;
    }
    cpuid_cache = cpuid;
    return cpuid;
}

static const struct gvm_para_features {
    int cap;
    int feature;
} para_features[] = {
    { GVM_CAP_CLOCKSOURCE, GVM_FEATURE_CLOCKSOURCE },
    { GVM_CAP_NOP_IO_DELAY, GVM_FEATURE_NOP_IO_DELAY },
    { GVM_CAP_ASYNC_PF, GVM_FEATURE_ASYNC_PF },
};

static int get_para_features(GVMState *s)
{
    int i, features = 0;

    for (i = 0; i < ARRAY_SIZE(para_features); i++) {
        if (gvm_check_extension(s, para_features[i].cap)) {
            features |= (1 << para_features[i].feature);
        }
    }

    return features;
}


/* Returns the value for a specific register on the cpuid entry
 */
static uint32_t cpuid_entry_get_reg(struct gvm_cpuid_entry2 *entry, int reg)
{
    uint32_t ret = 0;
    switch (reg) {
    case R_EAX:
        ret = entry->eax;
        break;
    case R_EBX:
        ret = entry->ebx;
        break;
    case R_ECX:
        ret = entry->ecx;
        break;
    case R_EDX:
        ret = entry->edx;
        break;
    }
    return ret;
}

/* Find matching entry for function/index on gvm_cpuid2 struct
 */
static struct gvm_cpuid_entry2 *cpuid_find_entry(struct gvm_cpuid2 *cpuid,
                                                 uint32_t function,
                                                 uint32_t index)
{
    int i;
    for (i = 0; i < cpuid->nent; ++i) {
        if (cpuid->entries[i].function == function &&
            cpuid->entries[i].index == index) {
            return &cpuid->entries[i];
        }
    }
    /* not found: */
    return NULL;
}

uint32_t gvm_arch_get_supported_cpuid(GVMState *s, uint32_t function,
                                      uint32_t index, int reg)
{
    struct gvm_cpuid2 *cpuid;
    uint32_t ret = 0;
    uint32_t cpuid_1_edx;
    bool found = false;

    cpuid = get_supported_cpuid(s);

    struct gvm_cpuid_entry2 *entry = cpuid_find_entry(cpuid, function, index);
    if (entry) {
        found = true;
        ret = cpuid_entry_get_reg(entry, reg);
    }

    /* Fixups for the data returned by GVM, below */

    if (function == 1 && reg == R_EDX) {
        /* GVM before 2.6.30 misreports the following features */
        ret |= CPUID_MTRR | CPUID_PAT | CPUID_MCE | CPUID_MCA;
    } else if (function == 1 && reg == R_ECX) {
        /* We can set the hypervisor flag, even if GVM does not return it on
         * GET_SUPPORTED_CPUID
         */
        ret |= CPUID_EXT_HYPERVISOR;
        /* tsc-deadline flag is not returned by GET_SUPPORTED_CPUID, but it
         * can be enabled if the kernel has GVM_CAP_TSC_DEADLINE_TIMER,
         * and the irqchip is in the kernel.
         */
        if (gvm_irqchip_in_kernel() &&
                gvm_check_extension(s, GVM_CAP_TSC_DEADLINE_TIMER)) {
            ret |= CPUID_EXT_TSC_DEADLINE_TIMER;
        }

        /* x2apic is reported by GET_SUPPORTED_CPUID, but it can't be enabled
         * without the in-kernel irqchip
         */
        if (!gvm_irqchip_in_kernel()) {
            ret &= ~CPUID_EXT_X2APIC;
        }
    } else if (function == 6 && reg == R_EAX) {
        ret |= CPUID_6_EAX_ARAT; /* safe to allow because of emulated APIC */
    } else if (function == 0x80000001 && reg == R_EDX) {
        /* On Intel, gvm returns cpuid according to the Intel spec,
         * so add missing bits according to the AMD spec:
         */
        cpuid_1_edx = gvm_arch_get_supported_cpuid(s, 1, 0, R_EDX);
        ret |= cpuid_1_edx & CPUID_EXT2_AMD_ALIASES;
    } else if (function == GVM_CPUID_FEATURES && reg == R_EAX) {
        /* gvm_pv_unhalt is reported by GET_SUPPORTED_CPUID, but it can't
         * be enabled without the in-kernel irqchip
         */
        if (!gvm_irqchip_in_kernel()) {
            //XXX
            //ret &= ~(1U << GVM_FEATURE_PV_UNHALT);
            ret &= ~(1U << 7);
        }
    }

    /* fallback for older kernels */
    if ((function == GVM_CPUID_FEATURES) && !found) {
        ret = get_para_features(s);
    }

    return ret;
}

static void cpu_update_state(void *opaque, int running, RunState state)
{
    CPUX86State *env = opaque;

    if (running) {
        env->tsc_valid = false;
    }
}

unsigned long gvm_arch_vcpu_id(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    return cpu->apic_id;
}

#ifndef GVM_CPUID_SIGNATURE_NEXT
#define GVM_CPUID_SIGNATURE_NEXT                0x40000100
#endif

static int gvm_arch_set_tsc_khz(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;
    int r;

    if (!env->tsc_khz) {
        return 0;
    }

    r = gvm_check_extension(cs->gvm_state, GVM_CAP_TSC_CONTROL) ?
        gvm_vcpu_ioctl(cs, GVM_SET_TSC_KHZ,
                &env->tsc_khz, sizeof(env->tsc_khz),
                NULL, 0) :
        -ENOTSUP;
    if (r < 0) {
        /* When GVM_SET_TSC_KHZ fails, it's an error only if the current
         * TSC frequency doesn't match the one we want.
         */
        int cur_freq = 0;
        r = gvm_check_extension(cs->gvm_state, GVM_CAP_GET_TSC_KHZ) ?
                       gvm_vcpu_ioctl(cs, GVM_GET_TSC_KHZ,
                               NULL, 0, &cur_freq, sizeof(cur_freq)) :
                       -ENOTSUP;
        if (cur_freq <= 0 || cur_freq != env->tsc_khz) {
            error_report("warning: TSC frequency mismatch between "
                         "VM (%" PRId64 " kHz) and host (%d kHz), "
                         "and TSC scaling unavailable",
                         env->tsc_khz, cur_freq);
            return r;
        }
    }

    return 0;
}

static Error *invtsc_mig_blocker;

#define GVM_MAX_CPUID_ENTRIES  100

int gvm_arch_init_vcpu(CPUState *cs)
{
    struct {
        struct gvm_cpuid2 cpuid;
        struct gvm_cpuid_entry2 entries[GVM_MAX_CPUID_ENTRIES];
    } QEMU_PACKED cpuid_data;
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;
    uint32_t limit, i, j, cpuid_i;
    uint32_t unused;
    struct gvm_cpuid_entry2 *c;
    // Disable KVM PV Features: we may decide to totally remove
    // it later but now let's just keep the code.
#if 0
    uint32_t signature[3];
    int kvm_base = GVM_CPUID_SIGNATURE;
#endif
    int r;
    Error *local_err = NULL;

    memset(&cpuid_data, 0, sizeof(cpuid_data));

    cpuid_i = 0;

    // Disable KVM PV Features: we may decide to totally remove
    // it later but now let's just keep the code.
#if 0
    if (cpu->expose_kvm) {
        memcpy(signature, "KVMKVMKVM\0\0\0", 12);
        c = &cpuid_data.entries[cpuid_i++];
        c->function = KVM_CPUID_SIGNATURE | kvm_base;
        c->eax = KVM_CPUID_FEATURES | kvm_base;
        c->ebx = signature[0];
        c->ecx = signature[1];
        c->edx = signature[2];

        c = &cpuid_data.entries[cpuid_i++];
        c->function = KVM_CPUID_FEATURES | kvm_base;
        c->eax = env->features[FEAT_KVM];

        has_msr_async_pf_en = c->eax & (1 << GVM_FEATURE_ASYNC_PF);

        has_msr_pv_eoi_en = c->eax & (1 << GVM_FEATURE_PV_EOI);

        has_msr_gvm_steal_time = c->eax & (1 << GVM_FEATURE_STEAL_TIME);
    }
#endif

    cpu_x86_cpuid(env, 0, 0, &limit, &unused, &unused, &unused);

    for (i = 0; i <= limit; i++) {
        if (cpuid_i == GVM_MAX_CPUID_ENTRIES) {
            fprintf(stderr, "unsupported level value: 0x%x\n", limit);
            abort();
        }
        c = &cpuid_data.entries[cpuid_i++];

        switch (i) {
        case 2: {
            /* Keep reading function 2 till all the input is received */
            int times;

            c->function = i;
            c->flags = GVM_CPUID_FLAG_STATEFUL_FUNC |
                       GVM_CPUID_FLAG_STATE_READ_NEXT;
            cpu_x86_cpuid(env, i, 0, &c->eax, &c->ebx, &c->ecx, &c->edx);
            times = c->eax & 0xff;

            for (j = 1; j < times; ++j) {
                if (cpuid_i == GVM_MAX_CPUID_ENTRIES) {
                    fprintf(stderr, "cpuid_data is full, no space for "
                            "cpuid(eax:2):eax & 0xf = 0x%x\n", times);
                    abort();
                }
                c = &cpuid_data.entries[cpuid_i++];
                c->function = i;
                c->flags = GVM_CPUID_FLAG_STATEFUL_FUNC;
                cpu_x86_cpuid(env, i, 0, &c->eax, &c->ebx, &c->ecx, &c->edx);
            }
            break;
        }
        case 4:
        case 0xb:
        case 0xd:
            for (j = 0; ; j++) {
                if (i == 0xd && j == 64) {
                    break;
                }
                c->function = i;
                c->flags = GVM_CPUID_FLAG_SIGNIFCANT_INDEX;
                c->index = j;
                cpu_x86_cpuid(env, i, j, &c->eax, &c->ebx, &c->ecx, &c->edx);

                if (i == 4 && c->eax == 0) {
                    break;
                }
                if (i == 0xb && !(c->ecx & 0xff00)) {
                    break;
                }
                if (i == 0xd && c->eax == 0) {
                    continue;
                }
                if (cpuid_i == GVM_MAX_CPUID_ENTRIES) {
                    fprintf(stderr, "cpuid_data is full, no space for "
                            "cpuid(eax:0x%x,ecx:0x%x)\n", i, j);
                    abort();
                }
                c = &cpuid_data.entries[cpuid_i++];
            }
            break;
        default:
            c->function = i;
            c->flags = 0;
            cpu_x86_cpuid(env, i, 0, &c->eax, &c->ebx, &c->ecx, &c->edx);
            break;
        }
    }

    if (limit >= 0x0a) {
        uint32_t ver;

        cpu_x86_cpuid(env, 0x0a, 0, &ver, &unused, &unused, &unused);
        if ((ver & 0xff) > 0) {
            has_msr_architectural_pmu = true;
            num_architectural_pmu_counters = (ver & 0xff00) >> 8;

            /* Shouldn't be more than 32, since that's the number of bits
             * available in EBX to tell us _which_ counters are available.
             * Play it safe.
             */
            if (num_architectural_pmu_counters > MAX_GP_COUNTERS) {
                num_architectural_pmu_counters = MAX_GP_COUNTERS;
            }
        }
    }

    cpu_x86_cpuid(env, 0x80000000, 0, &limit, &unused, &unused, &unused);

    for (i = 0x80000000; i <= limit; i++) {
        if (cpuid_i == GVM_MAX_CPUID_ENTRIES) {
            fprintf(stderr, "unsupported xlevel value: 0x%x\n", limit);
            abort();
        }
        c = &cpuid_data.entries[cpuid_i++];

        c->function = i;
        c->flags = 0;
        cpu_x86_cpuid(env, i, 0, &c->eax, &c->ebx, &c->ecx, &c->edx);
    }

    /* Call Centaur's CPUID instructions they are supported. */
    if (env->cpuid_xlevel2 > 0) {
        cpu_x86_cpuid(env, 0xC0000000, 0, &limit, &unused, &unused, &unused);

        for (i = 0xC0000000; i <= limit; i++) {
            if (cpuid_i == GVM_MAX_CPUID_ENTRIES) {
                fprintf(stderr, "unsupported xlevel2 value: 0x%x\n", limit);
                abort();
            }
            c = &cpuid_data.entries[cpuid_i++];

            c->function = i;
            c->flags = 0;
            cpu_x86_cpuid(env, i, 0, &c->eax, &c->ebx, &c->ecx, &c->edx);
        }
    }

    cpuid_data.cpuid.nent = cpuid_i;

    qemu_add_vm_change_state_handler(cpu_update_state, env);

    c = cpuid_find_entry(&cpuid_data.cpuid, 1, 0);
    if (c) {
        has_msr_feature_control = !!(c->ecx & CPUID_EXT_VMX) ||
                                  !!(c->ecx & CPUID_EXT_SMX);
    }

    c = cpuid_find_entry(&cpuid_data.cpuid, 0x80000007, 0);
    if (c && (c->edx & 1<<8) && invtsc_mig_blocker == NULL) {
        /* for migration */
        error_setg(&invtsc_mig_blocker,
                   "State blocked by non-migratable CPU device"
                   " (invtsc flag)");
        r = migrate_add_blocker(invtsc_mig_blocker, &local_err);
        if (local_err) {
            error_report_err(local_err);
            error_free(invtsc_mig_blocker);
            return r;
        }
        /* for savevm */
        vmstate_x86_cpu.unmigratable = 1;
    }

    cpuid_data.cpuid.padding = 0;
    r = gvm_vcpu_ioctl(cs, GVM_SET_CPUID2,
            &cpuid_data, sizeof(cpuid_data),
            NULL, 0);
    if (r) {
        return r;
    }

    // Disable complex tsc frequency support which is only seen
    // (to me) in KVM and is used for live migration.
    // We may simply remove it later but let's keep it til we
    // make final decision.
#if 0
    r = gvm_arch_set_tsc_khz(cs);
    if (r < 0) {
        return r;
    }

    /* vcpu's TSC frequency is either specified by user, or following
     * the value used by GVM if the former is not present. In the
     * latter case, we query it from GVM and record in env->tsc_khz,
     * so that vcpu's TSC frequency can be migrated later via this field.
     */
    if (!env->tsc_khz) {
        r = gvm_check_extension(cs->gvm_state, GVM_CAP_GET_TSC_KHZ) ?
            gvm_vcpu_ioctl(cs, GVM_GET_TSC_KHZ,
                    NULL, 0, &env->tsc_khz, sizeof(env->tsc_khz)) :
            -ENOTSUP;
        if (!r) {
            env->tsc_khz = 0;
        }
    }
#endif

    if (has_xsave) {
        env->gvm_xsave_buf = qemu_memalign(4096, sizeof(struct gvm_xsave));
    }
    cpu->gvm_msr_buf = g_malloc0(MSR_BUF_SIZE);

    if (env->features[FEAT_1_EDX] & CPUID_MTRR) {
        has_msr_mtrr = true;
    }
    if (!(env->features[FEAT_8000_0001_EDX] & CPUID_EXT2_RDTSCP)) {
        has_msr_tsc_aux = false;
    }

    return 0;
}

void gvm_arch_reset_vcpu(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;

    env->exception_injected = -1;
    env->interrupt_injected = -1;
    env->xcr0 = 1;
    if (gvm_irqchip_in_kernel()) {
        env->mp_state = cpu_is_bsp(cpu) ? GVM_MP_STATE_RUNNABLE :
                                          GVM_MP_STATE_UNINITIALIZED;
    } else {
        env->mp_state = GVM_MP_STATE_RUNNABLE;
    }
}

void gvm_arch_do_init_vcpu(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;

    /* APs get directly into wait-for-SIPI state.  */
    if (env->mp_state == GVM_MP_STATE_UNINITIALIZED) {
        env->mp_state = GVM_MP_STATE_INIT_RECEIVED;
    }
}

static int gvm_get_supported_msrs(GVMState *s)
{
    static int gvm_supported_msrs;
    int ret = 0;
    unsigned long msr_list_size;

    /* first time */
    if (gvm_supported_msrs == 0) {
        struct gvm_msr_list msr_list, *gvm_msr_list;

        gvm_supported_msrs = -1;

        /* Obtain MSR list from GVM.  These are the MSRs that we must
         * save/restore */
        msr_list.nmsrs = 0;
        ret = gvm_ioctl(s, GVM_GET_MSR_INDEX_LIST,
                &msr_list, sizeof(msr_list),
                &msr_list, sizeof(msr_list));
        if (ret < 0 && ret != -E2BIG) {
            return ret;
        }

        msr_list_size = sizeof(msr_list) + msr_list.nmsrs *
                                              sizeof(msr_list.indices[0]);
        gvm_msr_list = g_malloc0(msr_list_size);

        gvm_msr_list->nmsrs = msr_list.nmsrs;
        ret = gvm_ioctl(s, GVM_GET_MSR_INDEX_LIST,
                gvm_msr_list, msr_list_size,
                gvm_msr_list, msr_list_size);
        if (ret >= 0) {
            int i;

            for (i = 0; i < gvm_msr_list->nmsrs; i++) {
                if (gvm_msr_list->indices[i] == MSR_STAR) {
                    has_msr_star = true;
                    continue;
                }
                if (gvm_msr_list->indices[i] == MSR_VM_HSAVE_PA) {
                    has_msr_hsave_pa = true;
                    continue;
                }
                if (gvm_msr_list->indices[i] == MSR_TSC_AUX) {
                    has_msr_tsc_aux = true;
                    continue;
                }
                if (gvm_msr_list->indices[i] == MSR_TSC_ADJUST) {
                    has_msr_tsc_adjust = true;
                    continue;
                }
                if (gvm_msr_list->indices[i] == MSR_IA32_TSCDEADLINE) {
                    has_msr_tsc_deadline = true;
                    continue;
                }
                if (gvm_msr_list->indices[i] == MSR_IA32_SMBASE) {
                    has_msr_smbase = true;
                    continue;
                }
                if (gvm_msr_list->indices[i] == MSR_IA32_MISC_ENABLE) {
                    has_msr_misc_enable = true;
                    continue;
                }
                if (gvm_msr_list->indices[i] == MSR_IA32_BNDCFGS) {
                    has_msr_bndcfgs = true;
                    continue;
                }
                if (gvm_msr_list->indices[i] == MSR_IA32_XSS) {
                    has_msr_xss = true;
                    continue;
                }
            }
        }

        g_free(gvm_msr_list);
    }

    return ret;
}

int gvm_arch_init(MachineState *ms, GVMState *s)
{
    uint64_t identity_base = 0xfffbc000;
    uint64_t tss_base;
    int ret;

#ifdef GVM_CAP_XSAVE
    has_xsave = gvm_check_extension(s, GVM_CAP_XSAVE);
#endif

#ifdef GVM_CAP_XCRS
    has_xcrs = gvm_check_extension(s, GVM_CAP_XCRS);
#endif

    ret = gvm_get_supported_msrs(s);
    if (ret < 0) {
        return ret;
    }

    /*
     * On older Intel CPUs, GVM uses vm86 mode to emulate 16-bit code directly.
     * In order to use vm86 mode, an EPT identity map and a TSS  are needed.
     * Since these must be part of guest physical memory, we need to allocate
     * them, both by setting their start addresses in the kernel and by
     * creating a corresponding e820 entry. We need 4 pages before the BIOS.
     */
    if (gvm_check_extension(s, GVM_CAP_SET_IDENTITY_MAP_ADDR)) {
        /* Allows up to 16M BIOSes. */
        identity_base = 0xfeffc000;

        ret = gvm_vm_ioctl(s, GVM_SET_IDENTITY_MAP_ADDR,
                &identity_base, sizeof(identity_base),
                NULL, 0);
        if (ret < 0) {
            return ret;
        }
    }

    /* Set TSS base one page after EPT identity map. */
    tss_base = identity_base + 0x1000;
    ret = gvm_vm_ioctl(s, GVM_SET_TSS_ADDR,
            &tss_base, sizeof(tss_base),
            NULL, 0);
    if (ret < 0) {
        return ret;
    }

    /* Tell fw_cfg to notify the BIOS to reserve the range. */
    ret = e820_add_entry(identity_base, 0x4000, E820_RESERVED);
    if (ret < 0) {
        fprintf(stderr, "e820_add_entry() table is full\n");
        return ret;
    }

    return 0;
}

static void set_v8086_seg(struct gvm_segment *lhs, const SegmentCache *rhs)
{
    lhs->selector = rhs->selector;
    lhs->base = rhs->base;
    lhs->limit = rhs->limit;
    lhs->type = 3;
    lhs->present = 1;
    lhs->dpl = 3;
    lhs->db = 0;
    lhs->s = 1;
    lhs->l = 0;
    lhs->g = 0;
    lhs->avl = 0;
    lhs->unusable = 0;
}

static void set_seg(struct gvm_segment *lhs, const SegmentCache *rhs)
{
    unsigned flags = rhs->flags;
    lhs->selector = rhs->selector;
    lhs->base = rhs->base;
    lhs->limit = rhs->limit;
    lhs->type = (flags >> DESC_TYPE_SHIFT) & 15;
    lhs->present = (flags & DESC_P_MASK) != 0;
    lhs->dpl = (flags >> DESC_DPL_SHIFT) & 3;
    lhs->db = (flags >> DESC_B_SHIFT) & 1;
    lhs->s = (flags & DESC_S_MASK) != 0;
    lhs->l = (flags >> DESC_L_SHIFT) & 1;
    lhs->g = (flags & DESC_G_MASK) != 0;
    lhs->avl = (flags & DESC_AVL_MASK) != 0;
    lhs->unusable = !lhs->present;
    lhs->padding = 0;
}

static void get_seg(SegmentCache *lhs, const struct gvm_segment *rhs)
{
    lhs->selector = rhs->selector;
    lhs->base = rhs->base;
    lhs->limit = rhs->limit;
    if (rhs->unusable) {
        lhs->flags = 0;
    } else {
        lhs->flags = (rhs->type << DESC_TYPE_SHIFT) |
                     (rhs->present * DESC_P_MASK) |
                     (rhs->dpl << DESC_DPL_SHIFT) |
                     (rhs->db << DESC_B_SHIFT) |
                     (rhs->s * DESC_S_MASK) |
                     (rhs->l << DESC_L_SHIFT) |
                     (rhs->g * DESC_G_MASK) |
                     (rhs->avl * DESC_AVL_MASK);
    }
}

static void gvm_getput_reg(__u64 *gvm_reg, target_ulong *qemu_reg, int set)
{
    if (set) {
        *gvm_reg = *qemu_reg;
    } else {
        *qemu_reg = *gvm_reg;
    }
}

static int gvm_getput_regs(X86CPU *cpu, int set)
{
    CPUX86State *env = &cpu->env;
    struct gvm_regs regs;
    int ret = 0;

    if (!set) {
        ret = gvm_vcpu_ioctl(CPU(cpu), GVM_GET_REGS,
                NULL, 0, &regs, sizeof(regs));
        if (ret < 0) {
            return ret;
        }
    }

    gvm_getput_reg(&regs.rax, &env->regs[R_EAX], set);
    gvm_getput_reg(&regs.rbx, &env->regs[R_EBX], set);
    gvm_getput_reg(&regs.rcx, &env->regs[R_ECX], set);
    gvm_getput_reg(&regs.rdx, &env->regs[R_EDX], set);
    gvm_getput_reg(&regs.rsi, &env->regs[R_ESI], set);
    gvm_getput_reg(&regs.rdi, &env->regs[R_EDI], set);
    gvm_getput_reg(&regs.rsp, &env->regs[R_ESP], set);
    gvm_getput_reg(&regs.rbp, &env->regs[R_EBP], set);
#ifdef TARGET_X86_64
    gvm_getput_reg(&regs.r8, &env->regs[8], set);
    gvm_getput_reg(&regs.r9, &env->regs[9], set);
    gvm_getput_reg(&regs.r10, &env->regs[10], set);
    gvm_getput_reg(&regs.r11, &env->regs[11], set);
    gvm_getput_reg(&regs.r12, &env->regs[12], set);
    gvm_getput_reg(&regs.r13, &env->regs[13], set);
    gvm_getput_reg(&regs.r14, &env->regs[14], set);
    gvm_getput_reg(&regs.r15, &env->regs[15], set);
#endif

    gvm_getput_reg(&regs.rflags, &env->eflags, set);
    gvm_getput_reg(&regs.rip, &env->eip, set);

    if (set) {
        ret = gvm_vcpu_ioctl(CPU(cpu), GVM_SET_REGS,
                &regs, sizeof(regs), NULL, 0);
    }

    return ret;
}

static int gvm_put_fpu(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct gvm_fpu fpu;
    int i;

    memset(&fpu, 0, sizeof fpu);
    fpu.fsw = env->fpus & ~(7 << 11);
    fpu.fsw |= (env->fpstt & 7) << 11;
    fpu.fcw = env->fpuc;
    fpu.last_opcode = env->fpop;
    fpu.last_ip = env->fpip;
    fpu.last_dp = env->fpdp;
    for (i = 0; i < 8; ++i) {
        fpu.ftwx |= (!env->fptags[i]) << i;
    }
    memcpy(fpu.fpr, env->fpregs, sizeof env->fpregs);
    for (i = 0; i < CPU_NB_REGS; i++) {
        stq_p(&fpu.xmm[i][0], env->xmm_regs[i].ZMM_Q(0));
        stq_p(&fpu.xmm[i][8], env->xmm_regs[i].ZMM_Q(1));
    }
    fpu.mxcsr = env->mxcsr;

    return gvm_vcpu_ioctl(CPU(cpu), GVM_SET_FPU,
            &fpu, sizeof(fpu), NULL, 0);
}

#define XSAVE_FCW_FSW     0
#define XSAVE_FTW_FOP     1
#define XSAVE_CWD_RIP     2
#define XSAVE_CWD_RDP     4
#define XSAVE_MXCSR       6
#define XSAVE_ST_SPACE    8
#define XSAVE_XMM_SPACE   40
#define XSAVE_XSTATE_BV   128
#define XSAVE_YMMH_SPACE  144
#define XSAVE_BNDREGS     240
#define XSAVE_BNDCSR      256
#define XSAVE_OPMASK      272
#define XSAVE_ZMM_Hi256   288
#define XSAVE_Hi16_ZMM    416
#define XSAVE_PKRU        672

#define XSAVE_BYTE_OFFSET(word_offset) \
    ((word_offset) * sizeof(((struct gvm_xsave *)0)->region[0]))

#define ASSERT_OFFSET(word_offset, field) \
    QEMU_BUILD_BUG_ON(XSAVE_BYTE_OFFSET(word_offset) != \
                      offsetof(X86XSaveArea, field))

ASSERT_OFFSET(XSAVE_FCW_FSW, legacy.fcw);
ASSERT_OFFSET(XSAVE_FTW_FOP, legacy.ftw);
ASSERT_OFFSET(XSAVE_CWD_RIP, legacy.fpip);
ASSERT_OFFSET(XSAVE_CWD_RDP, legacy.fpdp);
ASSERT_OFFSET(XSAVE_MXCSR, legacy.mxcsr);
ASSERT_OFFSET(XSAVE_ST_SPACE, legacy.fpregs);
ASSERT_OFFSET(XSAVE_XMM_SPACE, legacy.xmm_regs);
ASSERT_OFFSET(XSAVE_XSTATE_BV, header.xstate_bv);
ASSERT_OFFSET(XSAVE_YMMH_SPACE, avx_state);
ASSERT_OFFSET(XSAVE_BNDREGS, bndreg_state);
ASSERT_OFFSET(XSAVE_BNDCSR, bndcsr_state);
ASSERT_OFFSET(XSAVE_OPMASK, opmask_state);
ASSERT_OFFSET(XSAVE_ZMM_Hi256, zmm_hi256_state);
ASSERT_OFFSET(XSAVE_Hi16_ZMM, hi16_zmm_state);
ASSERT_OFFSET(XSAVE_PKRU, pkru_state);

static int gvm_put_xsave(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    X86XSaveArea *xsave = env->gvm_xsave_buf;
    uint16_t cwd, swd, twd;
    int i;

    if (!has_xsave) {
        return gvm_put_fpu(cpu);
    }

    memset(xsave, 0, sizeof(struct gvm_xsave));
    twd = 0;
    swd = env->fpus & ~(7 << 11);
    swd |= (env->fpstt & 7) << 11;
    cwd = env->fpuc;
    for (i = 0; i < 8; ++i) {
        twd |= (!env->fptags[i]) << i;
    }
    xsave->legacy.fcw = cwd;
    xsave->legacy.fsw = swd;
    xsave->legacy.ftw = twd;
    xsave->legacy.fpop = env->fpop;
    xsave->legacy.fpip = env->fpip;
    xsave->legacy.fpdp = env->fpdp;
    memcpy(&xsave->legacy.fpregs, env->fpregs,
            sizeof env->fpregs);
    xsave->legacy.mxcsr = env->mxcsr;
    xsave->header.xstate_bv = env->xstate_bv;
    memcpy(&xsave->bndreg_state.bnd_regs, env->bnd_regs,
            sizeof env->bnd_regs);
    xsave->bndcsr_state.bndcsr = env->bndcs_regs;
    memcpy(&xsave->opmask_state.opmask_regs, env->opmask_regs,
            sizeof env->opmask_regs);

    for (i = 0; i < CPU_NB_REGS; i++) {
        uint8_t *xmm = xsave->legacy.xmm_regs[i];
        uint8_t *ymmh = xsave->avx_state.ymmh[i];
        uint8_t *zmmh = xsave->zmm_hi256_state.zmm_hi256[i];
        stq_p(xmm,     env->xmm_regs[i].ZMM_Q(0));
        stq_p(xmm+8,   env->xmm_regs[i].ZMM_Q(1));
        stq_p(ymmh,    env->xmm_regs[i].ZMM_Q(2));
        stq_p(ymmh+8,  env->xmm_regs[i].ZMM_Q(3));
        stq_p(zmmh,    env->xmm_regs[i].ZMM_Q(4));
        stq_p(zmmh+8,  env->xmm_regs[i].ZMM_Q(5));
        stq_p(zmmh+16, env->xmm_regs[i].ZMM_Q(6));
        stq_p(zmmh+24, env->xmm_regs[i].ZMM_Q(7));
    }

#ifdef TARGET_X86_64
    memcpy(&xsave->hi16_zmm_state.hi16_zmm, &env->xmm_regs[16],
            16 * sizeof env->xmm_regs[16]);
    memcpy(&xsave->pkru_state, &env->pkru, sizeof env->pkru);
#endif
    return gvm_vcpu_ioctl(CPU(cpu), GVM_SET_XSAVE,
            xsave, sizeof(*xsave), NULL, 0);
}

static int gvm_put_xcrs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct gvm_xcrs xcrs = {};

    if (!has_xcrs) {
        return 0;
    }

    xcrs.nr_xcrs = 1;
    xcrs.flags = 0;
    xcrs.xcrs[0].xcr = 0;
    xcrs.xcrs[0].value = env->xcr0;
    return gvm_vcpu_ioctl(CPU(cpu), GVM_SET_XCRS,
            &xcrs, sizeof(xcrs), NULL, 0);
}

static int gvm_put_sregs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct gvm_sregs sregs;

    memset(sregs.interrupt_bitmap, 0, sizeof(sregs.interrupt_bitmap));
    if (env->interrupt_injected >= 0) {
        sregs.interrupt_bitmap[env->interrupt_injected / 64] |=
                (uint64_t)1 << (env->interrupt_injected % 64);
    }

    if ((env->eflags & VM_MASK)) {
        set_v8086_seg(&sregs.cs, &env->segs[R_CS]);
        set_v8086_seg(&sregs.ds, &env->segs[R_DS]);
        set_v8086_seg(&sregs.es, &env->segs[R_ES]);
        set_v8086_seg(&sregs.fs, &env->segs[R_FS]);
        set_v8086_seg(&sregs.gs, &env->segs[R_GS]);
        set_v8086_seg(&sregs.ss, &env->segs[R_SS]);
    } else {
        set_seg(&sregs.cs, &env->segs[R_CS]);
        set_seg(&sregs.ds, &env->segs[R_DS]);
        set_seg(&sregs.es, &env->segs[R_ES]);
        set_seg(&sregs.fs, &env->segs[R_FS]);
        set_seg(&sregs.gs, &env->segs[R_GS]);
        set_seg(&sregs.ss, &env->segs[R_SS]);
    }

    set_seg(&sregs.tr, &env->tr);
    set_seg(&sregs.ldt, &env->ldt);

    sregs.idt.limit = env->idt.limit;
    sregs.idt.base = env->idt.base;
    memset(sregs.idt.padding, 0, sizeof sregs.idt.padding);
    sregs.gdt.limit = env->gdt.limit;
    sregs.gdt.base = env->gdt.base;
    memset(sregs.gdt.padding, 0, sizeof sregs.gdt.padding);

    sregs.cr0 = env->cr[0];
    sregs.cr2 = env->cr[2];
    sregs.cr3 = env->cr[3];
    sregs.cr4 = env->cr[4];

    sregs.cr8 = cpu_get_apic_tpr(cpu->apic_state);
    sregs.apic_base = cpu_get_apic_base(cpu->apic_state);

    sregs.efer = env->efer;

    return gvm_vcpu_ioctl(CPU(cpu), GVM_SET_SREGS,
            &sregs, sizeof(sregs), NULL, 0);
}

static void gvm_msr_buf_reset(X86CPU *cpu)
{
    memset(cpu->gvm_msr_buf, 0, MSR_BUF_SIZE);
}

static void gvm_msr_entry_add(X86CPU *cpu, uint32_t index, uint64_t value)
{
    struct gvm_msrs *msrs = cpu->gvm_msr_buf;
    void *limit = ((void *)msrs) + MSR_BUF_SIZE;
    struct gvm_msr_entry *entry = &msrs->entries[msrs->nmsrs];

    assert((void *)(entry + 1) <= limit);

    entry->index = index;
    entry->reserved = 0;
    entry->data = value;
    msrs->nmsrs++;
}

static int gvm_put_tscdeadline_msr(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    int ret;

    if (!has_msr_tsc_deadline) {
        return 0;
    }

    gvm_msr_buf_reset(cpu);
    gvm_msr_entry_add(cpu, MSR_IA32_TSCDEADLINE, env->tsc_deadline);

    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_SET_MSRS,
            cpu->gvm_msr_buf,
            sizeof(struct gvm_msrs) + sizeof(struct gvm_msr_entry),
            NULL, 0);
    if (ret < 0) {
        return ret;
    }

    assert(ret == 1);
    return 0;
}

/*
 * Provide a separate write service for the feature control MSR in order to
 * kick the VCPU out of VMXON or even guest mode on reset. This has to be done
 * before writing any other state because forcibly leaving nested mode
 * invalidates the VCPU state.
 */
static int gvm_put_msr_feature_control(X86CPU *cpu)
{
    int ret;

    if (!has_msr_feature_control) {
        return 0;
    }

    gvm_msr_buf_reset(cpu);
    gvm_msr_entry_add(cpu, MSR_IA32_FEATURE_CONTROL,
                      cpu->env.msr_ia32_feature_control);

    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_SET_MSRS,
            cpu->gvm_msr_buf,
            sizeof(struct gvm_msrs) + sizeof(struct gvm_msr_entry),
            NULL, 0);
    if (ret < 0) {
        return ret;
    }

    assert(ret == 1);
    return 0;
}

static int gvm_put_msrs(X86CPU *cpu, int level)
{
    CPUX86State *env = &cpu->env;
    int i;
    int ret;

    gvm_msr_buf_reset(cpu);

    gvm_msr_entry_add(cpu, MSR_IA32_SYSENTER_CS, env->sysenter_cs);
    gvm_msr_entry_add(cpu, MSR_IA32_SYSENTER_ESP, env->sysenter_esp);
    gvm_msr_entry_add(cpu, MSR_IA32_SYSENTER_EIP, env->sysenter_eip);
    gvm_msr_entry_add(cpu, MSR_PAT, env->pat);
    if (has_msr_star) {
        gvm_msr_entry_add(cpu, MSR_STAR, env->star);
    }
    if (has_msr_hsave_pa) {
        gvm_msr_entry_add(cpu, MSR_VM_HSAVE_PA, env->vm_hsave);
    }
    if (has_msr_tsc_aux) {
        gvm_msr_entry_add(cpu, MSR_TSC_AUX, env->tsc_aux);
    }
    if (has_msr_tsc_adjust) {
        gvm_msr_entry_add(cpu, MSR_TSC_ADJUST, env->tsc_adjust);
    }
    if (has_msr_misc_enable) {
        gvm_msr_entry_add(cpu, MSR_IA32_MISC_ENABLE,
                          env->msr_ia32_misc_enable);
    }
    if (has_msr_smbase) {
        gvm_msr_entry_add(cpu, MSR_IA32_SMBASE, env->smbase);
    }
    if (has_msr_bndcfgs) {
        gvm_msr_entry_add(cpu, MSR_IA32_BNDCFGS, env->msr_bndcfgs);
    }
    if (has_msr_xss) {
        gvm_msr_entry_add(cpu, MSR_IA32_XSS, env->xss);
    }
#ifdef TARGET_X86_64
    if (lm_capable_kernel) {
        gvm_msr_entry_add(cpu, MSR_CSTAR, env->cstar);
        gvm_msr_entry_add(cpu, MSR_KERNELGSBASE, env->kernelgsbase);
        gvm_msr_entry_add(cpu, MSR_FMASK, env->fmask);
        gvm_msr_entry_add(cpu, MSR_LSTAR, env->lstar);
    }
#endif
    /*
     * The following MSRs have side effects on the guest or are too heavy
     * for normal writeback. Limit them to reset or full state updates.
     */
    if (level >= GVM_PUT_RESET_STATE) {
        gvm_msr_entry_add(cpu, MSR_IA32_TSC, env->tsc);
    // Disable KVM PV Feature MSRs (software only MSRs). We may remove
    // it completely later. But let's keep it here before we make final
    // decision.
#if 0
        gvm_msr_entry_add(cpu, MSR_GVM_SYSTEM_TIME, env->system_time_msr);
        gvm_msr_entry_add(cpu, MSR_GVM_WALL_CLOCK, env->wall_clock_msr);
        if (has_msr_async_pf_en) {
            gvm_msr_entry_add(cpu, MSR_GVM_ASYNC_PF_EN, env->async_pf_en_msr);
        }
        if (has_msr_pv_eoi_en) {
            gvm_msr_entry_add(cpu, MSR_GVM_PV_EOI_EN, env->pv_eoi_en_msr);
        }
        if (has_msr_gvm_steal_time) {
            gvm_msr_entry_add(cpu, MSR_GVM_STEAL_TIME, env->steal_time_msr);
        }
#endif
        if (has_msr_architectural_pmu) {
            /* Stop the counter.  */
            gvm_msr_entry_add(cpu, MSR_CORE_PERF_FIXED_CTR_CTRL, 0);
            gvm_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_CTRL, 0);

            /* Set the counter values.  */
            for (i = 0; i < MAX_FIXED_COUNTERS; i++) {
                gvm_msr_entry_add(cpu, MSR_CORE_PERF_FIXED_CTR0 + i,
                                  env->msr_fixed_counters[i]);
            }
            for (i = 0; i < num_architectural_pmu_counters; i++) {
                gvm_msr_entry_add(cpu, MSR_P6_PERFCTR0 + i,
                                  env->msr_gp_counters[i]);
                gvm_msr_entry_add(cpu, MSR_P6_EVNTSEL0 + i,
                                  env->msr_gp_evtsel[i]);
            }
            gvm_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_STATUS,
                              env->msr_global_status);
            gvm_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_OVF_CTRL,
                              env->msr_global_ovf_ctrl);

            /* Now start the PMU.  */
            gvm_msr_entry_add(cpu, MSR_CORE_PERF_FIXED_CTR_CTRL,
                              env->msr_fixed_ctr_ctrl);
            gvm_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_CTRL,
                              env->msr_global_ctrl);
        }
        if (has_msr_mtrr) {
            uint64_t phys_mask = MAKE_64BIT_MASK(0, cpu->phys_bits);

            gvm_msr_entry_add(cpu, MSR_MTRRdefType, env->mtrr_deftype);
            gvm_msr_entry_add(cpu, MSR_MTRRfix64K_00000, env->mtrr_fixed[0]);
            gvm_msr_entry_add(cpu, MSR_MTRRfix16K_80000, env->mtrr_fixed[1]);
            gvm_msr_entry_add(cpu, MSR_MTRRfix16K_A0000, env->mtrr_fixed[2]);
            gvm_msr_entry_add(cpu, MSR_MTRRfix4K_C0000, env->mtrr_fixed[3]);
            gvm_msr_entry_add(cpu, MSR_MTRRfix4K_C8000, env->mtrr_fixed[4]);
            gvm_msr_entry_add(cpu, MSR_MTRRfix4K_D0000, env->mtrr_fixed[5]);
            gvm_msr_entry_add(cpu, MSR_MTRRfix4K_D8000, env->mtrr_fixed[6]);
            gvm_msr_entry_add(cpu, MSR_MTRRfix4K_E0000, env->mtrr_fixed[7]);
            gvm_msr_entry_add(cpu, MSR_MTRRfix4K_E8000, env->mtrr_fixed[8]);
            gvm_msr_entry_add(cpu, MSR_MTRRfix4K_F0000, env->mtrr_fixed[9]);
            gvm_msr_entry_add(cpu, MSR_MTRRfix4K_F8000, env->mtrr_fixed[10]);
            for (i = 0; i < MSR_MTRRcap_VCNT; i++) {
                /* The CPU GPs if we write to a bit above the physical limit of
                 * the host CPU (and GVM emulates that)
                 */
                uint64_t mask = env->mtrr_var[i].mask;
                mask &= phys_mask;

                gvm_msr_entry_add(cpu, MSR_MTRRphysBase(i),
                                  env->mtrr_var[i].base);
                gvm_msr_entry_add(cpu, MSR_MTRRphysMask(i), mask);
            }
        }

        /* Note: MSR_IA32_FEATURE_CONTROL is written separately, see
         *       gvm_put_msr_feature_control. */
    }

    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_SET_MSRS, cpu->gvm_msr_buf,
            sizeof(struct gvm_msrs) + cpu->gvm_msr_buf->nmsrs *
                sizeof(struct  gvm_msr_entry),
            NULL, 0);
    if (ret < 0) {
        return ret;
    }

    return 0;
}


static int gvm_get_fpu(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct gvm_fpu fpu;
    int i, ret;

    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_GET_FPU,
            NULL, 0,
            &fpu, sizeof(fpu));
    if (ret < 0) {
        return ret;
    }

    env->fpstt = (fpu.fsw >> 11) & 7;
    env->fpus = fpu.fsw;
    env->fpuc = fpu.fcw;
    env->fpop = fpu.last_opcode;
    env->fpip = fpu.last_ip;
    env->fpdp = fpu.last_dp;
    for (i = 0; i < 8; ++i) {
        env->fptags[i] = !((fpu.ftwx >> i) & 1);
    }
    memcpy(env->fpregs, fpu.fpr, sizeof env->fpregs);
    for (i = 0; i < CPU_NB_REGS; i++) {
        env->xmm_regs[i].ZMM_Q(0) = ldq_p(&fpu.xmm[i][0]);
        env->xmm_regs[i].ZMM_Q(1) = ldq_p(&fpu.xmm[i][8]);
    }
    env->mxcsr = fpu.mxcsr;

    return 0;
}

static int gvm_get_xsave(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    X86XSaveArea *xsave = env->gvm_xsave_buf;
    int ret, i;
    uint16_t cwd, swd, twd;

    if (!has_xsave) {
        return gvm_get_fpu(cpu);
    }

    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_GET_XSAVE,
            NULL, 0, xsave, sizeof(*xsave));
    if (ret < 0) {
        return ret;
    }

    cwd = xsave->legacy.fcw;
    swd = xsave->legacy.fsw;
    twd = xsave->legacy.ftw;
    env->fpop = xsave->legacy.fpop;
    env->fpstt = (swd >> 11) & 7;
    env->fpus = swd;
    env->fpuc = cwd;
    for (i = 0; i < 8; ++i) {
        env->fptags[i] = !((twd >> i) & 1);
    }
    env->fpip = xsave->legacy.fpip;
    env->fpdp = xsave->legacy.fpdp;
    env->mxcsr = xsave->legacy.mxcsr;
    memcpy(env->fpregs, &xsave->legacy.fpregs,
            sizeof env->fpregs);
    env->xstate_bv = xsave->header.xstate_bv;
    memcpy(env->bnd_regs, &xsave->bndreg_state.bnd_regs,
            sizeof env->bnd_regs);
    env->bndcs_regs = xsave->bndcsr_state.bndcsr;
    memcpy(env->opmask_regs, &xsave->opmask_state.opmask_regs,
            sizeof env->opmask_regs);

    for (i = 0; i < CPU_NB_REGS; i++) {
        uint8_t *xmm = xsave->legacy.xmm_regs[i];
        uint8_t *ymmh = xsave->avx_state.ymmh[i];
        uint8_t *zmmh = xsave->zmm_hi256_state.zmm_hi256[i];
        env->xmm_regs[i].ZMM_Q(0) = ldq_p(xmm);
        env->xmm_regs[i].ZMM_Q(1) = ldq_p(xmm+8);
        env->xmm_regs[i].ZMM_Q(2) = ldq_p(ymmh);
        env->xmm_regs[i].ZMM_Q(3) = ldq_p(ymmh+8);
        env->xmm_regs[i].ZMM_Q(4) = ldq_p(zmmh);
        env->xmm_regs[i].ZMM_Q(5) = ldq_p(zmmh+8);
        env->xmm_regs[i].ZMM_Q(6) = ldq_p(zmmh+16);
        env->xmm_regs[i].ZMM_Q(7) = ldq_p(zmmh+24);
    }

#ifdef TARGET_X86_64
    memcpy(&env->xmm_regs[16], &xsave->hi16_zmm_state.hi16_zmm,
           16 * sizeof env->xmm_regs[16]);
    memcpy(&env->pkru, &xsave->pkru_state, sizeof env->pkru);
#endif
    return 0;
}

static int gvm_get_xcrs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    int i, ret;
    struct gvm_xcrs xcrs;

    if (!has_xcrs) {
        return 0;
    }

    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_GET_XCRS,
            NULL, 0, &xcrs, sizeof(xcrs));
    if (ret < 0) {
        return ret;
    }

    for (i = 0; i < xcrs.nr_xcrs; i++) {
        /* Only support xcr0 now */
        if (xcrs.xcrs[i].xcr == 0) {
            env->xcr0 = xcrs.xcrs[i].value;
            break;
        }
    }
    return 0;
}

static int gvm_get_sregs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct gvm_sregs sregs;
    uint32_t hflags;
    int bit, i, ret;

    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_GET_SREGS,
            NULL, 0, &sregs, sizeof(sregs));
    if (ret < 0) {
        return ret;
    }

    /* There can only be one pending IRQ set in the bitmap at a time, so try
       to find it and save its number instead (-1 for none). */
    env->interrupt_injected = -1;
    for (i = 0; i < ARRAY_SIZE(sregs.interrupt_bitmap); i++) {
        if (sregs.interrupt_bitmap[i]) {
            bit = ctz64(sregs.interrupt_bitmap[i]);
            env->interrupt_injected = i * 64 + bit;
            break;
        }
    }

    get_seg(&env->segs[R_CS], &sregs.cs);
    get_seg(&env->segs[R_DS], &sregs.ds);
    get_seg(&env->segs[R_ES], &sregs.es);
    get_seg(&env->segs[R_FS], &sregs.fs);
    get_seg(&env->segs[R_GS], &sregs.gs);
    get_seg(&env->segs[R_SS], &sregs.ss);

    get_seg(&env->tr, &sregs.tr);
    get_seg(&env->ldt, &sregs.ldt);

    env->idt.limit = sregs.idt.limit;
    env->idt.base = sregs.idt.base;
    env->gdt.limit = sregs.gdt.limit;
    env->gdt.base = sregs.gdt.base;

    env->cr[0] = sregs.cr0;
    env->cr[2] = sregs.cr2;
    env->cr[3] = sregs.cr3;
    env->cr[4] = sregs.cr4;

    env->efer = sregs.efer;

    /* changes to apic base and cr8/tpr are read back via gvm_arch_post_run */

#define HFLAG_COPY_MASK \
    ~( HF_CPL_MASK | HF_PE_MASK | HF_MP_MASK | HF_EM_MASK | \
       HF_TS_MASK | HF_TF_MASK | HF_VM_MASK | HF_IOPL_MASK | \
       HF_OSFXSR_MASK | HF_LMA_MASK | HF_CS32_MASK | \
       HF_SS32_MASK | HF_CS64_MASK | HF_ADDSEG_MASK)

    hflags = env->hflags & HFLAG_COPY_MASK;
    hflags |= (env->segs[R_SS].flags >> DESC_DPL_SHIFT) & HF_CPL_MASK;
    hflags |= (env->cr[0] & CR0_PE_MASK) << (HF_PE_SHIFT - CR0_PE_SHIFT);
    hflags |= (env->cr[0] << (HF_MP_SHIFT - CR0_MP_SHIFT)) &
                (HF_MP_MASK | HF_EM_MASK | HF_TS_MASK);
    hflags |= (env->eflags & (HF_TF_MASK | HF_VM_MASK | HF_IOPL_MASK));

    if (env->cr[4] & CR4_OSFXSR_MASK) {
        hflags |= HF_OSFXSR_MASK;
    }

    if (env->efer & MSR_EFER_LMA) {
        hflags |= HF_LMA_MASK;
    }

    if ((hflags & HF_LMA_MASK) && (env->segs[R_CS].flags & DESC_L_MASK)) {
        hflags |= HF_CS32_MASK | HF_SS32_MASK | HF_CS64_MASK;
    } else {
        hflags |= (env->segs[R_CS].flags & DESC_B_MASK) >>
                    (DESC_B_SHIFT - HF_CS32_SHIFT);
        hflags |= (env->segs[R_SS].flags & DESC_B_MASK) >>
                    (DESC_B_SHIFT - HF_SS32_SHIFT);
        if (!(env->cr[0] & CR0_PE_MASK) || (env->eflags & VM_MASK) ||
            !(hflags & HF_CS32_MASK)) {
            hflags |= HF_ADDSEG_MASK;
        } else {
            hflags |= ((env->segs[R_DS].base | env->segs[R_ES].base |
                        env->segs[R_SS].base) != 0) << HF_ADDSEG_SHIFT;
        }
    }
    env->hflags = hflags;

    return 0;
}

static int gvm_get_msrs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct gvm_msr_entry *msrs = cpu->gvm_msr_buf->entries;
    int ret, i;
    uint64_t mtrr_top_bits;
    uint64_t bufsize;

    gvm_msr_buf_reset(cpu);

    gvm_msr_entry_add(cpu, MSR_IA32_SYSENTER_CS, 0);
    gvm_msr_entry_add(cpu, MSR_IA32_SYSENTER_ESP, 0);
    gvm_msr_entry_add(cpu, MSR_IA32_SYSENTER_EIP, 0);
    gvm_msr_entry_add(cpu, MSR_PAT, 0);
    if (has_msr_star) {
        gvm_msr_entry_add(cpu, MSR_STAR, 0);
    }
    if (has_msr_hsave_pa) {
        gvm_msr_entry_add(cpu, MSR_VM_HSAVE_PA, 0);
    }
    if (has_msr_tsc_aux) {
        gvm_msr_entry_add(cpu, MSR_TSC_AUX, 0);
    }
    if (has_msr_tsc_adjust) {
        gvm_msr_entry_add(cpu, MSR_TSC_ADJUST, 0);
    }
    if (has_msr_tsc_deadline) {
        gvm_msr_entry_add(cpu, MSR_IA32_TSCDEADLINE, 0);
    }
    if (has_msr_misc_enable) {
        gvm_msr_entry_add(cpu, MSR_IA32_MISC_ENABLE, 0);
    }
    if (has_msr_smbase) {
        gvm_msr_entry_add(cpu, MSR_IA32_SMBASE, 0);
    }
    if (has_msr_feature_control) {
        gvm_msr_entry_add(cpu, MSR_IA32_FEATURE_CONTROL, 0);
    }
    if (has_msr_bndcfgs) {
        gvm_msr_entry_add(cpu, MSR_IA32_BNDCFGS, 0);
    }
    if (has_msr_xss) {
        gvm_msr_entry_add(cpu, MSR_IA32_XSS, 0);
    }


    if (!env->tsc_valid) {
        gvm_msr_entry_add(cpu, MSR_IA32_TSC, 0);
        env->tsc_valid = !runstate_is_running();
    }

#ifdef TARGET_X86_64
    if (lm_capable_kernel) {
        gvm_msr_entry_add(cpu, MSR_CSTAR, 0);
        gvm_msr_entry_add(cpu, MSR_KERNELGSBASE, 0);
        gvm_msr_entry_add(cpu, MSR_FMASK, 0);
        gvm_msr_entry_add(cpu, MSR_LSTAR, 0);
    }
#endif
    // Disable KVM PV Feature MSRs (software only MSRs). We may remove
    // it completely later. But let's keep it here before we make final
    // decision.
#if 0
    gvm_msr_entry_add(cpu, MSR_GVM_SYSTEM_TIME, 0);
    gvm_msr_entry_add(cpu, MSR_GVM_WALL_CLOCK, 0);
    if (has_msr_async_pf_en) {
        gvm_msr_entry_add(cpu, MSR_GVM_ASYNC_PF_EN, 0);
    }
    if (has_msr_pv_eoi_en) {
        gvm_msr_entry_add(cpu, MSR_GVM_PV_EOI_EN, 0);
    }
    if (has_msr_gvm_steal_time) {
        gvm_msr_entry_add(cpu, MSR_GVM_STEAL_TIME, 0);
    }
#endif
    if (has_msr_architectural_pmu) {
        gvm_msr_entry_add(cpu, MSR_CORE_PERF_FIXED_CTR_CTRL, 0);
        gvm_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_CTRL, 0);
        gvm_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_STATUS, 0);
        gvm_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_OVF_CTRL, 0);
        for (i = 0; i < MAX_FIXED_COUNTERS; i++) {
            gvm_msr_entry_add(cpu, MSR_CORE_PERF_FIXED_CTR0 + i, 0);
        }
        for (i = 0; i < num_architectural_pmu_counters; i++) {
            gvm_msr_entry_add(cpu, MSR_P6_PERFCTR0 + i, 0);
            gvm_msr_entry_add(cpu, MSR_P6_EVNTSEL0 + i, 0);
        }
    }

    if (has_msr_mtrr) {
        gvm_msr_entry_add(cpu, MSR_MTRRdefType, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix64K_00000, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix16K_80000, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix16K_A0000, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix4K_C0000, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix4K_C8000, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix4K_D0000, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix4K_D8000, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix4K_E0000, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix4K_E8000, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix4K_F0000, 0);
        gvm_msr_entry_add(cpu, MSR_MTRRfix4K_F8000, 0);
        for (i = 0; i < MSR_MTRRcap_VCNT; i++) {
            gvm_msr_entry_add(cpu, MSR_MTRRphysBase(i), 0);
            gvm_msr_entry_add(cpu, MSR_MTRRphysMask(i), 0);
        }
    }

    bufsize = sizeof(struct gvm_msrs) + cpu->gvm_msr_buf->nmsrs *
        sizeof(struct gvm_msr_entry);
    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_GET_MSRS,
            cpu->gvm_msr_buf, bufsize,
            cpu->gvm_msr_buf, bufsize);
    if (ret < 0) {
        return ret;
    }

    /*
     * MTRR masks: Each mask consists of 5 parts
     * a  10..0: must be zero
     * b  11   : valid bit
     * c n-1.12: actual mask bits
     * d  51..n: reserved must be zero
     * e  63.52: reserved must be zero
     *
     * 'n' is the number of physical bits supported by the CPU and is
     * apparently always <= 52.   We know our 'n' but don't know what
     * the destinations 'n' is; it might be smaller, in which case
     * it masks (c) on loading. It might be larger, in which case
     * we fill 'd' so that d..c is consistent irrespetive of the 'n'
     * we're migrating to.
     */

    if (cpu->fill_mtrr_mask) {
        QEMU_BUILD_BUG_ON(TARGET_PHYS_ADDR_SPACE_BITS > 52);
        assert(cpu->phys_bits <= TARGET_PHYS_ADDR_SPACE_BITS);
        mtrr_top_bits = MAKE_64BIT_MASK(cpu->phys_bits, 52 - cpu->phys_bits);
    } else {
        mtrr_top_bits = 0;
    }

    for (i = 0; i < ret; i++) {
        uint32_t index = msrs[i].index;
        switch (index) {
        case MSR_IA32_SYSENTER_CS:
            env->sysenter_cs = msrs[i].data;
            break;
        case MSR_IA32_SYSENTER_ESP:
            env->sysenter_esp = msrs[i].data;
            break;
        case MSR_IA32_SYSENTER_EIP:
            env->sysenter_eip = msrs[i].data;
            break;
        case MSR_PAT:
            env->pat = msrs[i].data;
            break;
        case MSR_STAR:
            env->star = msrs[i].data;
            break;
#ifdef TARGET_X86_64
        case MSR_CSTAR:
            env->cstar = msrs[i].data;
            break;
        case MSR_KERNELGSBASE:
            env->kernelgsbase = msrs[i].data;
            break;
        case MSR_FMASK:
            env->fmask = msrs[i].data;
            break;
        case MSR_LSTAR:
            env->lstar = msrs[i].data;
            break;
#endif
        case MSR_IA32_TSC:
            env->tsc = msrs[i].data;
            break;
        case MSR_TSC_AUX:
            env->tsc_aux = msrs[i].data;
            break;
        case MSR_TSC_ADJUST:
            env->tsc_adjust = msrs[i].data;
            break;
        case MSR_IA32_TSCDEADLINE:
            env->tsc_deadline = msrs[i].data;
            break;
        case MSR_VM_HSAVE_PA:
            env->vm_hsave = msrs[i].data;
            break;
        case MSR_GVM_SYSTEM_TIME:
            env->system_time_msr = msrs[i].data;
            break;
        case MSR_GVM_WALL_CLOCK:
            env->wall_clock_msr = msrs[i].data;
            break;
        case MSR_MCG_STATUS:
            env->mcg_status = msrs[i].data;
            break;
        case MSR_MCG_CTL:
            env->mcg_ctl = msrs[i].data;
            break;
        case MSR_MCG_EXT_CTL:
            env->mcg_ext_ctl = msrs[i].data;
            break;
        case MSR_IA32_MISC_ENABLE:
            env->msr_ia32_misc_enable = msrs[i].data;
            break;
        case MSR_IA32_SMBASE:
            env->smbase = msrs[i].data;
            break;
        case MSR_IA32_FEATURE_CONTROL:
            env->msr_ia32_feature_control = msrs[i].data;
            break;
        case MSR_IA32_BNDCFGS:
            env->msr_bndcfgs = msrs[i].data;
            break;
        case MSR_IA32_XSS:
            env->xss = msrs[i].data;
            break;
        default:
            if (msrs[i].index >= MSR_MC0_CTL &&
                msrs[i].index < MSR_MC0_CTL + (env->mcg_cap & 0xff) * 4) {
                env->mce_banks[msrs[i].index - MSR_MC0_CTL] = msrs[i].data;
            }
            break;
    // Disable KVM PV Feature MSRs (software only MSRs). We may remove
    // it completely later. But let's keep it here before we make final
    // decision.
#if 0
        case MSR_GVM_ASYNC_PF_EN:
            env->async_pf_en_msr = msrs[i].data;
            break;
        case MSR_GVM_PV_EOI_EN:
            env->pv_eoi_en_msr = msrs[i].data;
            break;
        case MSR_GVM_STEAL_TIME:
            env->steal_time_msr = msrs[i].data;
            break;
#endif
        case MSR_CORE_PERF_FIXED_CTR_CTRL:
            env->msr_fixed_ctr_ctrl = msrs[i].data;
            break;
        case MSR_CORE_PERF_GLOBAL_CTRL:
            env->msr_global_ctrl = msrs[i].data;
            break;
        case MSR_CORE_PERF_GLOBAL_STATUS:
            env->msr_global_status = msrs[i].data;
            break;
        case MSR_CORE_PERF_GLOBAL_OVF_CTRL:
            env->msr_global_ovf_ctrl = msrs[i].data;
            break;
        case MSR_CORE_PERF_FIXED_CTR0 ... MSR_CORE_PERF_FIXED_CTR0 + MAX_FIXED_COUNTERS - 1:
            env->msr_fixed_counters[index - MSR_CORE_PERF_FIXED_CTR0] = msrs[i].data;
            break;
        case MSR_P6_PERFCTR0 ... MSR_P6_PERFCTR0 + MAX_GP_COUNTERS - 1:
            env->msr_gp_counters[index - MSR_P6_PERFCTR0] = msrs[i].data;
            break;
        case MSR_P6_EVNTSEL0 ... MSR_P6_EVNTSEL0 + MAX_GP_COUNTERS - 1:
            env->msr_gp_evtsel[index - MSR_P6_EVNTSEL0] = msrs[i].data;
            break;
        case MSR_MTRRdefType:
            env->mtrr_deftype = msrs[i].data;
            break;
        case MSR_MTRRfix64K_00000:
            env->mtrr_fixed[0] = msrs[i].data;
            break;
        case MSR_MTRRfix16K_80000:
            env->mtrr_fixed[1] = msrs[i].data;
            break;
        case MSR_MTRRfix16K_A0000:
            env->mtrr_fixed[2] = msrs[i].data;
            break;
        case MSR_MTRRfix4K_C0000:
            env->mtrr_fixed[3] = msrs[i].data;
            break;
        case MSR_MTRRfix4K_C8000:
            env->mtrr_fixed[4] = msrs[i].data;
            break;
        case MSR_MTRRfix4K_D0000:
            env->mtrr_fixed[5] = msrs[i].data;
            break;
        case MSR_MTRRfix4K_D8000:
            env->mtrr_fixed[6] = msrs[i].data;
            break;
        case MSR_MTRRfix4K_E0000:
            env->mtrr_fixed[7] = msrs[i].data;
            break;
        case MSR_MTRRfix4K_E8000:
            env->mtrr_fixed[8] = msrs[i].data;
            break;
        case MSR_MTRRfix4K_F0000:
            env->mtrr_fixed[9] = msrs[i].data;
            break;
        case MSR_MTRRfix4K_F8000:
            env->mtrr_fixed[10] = msrs[i].data;
            break;
        case MSR_MTRRphysBase(0) ... MSR_MTRRphysMask(MSR_MTRRcap_VCNT - 1):
            if (index & 1) {
                env->mtrr_var[MSR_MTRRphysIndex(index)].mask = msrs[i].data |
                                                               mtrr_top_bits;
            } else {
                env->mtrr_var[MSR_MTRRphysIndex(index)].base = msrs[i].data;
            }
            break;
        }
    }

    return 0;
}

static int gvm_put_mp_state(X86CPU *cpu)
{
    struct gvm_mp_state mp_state = { .mp_state = cpu->env.mp_state };

    return gvm_vcpu_ioctl(CPU(cpu), GVM_SET_MP_STATE,
            &mp_state, sizeof(mp_state), NULL, 0);
}

static int gvm_get_mp_state(X86CPU *cpu)
{
    CPUState *cs = CPU(cpu);
    CPUX86State *env = &cpu->env;
    struct gvm_mp_state mp_state;
    int ret;

    ret = gvm_vcpu_ioctl(cs, GVM_GET_MP_STATE,
            NULL, 0, &mp_state, sizeof(mp_state));
    if (ret < 0) {
        return ret;
    }
    env->mp_state = mp_state.mp_state;
    if (gvm_irqchip_in_kernel()) {
        cs->halted = (mp_state.mp_state == GVM_MP_STATE_HALTED);
    }
    return 0;
}

static int gvm_get_apic(X86CPU *cpu)
{
    DeviceState *apic = cpu->apic_state;
    struct gvm_lapic_state kapic;
    int ret;

    if (apic && gvm_irqchip_in_kernel()) {
        ret = gvm_vcpu_ioctl(CPU(cpu), GVM_GET_LAPIC,
                NULL, 0, &kapic, sizeof(kapic));
        if (ret < 0) {
            return ret;
        }

        gvm_get_apic_state(apic, &kapic);
    }
    return 0;
}

static int gvm_put_apic(X86CPU *cpu)
{
    DeviceState *apic = cpu->apic_state;
    struct gvm_lapic_state kapic;

    if (apic && gvm_irqchip_in_kernel()) {
        gvm_put_apic_state(apic, &kapic);

        return gvm_vcpu_ioctl(CPU(cpu), GVM_SET_LAPIC,
                &kapic, sizeof(kapic), NULL, 0);
    }
    return 0;
}

static int gvm_put_vcpu_events(X86CPU *cpu, int level)
{
    CPUState *cs = CPU(cpu);
    CPUX86State *env = &cpu->env;
    struct gvm_vcpu_events events = {};

    if (!gvm_has_vcpu_events()) {
        return 0;
    }

    events.exception.injected = (env->exception_injected >= 0);
    events.exception.nr = env->exception_injected;
    events.exception.has_error_code = env->has_error_code;
    events.exception.error_code = env->error_code;
    events.exception.pad = 0;

    events.interrupt.injected = (env->interrupt_injected >= 0);
    events.interrupt.nr = env->interrupt_injected;
    events.interrupt.soft = env->soft_interrupt;

    events.nmi.injected = env->nmi_injected;
    events.nmi.pending = env->nmi_pending;
    events.nmi.masked = !!(env->hflags2 & HF2_NMI_MASK);
    events.nmi.pad = 0;

    events.sipi_vector = env->sipi_vector;

    if (has_msr_smbase) {
        events.smi.smm = !!(env->hflags & HF_SMM_MASK);
        events.smi.smm_inside_nmi = !!(env->hflags2 & HF2_SMM_INSIDE_NMI_MASK);
        if (gvm_irqchip_in_kernel()) {
            /* As soon as these are moved to the kernel, remove them
             * from cs->interrupt_request.
             */
            events.smi.pending = cs->interrupt_request & CPU_INTERRUPT_SMI;
            events.smi.latched_init = cs->interrupt_request & CPU_INTERRUPT_INIT;
            cs->interrupt_request &= ~(CPU_INTERRUPT_INIT | CPU_INTERRUPT_SMI);
        } else {
            /* Keep these in cs->interrupt_request.  */
            events.smi.pending = 0;
            events.smi.latched_init = 0;
        }
        events.flags |= GVM_VCPUEVENT_VALID_SMM;
    }

    events.flags = 0;
    if (level >= GVM_PUT_RESET_STATE) {
        events.flags |=
            GVM_VCPUEVENT_VALID_NMI_PENDING | GVM_VCPUEVENT_VALID_SIPI_VECTOR;
    }

    return gvm_vcpu_ioctl(CPU(cpu), GVM_SET_VCPU_EVENTS,
            &events, sizeof(events), NULL, 0);
}

static int gvm_get_vcpu_events(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct gvm_vcpu_events events;
    int ret;

    if (!gvm_has_vcpu_events()) {
        return 0;
    }

    memset(&events, 0, sizeof(events));
    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_GET_VCPU_EVENTS,
            NULL, 0, &events, sizeof(events));
    if (ret < 0) {
       return ret;
    }
    env->exception_injected =
       events.exception.injected ? events.exception.nr : -1;
    env->has_error_code = events.exception.has_error_code;
    env->error_code = events.exception.error_code;

    env->interrupt_injected =
        events.interrupt.injected ? events.interrupt.nr : -1;
    env->soft_interrupt = events.interrupt.soft;

    env->nmi_injected = events.nmi.injected;
    env->nmi_pending = events.nmi.pending;
    if (events.nmi.masked) {
        env->hflags2 |= HF2_NMI_MASK;
    } else {
        env->hflags2 &= ~HF2_NMI_MASK;
    }

    if (events.flags & GVM_VCPUEVENT_VALID_SMM) {
        if (events.smi.smm) {
            env->hflags |= HF_SMM_MASK;
        } else {
            env->hflags &= ~HF_SMM_MASK;
        }
        if (events.smi.pending) {
            cpu_interrupt(CPU(cpu), CPU_INTERRUPT_SMI);
        } else {
            cpu_reset_interrupt(CPU(cpu), CPU_INTERRUPT_SMI);
        }
        if (events.smi.smm_inside_nmi) {
            env->hflags2 |= HF2_SMM_INSIDE_NMI_MASK;
        } else {
            env->hflags2 &= ~HF2_SMM_INSIDE_NMI_MASK;
        }
        if (events.smi.latched_init) {
            cpu_interrupt(CPU(cpu), CPU_INTERRUPT_INIT);
        } else {
            cpu_reset_interrupt(CPU(cpu), CPU_INTERRUPT_INIT);
        }
    }

    env->sipi_vector = events.sipi_vector;

    return 0;
}

static int gvm_guest_debug_workarounds(X86CPU *cpu)
{
    CPUState *cs = CPU(cpu);
    CPUX86State *env = &cpu->env;
    int ret = 0;
    unsigned long reinject_trap = 0;

    if (!gvm_has_vcpu_events()) {
        if (env->exception_injected == 1) {
            reinject_trap = GVM_GUESTDBG_INJECT_DB;
        } else if (env->exception_injected == 3) {
            reinject_trap = GVM_GUESTDBG_INJECT_BP;
        }
        env->exception_injected = -1;
    }

    /*
     * Kernels before GVM_CAP_X86_ROBUST_SINGLESTEP overwrote flags.TF
     * injected via SET_GUEST_DEBUG while updating GP regs. Work around this
     * by updating the debug state once again if single-stepping is on.
     * Another reason to call gvm_update_guest_debug here is a pending debug
     * trap raise by the guest. On kernels without SET_VCPU_EVENTS we have to
     * reinject them via SET_GUEST_DEBUG.
     */
    if (reinject_trap ||
        (!gvm_has_robust_singlestep() && cs->singlestep_enabled)) {
        ret = gvm_update_guest_debug(cs, reinject_trap);
    }
    return ret;
}

static int gvm_put_debugregs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct gvm_debugregs dbgregs;
    int i;

    if (!gvm_has_debugregs()) {
        return 0;
    }

    for (i = 0; i < 4; i++) {
        dbgregs.db[i] = env->dr[i];
    }
    dbgregs.dr6 = env->dr[6];
    dbgregs.dr7 = env->dr[7];
    dbgregs.flags = 0;

    return gvm_vcpu_ioctl(CPU(cpu), GVM_SET_DEBUGREGS,
            &dbgregs, sizeof(dbgregs), NULL, 0);
}

static int gvm_get_debugregs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct gvm_debugregs dbgregs;
    int i, ret;

    if (!gvm_has_debugregs()) {
        return 0;
    }

    ret = gvm_vcpu_ioctl(CPU(cpu), GVM_GET_DEBUGREGS,
            &dbgregs, sizeof(dbgregs), NULL, 0);
    if (ret < 0) {
        return ret;
    }
    for (i = 0; i < 4; i++) {
        env->dr[i] = dbgregs.db[i];
    }
    env->dr[4] = env->dr[6] = dbgregs.dr6;
    env->dr[5] = env->dr[7] = dbgregs.dr7;

    return 0;
}

int gvm_arch_put_registers(CPUState *cpu, int level)
{
    X86CPU *x86_cpu = X86_CPU(cpu);
    int ret;

    assert(cpu_is_stopped(cpu) || qemu_cpu_is_self(cpu));

    if (level >= GVM_PUT_RESET_STATE) {
        ret = gvm_put_msr_feature_control(x86_cpu);
        if (ret < 0) {
            return ret;
        }
    }

    if (level == GVM_PUT_FULL_STATE) {
        /* We don't check for gvm_arch_set_tsc_khz() errors here,
         * because TSC frequency mismatch shouldn't abort migration,
         * unless the user explicitly asked for a more strict TSC
         * setting (e.g. using an explicit "tsc-freq" option).
         */
        gvm_arch_set_tsc_khz(cpu);
    }

    ret = gvm_getput_regs(x86_cpu, 1);
    if (ret < 0) {
        return ret;
    }
    ret = gvm_put_xsave(x86_cpu);
    if (ret < 0) {
        return ret;
    }
    ret = gvm_put_xcrs(x86_cpu);
    if (ret < 0) {
        return ret;
    }
    ret = gvm_put_sregs(x86_cpu);
    if (ret < 0) {
        return ret;
    }
    ret = gvm_put_msrs(x86_cpu, level);
    if (ret < 0) {
        return ret;
    }
    if (level >= GVM_PUT_RESET_STATE) {
        ret = gvm_put_mp_state(x86_cpu);
        if (ret < 0) {
            return ret;
        }
        ret = gvm_put_apic(x86_cpu);
        if (ret < 0) {
            return ret;
        }
    }

    ret = gvm_put_tscdeadline_msr(x86_cpu);
    if (ret < 0) {
        return ret;
    }

    ret = gvm_put_vcpu_events(x86_cpu, level);
    if (ret < 0) {
        return ret;
    }
    ret = gvm_put_debugregs(x86_cpu);
    if (ret < 0) {
        return ret;
    }
    /* must be last */
    ret = gvm_guest_debug_workarounds(x86_cpu);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int gvm_arch_get_registers(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    int ret;

    assert(cpu_is_stopped(cs) || qemu_cpu_is_self(cs));

    ret = gvm_getput_regs(cpu, 0);
    if (ret < 0) {
        goto out;
    }
    ret = gvm_get_xsave(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = gvm_get_xcrs(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = gvm_get_sregs(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = gvm_get_msrs(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = gvm_get_mp_state(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = gvm_get_apic(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = gvm_get_vcpu_events(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = gvm_get_debugregs(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = 0;
 out:
    cpu_sync_bndcs_hflags(&cpu->env);
    return ret;
}

void gvm_arch_pre_run(CPUState *cpu, struct gvm_run *run)
{
    X86CPU *x86_cpu = X86_CPU(cpu);
    CPUX86State *env = &x86_cpu->env;
    int ret;

    /* Inject NMI */
    if (cpu->interrupt_request & (CPU_INTERRUPT_NMI | CPU_INTERRUPT_SMI)) {
        if (cpu->interrupt_request & CPU_INTERRUPT_NMI) {
            qemu_mutex_lock_iothread();
            cpu->interrupt_request &= ~CPU_INTERRUPT_NMI;
            qemu_mutex_unlock_iothread();
            DPRINTF("injected NMI\n");
            ret = gvm_vcpu_ioctl(cpu, GVM_NMI, NULL, 0, NULL, 0);
            if (ret < 0) {
                fprintf(stderr, "GVM: injection failed, NMI lost (%s)\n",
                        strerror(-ret));
            }
        }
        if (cpu->interrupt_request & CPU_INTERRUPT_SMI) {
            qemu_mutex_lock_iothread();
            cpu->interrupt_request &= ~CPU_INTERRUPT_SMI;
            qemu_mutex_unlock_iothread();
            DPRINTF("injected SMI\n");
            ret = gvm_vcpu_ioctl(cpu, GVM_SMI, NULL, 0, NULL, 0);
            if (ret < 0) {
                fprintf(stderr, "GVM: injection failed, SMI lost (%s)\n",
                        strerror(-ret));
            }
        }
    }

    if (!gvm_pic_in_kernel()) {
        qemu_mutex_lock_iothread();
    }

    /* Force the VCPU out of its inner loop to process any INIT requests
     * or (for userspace APIC, but it is cheap to combine the checks here)
     * pending TPR access reports.
     */
    if (cpu->interrupt_request & (CPU_INTERRUPT_INIT | CPU_INTERRUPT_TPR)) {
        if ((cpu->interrupt_request & CPU_INTERRUPT_INIT) &&
            !(env->hflags & HF_SMM_MASK)) {
            cpu->exit_request = 1;
        }
        if (cpu->interrupt_request & CPU_INTERRUPT_TPR) {
            cpu->exit_request = 1;
        }
    }

    if (!gvm_pic_in_kernel()) {
        /* Try to inject an interrupt if the guest can accept it */
        if (run->ready_for_interrupt_injection &&
            (cpu->interrupt_request & CPU_INTERRUPT_HARD) &&
            (env->eflags & IF_MASK)) {
            int irq;

            cpu->interrupt_request &= ~CPU_INTERRUPT_HARD;
            irq = cpu_get_pic_interrupt(env);
            if (irq >= 0) {
                struct gvm_interrupt intr;

                intr.irq = irq;
                DPRINTF("injected interrupt %d\n", irq);
                ret = gvm_vcpu_ioctl(cpu, GVM_INTERRUPT,
                        &intr, sizeof(intr), NULL, 0);
                if (ret < 0) {
                    fprintf(stderr,
                            "GVM: injection failed, interrupt lost (%s)\n",
                            strerror(-ret));
                }
            }
        }

        /* If we have an interrupt but the guest is not ready to receive an
         * interrupt, request an interrupt window exit.  This will
         * cause a return to userspace as soon as the guest is ready to
         * receive interrupts. */
        if ((cpu->interrupt_request & CPU_INTERRUPT_HARD)) {
            run->request_interrupt_window = 1;
        } else {
            run->request_interrupt_window = 0;
        }

        DPRINTF("setting tpr\n");
        run->cr8 = cpu_get_apic_tpr(x86_cpu->apic_state);

        qemu_mutex_unlock_iothread();
    }
}

MemTxAttrs gvm_arch_post_run(CPUState *cpu, struct gvm_run *run)
{
    X86CPU *x86_cpu = X86_CPU(cpu);
    CPUX86State *env = &x86_cpu->env;

    if (run->flags & GVM_RUN_X86_SMM) {
        env->hflags |= HF_SMM_MASK;
    } else {
        env->hflags &= HF_SMM_MASK;
    }
    if (run->if_flag) {
        env->eflags |= IF_MASK;
    } else {
        env->eflags &= ~IF_MASK;
    }

    /* We need to protect the apic state against concurrent accesses from
     * different threads in case the userspace irqchip is used. */
    if (!gvm_irqchip_in_kernel()) {
        qemu_mutex_lock_iothread();
    }
    cpu_set_apic_tpr(x86_cpu->apic_state, run->cr8);
    cpu_set_apic_base(x86_cpu->apic_state, run->apic_base);
    if (!gvm_irqchip_in_kernel()) {
        qemu_mutex_unlock_iothread();
    }
    return cpu_get_mem_attrs(env);
}

int gvm_arch_process_async_events(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;

    if (cs->interrupt_request & CPU_INTERRUPT_MCE) {
        /* We must not raise CPU_INTERRUPT_MCE if it's not supported. */
        assert(env->mcg_cap);

        cs->interrupt_request &= ~CPU_INTERRUPT_MCE;

        gvm_cpu_synchronize_state(cs);

        if (env->exception_injected == EXCP08_DBLE) {
            /* this means triple fault */
            qemu_system_reset_request(SHUTDOWN_CAUSE_GUEST_RESET);
            cs->exit_request = 1;
            return 0;
        }
        env->exception_injected = EXCP12_MCHK;
        env->has_error_code = 0;

        cs->halted = 0;
        if (gvm_irqchip_in_kernel() && env->mp_state == GVM_MP_STATE_HALTED) {
            env->mp_state = GVM_MP_STATE_RUNNABLE;
        }
    }

    if ((cs->interrupt_request & CPU_INTERRUPT_INIT) &&
        !(env->hflags & HF_SMM_MASK)) {
        gvm_cpu_synchronize_state(cs);
        do_cpu_init(cpu);
    }

    if (gvm_irqchip_in_kernel()) {
        return 0;
    }

    if (cs->interrupt_request & CPU_INTERRUPT_POLL) {
        cs->interrupt_request &= ~CPU_INTERRUPT_POLL;
        apic_poll_irq(cpu->apic_state);
    }
    if (((cs->interrupt_request & CPU_INTERRUPT_HARD) &&
         (env->eflags & IF_MASK)) ||
        (cs->interrupt_request & CPU_INTERRUPT_NMI)) {
        cs->halted = 0;
    }
    if (cs->interrupt_request & CPU_INTERRUPT_SIPI) {
        gvm_cpu_synchronize_state(cs);
        do_cpu_sipi(cpu);
    }
    if (cs->interrupt_request & CPU_INTERRUPT_TPR) {
        cs->interrupt_request &= ~CPU_INTERRUPT_TPR;
        gvm_cpu_synchronize_state(cs);
        apic_handle_tpr_access_report(cpu->apic_state, env->eip,
                                      env->tpr_access_type);
    }

    return cs->halted;
}

static int gvm_handle_halt(X86CPU *cpu)
{
    CPUState *cs = CPU(cpu);
    CPUX86State *env = &cpu->env;

    if (!((cs->interrupt_request & CPU_INTERRUPT_HARD) &&
          (env->eflags & IF_MASK)) &&
        !(cs->interrupt_request & CPU_INTERRUPT_NMI)) {
        cs->halted = 1;
        return EXCP_HLT;
    }

    return 0;
}

static int gvm_handle_tpr_access(X86CPU *cpu)
{
    CPUState *cs = CPU(cpu);
    struct gvm_run *run = cs->gvm_run;

    apic_handle_tpr_access_report(cpu->apic_state, run->tpr_access.rip,
                                  run->tpr_access.is_write ? TPR_ACCESS_WRITE
                                                           : TPR_ACCESS_READ);
    return 1;
}

int gvm_arch_insert_sw_breakpoint(CPUState *cs, struct gvm_sw_breakpoint *bp)
{
    static const uint8_t int3 = 0xcc;

    if (cpu_memory_rw_debug(cs, bp->pc, (uint8_t *)&bp->saved_insn, 1, 0) ||
        cpu_memory_rw_debug(cs, bp->pc, (uint8_t *)&int3, 1, 1)) {
        return -EINVAL;
    }
    return 0;
}

int gvm_arch_remove_sw_breakpoint(CPUState *cs, struct gvm_sw_breakpoint *bp)
{
    uint8_t int3;

    if (cpu_memory_rw_debug(cs, bp->pc, &int3, 1, 0) || int3 != 0xcc ||
        cpu_memory_rw_debug(cs, bp->pc, (uint8_t *)&bp->saved_insn, 1, 1)) {
        return -EINVAL;
    }
    return 0;
}

static struct {
    target_ulong addr;
    int len;
    int type;
} hw_breakpoint[4];

static int nb_hw_breakpoint;

static int find_hw_breakpoint(target_ulong addr, int len, int type)
{
    int n;

    for (n = 0; n < nb_hw_breakpoint; n++) {
        if (hw_breakpoint[n].addr == addr && hw_breakpoint[n].type == type &&
            (hw_breakpoint[n].len == len || len == -1)) {
            return n;
        }
    }
    return -1;
}

int gvm_arch_insert_hw_breakpoint(target_ulong addr,
                                  target_ulong len, int type)
{
    switch (type) {
    case GDB_BREAKPOINT_HW:
        len = 1;
        break;
    case GDB_WATCHPOINT_WRITE:
    case GDB_WATCHPOINT_ACCESS:
        switch (len) {
        case 1:
            break;
        case 2:
        case 4:
        case 8:
            if (addr & (len - 1)) {
                return -EINVAL;
            }
            break;
        default:
            return -EINVAL;
        }
        break;
    default:
        return -ENOSYS;
    }

    if (nb_hw_breakpoint == 4) {
        return -ENOBUFS;
    }
    if (find_hw_breakpoint(addr, len, type) >= 0) {
        return -EEXIST;
    }
    hw_breakpoint[nb_hw_breakpoint].addr = addr;
    hw_breakpoint[nb_hw_breakpoint].len = len;
    hw_breakpoint[nb_hw_breakpoint].type = type;
    nb_hw_breakpoint++;

    return 0;
}

int gvm_arch_remove_hw_breakpoint(target_ulong addr,
                                  target_ulong len, int type)
{
    int n;

    n = find_hw_breakpoint(addr, (type == GDB_BREAKPOINT_HW) ? 1 : len, type);
    if (n < 0) {
        return -ENOENT;
    }
    nb_hw_breakpoint--;
    hw_breakpoint[n] = hw_breakpoint[nb_hw_breakpoint];

    return 0;
}

void gvm_arch_remove_all_hw_breakpoints(void)
{
    nb_hw_breakpoint = 0;
}

//static CPUWatchpoint hw_watchpoint;

static int gvm_handle_debug(X86CPU *cpu,
                            struct gvm_debug_exit_arch *arch_info)
{
#if 0
    CPUState *cs = CPU(cpu);
    CPUX86State *env = &cpu->env;
    int ret = 0;
    int n;

    if (arch_info->exception == 1) {
        if (arch_info->dr6 & (1 << 14)) {
            if (cs->singlestep_enabled) {
                ret = EXCP_DEBUG;
            }
        } else {
            for (n = 0; n < 4; n++) {
                if (arch_info->dr6 & (1 << n)) {
                    switch ((arch_info->dr7 >> (16 + n*4)) & 0x3) {
                    case 0x0:
                        ret = EXCP_DEBUG;
                        break;
                    case 0x1:
                        ret = EXCP_DEBUG;
                        cs->watchpoint_hit = &hw_watchpoint;
                        hw_watchpoint.vaddr = hw_breakpoint[n].addr;
                        hw_watchpoint.flags = BP_MEM_WRITE;
                        break;
                    case 0x3:
                        ret = EXCP_DEBUG;
                        cs->watchpoint_hit = &hw_watchpoint;
                        hw_watchpoint.vaddr = hw_breakpoint[n].addr;
                        hw_watchpoint.flags = BP_MEM_ACCESS;
                        break;
                    }
                }
            }
        }
    } else if (gvm_find_sw_breakpoint(cs, arch_info->pc)) {
        ret = EXCP_DEBUG;
    }
    if (ret == 0) {
        cpu_synchronize_state(cs);
        assert(env->exception_injected == -1);

        /* pass to guest */
        env->exception_injected = arch_info->exception;
        env->has_error_code = 0;
    }

    return ret;
#endif
    return 0;
}

void gvm_arch_update_guest_debug(CPUState *cpu, struct gvm_guest_debug *dbg)
{
    const uint8_t type_code[] = {
        [GDB_BREAKPOINT_HW] = 0x0,
        [GDB_WATCHPOINT_WRITE] = 0x1,
        [GDB_WATCHPOINT_ACCESS] = 0x3
    };
    const uint8_t len_code[] = {
        [1] = 0x0, [2] = 0x1, [4] = 0x3, [8] = 0x2
    };
    int n;

    if (gvm_sw_breakpoints_active(cpu)) {
        dbg->control |= GVM_GUESTDBG_ENABLE | GVM_GUESTDBG_USE_SW_BP;
    }
    if (nb_hw_breakpoint > 0) {
        dbg->control |= GVM_GUESTDBG_ENABLE | GVM_GUESTDBG_USE_HW_BP;
        dbg->arch.debugreg[7] = 0x0600;
        for (n = 0; n < nb_hw_breakpoint; n++) {
            dbg->arch.debugreg[n] = hw_breakpoint[n].addr;
            dbg->arch.debugreg[7] |= (2 << (n * 2)) |
                (type_code[hw_breakpoint[n].type] << (16 + n*4)) |
                ((uint32_t)len_code[hw_breakpoint[n].len] << (18 + n*4));
        }
    }
}

static bool host_supports_vmx(void)
{
    uint32_t ecx, unused;

    host_cpuid(1, 0, &unused, &unused, &ecx, &unused);
    return ecx & CPUID_EXT_VMX;
}

#define VMX_INVALID_GUEST_STATE 0x80000021

int gvm_arch_handle_exit(CPUState *cs, struct gvm_run *run)
{
    X86CPU *cpu = X86_CPU(cs);
    uint64_t code;
    int ret;

    switch (run->exit_reason) {
    case GVM_EXIT_HLT:
        DPRINTF("handle_hlt\n");
        qemu_mutex_lock_iothread();
        ret = gvm_handle_halt(cpu);
        qemu_mutex_unlock_iothread();
        break;
    case GVM_EXIT_SET_TPR:
        ret = 0;
        break;
    case GVM_EXIT_TPR_ACCESS:
        qemu_mutex_lock_iothread();
        ret = gvm_handle_tpr_access(cpu);
        qemu_mutex_unlock_iothread();
        break;
    case GVM_EXIT_FAIL_ENTRY:
        code = run->fail_entry.hardware_entry_failure_reason;
        fprintf(stderr, "GVM: entry failed, hardware error 0x%" PRIx64 "\n",
                code);
        if (host_supports_vmx() && code == VMX_INVALID_GUEST_STATE) {
            fprintf(stderr,
                    "\nIf you're running a guest on an Intel machine without "
                        "unrestricted mode\n"
                    "support, the failure can be most likely due to the guest "
                        "entering an invalid\n"
                    "state for Intel VT. For example, the guest maybe running "
                        "in big real mode\n"
                    "which is not supported on less recent Intel processors."
                        "\n\n");
        }
        ret = -1;
        break;
    case GVM_EXIT_EXCEPTION:
        fprintf(stderr, "GVM: exception %d exit (error code 0x%x)\n",
                run->ex.exception, run->ex.error_code);
        ret = -1;
        break;
    case GVM_EXIT_DEBUG:
        DPRINTF("gvm_exit_debug\n");
        qemu_mutex_lock_iothread();
        ret = gvm_handle_debug(cpu, &run->debug.arch);
        qemu_mutex_unlock_iothread();
        break;
    case GVM_EXIT_IOAPIC_EOI:
        ioapic_eoi_broadcast(run->eoi.vector);
        ret = 0;
        break;
    default:
        fprintf(stderr, "GVM: unknown exit reason %d\n", run->exit_reason);
        ret = -1;
        break;
    }

    return ret;
}

bool gvm_arch_stop_on_emulation_error(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;

    gvm_cpu_synchronize_state(cs);
    return !(env->cr[0] & CR0_PE_MASK) ||
           ((env->segs[R_CS].selector  & 3) != 3);
}

void gvm_arch_init_irq_routing(GVMState *s)
{
    if (!gvm_check_extension(s, GVM_CAP_IRQ_ROUTING)) {
        /* If kernel can't do irq routing, interrupt source
         * override 0->2 cannot be set up as required by HPET.
         * So we have to disable it.
         */
        no_hpet = 1;
    }
    /* We know at this point that we're using the in-kernel
     * irqchip, so we can use irqfds, and on x86 we know
     * we can use msi via irqfd and GSI routing.
     */
    // Disable in-kernel irq chip related codes til hypervisor side
    // is ready. However, we may completely remove the following
    // since it is unlikely to implement split irqchip for gvm at
    // the moment.
#if 0
    gvm_gsi_routing_allowed = true;

    if (gvm_irqchip_is_split()) {
        int i;

        /* If the ioapic is in QEMU and the lapics are in GVM, reserve
           MSI routes for signaling interrupts to the local apics. */
        for (i = 0; i < IOAPIC_NUM_PINS; i++) {
            if (gvm_irqchip_add_msi_route(s, 0, NULL) < 0) {
                error_report("Could not enable split IRQ mode.");
                exit(1);
            }
        }
    }
#endif
}

int gvm_arch_irqchip_create(MachineState *ms, GVMState *s)
{
    if (machine_kernel_irqchip_split(ms)) {
        error_report("Do not support split irqchip mode for GVM");
        exit(1);
    } else {
        return 0;
    }
}

