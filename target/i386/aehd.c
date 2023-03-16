/*
 * QEMU AEHD support
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
#include "aehd_i386.h"
#include "sysemu/sysemu.h"
#include "sysemu/aehd_int.h"
#include "sysemu/hw_accel.h"

#include "exec/gdbstub.h"
#include "qemu/host-utils.h"
#include "qemu/config-file.h"
#include "qemu/error-report.h"
#include "hw/i386/pc.h"
#include "hw/i386/apic.h"
#include "hw/i386/apic_internal.h"
#include "hw/i386/apic-msidef.h"

#include "exec/ioport.h"
#include "hw/pci/pci.h"
#include "hw/pci/msi.h"
#include "migration/migration.h"
#include "exec/memattrs.h"

#ifdef DEBUG_AEHD
#define DPRINTF(fmt, ...) \
    do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

/* A 4096-byte buffer can hold the 8-byte aehd_msrs header, plus
 * 255 aehd_msr_entry structs */
#define MSR_BUF_SIZE 4096

#ifndef BUS_MCEERR_AR
#define BUS_MCEERR_AR 4
#endif
#ifndef BUS_MCEERR_AO
#define BUS_MCEERR_AO 5
#endif

const AEHDCapabilityInfo aehd_arch_required_capabilities[] = {
    AEHD_CAP_LAST_INFO
};

static bool has_msr_star;
static bool has_msr_hsave_pa;
static bool has_msr_tsc_aux;
static bool has_msr_tsc_adjust;
static bool has_msr_tsc_deadline;
static bool has_msr_feature_control;
static bool has_msr_misc_enable;
static bool has_msr_smbase;
static bool has_msr_bndcfgs;
static bool has_msr_mtrr;
static bool has_msr_xss;

static bool has_msr_architectural_pmu;
static uint32_t num_architectural_pmu_counters;

static int has_xsave;
static int has_xcrs;

static struct aehd_cpuid *cpuid_cache;

static int aehd_get_tsc(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;
    struct {
        struct aehd_msrs info;
        struct aehd_msr_entry entries[1];
    } msr_data;
    int ret;

    if (env->tsc_valid) {
        return 0;
    }

    msr_data.info.nmsrs = 1;
    msr_data.entries[0].index = MSR_IA32_TSC;
    env->tsc_valid = !runstate_is_running();

    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_GET_MSRS,
            &msr_data, sizeof(msr_data),
            &msr_data, sizeof(msr_data));
    if (ret < 0)
        return ret;
    else
        ret = msr_data.info.nmsrs;

    assert(ret == 1);
    env->tsc = msr_data.entries[0].data;
    return 0;
}

static inline void do_aehd_synchronize_tsc(CPUState *cpu, run_on_cpu_data arg)
{
    aehd_get_tsc(cpu);
}

void aehd_synchronize_all_tsc(void)
{
    CPUState *cpu;

    if (aehd_enabled()) {
        CPU_FOREACH(cpu) {
            run_on_cpu(cpu, do_aehd_synchronize_tsc, RUN_ON_CPU_NULL);
        }
    }
}

static struct aehd_cpuid *try_get_cpuid(AEHDState *s, int max)
{
    struct aehd_cpuid *cpuid;
    int r, size;

    size = sizeof(*cpuid) + max * sizeof(*cpuid->entries);
    cpuid = g_malloc0(size);
    cpuid->nent = max;
    r = aehd_ioctl(s, AEHD_GET_SUPPORTED_CPUID,
            cpuid, size, cpuid, size);
    if (r == 0 && cpuid->nent >= max) {
        r = -E2BIG;
    }
    if (r < 0) {
        if (r == -E2BIG) {
            g_free(cpuid);
            return NULL;
        } else {
            fprintf(stderr, "AEHD_GET_SUPPORTED_CPUID failed: %s\n",
                    strerror(-r));
            exit(1);
        }
    }
    return cpuid;
}

/* Run AEHD_GET_SUPPORTED_CPUID ioctl(), allocating a buffer large enough
 * for all entries.
 */
