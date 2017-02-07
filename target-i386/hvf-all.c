#include "qemu/osdep.h"
#include "qemu-common.h"

#include "hvf-i386.h"
#include "hvf-utils/vmcs.h"
#include "hvf-utils/vmx.h"

#include <Hypervisor/hv.h>
#include <Hypervisor/hv_vmx.h>

#include "exec/address-spaces.h"
#include "exec/exec-all.h"
#include "exec/ioport.h"
#include "qemu/main-loop.h"
#include "strings.h"
#include "sysemu/accel.h"

static const char kHVFVcpuSyncFailed[] = "Failed to sync HVF vcpu context";

#define derror(msg) do { fprintf(stderr, (msg)); } while (0)

/* #define DEBUG_HVF */

#ifdef DEBUG_HVF
#define DPRINTF(fmt, ...) \
    do { fprintf(stdout, fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

/* Current version */
const uint32_t hvf_cur_version = 0x0; // version 0
/* Minimum  HVF kernel version */
const uint32_t hvf_min_version = 0x0;

#define TYPE_HVF_ACCEL ACCEL_CLASS_NAME("hvf")

static void hvf_vcpu_sync_state(CPUArchState * env, int modified);
static int hvf_arch_get_registers(CPUArchState * env);
static int hvf_handle_io(CPUArchState * env, uint32_t df, uint16_t port,
                         int direction, int size, int count, void *buffer);
struct hvf_state hvf_global;
int ret_hvf_init = 0;
static int hvf_disabled = 1;

typedef struct hvf_slot {
    uint64_t start;
    uint64_t size;
    uint8_t* mem;
    int slot_id;
} hvf_slot;

struct hvf_vcpu_caps {
    uint64_t vmx_cap_pinbased;
    uint64_t vmx_cap_procbased;
    uint64_t vmx_cap_procbased2;
    uint64_t vmx_cap_entry;
    uint64_t vmx_cap_exit;
    uint64_t vmx_cap_preemption_timer;
};

struct hvf_accel_state {
    AccelState parent;
    hvf_slot slots[32];
    int num_slots;
};

pthread_rwlock_t mem_lock = PTHREAD_RWLOCK_INITIALIZER;
struct hvf_accel_state* hvf_state;

int hvf_support = -1;

// Memory slots/////////////////////////////////////////////////////////////////

// TODO: Why????
hvf_slot *hvf_find_overlap_slot(uint64_t start, uint64_t end) {
    hvf_slot *slot;
    int x;
    for (x = 0; x < hvf_state->num_slots; ++x) {
        slot = &hvf_state->slots[x];
        if (slot->size && start < (slot->start + slot->size) && end >= slot->start)
            return slot;
    }
    return NULL;
}

struct mac_slot {
    int present;
    uint64_t size;
    uint64_t gpa_start;
    uint64_t gva;
};

struct mac_slot mac_slots[32];
#define ALIGN(x, y)  (((x)+(y)-1) & ~((y)-1))

int __hvf_set_memory(hvf_slot *slot) {
    // TODO: Why separate macslot?
    struct mac_slot *macslot;
    hv_memory_flags_t flags;
    pthread_rwlock_wrlock(&mem_lock);

    macslot = &mac_slots[slot->slot_id];

    if (macslot->present) {
        if (macslot->size != slot->size) {
            macslot->present = 0;
            if (hv_vm_unmap(macslot->gpa_start, macslot->size)) {
                printf("unmap failed\n");
                abort();
                return -1;
            }
        }
    }

    if (!slot->size) {
        pthread_rwlock_unlock(&mem_lock);
        return 0;
    }

    flags = HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC;
    
    macslot->present = 1;
    macslot->gpa_start = slot->start;
    macslot->size = slot->size;
    if (hv_vm_map((hv_uvaddr_t)slot->mem, slot->start, slot->size, flags)) {
        printf("register failed\n");
        abort();
        return -1;
    }
    pthread_rwlock_unlock(&mem_lock);
    return 0;
}

void hvf_set_phys_mem(MemoryRegionSection* section) {
    hvf_slot *mem;
    MemoryRegion *area = section->mr;

    if (!memory_region_is_ram(area)) return;

    mem = hvf_find_overlap_slot(
            section->offset_within_address_space,
            section->offset_within_address_space + int128_get64(section->size));

    if (mem) {
        if (mem->size == int128_get64(section->size) &&
                mem->start == section->offset_within_address_space &&
                mem->mem == (memory_region_get_ram_ptr(area) + section->offset_within_region))
            return; // Same region was attempted to register, go away.
    }

    // Region needs to be reset. set the size to 0 and remap it. 
    if (mem) {
        mem->size = 0;
        if (__hvf_set_memory(mem)) {
            fprintf(stderr, "error re-registering memory\n");
            abort();
        }
    }

    // Now make a new slot.
    int x;

    for (x = 0; x < hvf_state->num_slots; ++x) {
        mem = &hvf_state->slots[x];
        if (!mem->size)
            break;
    }

    if (x == hvf_state->num_slots) {
        printf("no free slots\n");
        abort();
    }

    mem->size = int128_get64(section->size);
    mem->mem = memory_region_get_ram_ptr(area) + section->offset_within_region;
    mem->start = section->offset_within_address_space;

    if (__hvf_set_memory(mem)) {
        fprintf(stderr, "error registering memory\n");
        abort();
    }
}

static void hvf_region_add(MemoryListener * listener,
                           MemoryRegionSection * section)
{
    hvf_set_phys_mem(section);
}

static void hvf_region_del(MemoryListener * listener,
                           MemoryRegionSection * section)
{
    // Memory mappings will be removed at VM close.
}

static MemoryListener hvf_memory_listener = {
    .region_add = hvf_region_add,
    .region_del = hvf_region_del,
};

static MemoryListener hvf_io_listener = { };

// VCPU init////////////////////////////////////////////////////////////////////

void vmx_reset_vcpu(CPUState *cpu) {
    uint64_t msr = 0xfee00000 | MSR_IA32_APICBASE_ENABLE;
    if (cpu_is_bsp(X86_CPU(cpu))) {
        msr |= MSR_IA32_APICBASE_BSP;
    }

    wvmcs(cpu->hvf_fd, VMCS_ENTRY_CTLS, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_IA32_EFER, 0);
    macvm_set_cr0(cpu->hvf_fd, 0x60000010);

    wvmcs(cpu->hvf_fd, VMCS_CR4_MASK, CR4_VMXE_MASK);
    wvmcs(cpu->hvf_fd, VMCS_CR4_SHADOW, 0x0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_CR4, CR4_VMXE_MASK);

    // set VMCS guest state fields
    wvmcs(cpu->hvf_fd, VMCS_GUEST_CS_SELECTOR, 0xf000);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_CS_LIMIT, 0xffff);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_CS_ACCESS_RIGHTS, 0x9b);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_CS_BASE, 0xffff0000);

    wvmcs(cpu->hvf_fd, VMCS_GUEST_DS_SELECTOR, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_DS_LIMIT, 0xffff);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_DS_ACCESS_RIGHTS, 0x93);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_DS_BASE, 0);

    wvmcs(cpu->hvf_fd, VMCS_GUEST_ES_SELECTOR, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_ES_LIMIT, 0xffff);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_ES_ACCESS_RIGHTS, 0x93);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_ES_BASE, 0);

    wvmcs(cpu->hvf_fd, VMCS_GUEST_FS_SELECTOR, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_FS_LIMIT, 0xffff);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_FS_ACCESS_RIGHTS, 0x93);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_FS_BASE, 0);

    wvmcs(cpu->hvf_fd, VMCS_GUEST_GS_SELECTOR, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_GS_LIMIT, 0xffff);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_GS_ACCESS_RIGHTS, 0x93);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_GS_BASE, 0);

    wvmcs(cpu->hvf_fd, VMCS_GUEST_SS_SELECTOR, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_SS_LIMIT, 0xffff);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_SS_ACCESS_RIGHTS, 0x93);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_SS_BASE, 0);

    wvmcs(cpu->hvf_fd, VMCS_GUEST_LDTR_SELECTOR, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_LDTR_LIMIT, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_LDTR_ACCESS_RIGHTS, 0x10000);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_LDTR_BASE, 0);

    wvmcs(cpu->hvf_fd, VMCS_GUEST_TR_SELECTOR, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_TR_LIMIT, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_TR_ACCESS_RIGHTS, 0x83);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_TR_BASE, 0);
    
    wvmcs(cpu->hvf_fd, VMCS_GUEST_GDTR_LIMIT, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_GDTR_BASE, 0);

    wvmcs(cpu->hvf_fd, VMCS_GUEST_IDTR_LIMIT, 0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_IDTR_BASE, 0);

    //wvmcs(cpu->hvf_fd, VMCS_GUEST_CR2, 0x0);
    wvmcs(cpu->hvf_fd, VMCS_GUEST_CR3, 0x0);

    wreg(cpu->hvf_fd, HV_X86_RIP, 0xfff0);
    wreg(cpu->hvf_fd, HV_X86_RDX, 0x623);
    wreg(cpu->hvf_fd, HV_X86_RFLAGS, 0x2);
    wreg(cpu->hvf_fd, HV_X86_RSP, 0x0);
    wreg(cpu->hvf_fd, HV_X86_RAX, 0x0);
    wreg(cpu->hvf_fd, HV_X86_RBX, 0x0);
    wreg(cpu->hvf_fd, HV_X86_RCX, 0x0);
    wreg(cpu->hvf_fd, HV_X86_RSI, 0x0);
    wreg(cpu->hvf_fd, HV_X86_RDI, 0x0);
    wreg(cpu->hvf_fd, HV_X86_RBP, 0x0);

    for (int i = 0; i < 8; i++)
         wreg(cpu->hvf_fd, HV_X86_R8+i, 0x0);

    // TODO: interface with goldfish acpi system
    // memset(cpu->apic_page, 0, 4096);
    // cpu->init_tsc = rdtscp();
    // if (!osx_is_sierra()) // Wonder if had to do with sierra bug?
    //     wvmcs(cpu->hvf_fd, VMCS_TSC_OFFSET, -cpu->init_tsc);

    hv_vm_sync_tsc(0);
    // cpu->hlt = 0;
    hv_vcpu_invalidate_tlb(cpu->hvf_fd);
    hv_vcpu_flush(cpu->hvf_fd);
}

int hvf_init_vcpu(CPUState * cpu) {

    // TODO: figure out instruction emulation needs for MMIO
    // Probably interface with TCG
    int r = hv_vcpu_create(&cpu->hvf_fd, HV_VCPU_DEFAULT);
    cpu->hvf_vcpu_dirty = 1;
    if (r)
        fprintf(stderr, "%s: create vcpu failed. errcode %d \n", __func__, r);

	if (hv_vmx_read_capability(HV_VMX_CAP_PINBASED, &cpu->hvf_caps->vmx_cap_pinbased))
		abort();
	if (hv_vmx_read_capability(HV_VMX_CAP_PROCBASED, &cpu->hvf_caps->vmx_cap_procbased))
		abort();
	if (hv_vmx_read_capability(HV_VMX_CAP_PROCBASED2, &cpu->hvf_caps->vmx_cap_procbased2))
		abort();
	if (hv_vmx_read_capability(HV_VMX_CAP_ENTRY, &cpu->hvf_caps->vmx_cap_entry))
		abort();

	/* set VMCS control fields */
    wvmcs(cpu->hvf_fd, VMCS_PIN_BASED_CTLS, cap2ctrl(cpu->hvf_caps->vmx_cap_pinbased, 0));
    wvmcs(cpu->hvf_fd, VMCS_PRI_PROC_BASED_CTLS, cap2ctrl(cpu->hvf_caps->vmx_cap_procbased,
                                                   VMCS_PRI_PROC_BASED_CTLS_HLT |
                                                   VMCS_PRI_PROC_BASED_CTLS_MWAIT |
                                                   VMCS_PRI_PROC_BASED_CTLS_TSC_OFFSET |
                                                   VMCS_PRI_PROC_BASED_CTLS_TPR_SHADOW) |
                                                   VMCS_PRI_PROC_BASED_CTLS_SEC_CONTROL);
	wvmcs(cpu->hvf_fd, VMCS_SEC_PROC_BASED_CTLS,
          cap2ctrl(cpu->hvf_caps->vmx_cap_procbased2,VMCS_PRI_PROC_BASED2_CTLS_APIC_ACCESSES));

	wvmcs(cpu->hvf_fd, VMCS_ENTRY_CTLS, cap2ctrl(cpu->hvf_caps->vmx_cap_entry, 0));
	wvmcs(cpu->hvf_fd, VMCS_EXCEPTION_BITMAP, 0); /* Double fault */

    wvmcs(cpu->hvf_fd, VMCS_TPR_THRESHOLD, 0);

    // TODO: hook into goldfish acpi defs.
    // addr_t apic_gpa = 0xfee00000;
    // if (!cpu->apic_page) {
    //     posix_memalign(&cpu->apic_page, 4096, 4096);
    //     memset(cpu->apic_page, 0, 4096);
    //     hv_vm_map((hv_uvaddr_t)cpu->apic_page, apic_gpa, 4096, HV_MEMORY_READ | HV_MEMORY_WRITE);
    //     hv_vmx_vcpu_set_apic_address(cpu->hvf_fd, apic_gpa);
    // }

    vmx_reset_vcpu(cpu);

    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_STAR, 1);
    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_LSTAR, 1);
    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_CSTAR, 1);
    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_FMASK, 1);
    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_FSBASE, 1);
    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_GSBASE, 1);
    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_KERNELGSBASE, 1);
    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_TSC_AUX, 1);
    //hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_IA32_TSC, 1);
    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_IA32_SYSENTER_CS, 1);
    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_IA32_SYSENTER_EIP, 1);
    hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_IA32_SYSENTER_ESP, 1);
    
    return 0;
}