static struct aehd_cpuid *get_supported_cpuid(AEHDState *s)
{
    struct aehd_cpuid *cpuid;
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

/* Returns the value for a specific register on the cpuid entry
 */
static uint32_t cpuid_entry_get_reg(struct aehd_cpuid_entry *entry, int reg)
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

/* Find matching entry for function/index on aehd_cpuid struct
 */
static struct aehd_cpuid_entry *cpuid_find_entry(struct aehd_cpuid *cpuid,
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

uint32_t aehd_arch_get_supported_cpuid(AEHDState *s, uint32_t function,
                                      uint32_t index, int reg)
{
    struct aehd_cpuid *cpuid;
    uint32_t ret = 0;
    uint32_t cpuid_1_edx;
    bool found = false;

    cpuid = get_supported_cpuid(s);

    struct aehd_cpuid_entry *entry = cpuid_find_entry(cpuid, function, index);
    if (entry) {
        found = true;
        ret = cpuid_entry_get_reg(entry, reg);
    }

    /* Fixups for the data returned by AEHD, below */

    if (function == 1 && reg == R_ECX) {
        /* We can set the hypervisor flag, even if AEHD does not return it on
         * GET_SUPPORTED_CPUID
         */
        ret |= CPUID_EXT_HYPERVISOR;

        /* x2apic is reported by GET_SUPPORTED_CPUID, but it can't be enabled
         * without the in-kernel irqchip
         */
        if (!aehd_irqchip_in_kernel()) {
            ret &= ~CPUID_EXT_X2APIC;
        }
    } else if (function == 6 && reg == R_EAX) {
        ret |= CPUID_6_EAX_ARAT; /* safe to allow because of emulated APIC */
    } else if (function == 0x80000001 && reg == R_EDX) {
        /* On Intel, aehd returns cpuid according to the Intel spec,
         * so add missing bits according to the AMD spec:
         */
        cpuid_1_edx = aehd_arch_get_supported_cpuid(s, 1, 0, R_EDX);
        ret |= cpuid_1_edx & CPUID_EXT2_AMD_ALIASES;
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

unsigned long aehd_arch_vcpu_id(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    return cpu->apic_id;
}

static Error *invtsc_mig_blocker;

#define AEHD_MAX_CPUID_ENTRIES  100

int aehd_arch_init_vcpu(CPUState *cs)
{
    struct {
        struct aehd_cpuid cpuid;
        struct aehd_cpuid_entry entries[AEHD_MAX_CPUID_ENTRIES];
    } QEMU_PACKED cpuid_data;
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;
    uint32_t limit, i, j, cpuid_i;
    uint32_t unused;
    struct aehd_cpuid_entry *c;
    int r;
    Error *local_err = NULL;

    memset(&cpuid_data, 0, sizeof(cpuid_data));

    cpuid_i = 0;

    cpu_x86_cpuid(env, 0, 0, &limit, &unused, &unused, &unused);

    for (i = 0; i <= limit; i++) {
        if (cpuid_i == AEHD_MAX_CPUID_ENTRIES) {
            fprintf(stderr, "unsupported level value: 0x%x\n", limit);
            abort();
        }
        c = &cpuid_data.entries[cpuid_i++];

        switch (i) {
        case 2: {
            /* Keep reading function 2 till all the input is received */
            int times;

            c->function = i;
            c->flags = AEHD_CPUID_FLAG_STATEFUL_FUNC |
                       AEHD_CPUID_FLAG_STATE_READ_NEXT;
            cpu_x86_cpuid(env, i, 0, &c->eax, &c->ebx, &c->ecx, &c->edx);
            times = c->eax & 0xff;

            for (j = 1; j < times; ++j) {
                if (cpuid_i == AEHD_MAX_CPUID_ENTRIES) {
                    fprintf(stderr, "cpuid_data is full, no space for "
                            "cpuid(eax:2):eax & 0xf = 0x%x\n", times);
                    abort();
                }
                c = &cpuid_data.entries[cpuid_i++];
                c->function = i;
                c->flags = AEHD_CPUID_FLAG_STATEFUL_FUNC;
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
                c->flags = AEHD_CPUID_FLAG_SIGNIFCANT_INDEX;
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
                if (cpuid_i == AEHD_MAX_CPUID_ENTRIES) {
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
        if (cpuid_i == AEHD_MAX_CPUID_ENTRIES) {
            fprintf(stderr, "unsupported xlevel value: 0x%x\n", limit);
            abort();
        }
        c = &cpuid_data.entries[cpuid_i++];

        c->function = i;
        c->flags = 0;
        cpu_x86_cpuid(env, i, 0, &c->eax, &c->ebx, &c->ecx, &c->edx);
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
    r = aehd_vcpu_ioctl(cs, AEHD_SET_CPUID,
            &cpuid_data, sizeof(cpuid_data),
            NULL, 0);
    if (r) {
        return r;
    }

    if (has_xsave) {
        env->aehd_xsave_buf = qemu_memalign(4096, sizeof(struct aehd_xsave));
    }
    cpu->aehd_msr_buf = g_malloc0(MSR_BUF_SIZE);

    if (env->features[FEAT_1_EDX] & CPUID_MTRR) {
        has_msr_mtrr = true;
    }
    if (!(env->features[FEAT_8000_0001_EDX] & CPUID_EXT2_RDTSCP)) {
        has_msr_tsc_aux = false;
    }

    return 0;
}

void aehd_arch_reset_vcpu(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;

    env->exception_injected = -1;
    env->interrupt_injected = -1;
    env->xcr0 = 1;
    if (aehd_irqchip_in_kernel()) {
        env->mp_state = cpu_is_bsp(cpu) ? AEHD_MP_STATE_RUNNABLE :
                                          AEHD_MP_STATE_UNINITIALIZED;
    } else {
        env->mp_state = AEHD_MP_STATE_RUNNABLE;
    }
}

void aehd_arch_do_init_vcpu(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;

    /* APs get directly into wait-for-SIPI state.  */
    if (env->mp_state == AEHD_MP_STATE_UNINITIALIZED) {
        env->mp_state = AEHD_MP_STATE_INIT_RECEIVED;
    }
}

static int aehd_get_supported_msrs(AEHDState *s)
{
    static int aehd_supported_msrs;
    int ret = 0;
    unsigned long msr_list_size;

    /* first time */
    if (aehd_supported_msrs == 0) {
        struct aehd_msr_list msr_list, *aehd_msr_list;

        aehd_supported_msrs = -1;

        /* Obtain MSR list from AEHD.  These are the MSRs that we must
         * save/restore */
        msr_list.nmsrs = 0;
        ret = aehd_ioctl(s, AEHD_GET_MSR_INDEX_LIST,
                &msr_list, sizeof(msr_list),
                &msr_list, sizeof(msr_list));
        if (ret < 0 && ret != -E2BIG) {
            return ret;
        }

        msr_list_size = sizeof(msr_list) + msr_list.nmsrs *
                                              sizeof(msr_list.indices[0]);
        aehd_msr_list = g_malloc0(msr_list_size);

        aehd_msr_list->nmsrs = msr_list.nmsrs;
        ret = aehd_ioctl(s, AEHD_GET_MSR_INDEX_LIST,
                aehd_msr_list, msr_list_size,
                aehd_msr_list, msr_list_size);
        if (ret >= 0) {
            int i;

            for (i = 0; i < aehd_msr_list->nmsrs; i++) {
                if (aehd_msr_list->indices[i] == MSR_STAR) {
                    has_msr_star = true;
                    continue;
                }
                if (aehd_msr_list->indices[i] == MSR_VM_HSAVE_PA) {
                    has_msr_hsave_pa = true;
                    continue;
                }
                if (aehd_msr_list->indices[i] == MSR_TSC_AUX) {
                    has_msr_tsc_aux = true;
                    continue;
                }
                if (aehd_msr_list->indices[i] == MSR_TSC_ADJUST) {
                    has_msr_tsc_adjust = true;
                    continue;
                }
                if (aehd_msr_list->indices[i] == MSR_IA32_TSCDEADLINE) {
                    has_msr_tsc_deadline = true;
                    continue;
                }
                if (aehd_msr_list->indices[i] == MSR_IA32_SMBASE) {
                    has_msr_smbase = true;
                    continue;
                }
                if (aehd_msr_list->indices[i] == MSR_IA32_MISC_ENABLE) {
                    has_msr_misc_enable = true;
                    continue;
                }
                if (aehd_msr_list->indices[i] == MSR_IA32_BNDCFGS) {
                    has_msr_bndcfgs = true;
                    continue;
                }
                if (aehd_msr_list->indices[i] == MSR_IA32_XSS) {
                    has_msr_xss = true;
                    continue;
                }
            }
        }

        g_free(aehd_msr_list);
    }

    return ret;
}

static Notifier smram_machine_done;
static AEHDMemoryListener smram_listener;
static AddressSpace smram_address_space;
static MemoryRegion smram_as_root;
static MemoryRegion smram_as_mem;

static void register_smram_listener(Notifier *n, void *unused)
{
    MemoryRegion *smram =
        (MemoryRegion *) object_resolve_path("/machine/smram", NULL);

    /* Outer container... */
    memory_region_init(&smram_as_root, OBJECT(aehd_state), "mem-container-smram", ~0ull);
    memory_region_set_enabled(&smram_as_root, true);

    /* ... with two regions inside: normal system memory with low
     * priority, and...
     */
    memory_region_init_alias(&smram_as_mem, OBJECT(aehd_state), "mem-smram",
                             get_system_memory(), 0, ~0ull);
    memory_region_add_subregion_overlap(&smram_as_root, 0, &smram_as_mem, 0);
    memory_region_set_enabled(&smram_as_mem, true);

    if (smram) {
        /* ... SMRAM with higher priority */
        memory_region_add_subregion_overlap(&smram_as_root, 0, smram, 10);
        memory_region_set_enabled(smram, true);
    }

    address_space_init(&smram_address_space, &smram_as_root, "AEHD-SMRAM");
    aehd_memory_listener_register(aehd_state, &smram_listener,
                                 &smram_address_space, 1);
}

int aehd_arch_init(MachineState *ms, AEHDState *s)
{
    /* Allows up to 16M BIOSes. */
    uint64_t identity_base = 0xfeffc000;
    uint64_t tss_base;
    int ret;

    has_xsave = aehd_check_extension(s, AEHD_CAP_XSAVE);

    has_xcrs = aehd_check_extension(s, AEHD_CAP_XCRS);

    ret = aehd_get_supported_msrs(s);
    if (ret < 0) {
        return ret;
    }

    /*
     * On older Intel CPUs, AEHD uses vm86 mode to emulate 16-bit code directly.
     * In order to use vm86 mode, an EPT identity map and a TSS  are needed.
     * Since these must be part of guest physical memory, we need to allocate
     * them, both by setting their start addresses in the kernel and by
     * creating a corresponding e820 entry. We need 4 pages before the BIOS.
     */
    ret = aehd_vm_ioctl(s, AEHD_SET_IDENTITY_MAP_ADDR,
            &identity_base, sizeof(identity_base),
            NULL, 0);
    if (ret < 0)
        return ret;

    /* Set TSS base one page after EPT identity map. */
    tss_base = identity_base + 0x1000;
    ret = aehd_vm_ioctl(s, AEHD_SET_TSS_ADDR,
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

    if (object_dynamic_cast(OBJECT(ms), TYPE_PC_MACHINE) &&
        pc_machine_is_smm_enabled(PC_MACHINE(ms))) {
        smram_machine_done.notify = register_smram_listener;
        qemu_add_machine_init_done_notifier(&smram_machine_done);
    }
    return 0;
}

static void set_v8086_seg(struct aehd_segment *lhs, const SegmentCache *rhs)
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

static void set_seg(struct aehd_segment *lhs, const SegmentCache *rhs)
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

static void get_seg(SegmentCache *lhs, const struct aehd_segment *rhs)
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

static void aehd_getput_reg(__u64 *aehd_reg, target_ulong *qemu_reg, int set)
{
    if (set) {
        *aehd_reg = *qemu_reg;
    } else {
        *qemu_reg = *aehd_reg;
    }
}

static int aehd_getput_regs(X86CPU *cpu, int set)
{
    CPUX86State *env = &cpu->env;
    struct aehd_regs regs;
    int ret = 0;

    if (!set) {
        ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_GET_REGS,
                NULL, 0, &regs, sizeof(regs));
        if (ret < 0) {
            return ret;
        }
    }

    aehd_getput_reg(&regs.rax, &env->regs[R_EAX], set);
    aehd_getput_reg(&regs.rbx, &env->regs[R_EBX], set);
    aehd_getput_reg(&regs.rcx, &env->regs[R_ECX], set);
    aehd_getput_reg(&regs.rdx, &env->regs[R_EDX], set);
    aehd_getput_reg(&regs.rsi, &env->regs[R_ESI], set);
    aehd_getput_reg(&regs.rdi, &env->regs[R_EDI], set);
    aehd_getput_reg(&regs.rsp, &env->regs[R_ESP], set);
    aehd_getput_reg(&regs.rbp, &env->regs[R_EBP], set);
#ifdef TARGET_X86_64
    aehd_getput_reg(&regs.r8, &env->regs[8], set);
    aehd_getput_reg(&regs.r9, &env->regs[9], set);
    aehd_getput_reg(&regs.r10, &env->regs[10], set);
    aehd_getput_reg(&regs.r11, &env->regs[11], set);
    aehd_getput_reg(&regs.r12, &env->regs[12], set);
    aehd_getput_reg(&regs.r13, &env->regs[13], set);
    aehd_getput_reg(&regs.r14, &env->regs[14], set);
    aehd_getput_reg(&regs.r15, &env->regs[15], set);
#endif

    aehd_getput_reg(&regs.rflags, &env->eflags, set);
    aehd_getput_reg(&regs.rip, &env->eip, set);

    if (set) {
        ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_REGS,
                &regs, sizeof(regs), NULL, 0);
    }

    return ret;
}

static int aehd_put_fpu(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct aehd_fpu fpu;
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

    return aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_FPU,
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
    ((word_offset) * sizeof(((struct aehd_xsave *)0)->region[0]))

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

static int aehd_put_xsave(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    X86XSaveArea *xsave = env->aehd_xsave_buf;
    uint16_t cwd, swd, twd;
    int i;

    if (!has_xsave) {
        return aehd_put_fpu(cpu);
    }

    memset(xsave, 0, sizeof(struct aehd_xsave));
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
    return aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_XSAVE,
            xsave, sizeof(*xsave), NULL, 0);
}

static int aehd_put_xcrs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct aehd_xcrs xcrs = {};

    if (!has_xcrs) {
        return 0;
    }

    xcrs.nr_xcrs = 1;
    xcrs.flags = 0;
    xcrs.xcrs[0].xcr = 0;
    xcrs.xcrs[0].value = env->xcr0;
    return aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_XCRS,
            &xcrs, sizeof(xcrs), NULL, 0);
}

static int aehd_put_sregs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct aehd_sregs sregs;

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

    return aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_SREGS,
            &sregs, sizeof(sregs), NULL, 0);
}

static void aehd_msr_buf_reset(X86CPU *cpu)
{
    memset(cpu->aehd_msr_buf, 0, MSR_BUF_SIZE);
}

static void aehd_msr_entry_add(X86CPU *cpu, uint32_t index, uint64_t value)
{
    struct aehd_msrs *msrs = cpu->aehd_msr_buf;
    void *limit = ((void *)msrs) + MSR_BUF_SIZE;
    struct aehd_msr_entry *entry = &msrs->entries[msrs->nmsrs];

    assert((void *)(entry + 1) <= limit);

    entry->index = index;
    entry->reserved = 0;
    entry->data = value;
    msrs->nmsrs++;
}

static int aehd_put_tscdeadline_msr(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    int ret;

    if (!has_msr_tsc_deadline) {
        return 0;
    }

    aehd_msr_buf_reset(cpu);
    aehd_msr_entry_add(cpu, MSR_IA32_TSCDEADLINE, env->tsc_deadline);

    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_MSRS,
            cpu->aehd_msr_buf,
            sizeof(struct aehd_msrs) + sizeof(struct aehd_msr_entry),
            cpu->aehd_msr_buf, sizeof(struct aehd_msrs));
    if (ret < 0)
        return ret;
    else
        ret = cpu->aehd_msr_buf->nmsrs;

    assert(ret == 1);
    return 0;
}

/*
 * Provide a separate write service for the feature control MSR in order to
 * kick the VCPU out of VMXON or even guest mode on reset. This has to be done
 * before writing any other state because forcibly leaving nested mode
 * invalidates the VCPU state.
 */
static int aehd_put_msr_feature_control(X86CPU *cpu)
{
    int ret;

    if (!has_msr_feature_control) {
        return 0;
    }

    aehd_msr_buf_reset(cpu);
    aehd_msr_entry_add(cpu, MSR_IA32_FEATURE_CONTROL,
                      cpu->env.msr_ia32_feature_control);

    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_MSRS,
            cpu->aehd_msr_buf,
            sizeof(struct aehd_msrs) + sizeof(struct aehd_msr_entry),
            cpu->aehd_msr_buf, sizeof(struct aehd_msrs));
    if (ret < 0)
        return ret;
    else
        ret = cpu->aehd_msr_buf->nmsrs;

    assert(ret == 1);
    return 0;
}

static int aehd_put_msrs(X86CPU *cpu, int level)
{
    CPUX86State *env = &cpu->env;
    int i;
    int ret;

    aehd_msr_buf_reset(cpu);

    aehd_msr_entry_add(cpu, MSR_IA32_SYSENTER_CS, env->sysenter_cs);
    aehd_msr_entry_add(cpu, MSR_IA32_SYSENTER_ESP, env->sysenter_esp);
    aehd_msr_entry_add(cpu, MSR_IA32_SYSENTER_EIP, env->sysenter_eip);
    aehd_msr_entry_add(cpu, MSR_PAT, env->pat);
    if (has_msr_star) {
        aehd_msr_entry_add(cpu, MSR_STAR, env->star);
    }
    if (has_msr_hsave_pa) {
        aehd_msr_entry_add(cpu, MSR_VM_HSAVE_PA, env->vm_hsave);
    }
    if (has_msr_tsc_aux) {
        aehd_msr_entry_add(cpu, MSR_TSC_AUX, env->tsc_aux);
    }
    if (has_msr_tsc_adjust) {
        aehd_msr_entry_add(cpu, MSR_TSC_ADJUST, env->tsc_adjust);
    }
    if (has_msr_misc_enable) {
        aehd_msr_entry_add(cpu, MSR_IA32_MISC_ENABLE,
                          env->msr_ia32_misc_enable);
    }
    if (has_msr_smbase) {
        aehd_msr_entry_add(cpu, MSR_IA32_SMBASE, env->smbase);
    }
    if (has_msr_bndcfgs) {
        aehd_msr_entry_add(cpu, MSR_IA32_BNDCFGS, env->msr_bndcfgs);
    }
    if (has_msr_xss) {
        aehd_msr_entry_add(cpu, MSR_IA32_XSS, env->xss);
    }
#ifdef TARGET_X86_64
    aehd_msr_entry_add(cpu, MSR_CSTAR, env->cstar);
    aehd_msr_entry_add(cpu, MSR_KERNELGSBASE, env->kernelgsbase);
    aehd_msr_entry_add(cpu, MSR_FMASK, env->fmask);
    aehd_msr_entry_add(cpu, MSR_LSTAR, env->lstar);
#endif
    /*
     * The following MSRs have side effects on the guest or are too heavy
     * for normal writeback. Limit them to reset or full state updates.
     */
    if (level >= AEHD_PUT_RESET_STATE) {
        aehd_msr_entry_add(cpu, MSR_IA32_TSC, env->tsc);
        if (has_msr_architectural_pmu) {
            /* Stop the counter.  */
            aehd_msr_entry_add(cpu, MSR_CORE_PERF_FIXED_CTR_CTRL, 0);
            aehd_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_CTRL, 0);

            /* Set the counter values.  */
            for (i = 0; i < MAX_FIXED_COUNTERS; i++) {
                aehd_msr_entry_add(cpu, MSR_CORE_PERF_FIXED_CTR0 + i,
                                  env->msr_fixed_counters[i]);
            }
            for (i = 0; i < num_architectural_pmu_counters; i++) {
                aehd_msr_entry_add(cpu, MSR_P6_PERFCTR0 + i,
                                  env->msr_gp_counters[i]);
                aehd_msr_entry_add(cpu, MSR_P6_EVNTSEL0 + i,
                                  env->msr_gp_evtsel[i]);
            }
            aehd_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_STATUS,
                              env->msr_global_status);
            aehd_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_OVF_CTRL,
                              env->msr_global_ovf_ctrl);

            /* Now start the PMU.  */
            aehd_msr_entry_add(cpu, MSR_CORE_PERF_FIXED_CTR_CTRL,
                              env->msr_fixed_ctr_ctrl);
            aehd_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_CTRL,
                              env->msr_global_ctrl);
        }
        if (has_msr_mtrr) {
            uint64_t phys_mask = MAKE_64BIT_MASK(0, cpu->phys_bits);

            aehd_msr_entry_add(cpu, MSR_MTRRdefType, env->mtrr_deftype);
            aehd_msr_entry_add(cpu, MSR_MTRRfix64K_00000, env->mtrr_fixed[0]);
            aehd_msr_entry_add(cpu, MSR_MTRRfix16K_80000, env->mtrr_fixed[1]);
            aehd_msr_entry_add(cpu, MSR_MTRRfix16K_A0000, env->mtrr_fixed[2]);
            aehd_msr_entry_add(cpu, MSR_MTRRfix4K_C0000, env->mtrr_fixed[3]);
            aehd_msr_entry_add(cpu, MSR_MTRRfix4K_C8000, env->mtrr_fixed[4]);
            aehd_msr_entry_add(cpu, MSR_MTRRfix4K_D0000, env->mtrr_fixed[5]);
            aehd_msr_entry_add(cpu, MSR_MTRRfix4K_D8000, env->mtrr_fixed[6]);
            aehd_msr_entry_add(cpu, MSR_MTRRfix4K_E0000, env->mtrr_fixed[7]);
            aehd_msr_entry_add(cpu, MSR_MTRRfix4K_E8000, env->mtrr_fixed[8]);
            aehd_msr_entry_add(cpu, MSR_MTRRfix4K_F0000, env->mtrr_fixed[9]);
            aehd_msr_entry_add(cpu, MSR_MTRRfix4K_F8000, env->mtrr_fixed[10]);
            for (i = 0; i < MSR_MTRRcap_VCNT; i++) {
                /* The CPU GPs if we write to a bit above the physical limit of
                 * the host CPU (and AEHD emulates that)
                 */
                uint64_t mask = env->mtrr_var[i].mask;
                mask &= phys_mask;

                aehd_msr_entry_add(cpu, MSR_MTRRphysBase(i),
                                  env->mtrr_var[i].base);
                aehd_msr_entry_add(cpu, MSR_MTRRphysMask(i), mask);
            }
        }

        /* Note: MSR_IA32_FEATURE_CONTROL is written separately, see
         *       aehd_put_msr_feature_control. */
    }

    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_MSRS, cpu->aehd_msr_buf,
            sizeof(struct aehd_msrs) + cpu->aehd_msr_buf->nmsrs *
                sizeof(struct  aehd_msr_entry),
            cpu->aehd_msr_buf, sizeof(struct aehd_msrs));
    if (ret < 0) {
        return ret;
    }

    return 0;
}


static int aehd_get_fpu(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct aehd_fpu fpu;
    int i, ret;

    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_GET_FPU,
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

static int aehd_get_xsave(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    X86XSaveArea *xsave = env->aehd_xsave_buf;
    int ret, i;
    uint16_t cwd, swd, twd;

    if (!has_xsave) {
        return aehd_get_fpu(cpu);
    }

    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_GET_XSAVE,
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

static int aehd_get_xcrs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    int i, ret;
    struct aehd_xcrs xcrs;

    if (!has_xcrs) {
        return 0;
    }

    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_GET_XCRS,
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

static int aehd_get_sregs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct aehd_sregs sregs;
    uint32_t hflags;
    int bit, i, ret;

    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_GET_SREGS,
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

    /* changes to apic base and cr8/tpr are read back via aehd_arch_post_run */

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

static int aehd_get_msrs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct aehd_msr_entry *msrs = cpu->aehd_msr_buf->entries;
    int ret, i;
    uint64_t mtrr_top_bits;
    uint64_t bufsize;

    aehd_msr_buf_reset(cpu);

    aehd_msr_entry_add(cpu, MSR_IA32_SYSENTER_CS, 0);
    aehd_msr_entry_add(cpu, MSR_IA32_SYSENTER_ESP, 0);
    aehd_msr_entry_add(cpu, MSR_IA32_SYSENTER_EIP, 0);
    aehd_msr_entry_add(cpu, MSR_PAT, 0);
    if (has_msr_star) {
        aehd_msr_entry_add(cpu, MSR_STAR, 0);
    }
    if (has_msr_hsave_pa) {
        aehd_msr_entry_add(cpu, MSR_VM_HSAVE_PA, 0);
    }
    if (has_msr_tsc_aux) {
        aehd_msr_entry_add(cpu, MSR_TSC_AUX, 0);
    }
    if (has_msr_tsc_adjust) {
        aehd_msr_entry_add(cpu, MSR_TSC_ADJUST, 0);
    }
    if (has_msr_tsc_deadline) {
        aehd_msr_entry_add(cpu, MSR_IA32_TSCDEADLINE, 0);
    }
    if (has_msr_misc_enable) {
        aehd_msr_entry_add(cpu, MSR_IA32_MISC_ENABLE, 0);
    }
    if (has_msr_smbase) {
        aehd_msr_entry_add(cpu, MSR_IA32_SMBASE, 0);
    }
    if (has_msr_feature_control) {
        aehd_msr_entry_add(cpu, MSR_IA32_FEATURE_CONTROL, 0);
    }
    if (has_msr_bndcfgs) {
        aehd_msr_entry_add(cpu, MSR_IA32_BNDCFGS, 0);
    }
    if (has_msr_xss) {
        aehd_msr_entry_add(cpu, MSR_IA32_XSS, 0);
    }


    if (!env->tsc_valid) {
        aehd_msr_entry_add(cpu, MSR_IA32_TSC, 0);
        env->tsc_valid = !runstate_is_running();
    }

#ifdef TARGET_X86_64
    aehd_msr_entry_add(cpu, MSR_CSTAR, 0);
    aehd_msr_entry_add(cpu, MSR_KERNELGSBASE, 0);
    aehd_msr_entry_add(cpu, MSR_FMASK, 0);
    aehd_msr_entry_add(cpu, MSR_LSTAR, 0);
#endif
    if (has_msr_architectural_pmu) {
        aehd_msr_entry_add(cpu, MSR_CORE_PERF_FIXED_CTR_CTRL, 0);
        aehd_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_CTRL, 0);
        aehd_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_STATUS, 0);
        aehd_msr_entry_add(cpu, MSR_CORE_PERF_GLOBAL_OVF_CTRL, 0);
        for (i = 0; i < MAX_FIXED_COUNTERS; i++) {
            aehd_msr_entry_add(cpu, MSR_CORE_PERF_FIXED_CTR0 + i, 0);
        }
        for (i = 0; i < num_architectural_pmu_counters; i++) {
            aehd_msr_entry_add(cpu, MSR_P6_PERFCTR0 + i, 0);
            aehd_msr_entry_add(cpu, MSR_P6_EVNTSEL0 + i, 0);
        }
    }

    if (has_msr_mtrr) {
        aehd_msr_entry_add(cpu, MSR_MTRRdefType, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix64K_00000, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix16K_80000, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix16K_A0000, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix4K_C0000, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix4K_C8000, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix4K_D0000, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix4K_D8000, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix4K_E0000, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix4K_E8000, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix4K_F0000, 0);
        aehd_msr_entry_add(cpu, MSR_MTRRfix4K_F8000, 0);
        for (i = 0; i < MSR_MTRRcap_VCNT; i++) {
            aehd_msr_entry_add(cpu, MSR_MTRRphysBase(i), 0);
            aehd_msr_entry_add(cpu, MSR_MTRRphysMask(i), 0);
        }
    }

    bufsize = sizeof(struct aehd_msrs) + cpu->aehd_msr_buf->nmsrs *
        sizeof(struct aehd_msr_entry);
    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_GET_MSRS,
            cpu->aehd_msr_buf, bufsize,
            cpu->aehd_msr_buf, bufsize);
    if (ret < 0)
        return ret;
    else
        ret = cpu->aehd_msr_buf->nmsrs;

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