// VCPU run/////////////////////////////////////////////////////////////////////

// TODO: import address_space_rw: we seem not to use attrs in our devices so we are ok.
// TODO: IO handler


// TODO: synchronize vcpu state


// TODO: ept fault handlig
// TODO: taskswitch handling
// TODO: tpr handler + apic tpr handler
// TODO: vcpu exec, one case at a time!

static void hvf_handle_interrupt(CPUState * cpu, int mask) {
    cpu->interrupt_request |= mask;
    if (!qemu_cpu_is_self(cpu))
        qemu_cpu_kick(cpu);
}

static int hvf_vcpu_hvf_exec(CPUArchState * env, int ug_platform) {
    return 0;
}

static int hvf_accel_init(MachineState *ms) {
    int x;
    int r = hv_vm_create(HV_VM_DEFAULT);

    if (r)
        fprintf(stderr, "%s: hv_vm_create failed with err code %d\n", __func__, r);

    struct hvf_accel_state* s =
        (struct hvf_accel_state*)malloc(sizeof(struct hvf_accel_state));

    s->num_slots = 32;
    for (x = 0; x < s->num_slots; ++x) {
        s->slots[x].size = 0;
        s->slots[x].slot_id = x;
    }

    hvf_state = s;
    cpu_interrupt_handler = hvf_handle_interrupt;
    memory_listener_register(&hvf_memory_listener, &address_space_memory);
    return 0;
}

bool hvf_allowed;

static void hvf_accel_class_init(ObjectClass *oc, void *data)
{
    AccelClass *ac = ACCEL_CLASS(oc);
    ac->name = "HVF";
    ac->init_machine = hvf_accel_init;
    ac->allowed = &hvf_allowed;
}

static const TypeInfo hvf_accel_type = {
    .name = TYPE_HVF_ACCEL,
    .parent = TYPE_ACCEL,
    .class_init = hvf_accel_class_init,
};

static void hvf_type_init(void)
{
    type_register_static(&hvf_accel_type);
}

type_init(hvf_type_init);