static int aehd_put_mp_state(X86CPU *cpu)
{
    struct aehd_mp_state mp_state = { .mp_state = cpu->env.mp_state };

    return aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_MP_STATE,
            &mp_state, sizeof(mp_state), NULL, 0);
}

static int aehd_get_mp_state(X86CPU *cpu)
{
    CPUState *cs = CPU(cpu);
    CPUX86State *env = &cpu->env;
    struct aehd_mp_state mp_state;
    int ret;

    ret = aehd_vcpu_ioctl(cs, AEHD_GET_MP_STATE,
            NULL, 0, &mp_state, sizeof(mp_state));
    if (ret < 0) {
        return ret;
    }
    env->mp_state = mp_state.mp_state;
    if (aehd_irqchip_in_kernel()) {
        cs->halted = (mp_state.mp_state == AEHD_MP_STATE_HALTED);
    }
    return 0;
}

static int aehd_get_apic(X86CPU *cpu)
{
    DeviceState *apic = cpu->apic_state;
    struct aehd_lapic_state aapic;
    int ret;

    if (apic && aehd_irqchip_in_kernel()) {
        ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_GET_LAPIC,
                NULL, 0, &aapic, sizeof(aapic));
        if (ret < 0) {
            return ret;
        }

        aehd_get_apic_state(apic, &aapic);
    }
    return 0;
}

static int aehd_put_apic(X86CPU *cpu)
{
    DeviceState *apic = cpu->apic_state;
    struct aehd_lapic_state aapic;

    if (apic && aehd_irqchip_in_kernel()) {
        aehd_put_apic_state(apic, &aapic);

        return aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_LAPIC,
                &aapic, sizeof(aapic), NULL, 0);
    }
    return 0;
}

static int aehd_put_vcpu_events(X86CPU *cpu, int level)
{
    CPUState *cs = CPU(cpu);
    CPUX86State *env = &cpu->env;
    struct aehd_vcpu_events events = {};

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
        if (aehd_irqchip_in_kernel()) {
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
        events.flags |= AEHD_VCPUEVENT_VALID_SMM;
    }

    events.flags = 0;
    if (level >= AEHD_PUT_RESET_STATE) {
        events.flags |=
            AEHD_VCPUEVENT_VALID_NMI_PENDING | AEHD_VCPUEVENT_VALID_SIPI_VECTOR;
    }

    return aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_VCPU_EVENTS,
            &events, sizeof(events), NULL, 0);
}

static int aehd_get_vcpu_events(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct aehd_vcpu_events events;
    int ret;

    memset(&events, 0, sizeof(events));
    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_GET_VCPU_EVENTS,
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

    if (events.flags & AEHD_VCPUEVENT_VALID_SMM) {
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

static int aehd_put_debugregs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct aehd_debugregs dbgregs;
    int i;

    for (i = 0; i < 4; i++) {
        dbgregs.db[i] = env->dr[i];
    }
    dbgregs.dr6 = env->dr[6];
    dbgregs.dr7 = env->dr[7];
    dbgregs.flags = 0;

    return aehd_vcpu_ioctl(CPU(cpu), AEHD_SET_DEBUGREGS,
            &dbgregs, sizeof(dbgregs), NULL, 0);
}

static int aehd_get_debugregs(X86CPU *cpu)
{
    CPUX86State *env = &cpu->env;
    struct aehd_debugregs dbgregs;
    int i, ret;

    ret = aehd_vcpu_ioctl(CPU(cpu), AEHD_GET_DEBUGREGS,
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

int aehd_arch_put_registers(CPUState *cpu, int level)
{
    X86CPU *x86_cpu = X86_CPU(cpu);
    int ret;

    assert(cpu_is_stopped(cpu) || qemu_cpu_is_self(cpu));

    if (level >= AEHD_PUT_RESET_STATE) {
        ret = aehd_put_msr_feature_control(x86_cpu);
        if (ret < 0) {
            return ret;
        }
    }

    ret = aehd_getput_regs(x86_cpu, 1);
    if (ret < 0) {
        return ret;
    }
    ret = aehd_put_xsave(x86_cpu);
    if (ret < 0) {
        return ret;
    }
    ret = aehd_put_xcrs(x86_cpu);
    if (ret < 0) {
        return ret;
    }
    ret = aehd_put_sregs(x86_cpu);
    if (ret < 0) {
        return ret;
    }
    ret = aehd_put_msrs(x86_cpu, level);
    if (ret < 0) {
        return ret;
    }
    if (level >= AEHD_PUT_RESET_STATE) {
        ret = aehd_put_mp_state(x86_cpu);
        if (ret < 0) {
            return ret;
        }
        ret = aehd_put_apic(x86_cpu);
        if (ret < 0) {
            return ret;
        }
    }

    ret = aehd_put_tscdeadline_msr(x86_cpu);
    if (ret < 0) {
        return ret;
    }

    ret = aehd_put_vcpu_events(x86_cpu, level);
    if (ret < 0) {
        return ret;
    }
    ret = aehd_put_debugregs(x86_cpu);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int aehd_arch_get_registers(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    int ret;

    assert(cpu_is_stopped(cs) || qemu_cpu_is_self(cs));

    ret = aehd_getput_regs(cpu, 0);
    if (ret < 0) {
        goto out;
    }
    ret = aehd_get_xsave(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = aehd_get_xcrs(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = aehd_get_sregs(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = aehd_get_msrs(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = aehd_get_mp_state(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = aehd_get_apic(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = aehd_get_vcpu_events(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = aehd_get_debugregs(cpu);
    if (ret < 0) {
        goto out;
    }
    ret = 0;
 out:
    cpu_sync_bndcs_hflags(&cpu->env);
    return ret;
}

void aehd_arch_pre_run(CPUState *cpu, struct aehd_run *run)
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
            ret = aehd_vcpu_ioctl(cpu, AEHD_NMI, NULL, 0, NULL, 0);
            if (ret < 0) {
                fprintf(stderr, "AEHD: injection failed, NMI lost (%s)\n",
                        strerror(-ret));
            }
        }
        if (cpu->interrupt_request & CPU_INTERRUPT_SMI) {
            qemu_mutex_lock_iothread();
            cpu->interrupt_request &= ~CPU_INTERRUPT_SMI;
            qemu_mutex_unlock_iothread();
            DPRINTF("injected SMI\n");
            ret = aehd_vcpu_ioctl(cpu, AEHD_SMI, NULL, 0, NULL, 0);
            if (ret < 0) {
                fprintf(stderr, "AEHD: injection failed, SMI lost (%s)\n",
                        strerror(-ret));
            }
        }
    }

    if (!aehd_pic_in_kernel()) {
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

    if (!aehd_pic_in_kernel()) {
        /* Try to inject an interrupt if the guest can accept it */
        if (run->ready_for_interrupt_injection &&
            (cpu->interrupt_request & CPU_INTERRUPT_HARD) &&
            (env->eflags & IF_MASK)) {
            int irq;

            cpu->interrupt_request &= ~CPU_INTERRUPT_HARD;
            irq = cpu_get_pic_interrupt(env);
            if (irq >= 0) {
                struct aehd_interrupt intr;

                intr.irq = irq;
                DPRINTF("injected interrupt %d\n", irq);
                ret = aehd_vcpu_ioctl(cpu, AEHD_INTERRUPT,
                        &intr, sizeof(intr), NULL, 0);
                if (ret < 0) {
                    fprintf(stderr,
                            "AEHD: injection failed, interrupt lost (%s)\n",
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

MemTxAttrs aehd_arch_post_run(CPUState *cpu, struct aehd_run *run)
{
    X86CPU *x86_cpu = X86_CPU(cpu);
    CPUX86State *env = &x86_cpu->env;

    if (run->flags & AEHD_RUN_X86_SMM) {
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
    if (!aehd_irqchip_in_kernel()) {
        qemu_mutex_lock_iothread();
    }
    cpu_set_apic_tpr(x86_cpu->apic_state, run->cr8);
    cpu_set_apic_base(x86_cpu->apic_state, run->apic_base);
    if (!aehd_irqchip_in_kernel()) {
        qemu_mutex_unlock_iothread();
    }
    return cpu_get_mem_attrs(env);
}

int aehd_arch_process_async_events(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;

    if (cs->interrupt_request & CPU_INTERRUPT_MCE) {
        /* We must not raise CPU_INTERRUPT_MCE if it's not supported. */
        assert(env->mcg_cap);

        cs->interrupt_request &= ~CPU_INTERRUPT_MCE;

        aehd_cpu_synchronize_state(cs);

        if (env->exception_injected == EXCP08_DBLE) {
            /* this means triple fault */
            qemu_system_reset_request(SHUTDOWN_CAUSE_GUEST_RESET);
            cs->exit_request = 1;
            return 0;
        }
        env->exception_injected = EXCP12_MCHK;
        env->has_error_code = 0;

        cs->halted = 0;
        if (aehd_irqchip_in_kernel() && env->mp_state == AEHD_MP_STATE_HALTED) {
            env->mp_state = AEHD_MP_STATE_RUNNABLE;
        }
    }

    if ((cs->interrupt_request & CPU_INTERRUPT_INIT) &&
        !(env->hflags & HF_SMM_MASK)) {
        aehd_cpu_synchronize_state(cs);
        do_cpu_init(cpu);
    }

    if (aehd_irqchip_in_kernel()) {
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
        aehd_cpu_synchronize_state(cs);
        do_cpu_sipi(cpu);
    }
    if (cs->interrupt_request & CPU_INTERRUPT_TPR) {
        cs->interrupt_request &= ~CPU_INTERRUPT_TPR;
        aehd_cpu_synchronize_state(cs);
        apic_handle_tpr_access_report(cpu->apic_state, env->eip,
                                      env->tpr_access_type);
    }

    return cs->halted;
}

static int aehd_handle_halt(X86CPU *cpu)
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

static int aehd_handle_tpr_access(X86CPU *cpu)
{
    CPUState *cs = CPU(cpu);
    struct aehd_run *run = cs->aehd_run;

    apic_handle_tpr_access_report(cpu->apic_state, run->tpr_access.rip,
                                  run->tpr_access.is_write ? TPR_ACCESS_WRITE
                                                           : TPR_ACCESS_READ);
    return 1;
}

int aehd_arch_insert_sw_breakpoint(CPUState *cs, struct aehd_sw_breakpoint *bp)
{
    static const uint8_t int3 = 0xcc;

    if (cpu_memory_rw_debug(cs, bp->pc, (uint8_t *)&bp->saved_insn, 1, 0) ||
        cpu_memory_rw_debug(cs, bp->pc, (uint8_t *)&int3, 1, 1)) {
        return -EINVAL;
    }
    return 0;
}

int aehd_arch_remove_sw_breakpoint(CPUState *cs, struct aehd_sw_breakpoint *bp)
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

int aehd_arch_insert_hw_breakpoint(target_ulong addr,
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

int aehd_arch_remove_hw_breakpoint(target_ulong addr,
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

void aehd_arch_remove_all_hw_breakpoints(void)
{
    nb_hw_breakpoint = 0;
}

static CPUWatchpoint hw_watchpoint;

static int aehd_handle_debug(X86CPU *cpu,
                            struct aehd_debug_exit_arch *arch_info)
{
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
                        hw_watchpoint.vaddress = hw_breakpoint[n].addr;
                        hw_watchpoint.flags = BP_MEM_WRITE;
                        break;
                    case 0x3:
                        ret = EXCP_DEBUG;
                        cs->watchpoint_hit = &hw_watchpoint;
                        hw_watchpoint.vaddress = hw_breakpoint[n].addr;
                        hw_watchpoint.flags = BP_MEM_ACCESS;
                        break;
                    }
                }
            }
        }
    } else if (aehd_find_sw_breakpoint(cs, arch_info->pc)) {
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
}

void aehd_arch_update_guest_debug(CPUState *cpu, struct aehd_guest_debug *dbg)
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

    if (aehd_sw_breakpoints_active(cpu)) {
        dbg->control |= AEHD_GUESTDBG_ENABLE | AEHD_GUESTDBG_USE_SW_BP;
    }
    if (nb_hw_breakpoint > 0) {
        dbg->control |= AEHD_GUESTDBG_ENABLE | AEHD_GUESTDBG_USE_HW_BP;
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

int aehd_arch_handle_exit(CPUState *cs, struct aehd_run *run)
{
    X86CPU *cpu = X86_CPU(cs);
    uint64_t code;
    int ret;

    switch (run->exit_reason) {
    case AEHD_EXIT_HLT:
        DPRINTF("handle_hlt\n");
        qemu_mutex_lock_iothread();
        ret = aehd_handle_halt(cpu);
        qemu_mutex_unlock_iothread();
        break;
    case AEHD_EXIT_SET_TPR:
        ret = 0;
        break;
    case AEHD_EXIT_TPR_ACCESS:
        qemu_mutex_lock_iothread();
        ret = aehd_handle_tpr_access(cpu);
        qemu_mutex_unlock_iothread();
        break;
    case AEHD_EXIT_FAIL_ENTRY:
        code = run->fail_entry.hardware_entry_failure_reason;
        fprintf(stderr, "AEHD: entry failed, hardware error 0x%" PRIx64 "\n",
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
    case AEHD_EXIT_EXCEPTION:
        fprintf(stderr, "AEHD: exception %d exit (error code 0x%x)\n",
                run->ex.exception, run->ex.error_code);
        ret = -1;
        break;
    case AEHD_EXIT_DEBUG:
        DPRINTF("aehd_exit_debug\n");
        qemu_mutex_lock_iothread();
        ret = aehd_handle_debug(cpu, &run->debug.arch);
        qemu_mutex_unlock_iothread();
        break;
    case AEHD_EXIT_IOAPIC_EOI:
        ioapic_eoi_broadcast(run->eoi.vector);
        ret = 0;
        break;
    default:
        fprintf(stderr, "AEHD: unknown exit reason %d\n", run->exit_reason);
        ret = -1;
        break;
    }

    return ret;
}

bool aehd_arch_stop_on_emulation_error(CPUState *cs)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;

    aehd_cpu_synchronize_state(cs);
    return !(env->cr[0] & CR0_PE_MASK) ||
           ((env->segs[R_CS].selector  & 3) != 3);
}

int aehd_arch_irqchip_create(MachineState *ms, AEHDState *s)
{
    return 0;
}

typedef struct MSIRouteEntry MSIRouteEntry;

struct MSIRouteEntry {
    PCIDevice *dev;             /* Device pointer */
    int vector;                 /* MSI/MSIX vector index */
    int virq;                   /* Virtual IRQ index */
    QLIST_ENTRY(MSIRouteEntry) list;
};

/* List of used GSI routes */
static QLIST_HEAD(, MSIRouteEntry) msi_route_list = \
    QLIST_HEAD_INITIALIZER(msi_route_list);

int aehd_arch_add_msi_route_post(struct aehd_irq_routing_entry *route,
                                int vector, PCIDevice *dev)
{
    MSIRouteEntry *entry;

    if (!dev) {
        /* These are (possibly) IOAPIC routes only used for split
         * kernel irqchip mode, while what we are housekeeping are
         * PCI devices only. */
        return 0;
    }

    entry = g_new0(MSIRouteEntry, 1);
    entry->dev = dev;
    entry->vector = vector;
    entry->virq = route->gsi;
    QLIST_INSERT_HEAD(&msi_route_list, entry, list);
    return 0;
}

int aehd_arch_release_virq_post(int virq)
{
    MSIRouteEntry *entry, *next;
    QLIST_FOREACH_SAFE(entry, &msi_route_list, list, next) {
        if (entry->virq == virq) {
            QLIST_REMOVE(entry, list);
            break;
        }
    }
    return 0;
}

