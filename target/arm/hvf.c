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
#include "qemu/osdep.h"
#include "qemu-common.h"

#include <Hypervisor/Hypervisor.h>

#include "hvf-arm64.h"

#include "internals.h"

#include "exec/address-spaces.h"
#include "exec/exec-all.h"
#include "exec/ioport.h"
#include "exec/ram_addr.h"
#include "exec/memory-remap.h"
#include "qemu/main-loop.h"
#include "qemu/abort.h"
#include "strings.h"
#include "sysemu/accel.h"
#include "sysemu/sysemu.h"

static const char kHVFVcpuSyncFailed[] = "Failed to sync HVF vcpu context";

#define derror(msg) do { fprintf(stderr, (msg)); } while (0)

#define HVF_CHECKED_CALL(c) do { \
        int ret = c; \
        if (ret != HV_SUCCESS) { \
            fprintf(stderr, "%s:%d hv error: [" #c "] err 0x%x" "\n", __func__, __LINE__, (uint32_t)ret); \
        } \
    } while(0) \

#define DEBUG_HVF

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
void hvf_handle_io( uint16_t port, void* buffer, int direction, int size, int count);

struct hvf_state hvf_global;
int ret_hvf_init = 0;
static int hvf_disabled = 1;

#define HVF_MAX_SLOTS 512

typedef struct hvf_slot {
    uint64_t start;
    uint64_t size;
    uint8_t* mem;
    int slot_id;
} hvf_slot;

struct hvf_vcpu_caps {
    // TODO-convert-to-arm64
    uint64_t vmx_cap_pinbased;
    uint64_t vmx_cap_procbased;
    uint64_t vmx_cap_procbased2;
    uint64_t vmx_cap_entry;
    uint64_t vmx_cap_exit;
    uint64_t vmx_cap_preemption_timer;
};

struct hvf_accel_state {
    AccelState parent;
    hvf_slot slots[HVF_MAX_SLOTS];
    int num_slots;
};

pthread_rwlock_t mem_lock = PTHREAD_RWLOCK_INITIALIZER;
struct hvf_accel_state* hvf_state;

int hvf_support = -1;

bool check_hvf_ok(int r) {
    if (r == HV_SUCCESS) {
        return true;
    }

    switch (r) {
      case HV_ERROR:
        fprintf(stderr, "HVF error: HV_ERROR\n");
        break;
      case HV_BUSY:
        fprintf(stderr, "HVF error: HV_BUSY\n");
        break;
      case HV_BAD_ARGUMENT:
        fprintf(stderr, "HVF error: HV_BAD_ARGUMENT\n");
        break;
      case HV_NO_RESOURCES:
        fprintf(stderr, "HVF error: HV_NO_RESOURCES\n");
        break;
      case HV_NO_DEVICE:
        fprintf(stderr, "HVF error: HV_NO_DEVICE\n");
        break;
      case HV_UNSUPPORTED:
        fprintf(stderr, "HVF error: HV_UNSUPPORTED\n");
        break;
      case HV_DENIED:
        fprintf(stderr, "HVF error: HV_DENIED\n");
        break;
      default:
        fprintf(stderr, "HVF Unknown error 0x%x\n", r);
        break;
    }
    return false;
}

void assert_hvf_ok(int r) {
    if (check_hvf_ok(r)) return;

    qemu_abort("HVF fatal error\n");
}

// Memory slots/////////////////////////////////////////////////////////////////

hvf_slot *hvf_find_overlap_slot(uint64_t start, uint64_t end) {
    hvf_slot *slot;
    int x;
    for (x = 0; x < hvf_state->num_slots; ++x) {
        slot = &hvf_state->slots[x];
        if (slot->size && start < (slot->start + slot->size) && end > slot->start)
            return slot;
    }
    return NULL;
}

struct mac_slot {
    int present;
    uint64_t size;
    uint64_t gpa_start;
    uint64_t gva;
    void* hva;
};

struct mac_slot mac_slots[HVF_MAX_SLOTS];

#define ALIGN(x, y)  (((x)+(y)-1) & ~((y)-1))

void* hvf_gpa2hva(uint64_t gpa, bool* found) {
    struct mac_slot *mslot;
    *found = false;

    for (uint32_t i = 0; i < HVF_MAX_SLOTS; i++) {
        mslot = &mac_slots[i];
        if (!mslot->present) continue;
        if (gpa >= mslot->gpa_start &&
            gpa < mslot->gpa_start + mslot->size) {
            *found = true;
            return (void*)((char*)(mslot->hva) + (gpa - mslot->gpa_start));
        }
    }

    return 0;
}

#define min(a, b) ((a) < (b) ? (a) : (b))
int hvf_hva2gpa(void* hva, uint64_t length, int array_size,
                uint64_t* gpa, uint64_t* size) {
    struct mac_slot *mslot;
    int count = 0;

    for (uint32_t i = 0; i < HVF_MAX_SLOTS; i++) {
        mslot = &mac_slots[i];
        if (!mslot->present) continue;

        uintptr_t hva_start_num = (uintptr_t)mslot->hva;
        uintptr_t hva_num = (uintptr_t)hva;
        // Start of this hva region is in this slot.
        if (hva_num >= hva_start_num &&
            hva_num < hva_start_num + mslot->size) {
            if (count < array_size) {
                gpa[count] = mslot->gpa_start + (hva_num - hva_start_num);
                size[count] = min(length,
                                  mslot->size - (hva_num - hva_start_num));
            }
            count++;
        // End of this hva region is in this slot.
        // Its start is outside of this slot.
        } else if (hva_num + length <= hva_start_num + mslot->size &&
                   hva_num + length > hva_start_num) {
            if (count < array_size) {
                gpa[count] = mslot->gpa_start;
                size[count] = hva_num + length - hva_start_num;
            }
            count++;
        // This slot belongs to this hva region completely.
        } else if (hva_num + length > hva_start_num +  mslot->size &&
                   hva_num < hva_start_num)  {
            if (count < array_size) {
                gpa[count] = mslot->gpa_start;
                size[count] = mslot->size;
            }
            count++;
        }
    }
    return count;
}

hvf_slot* hvf_next_free_slot() {
    hvf_slot* mem = 0;
    int x;

    for (x = 0; x < hvf_state->num_slots; ++x) {
        mem = &hvf_state->slots[x];
        if (!mem->size) {
            return mem;
        }
    }

    return mem;
}

int __hvf_set_memory(hvf_slot *slot);
int __hvf_set_memory_with_flags_locked(hvf_slot *slot, hv_memory_flags_t flags);

int hvf_map_safe(void* hva, uint64_t gpa, uint64_t size, uint64_t flags) {
    pthread_rwlock_wrlock(&mem_lock);
    DPRINTF("%s: hva: [%p 0x%llx] gpa: [0x%llx 0x%llx]\n", __func__,
            hva, (unsigned long long)(uintptr_t)(((char*)hva) + size),
            (unsigned long long)gpa,
            (unsigned long long)gpa + size);

    hvf_slot *mem;
    mem = hvf_find_overlap_slot(gpa, gpa + size);

    if (mem &&
        mem->mem == hva &&
        mem->start == gpa &&
        mem->size == size) {

        pthread_rwlock_unlock(&mem_lock);
        return HV_SUCCESS;
    } else if (mem &&
        mem->start == gpa &&
        mem->size == size) {
        // unmap existing mapping, but only if it coincides
        mem->size = 0;
        __hvf_set_memory_with_flags_locked(mem, 0);
    } else if (mem) {
        // TODO: Manage and support partially-overlapping user-backed RAM mappings.
        // for now, consider it fatal.
        pthread_rwlock_unlock(&mem_lock);
        qemu_abort("%s: FATAL: tried to map [0x%llx 0x%llx) to %p "
                   "while it was mapped to [0x%llx 0x%llx), %p",
                   __func__,
                   (unsigned long long)gpa,
                   (unsigned long long)gpa + size,
                   hva,
                   (unsigned long long)mem->start,
                   (unsigned long long)mem->start + mem->size,
                   mem->mem);
    }

    mem = hvf_next_free_slot();

    if (mem->size) {
        qemu_abort("%s: no free slots\n", __func__);
    }

    mem->mem = (uint8_t*)hva;
    mem->start = gpa;
    mem->size = size;

    int res = __hvf_set_memory_with_flags_locked(mem, (hv_memory_flags_t)flags);

    pthread_rwlock_unlock(&mem_lock);
    return res;
}

int hvf_unmap_safe(uint64_t gpa, uint64_t size) {
    DPRINTF("%s: gpa: [0x%llx 0x%llx]\n", __func__,
            (unsigned long long)gpa,
            (unsigned long long)gpa + size);
    pthread_rwlock_wrlock(&mem_lock);

    hvf_slot *mem;
    int res = 0;
    mem = hvf_find_overlap_slot(gpa, gpa + size);

    if (mem &&
        (mem->start != gpa ||
         mem->size != size)) {

        pthread_rwlock_unlock(&mem_lock);

        qemu_abort("%s: tried to unmap [0x%llx 0x%llx) but partially overlapping "
                   "[0x%llx 0x%llx), %p was encountered",
                   __func__, gpa, gpa + size,
                   mem->start, mem->start + mem->size, mem->mem);
    } else if (mem) {
        mem->size = 0;
        res = __hvf_set_memory_with_flags_locked(mem, 0);
    } else {
        // fall through, allow res to be 0 still if slot was not found.
    }

    pthread_rwlock_unlock(&mem_lock);
    return res;
}

int hvf_protect_safe(uint64_t gpa, uint64_t size, uint64_t flags) {
    pthread_rwlock_wrlock(&mem_lock);
    int res = hv_vm_protect(gpa, size, flags);
    pthread_rwlock_unlock(&mem_lock);
    return res;
}

int hvf_remap_safe(void* hva, uint64_t gpa, uint64_t size, uint64_t flags) {
    pthread_rwlock_wrlock(&mem_lock);
    int res = hv_vm_unmap(gpa, size);
    check_hvf_ok(res);
    res = hv_vm_map(hva, gpa, size, flags);
    check_hvf_ok(res);
    pthread_rwlock_unlock(&mem_lock);
    return res;
}

// API for adding and removing mappings of guest RAM and host addrs.
// Implementation depends on the hypervisor.
static hv_memory_flags_t user_backed_flags_to_hvf_flags(int flags) {
    hv_memory_flags_t hvf_flags = 0;
    if (flags & USER_BACKED_RAM_FLAGS_READ) {
        hvf_flags |= HV_MEMORY_READ;
    }
    if (flags & USER_BACKED_RAM_FLAGS_WRITE) {
        hvf_flags |= HV_MEMORY_WRITE;
    }
    if (flags & USER_BACKED_RAM_FLAGS_EXEC) {
        hvf_flags |= HV_MEMORY_EXEC;
    }
    return hvf_flags;
}

static void hvf_user_backed_ram_map(uint64_t gpa, void* hva, uint64_t size, int flags) {
    hvf_map_safe(hva, gpa, size, user_backed_flags_to_hvf_flags(flags));
}

static void hvf_user_backed_ram_unmap(uint64_t gpa, uint64_t size) {
    hvf_unmap_safe(gpa, size);
}

int __hvf_set_memory(hvf_slot *slot) {
    pthread_rwlock_wrlock(&mem_lock);
    int res = __hvf_set_memory_with_flags_locked(slot, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
    pthread_rwlock_unlock(&mem_lock);
    return res;
}

int __hvf_set_memory_with_flags_locked(hvf_slot *slot, hv_memory_flags_t flags) {
    struct mac_slot *macslot;

    macslot = &mac_slots[slot->slot_id];

    if (macslot->present) {
        if (macslot->size != slot->size) {
            macslot->present = 0;
            DPRINTF("%s: hv_vm_unmap for gpa [0x%llx 0x%llx]\n", __func__,
                    (unsigned long long)macslot->gpa_start,
                    (unsigned long long)(macslot->gpa_start + macslot->size));
            int unmapres = hv_vm_unmap(macslot->gpa_start, macslot->size);
            assert_hvf_ok(unmapres);
        }
    }

    if (!slot->size) {
        return 0;
    }

    macslot->present = 1;
    macslot->gpa_start = slot->start;
    macslot->size = slot->size;
    macslot->hva = slot->mem;
    DPRINTF("%s: hv_vm_map for hva 0x%llx gpa [0x%llx 0x%llx]\n", __func__,
            (unsigned long long)(slot->mem),
            (unsigned long long)macslot->gpa_start,
            (unsigned long long)(macslot->gpa_start + macslot->size));
    int mapres = (hv_vm_map(slot->mem, slot->start, slot->size, flags));
    assert_hvf_ok(mapres);
    return 0;
}

void hvf_set_phys_mem(MemoryRegionSection* section, bool add) {
    hvf_slot *mem;
    MemoryRegion *area = section->mr;

    if (!memory_region_is_ram(area)) return;
    if (memory_region_is_user_backed(area)) return;

    mem = hvf_find_overlap_slot(
            section->offset_within_address_space,
            section->offset_within_address_space + int128_get64(section->size));

    if (mem && add) {
        if (mem->size == int128_get64(section->size) &&
                mem->start == section->offset_within_address_space &&
                mem->mem == (memory_region_get_ram_ptr(area) + section->offset_within_region))
            return; // Same region was attempted to register, go away.
    }

    // Region needs to be reset. set the size to 0 and remap it.
    if (mem) {
        mem->size = 0;
        if (__hvf_set_memory(mem)) {
            qemu_abort("%s: Failed to reset overlapping slot\n", __func__);
        }
    }

    if (!add) return;

    // Now make a new slot.
    int x;

    mem = hvf_next_free_slot();

    if (!mem) {
        qemu_abort("%s: no free slots\n", __func__);
    }

    mem->size = int128_get64(section->size);
    mem->mem = memory_region_get_ram_ptr(area) + section->offset_within_region;
    mem->start = section->offset_within_address_space;

    if (__hvf_set_memory(mem)) {
        qemu_abort("%s: error regsitering new memory slot\n", __func__);
    }
}

static void hvf_region_add(MemoryListener * listener,
                           MemoryRegionSection * section)
{
    DPRINTF("%s: call. for [0x%llx 0x%llx]\n", __func__,
             (unsigned long long)section->offset_within_address_space,
             (unsigned long long)(section->offset_within_address_space + int128_get64(section->size)));
    hvf_set_phys_mem(section, true);
    DPRINTF("%s: done\n", __func__);
}

static void hvf_region_del(MemoryListener * listener,
                           MemoryRegionSection * section)
{
    DPRINTF("%s: call. for [0x%llx 0x%llx]\n", __func__,
             (unsigned long long)section->offset_within_address_space,
             (unsigned long long)(section->offset_within_address_space + int128_get64(section->size)));
    hvf_set_phys_mem(section, false);
}

static MemoryListener hvf_memory_listener = {
    .priority = 10,
    .region_add = hvf_region_add,
    .region_del = hvf_region_del,
};

static MemoryListener hvf_io_listener = {
    .priority = 10,
};

// VCPU init////////////////////////////////////////////////////////////////////

int hvf_enabled() { return !hvf_disabled; }
void hvf_disable(int shouldDisable) {
    hvf_disabled = shouldDisable;
}

void vmx_reset_vcpu(CPUState *cpu) {

    // TODO-convert-to-arm64
    // wvmcs(cpu->hvf_fd, VMCS_ENTRY_CTLS, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_IA32_EFER, 0);
    // macvm_set_cr0(cpu->hvf_fd, 0x60000010);

    // wvmcs(cpu->hvf_fd, VMCS_CR4_MASK, CR4_VMXE_MASK);
    // wvmcs(cpu->hvf_fd, VMCS_CR4_SHADOW, 0x0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_CR4, CR4_VMXE_MASK);

    // // set VMCS guest state fields
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_CS_SELECTOR, 0xf000);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_CS_LIMIT, 0xffff);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_CS_ACCESS_RIGHTS, 0x9b);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_CS_BASE, 0xffff0000);

    // wvmcs(cpu->hvf_fd, VMCS_GUEST_DS_SELECTOR, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_DS_LIMIT, 0xffff);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_DS_ACCESS_RIGHTS, 0x93);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_DS_BASE, 0);

    // wvmcs(cpu->hvf_fd, VMCS_GUEST_ES_SELECTOR, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_ES_LIMIT, 0xffff);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_ES_ACCESS_RIGHTS, 0x93);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_ES_BASE, 0);

    // wvmcs(cpu->hvf_fd, VMCS_GUEST_FS_SELECTOR, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_FS_LIMIT, 0xffff);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_FS_ACCESS_RIGHTS, 0x93);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_FS_BASE, 0);

    // wvmcs(cpu->hvf_fd, VMCS_GUEST_GS_SELECTOR, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_GS_LIMIT, 0xffff);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_GS_ACCESS_RIGHTS, 0x93);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_GS_BASE, 0);

    // wvmcs(cpu->hvf_fd, VMCS_GUEST_SS_SELECTOR, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_SS_LIMIT, 0xffff);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_SS_ACCESS_RIGHTS, 0x93);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_SS_BASE, 0);

    // wvmcs(cpu->hvf_fd, VMCS_GUEST_LDTR_SELECTOR, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_LDTR_LIMIT, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_LDTR_ACCESS_RIGHTS, 0x10000);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_LDTR_BASE, 0);

    // wvmcs(cpu->hvf_fd, VMCS_GUEST_TR_SELECTOR, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_TR_LIMIT, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_TR_ACCESS_RIGHTS, 0x83);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_TR_BASE, 0);

    // wvmcs(cpu->hvf_fd, VMCS_GUEST_GDTR_LIMIT, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_GDTR_BASE, 0);

    // wvmcs(cpu->hvf_fd, VMCS_GUEST_IDTR_LIMIT, 0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_IDTR_BASE, 0);

    // //wvmcs(cpu->hvf_fd, VMCS_GUEST_CR2, 0x0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_CR3, 0x0);
    // wvmcs(cpu->hvf_fd, VMCS_GUEST_DR7, 0x0);

    // wreg(cpu->hvf_fd, HV_X86_RIP, 0xfff0);
    // wreg(cpu->hvf_fd, HV_X86_RDX, 0x623);
    // wreg(cpu->hvf_fd, HV_X86_RFLAGS, 0x2);
    // wreg(cpu->hvf_fd, HV_X86_RSP, 0x0);
    // wreg(cpu->hvf_fd, HV_X86_RAX, 0x0);
    // wreg(cpu->hvf_fd, HV_X86_RBX, 0x0);
    // wreg(cpu->hvf_fd, HV_X86_RCX, 0x0);
    // wreg(cpu->hvf_fd, HV_X86_RSI, 0x0);
    // wreg(cpu->hvf_fd, HV_X86_RDI, 0x0);
    // wreg(cpu->hvf_fd, HV_X86_RBP, 0x0);

    // for (int i = 0; i < 8; i++)
    //      wreg(cpu->hvf_fd, HV_X86_R8+i, 0x0);

    // hv_vm_sync_tsc(0);
    // cpu->halted = 0;
    // hv_vcpu_invalidate_tlb(cpu->hvf_fd);
    // hv_vcpu_flush(cpu->hvf_fd);
}

int hvf_init_vcpu(CPUState * cpu) {
    DPRINTF("%s: entry. cpu: %p\n", __func__, cpu);

    ARMCPU *armcpu;

    int r;
    // TODO-convert-to-arm64
    // init_emu(cpu);
    // init_decoder(cpu);
    // init_cpuid(cpu);

    cpu->hvf_caps = (struct hvf_vcpu_caps*)g_malloc0(sizeof(struct hvf_vcpu_caps));
    cpu->hvf_arm64 = (struct hvf_arm64_state*)g_malloc0(sizeof(struct hvf_arm64_state));
    DPRINTF("%s: create a vcpu config and query its values\n", __func__);

    uint64_t configval;
    hv_vcpu_config_t config = hv_vcpu_config_create();

#define PRINT_FEATURE_REGISTER(r) \
    HVF_CHECKED_CALL(hv_vcpu_config_get_feature_reg(config, r, &configval)); \
    DPRINTF("%s: value of %s: 0x%llx\n", __func__, #r, (unsigned long long)configval); \

    LIST_HVF_FEATURE_REGISTERS(PRINT_FEATURE_REGISTER)

    DPRINTF("%s: attempt hv_vcpu_create\n", __func__);
    r = hv_vcpu_create(&cpu->hvf_fd, &cpu->hvf_vcpu_exit_info, 0);

    bool debugExceptionQuery;
    HVF_CHECKED_CALL(hv_vcpu_get_trap_debug_exceptions(cpu->hvf_fd, &debugExceptionQuery));
    DPRINTF("%s: Do debug excecptions exit the guest? %d\n", __func__, debugExceptionQuery);
    DPRINTF("%s: Setting debug exceptions to not exit the guest...\n", __func__);
    HVF_CHECKED_CALL(hv_vcpu_set_trap_debug_exceptions(cpu->hvf_fd, false));

    HVF_CHECKED_CALL(hv_vcpu_get_trap_debug_reg_accesses(cpu->hvf_fd, &debugExceptionQuery));
    DPRINTF("%s: Do debug register accesses exit the guest? %d\n", __func__, debugExceptionQuery);
    DPRINTF("%s: Setting debug register accesses to not exit the guest...\n", __func__);
    HVF_CHECKED_CALL(hv_vcpu_set_trap_debug_reg_accesses(cpu->hvf_fd, false));

    // DPRINTF("%s: Setting pc to 0x8a0\n", __func__);
    // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_REG_PC, 0x40000000ULL));

    cpu->hvf_vcpu_dirty = 1;
    assert_hvf_ok(r);

    // TODO-convert-to-arm64
	// if (hv_vmx_read_capability(HV_VMX_CAP_PINBASED, &cpu->hvf_caps->vmx_cap_pinbased))
	// 	qemu_abort("%s: error getting vmx capability HV_VMX_CAP_PINBASED\n", __func__);
	// if (hv_vmx_read_capability(HV_VMX_CAP_PROCBASED, &cpu->hvf_caps->vmx_cap_procbased))
	// 	qemu_abort("%s: error getting vmx capability HV_VMX_CAP_PROCBASED\n", __func__);
	// if (hv_vmx_read_capability(HV_VMX_CAP_PROCBASED2, &cpu->hvf_caps->vmx_cap_procbased2))
	// 	qemu_abort("%s: error getting vmx capability HV_VMX_CAP_PROCBASED2\n", __func__);
	// if (hv_vmx_read_capability(HV_VMX_CAP_ENTRY, &cpu->hvf_caps->vmx_cap_entry))
	// 	qemu_abort("%s: error getting vmx capability HV_VMX_CAP_ENTRY\n", __func__);

	// /* set VMCS control fields */
    // wvmcs(cpu->hvf_fd, VMCS_PIN_BASED_CTLS, cap2ctrl(cpu->hvf_caps->vmx_cap_pinbased, 0));
    // wvmcs(cpu->hvf_fd, VMCS_PRI_PROC_BASED_CTLS, cap2ctrl(cpu->hvf_caps->vmx_cap_procbased,
    //                                                VMCS_PRI_PROC_BASED_CTLS_HLT |
    //                                                VMCS_PRI_PROC_BASED_CTLS_MWAIT |
    //                                                VMCS_PRI_PROC_BASED_CTLS_TSC_OFFSET |
    //                                                VMCS_PRI_PROC_BASED_CTLS_TPR_SHADOW) |
    //                                                VMCS_PRI_PROC_BASED_CTLS_SEC_CONTROL);
	// wvmcs(cpu->hvf_fd, VMCS_SEC_PROC_BASED_CTLS,
    //       cap2ctrl(cpu->hvf_caps->vmx_cap_procbased2,VMCS_PRI_PROC_BASED2_CTLS_APIC_ACCESSES));

	// wvmcs(cpu->hvf_fd, VMCS_ENTRY_CTLS, cap2ctrl(cpu->hvf_caps->vmx_cap_entry, 0));
	// wvmcs(cpu->hvf_fd, VMCS_EXCEPTION_BITMAP, 0); /* Double fault */

    // wvmcs(cpu->hvf_fd, VMCS_TPR_THRESHOLD, 0);

    // vmx_reset_vcpu(cpu);

    armcpu = ARM_CPU(cpu);
    // x86cpu->env.kvm_xsave_buf = qemu_memalign(16384, sizeof(struct hvf_xsave_buf));

    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_STAR, 1);
    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_LSTAR, 1);
    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_CSTAR, 1);
    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_FMASK, 1);
    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_FSBASE, 1);
    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_GSBASE, 1);
    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_KERNELGSBASE, 1);
    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_TSC_AUX, 1);
    // //hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_IA32_TSC, 1);
    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_IA32_SYSENTER_CS, 1);
    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_IA32_SYSENTER_EIP, 1);
    // hv_vcpu_enable_native_msr(cpu->hvf_fd, MSR_IA32_SYSENTER_ESP, 1);

    return 0;
}

// VCPU run/////////////////////////////////////////////////////////////////////

int hvf_vcpu_emulation_mode(CPUState* cpu) {
    fprintf(stderr, "%s: call\n", __func__);
    CPUArchState *env = (CPUArchState *) (cpu->env_ptr);
    // TODO-convert-to-arm64
    // return !(env->cr[0] & CR0_PG_MASK);
    return 0;
}

int hvf_vcpu_destroy(CPUState* cpu) {
    fprintf(stderr, "%s: call\n", __func__);
    return 0;
}

void hvf_raise_event(CPUState* cpu) {
    fprintf(stderr, "%s: call\n", __func__);
    // TODO
}

// TODO-convert-to-arm64
void hvf_inject_interrupts(CPUState *cpu) {
    fprintf(stderr, "%s: call\n", __func__);
}

// TODO-convert-to-arm64
int hvf_process_events(CPUState *cpu) {
    fprintf(stderr, "%s: call\n", __func__);
    return 0;
}
static hv_reg_t regno_to_hv_xreg(int i) {
    switch (i) {
        case 0: return HV_REG_X0;
        case 1: return HV_REG_X1;
        case 2: return HV_REG_X2;
        case 3: return HV_REG_X3;
        case 4: return HV_REG_X4;
        case 5: return HV_REG_X5;
        case 6: return HV_REG_X6;
        case 7: return HV_REG_X7;
        case 8: return HV_REG_X8;
        case 9: return HV_REG_X9;
        case 10: return HV_REG_X10;
        case 11: return HV_REG_X11;
        case 12: return HV_REG_X12;
        case 13: return HV_REG_X13;
        case 14: return HV_REG_X14;
        case 15: return HV_REG_X15;
        case 16: return HV_REG_X16;
        case 17: return HV_REG_X17;
        case 18: return HV_REG_X18;
        case 19: return HV_REG_X19;
        case 20: return HV_REG_X20;
        case 21: return HV_REG_X21;
        case 22: return HV_REG_X22;
        case 23: return HV_REG_X23;
        case 24: return HV_REG_X24;
        case 25: return HV_REG_X25;
        case 26: return HV_REG_X26;
        case 27: return HV_REG_X27;
        case 28: return HV_REG_X28;
        case 29: return HV_REG_X29;
        case 30: return HV_REG_X30;
        default:
            return HV_REG_X30;
    }
}

static hv_simd_fp_reg_t regno_to_hv_simd_fp_reg_type(int i) {
    switch (i) {
        case 0:
            return HV_SIMD_FP_REG_Q0;
        case 1:
            return HV_SIMD_FP_REG_Q1;
        case 2:
            return HV_SIMD_FP_REG_Q2;
        case 3:
            return HV_SIMD_FP_REG_Q3;
        case 4:
            return HV_SIMD_FP_REG_Q4;
        case 5:
            return HV_SIMD_FP_REG_Q5;
        case 6:
            return HV_SIMD_FP_REG_Q6;
        case 7:
            return HV_SIMD_FP_REG_Q7;
        case 8:
            return HV_SIMD_FP_REG_Q8;
        case 9:
            return HV_SIMD_FP_REG_Q9;
        case 10:
            return HV_SIMD_FP_REG_Q10;
        case 11:
            return HV_SIMD_FP_REG_Q11;
        case 12:
            return HV_SIMD_FP_REG_Q12;
        case 13:
            return HV_SIMD_FP_REG_Q13;
        case 14:
            return HV_SIMD_FP_REG_Q14;
        case 15:
            return HV_SIMD_FP_REG_Q15;
        case 16:
            return HV_SIMD_FP_REG_Q16;
        case 17:
            return HV_SIMD_FP_REG_Q17;
        case 18:
            return HV_SIMD_FP_REG_Q18;
        case 19:
            return HV_SIMD_FP_REG_Q19;
        case 20:
            return HV_SIMD_FP_REG_Q20;
        case 21:
            return HV_SIMD_FP_REG_Q21;
        case 22:
            return HV_SIMD_FP_REG_Q22;
        case 23:
            return HV_SIMD_FP_REG_Q23;
        case 24:
            return HV_SIMD_FP_REG_Q24;
        case 25:
            return HV_SIMD_FP_REG_Q25;
        case 26:
            return HV_SIMD_FP_REG_Q26;
        case 27:
            return HV_SIMD_FP_REG_Q27;
        case 28:
            return HV_SIMD_FP_REG_Q28;
        case 29:
            return HV_SIMD_FP_REG_Q29;
        case 30:
            return HV_SIMD_FP_REG_Q20;
        case 31:
            return HV_SIMD_FP_REG_Q31;
    }
}

int hvf_put_registers(CPUState *cpu) {
    int i;
    int ret;
    unsigned int el;

    fprintf(stderr, "%s: call\n", __func__);
    ARMCPU *armcpu = ARM_CPU(cpu);
    CPUARMState *env = &armcpu->env;

    /* If we are in AArch32 mode then we need to copy the AArch32 regs to the
     * AArch64 registers before pushing them out to 64-bit HVF.
     */
    if (!is_a64(env)) {
        fprintf(stderr, "%s: syncing 32 to 64!\n", __func__);
        aarch64_sync_32_to_64(env);
    }

    // Set HVF general registers
    {
        // HV_REG_LR = HV_REG_X30,
        // HV_REG_FP = HV_REG_X29,

        for (i = 0; i < 31; ++i) {
            HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, regno_to_hv_xreg(i), env->xregs[i]));
            DPRINTF("%s: xregs[%d]: 0x%llx\n", __func__, i, (unsigned long long)env->regs[i]);
        }
    }

    // Set SP
    {
        aarch64_save_sp(env, 1);

        DPRINTF("%s: HV_SYS_REG_SP_EL0 0x%llx\n", __func__, (unsigned long long)env->sp_el[0]);
        DPRINTF("%s: HV_SYS_REG_SP_EL1 0x%llx\n", __func__, (unsigned long long)env->sp_el[1]);

        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_SP_EL0, env->sp_el[0]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_SP_EL1, env->sp_el[1]));
    }

    // Set pstate
    {
        // Note:
        if (is_a64(env)) {
            DPRINTF("%s: HV_REG_CPSR 0x%llx (a64)\n", __func__, (unsigned long long)pstate_read(env));
            HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_REG_CPSR, pstate_read(env)));
        } else {
            DPRINTF("%s: HV_REG_CPSR 0x%llx\n", __func__, (unsigned long long)cpsr_read(env));
            HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_REG_CPSR, cpsr_read(env)));
        }
    }

    // Set PC
    {
        DPRINTF("%s: HV_REG_PC 0x%llx\n", __func__, (unsigned long long)env->pc);
        HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_REG_PC, env->pc));
    }

    // Set ELR_EL1
    {
        DPRINTF("%s: HV_SYS_REG_ELR_EL1 0x%llx\n", __func__, (unsigned long long)env->elr_el[1]);
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_ELR_EL1, env->elr_el[1]));
    }

    // Set SPSR
    {
        /* Saved Program State Registers
         *
         * Before we restore from the banked_spsr[] array we need to
         * ensure that any modifications to env->spsr are correctly
         * reflected in the banks.
         */
        el = arm_current_el(env);
        if (el > 0 && !is_a64(env)) {
            i = bank_number(env->uncached_cpsr & CPSR_M);
            env->banked_spsr[i] = env->spsr;
        }

        DPRINTF("%s: HV_SYS_REG_SPSR_EL1 0x%llx\n", __func__, env->banked_spsr[aarch64_banked_spsr_index(/* el */ 1)]);
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_SPSR_EL1, env->banked_spsr[aarch64_banked_spsr_index(/* el */ 1)]));
    }

    // Set HVF SIMD FP registers
    {
        hv_simd_fp_uchar16_t val;
        for (i = 0; i < 32; ++i) {
            memcpy(&val, aa64_vfp_qreg(env, i), sizeof(hv_simd_fp_uchar16_t));
            HVF_CHECKED_CALL(hv_vcpu_set_simd_fp_reg(cpu->hvf_fd, regno_to_hv_simd_fp_reg_type(i), val));
        }

        HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_REG_FPSR, vfp_get_fpsr(env)));
        HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_REG_FPCR, vfp_get_fpcr(env)));
    }

    // Set other HVF system registers
    {
        // TODO: Pointer auth
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_APDAKEYHI_EL1, env->keys.apda.hi));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_APDAKEYLO_EL1, env->keys.apda.lo));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_APDBKEYHI_EL1, env->keys.apdb.hi));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_APDBKEYLO_EL1, env->keys.apdb.lo));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_APGAKEYHI_EL1, env->keys.apga.hi));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_APGAKEYLO_EL1, env->keys.apga.lo));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_APIAKEYHI_EL1, env->keys.apia.hi));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_APIAKEYLO_EL1, env->keys.apia.lo));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_APIBKEYHI_EL1, env->keys.apib.hi));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_APIBKEYLO_EL1, env->keys.apib.lo));

        // TODO: Allow debug
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR0_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR10_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR11_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR12_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR13_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR14_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR15_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR1_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR2_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR3_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR4_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR5_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR6_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR7_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR8_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR9_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR0_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR10_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR11_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR12_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR13_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR14_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR15_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR1_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR2_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR3_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR4_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR5_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR6_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR7_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR8_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR9_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR0_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR10_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR11_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR12_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR13_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR14_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR15_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR1_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR2_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR3_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR4_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR5_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR6_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR7_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR8_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR9_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR0_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR10_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR11_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR12_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR13_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR14_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR15_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR1_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR2_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR3_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR4_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR5_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR6_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR7_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR8_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR9_EL1, ???));

        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_MDCCINT_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_AFSR0_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_AFSR1_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_AMAIR_EL1, ???));

        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_CNTKCTL_EL1, env->cp15.c14_cntkctl));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_CNTV_CVAL_EL0, ???));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_CONTEXTIDR_EL1, env->cp15.contextidr_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_CPACR_EL1, env->cp15.cpacr_el1));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_CSSELR_EL1, env->cp15.csselr_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_ESR_EL1, env->cp15.esr_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_FAR_EL1, env->cp15.far_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_MAIR_EL1, env->cp15.mair_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_MDSCR_EL1, env->cp15.mdscr_el1));

        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_MIDR_EL1);
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_MPIDR_EL1);

        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_PAR_EL1, env->cp15.par_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_SCTLR_EL1, env->cp15.sctlr_el[1]));

        DPRINTF("%s: HV_SYS_REG_TCR_EL1 0x%llx\n", __func__, (unsigned long long)env->cp15.tcr_el[1].raw_tcr);
        DPRINTF("%s: HV_SYS_REG_TPIDRRO_EL0 0x%llx\n", __func__, (unsigned long long)env->cp15.tpidrro_el[0]);

        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_TCR_EL1, env->cp15.tcr_el[1].raw_tcr));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_TPIDRRO_EL0, env->cp15.tpidrro_el[0]));

        DPRINTF("%s: HV_SYS_REG_TPIDR_EL0 0x%llx\n", __func__, (unsigned long long)env->cp15.tpidr_el[0]);
        DPRINTF("%s: HV_SYS_REG_TPIDR_EL1 0x%llx\n", __func__, (unsigned long long)env->cp15.tpidr_el[1]);
        DPRINTF("%s: HV_SYS_REG_TTBR0_EL1 0x%llx\n", __func__, (unsigned long long)env->cp15.ttbr0_el[1]);
        DPRINTF("%s: HV_SYS_REG_TTBR1_EL1 0x%llx\n", __func__, (unsigned long long)env->cp15.ttbr1_el[1]);
        DPRINTF("%s: HV_SYS_REG_VBAR_EL1 0x%llx\n", __func__, (unsigned long long)env->cp15.vbar_el[1]);

        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_TPIDR_EL0, env->cp15.tpidr_el[0]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_TPIDR_EL1, env->cp15.tpidr_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_TTBR0_EL1, env->cp15.ttbr0_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_TTBR1_EL1, env->cp15.ttbr1_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_set_sys_reg(cpu->hvf_fd, HV_SYS_REG_VBAR_EL1, env->cp15.vbar_el[1]));

        // TODO: Did we save pstate in CPSR/SPSR correctly?
    }

    return 0;
}

int hvf_get_registers(CPUState *cpu) {
    fprintf(stderr, "%s: call\n", __func__);
    uint64_t val;
    unsigned int el;
    int i;
    ARMCPU *armcpu = ARM_CPU(cpu);
    CPUARMState *env = &armcpu->env;

    // Get general registers first
    {
        // HV_REG_LR = HV_REG_X30,
        // HV_REG_FP = HV_REG_X29,
        for (i = 0; i < 31; ++i) {
            HVF_CHECKED_CALL(hv_vcpu_get_reg(cpu->hvf_fd, regno_to_hv_xreg(i), &env->xregs[i]));
        }
    }

    // Get SP
    {
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_SP_EL0, &env->sp_el[0]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_SP_EL1, &env->sp_el[1]));
    }

    // Get pstate. This tells us if we are in aarch64 mode.
    {
        HVF_CHECKED_CALL(hv_vcpu_get_reg(cpu->hvf_fd, HV_REG_CPSR, &val));
        DPRINTF("%s: HV_REG_CPSR 0x%llx\n", __func__,
                (unsigned long long)val);
        env->aarch64 = ((val & PSTATE_nRW) == 0);

        if (is_a64(env)) {
            pstate_write(env, val);
        } else {
            cpsr_write(env, val, 0xffffffff, CPSRWriteRaw);
        }
    }

    /* KVM puts SP_EL0 in regs.sp and SP_EL1 in regs.sp_el1. On the
     * QEMU side we keep the current SP in xregs[31] as well.
     */
    aarch64_restore_sp(env, 1);

    // Get the PC.
    {
        HVF_CHECKED_CALL(hv_vcpu_get_reg(cpu->hvf_fd, HV_REG_PC, &env->pc));
    }

    /* If we are in AArch32 mode then we need to sync the AArch32 regs with the
     * incoming AArch64 regs received from 64-bit KVM.
     * We must perform this after all of the registers have been acquired from
     * the kernel.
     */
    if (!is_a64(env)) {
        aarch64_sync_64_to_32(env);
    }

    // Get ELR_EL
    {
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_ELR_EL1, &env->elr_el[1]));
    }

    // Get SPSR
    {
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_SPSR_EL1, &env->banked_spsr[aarch64_banked_spsr_index(/* el */ 1)]));

        /* Saved Program State Registers
         *
         * Before we restore from the banked_spsr[] array we need to
         * ensure that any modifications to env->spsr are correctly
         * reflected in the banks.
         */
        el = arm_current_el(env);
        if (el > 0 && !is_a64(env)) {
            i = bank_number(env->uncached_cpsr & CPSR_M);
            env->spsr = env->banked_spsr[i];
        }

    }

    // SIMD/FP registers

    {
        hv_simd_fp_uchar16_t val;
        for (i = 0; i < 32; ++i) {
            HVF_CHECKED_CALL(hv_vcpu_get_simd_fp_reg(cpu->hvf_fd, regno_to_hv_simd_fp_reg_type(i), &val));
            memcpy(aa64_vfp_qreg(env, i), &val, sizeof(hv_simd_fp_uchar16_t));
        }

        {
            uint32_t val;
            HVF_CHECKED_CALL(hv_vcpu_get_reg(cpu->hvf_fd, HV_REG_FPSR, &val));
            vfp_set_fpsr(env, val);
            HVF_CHECKED_CALL(hv_vcpu_get_reg(cpu->hvf_fd, HV_REG_FPCR, &val));
            vfp_set_fpcr(env, val);
        }
    }

    // Get other HVF system registers
    {
        // TODO: Pointer auth
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_APDAKEYHI_EL1, &env->keys.apda.hi));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_APDAKEYLO_EL1, &env->keys.apda.lo));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_APDBKEYHI_EL1, &env->keys.apdb.hi));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_APDBKEYLO_EL1, &env->keys.apdb.lo));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_APGAKEYHI_EL1, &env->keys.apga.hi));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_APGAKEYLO_EL1, &env->keys.apga.lo));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_APIAKEYHI_EL1, &env->keys.apia.hi));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_APIAKEYLO_EL1, &env->keys.apia.lo));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_APIBKEYHI_EL1, &env->keys.apib.hi));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_APIBKEYLO_EL1, &env->keys.apib.lo));

        // TODO: Allow debug
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR0_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR10_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR11_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR12_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR13_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR14_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR15_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR1_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR2_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR3_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR4_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR5_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR6_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR7_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR8_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBCR9_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR0_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR10_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR11_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR12_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR13_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR14_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR15_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR1_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR2_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR3_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR4_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR5_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR6_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR7_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR8_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGBVR9_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR0_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR10_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR11_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR12_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR13_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR14_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR15_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR1_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR2_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR3_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR4_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR5_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR6_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR7_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR8_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWCR9_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR0_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR10_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR11_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR12_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR13_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR14_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR15_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR1_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR2_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR3_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR4_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR5_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR6_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR7_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR8_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_DBGWVR9_EL1, ???));

        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_MDCCINT_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_AFSR0_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_AFSR1_EL1, ???));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_AMAIR_EL1, ???));

        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_CNTKCTL_EL1, &env->cp15.c14_cntkctl));
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_CNTV_CVAL_EL0, ???));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_CONTEXTIDR_EL1, &env->cp15.contextidr_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_CPACR_EL1, &env->cp15.cpacr_el1));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_CSSELR_EL1, &env->cp15.csselr_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_ESR_EL1, &env->cp15.esr_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_FAR_EL1, &env->cp15.far_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_MAIR_EL1, &env->cp15.mair_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_MDSCR_EL1, &env->cp15.mdscr_el1));

        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_MIDR_EL1);
        // HVF_CHECKED_CALL(hv_vcpu_set_reg(cpu->hvf_fd, HV_SYS_REG_MPIDR_EL1);

        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_PAR_EL1, &env->cp15.par_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_SCTLR_EL1, &env->cp15.sctlr_el[1]));

        // env->banked_spsr[aarch64_banked_spsr_index(/* el */ 2)] = pstate_read(env);

        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_TCR_EL1, &env->cp15.tcr_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_TPIDRRO_EL0, &env->cp15.tpidrro_el[0]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_TPIDR_EL0, &env->cp15.tpidr_el[0]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_TPIDR_EL1, &env->cp15.tpidr_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_TTBR0_EL1, &env->cp15.ttbr0_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_TTBR1_EL1, &env->cp15.ttbr1_el[1]));
        HVF_CHECKED_CALL(hv_vcpu_get_sys_reg(cpu->hvf_fd, HV_SYS_REG_VBAR_EL1, &env->cp15.vbar_el[1]));
    }

    return 0;
}

void hvf_handle_io(uint16_t port, void* buffer,
                   int direction, int size, int count) {
    int i;
    uint8_t *ptr = buffer;

    for (i = 0; i < count; i++) {
        address_space_rw(&address_space_io, port, MEMTXATTRS_UNSPECIFIED,
                         ptr, size,
                         direction);
        ptr += size;
    }
}

// TODO: synchronize vcpu state
void __hvf_cpu_synchronize_state(CPUState* cpu_state, run_on_cpu_data data)
{
    fprintf(stderr, "%s: call\n", __func__);
    (void)data;
    if (cpu_state->hvf_vcpu_dirty == 0)
        hvf_get_registers(cpu_state);

    cpu_state->hvf_vcpu_dirty = 1;
}

void hvf_cpu_synchronize_state(CPUState *cpu_state)
{
    if (cpu_state->hvf_vcpu_dirty == 0)
        run_on_cpu(cpu_state, __hvf_cpu_synchronize_state, RUN_ON_CPU_NULL);
}

void __hvf_cpu_synchronize_post_reset(CPUState* cpu_state, run_on_cpu_data data)
{
    fprintf(stderr, "%s: call\n", __func__);
    (void)data;
    hvf_put_registers(cpu_state);

    // TODO-convert-to-arm64
    // wvmcs(cpu_state->hvf_fd, VMCS_ENTRY_CTLS, 0);

    cpu_state->hvf_vcpu_dirty = false;
}

void hvf_cpu_synchronize_post_reset(CPUState *cpu_state)
{
    run_on_cpu(cpu_state, __hvf_cpu_synchronize_post_reset, RUN_ON_CPU_NULL);
}

void _hvf_cpu_synchronize_post_init(CPUState* cpu_state, run_on_cpu_data data)
{
    fprintf(stderr, "%s: call\n", __func__);
    (void)data;
    hvf_put_registers(cpu_state);
    cpu_state->hvf_vcpu_dirty = false;
}

void hvf_cpu_synchronize_post_init(CPUState *cpu_state)
{
    run_on_cpu(cpu_state, _hvf_cpu_synchronize_post_init, RUN_ON_CPU_NULL);
}

void hvf_cpu_clean_state(CPUState *cpu_state)
{
    cpu_state->hvf_vcpu_dirty = 0;
}

void vmx_clear_int_window_exiting(CPUState *cpu);

// TODO-convert-to-arm64
static bool ept_emulation_fault(uint64_t ept_qual)
{
    return false;

	// int read, write;

	// /* EPT fault on an instruction fetch doesn't make sense here */
	// if (ept_qual & EPT_VIOLATION_INST_FETCH)
	// 	return false;

	// /* EPT fault must be a read fault or a write fault */
	// read = ept_qual & EPT_VIOLATION_DATA_READ ? 1 : 0;
	// write = ept_qual & EPT_VIOLATION_DATA_WRITE ? 1 : 0;
	// if ((read | write) == 0)
	// 	return false;

	// /*
	//  * The EPT violation must have been caused by accessing a
	//  * guest-physical address that is a translation of a guest-linear
	//  * address.
	//  */
	// if ((ept_qual & EPT_VIOLATION_GLA_VALID) == 0 ||
	//     (ept_qual & EPT_VIOLATION_XLAT_VALID) == 0) {
	// 	return false;
	// }

	// return true;
}

// TODO: taskswitch handling
static void save_state_to_tss32(CPUState *cpu, struct x86_tss_segment32 *tss)
{
    /* CR3 and ldt selector are not saved intentionally */
    // TODO-convert-to-arm64
    // tss->eip = EIP(cpu);
    // tss->eflags = EFLAGS(cpu);
    // tss->eax = EAX(cpu);
    // tss->ecx = ECX(cpu);
    // tss->edx = EDX(cpu);
    // tss->ebx = EBX(cpu);
    // tss->esp = ESP(cpu);
    // tss->ebp = EBP(cpu);
    // tss->esi = ESI(cpu);
    // tss->edi = EDI(cpu);

    // tss->es = vmx_read_segment_selector(cpu, REG_SEG_ES).sel;
    // tss->cs = vmx_read_segment_selector(cpu, REG_SEG_CS).sel;
    // tss->ss = vmx_read_segment_selector(cpu, REG_SEG_SS).sel;
    // tss->ds = vmx_read_segment_selector(cpu, REG_SEG_DS).sel;
    // tss->fs = vmx_read_segment_selector(cpu, REG_SEG_FS).sel;
    // tss->gs = vmx_read_segment_selector(cpu, REG_SEG_GS).sel;
}

static void load_state_from_tss32(CPUState *cpu, struct x86_tss_segment32 *tss)
{
//     wvmcs(cpu->hvf_fd, VMCS_GUEST_CR3, tss->cr3);
// 
//     RIP(cpu) = tss->eip;
//     EFLAGS(cpu) = tss->eflags | 2;
// 
//     /* General purpose registers */
//     RAX(cpu) = tss->eax;
//     RCX(cpu) = tss->ecx;
//     RDX(cpu) = tss->edx;
//     RBX(cpu) = tss->ebx;
//     RSP(cpu) = tss->esp;
//     RBP(cpu) = tss->ebp;
//     RSI(cpu) = tss->esi;
//     RDI(cpu) = tss->edi;
// 
//     vmx_write_segment_selector(cpu, (x68_segment_selector){tss->ldt}, REG_SEG_LDTR);
//     vmx_write_segment_selector(cpu, (x68_segment_selector){tss->es}, REG_SEG_ES);
//     vmx_write_segment_selector(cpu, (x68_segment_selector){tss->cs}, REG_SEG_CS);
//     vmx_write_segment_selector(cpu, (x68_segment_selector){tss->ss}, REG_SEG_SS);
//     vmx_write_segment_selector(cpu, (x68_segment_selector){tss->ds}, REG_SEG_DS);
//     vmx_write_segment_selector(cpu, (x68_segment_selector){tss->fs}, REG_SEG_FS);
//     vmx_write_segment_selector(cpu, (x68_segment_selector){tss->gs}, REG_SEG_GS);
// 
// #if 0
//     load_segment(cpu, REG_SEG_LDTR, tss->ldt);
//     load_segment(cpu, REG_SEG_ES, tss->es);
//     load_segment(cpu, REG_SEG_CS, tss->cs);
//     load_segment(cpu, REG_SEG_SS, tss->ss);
//     load_segment(cpu, REG_SEG_DS, tss->ds);
//     load_segment(cpu, REG_SEG_FS, tss->fs);
//     load_segment(cpu, REG_SEG_GS, tss->gs);
// #endif
}

// static int task_switch_32(CPUState *cpu, x68_segment_selector tss_sel, x68_segment_selector old_tss_sel,
//                           uint64_t old_tss_base, struct x86_segment_descriptor *new_desc)
// {
//     struct x86_tss_segment32 tss_seg;
//     uint32_t new_tss_base = x86_segment_base(new_desc);
//     uint32_t eip_offset = offsetof(struct x86_tss_segment32, eip);
//     uint32_t ldt_sel_offset = offsetof(struct x86_tss_segment32, ldt);
// 
//     vmx_read_mem(cpu, &tss_seg, old_tss_base, sizeof(tss_seg));
//     save_state_to_tss32(cpu, &tss_seg);
// 
//     vmx_write_mem(cpu, old_tss_base + eip_offset, &tss_seg.eip, ldt_sel_offset - eip_offset);
//     vmx_read_mem(cpu, &tss_seg, new_tss_base, sizeof(tss_seg));
// 
//     if (old_tss_sel.sel != 0xffff) {
//         tss_seg.prev_tss = old_tss_sel.sel;
// 
//         vmx_write_mem(cpu, new_tss_base, &tss_seg.prev_tss, sizeof(tss_seg.prev_tss));
//     }
//     load_state_from_tss32(cpu, &tss_seg);
//     return 0;
// }

// static void vmx_handle_task_switch(CPUState *cpu, x68_segment_selector tss_sel, int reason, bool gate_valid, uint8_t gate, uint64_t gate_type)
// {
//     uint64_t rip = rreg(cpu->hvf_fd, HV_X86_RIP);
//     if (!gate_valid || (gate_type != VMCS_INTR_T_HWEXCEPTION &&
//                         gate_type != VMCS_INTR_T_HWINTR &&
//                         gate_type != VMCS_INTR_T_NMI)) {
//         int ins_len = rvmcs(cpu->hvf_fd, VMCS_EXIT_INSTRUCTION_LENGTH);
//         macvm_set_rip(cpu, rip + ins_len);
//         return;
//     }
// 
//     load_regs(cpu);
// 
//     struct x86_segment_descriptor curr_tss_desc, next_tss_desc;
//     int ret;
//     x68_segment_selector old_tss_sel = vmx_read_segment_selector(cpu, REG_SEG_TR);
//     uint64_t old_tss_base = vmx_read_segment_base(cpu, REG_SEG_TR);
//     uint32_t desc_limit;
//     struct x86_call_gate task_gate_desc;
//     struct vmx_segment vmx_seg;
// 
//     x86_read_segment_descriptor(cpu, &next_tss_desc, tss_sel);
//     x86_read_segment_descriptor(cpu, &curr_tss_desc, old_tss_sel);
// 
//     if (reason == TSR_IDT_GATE && gate_valid) {
//         int dpl;
// 
//         ret = x86_read_call_gate(cpu, &task_gate_desc, gate);
// 
//         dpl = task_gate_desc.dpl;
//         x68_segment_selector cs = vmx_read_segment_selector(cpu, REG_SEG_CS);
//         if (tss_sel.rpl > dpl || cs.rpl > dpl)
//             DPRINTF("emulate_gp");
//     }
// 
//     desc_limit = x86_segment_limit(&next_tss_desc);
//     if (!next_tss_desc.p || ((desc_limit < 0x67 && (next_tss_desc.type & 8)) || desc_limit < 0x2b)) {
//         VM_PANIC("emulate_ts");
//     }
// 
//     if (reason == TSR_IRET || reason == TSR_JMP) {
//         curr_tss_desc.type &= ~(1 << 1); /* clear busy flag */
//         x86_write_segment_descriptor(cpu, &curr_tss_desc, old_tss_sel);
//     }
// 
//     if (reason == TSR_IRET)
//         EFLAGS(cpu) &= ~RFLAGS_NT;
// 
//     if (reason != TSR_CALL && reason != TSR_IDT_GATE)
//         old_tss_sel.sel = 0xffff;
// 
//     if (reason != TSR_IRET) {
//         next_tss_desc.type |= (1 << 1); /* set busy flag */
//         x86_write_segment_descriptor(cpu, &next_tss_desc, tss_sel);
//     }
// 
//     if (next_tss_desc.type & 8)
//         ret = task_switch_32(cpu, tss_sel, old_tss_sel, old_tss_base, &next_tss_desc);
//     else
//         //ret = task_switch_16(cpu, tss_sel, old_tss_sel, old_tss_base, &next_tss_desc);
//         VM_PANIC("task_switch_16");
// 
//     macvm_set_cr0(cpu->hvf_fd, rvmcs(cpu->hvf_fd, VMCS_GUEST_CR0) | CR0_TS);
//     x86_segment_descriptor_to_vmx(cpu, tss_sel, &next_tss_desc, &vmx_seg);
//     vmx_write_segment_descriptor(cpu, &vmx_seg, REG_SEG_TR);
// 
//     store_regs(cpu);
// 
//     hv_vcpu_invalidate_tlb(cpu->hvf_fd);
//     hv_vcpu_flush(cpu->hvf_fd);
// }

/* Find first bit starting from msb */
static int apic_fls_bit(uint32_t value)
{
    return 31 - clz32(value);
}

/* Find first bit starting from lsb */
static int apic_ffs_bit(uint32_t value)
{
    return ctz32(value);
}

static inline void apic_reset_bit(uint32_t *tab, int index)
{
    int i, mask;
    i = index >> 5;
    mask = 1 << (index & 0x1f);
    tab[i] &= ~mask;
}

#define VECTORING_INFO_VECTOR_MASK     0xff

static void hvf_handle_interrupt(CPUState * cpu, int mask) {
    cpu->interrupt_request |= mask;
    if (!qemu_cpu_is_self(cpu)) {
        qemu_cpu_kick(cpu);
    }
}

#define ESR_ELx_EC_SHIFT	(26ULL)
#define ESR_ELx_EC_MASK		((0x3FULL) << ESR_ELx_EC_SHIFT)
#define ESR_ELx_EC(esr)		(((esr) & ESR_ELx_EC_MASK) >> ESR_ELx_EC_SHIFT)

static void hvf_read_mem(struct CPUState* cpu, void *data, uint64_t gpa, int bytes) {
    address_space_rw(&address_space_memory, gpa, MEMTXATTRS_UNSPECIFIED, data, bytes, 0);
}

static void hvf_handle_exception(CPUState* cpu) {
    // We have an exception in EL2.
    uint32_t syndrome = cpu->hvf_vcpu_exit_info->exception.syndrome;
    DPRINTF("%s: syndrome 0x%x\n", __func__,
            (syndrome));
    uint64_t va = cpu->hvf_vcpu_exit_info->exception.virtual_address;
    uint64_t pa = cpu->hvf_vcpu_exit_info->exception.physical_address;

    // Obtain the EC:
    //
    uint32_t ec = ESR_ELx_EC(syndrome);

    DPRINTF("%s: Exception class: 0x%x\n", __func__, ec);

    uint8_t scratch[1024];

    switch (ec) {
        case 0x20:
            hvf_read_mem(cpu, scratch, pa, 4);
            DPRINTF("Guest abort, aborting. pa %#llx mem: 0x%x\n", (unsigned long long)pa, *(uint32_t*)scratch);
            abort();
            break;
        default:
            DPRINTF("Some other exception class: 0x%x\n", __func__, ec);
    };
}

int hvf_vcpu_exec(CPUState* cpu) {
    ARMCPU* armcpu = ARM_CPU(cpu);
    CPUARMState* env = &armcpu->env;
    int ret = 0;
    uint64_t val;

    // TODO-convert-to-arm64
    // uint64_t rip = 0;

    // armcpu->halted = 0;

    // if (hvf_process_events(armcpu)) {
    //     qemu_mutex_unlock_iothread();
    //     pthread_yield_np();
    //     qemu_mutex_lock_iothread();
    //     return EXCP_HLT;
    // }

again:


    do {
        if (cpu->hvf_vcpu_dirty) {
            fprintf(stderr, "%s: should put registers\n", __func__);
            hvf_put_registers(cpu);
            cpu->hvf_vcpu_dirty = false;
        }

        // TODO-convert-to-arm64
        // cpu->hvf_x86->interruptable =
        //     !(rvmcs(cpu->hvf_fd, VMCS_GUEST_INTERRUPTIBILITY) &
        //     (VMCS_INTERRUPTIBILITY_STI_BLOCKING | VMCS_INTERRUPTIBILITY_MOVSS_BLOCKING));

        hvf_inject_interrupts(cpu);
        // TODO-convert-to-arm64
        // vmx_update_tpr(cpu);


        qemu_mutex_unlock_iothread();
        // TODO-convert-to-arm64
        // while (!cpu_is_bsp(X86_CPU(cpu)) && cpu->halted) {
        //     qemu_mutex_lock_iothread();
        //     return EXCP_HLT;
        // }


        HVF_CHECKED_CALL(hv_vcpu_get_reg(cpu->hvf_fd, HV_REG_PC, &val));
        DPRINTF("%s: run vcpu. pc: 0x%llx\n", __func__, (unsigned long long)val);
        int r  = hv_vcpu_run(cpu->hvf_fd);

        if (r) {
            qemu_abort("%s: run failed with 0x%x\n", __func__, r);
        }

//  * @typedef    hv_vcpu_exit_t
//  * @abstract   Contains information about an exit from the vcpu to the host.
//
//  * @typedef    hv_vcpu_exit_exception_t
//  * @abstract   Contains details of a vcpu exception.
//  */
// typedef struct {
//     hv_exception_syndrome_t syndrome;
//     hv_exception_address_t virtual_address;
//     hv_ipa_t physical_address;
// } hv_vcpu_exit_exception_t;
//  
//  */
// typedef struct {
//     hv_exit_reason_t reason;
//     hv_vcpu_exit_exception_t exception;
// } hv_vcpu_exit_t;


        DPRINTF("%s: Exit info: reason: %#x exception: syndrome %#x va pa %#llx %#llx\n", __func__,
                cpu->hvf_vcpu_exit_info->reason,
                cpu->hvf_vcpu_exit_info->exception.syndrome,
                (unsigned long long)cpu->hvf_vcpu_exit_info->exception.virtual_address,
                (unsigned long long)cpu->hvf_vcpu_exit_info->exception.physical_address);
        /* handle VMEXIT */
        // TODO-convert-to-arm64
        // uint64_t exit_reason = rvmcs(cpu->hvf_fd, VMCS_EXIT_REASON);
        // uint64_t exit_qual = rvmcs(cpu->hvf_fd, VMCS_EXIT_QUALIFICATION);
        // uint32_t ins_len = (uint32_t)rvmcs(cpu->hvf_fd, VMCS_EXIT_INSTRUCTION_LENGTH);
        // uint64_t idtvec_info = rvmcs(cpu->hvf_fd, VMCS_IDT_VECTORING_INFO);
        // rip = rreg(cpu->hvf_fd, HV_X86_RIP);
        // RFLAGS(cpu) = rreg(cpu->hvf_fd, HV_X86_RFLAGS);
        // env->eflags = RFLAGS(cpu);

        qemu_mutex_lock_iothread();

        // TODO-convert-to-arm64
        // update_apic_tpr(cpu);
        current_cpu = cpu;

        ret = 0;

        // TODO-convert-to-arm64
        uint8_t ec = 0x3f & ((cpu->hvf_vcpu_exit_info->exception.syndrome) >> 26);
        uint64_t val;
        HVF_CHECKED_CALL(hv_vcpu_get_reg(cpu->hvf_fd, HV_REG_PC, &val));
        DPRINTF("%s: Exit at PC 0x%llx\n", __func__, (unsigned long long)val);
        switch (cpu->hvf_vcpu_exit_info->reason) {
            case HV_EXIT_REASON_EXCEPTION:
                DPRINTF("%s: handle exception\n", __func__);
                hvf_handle_exception(cpu);
                break;
            // TODO-convert-to-arm64
            // case EXIT_REASON_HLT: {
            //     macvm_set_rip(cpu, rip + ins_len);
            //     if (!((cpu->interrupt_request & CPU_INTERRUPT_HARD) && (EFLAGS(cpu) & IF_MASK)) && !(cpu->interrupt_request & CPU_INTERRUPT_NMI) && !(idtvec_info & VMCS_IDT_VEC_VALID)) {
            //         cpu->halted = 1;
            //         ret = EXCP_HLT;
            //     }
            //     ret = EXCP_INTERRUPT;
            //     break;
            // }
            // case EXIT_REASON_MWAIT: {
            //     ret = EXCP_INTERRUPT;
            //     break;
            // }
                /* Need to check if MMIO or unmmaped fault */
            // case EXIT_REASON_EPT_FAULT:
            // {
                // TODO-convert-to-arm64
                // hvf_slot *slot;
                // addr_t gpa = rvmcs(cpu->hvf_fd, VMCS_GUEST_PHYSICAL_ADDRESS);

                // if ((idtvec_info & VMCS_IDT_VEC_VALID) == 0 && (exit_qual & EXIT_QUAL_NMIUDTI) != 0)
                //     vmx_set_nmi_blocking(cpu);

                // slot = hvf_find_overlap_slot(gpa, gpa + 1);
                // mmio
                // if (ept_emulation_fault(exit_qual) && !slot) {
                // TODO-convert-to-arm64!!!!!!!!!!!
//                     struct x86_decode decode;
// 
//                     load_regs(cpu);
//                     cpu->hvf_x86->fetch_rip = rip;
// 
//                     decode_instruction(cpu, &decode);
// #if 0
//                     DPRINTF("%llx: fetched %s, %x %x modrm %x len %d, gpa %lx\n", rip, decode_cmd_to_string(decode.cmd),
//                            decode.opcode[0], decode.opcode[1], decode.modrm.byte, decode.len, gpa);
// #endif
//                     exec_instruction(cpu, &decode);
//                     store_regs(cpu);
                    // break;
                // }

#ifdef DIRTY_VGA_TRACKING // Don't try to build HVF with dirty vga tracking for now.
#error
#endif
           //      if (slot) {
           //          pthread_rwlock_wrlock(&mem_lock);
           //          bool found = false;
           //          void* hva = hvf_gpa2hva(gpa & ~0xfff, &found);
           //          pthread_rwlock_unlock(&mem_lock);
           //          if (found) {
           //              qemu_ram_load(hva, 16384);
           //          }
           //      }
           //      break;
           //  }
            // TODO-convert-to-arm64
//             case EXIT_REASON_INOUT:
//             {
//                 uint32_t in = (exit_qual & 8) != 0;
//                 uint32_t size =  (exit_qual & 7) + 1;
//                 uint32_t string =  (exit_qual & 16) != 0;
//                 uint32_t port =  exit_qual >> 16;
//                 uint32_t rep = (exit_qual & 0x20) != 0;
// 
// #if 1
//                 if (!string && in) {
//                     uint64_t val = 0;
//                     load_regs(cpu);
//                     hvf_handle_io(port, &val, 0, size, 1);
//                     if (size == 1) AL(cpu) = val;
//                     else if (size == 2) AX(cpu) = val;
//                     else if (size == 4) RAX(cpu) = (uint32_t)val;
//                     else VM_PANIC("size");
//                     RIP(cpu) += ins_len;
//                     store_regs(cpu);
//                     break;
//                 } else if (!string && !in) {
//                     RAX(cpu) = rreg(cpu->hvf_fd, HV_X86_RAX);
//                     hvf_handle_io(port, &RAX(cpu), 1, size, 1);
//                     macvm_set_rip(cpu, rip + ins_len);
//                     break;
//                 }
// #endif
//                 struct x86_decode decode;
// 
//                 load_regs(cpu);
//                 cpu->hvf_x86->fetch_rip = rip;
// 
//                 decode_instruction(cpu, &decode);
//                 VM_PANIC_ON(ins_len != decode.len);
//                 exec_instruction(cpu, &decode);
//                 store_regs(cpu);
// 
//                 break;
//             }
            // TODO-convert-to-arm64
            // case EXIT_REASON_CPUID: {
            //     uint32_t rax = (uint32_t)rreg(cpu->hvf_fd, HV_X86_RAX);
            //     uint32_t rbx = (uint32_t)rreg(cpu->hvf_fd, HV_X86_RBX);
            //     uint32_t rcx = (uint32_t)rreg(cpu->hvf_fd, HV_X86_RCX);
            //     uint32_t rdx = (uint32_t)rreg(cpu->hvf_fd, HV_X86_RDX);

            //     get_cpuid_func(cpu, rax, rcx, &rax, &rbx, &rcx, &rdx);

            //     wreg(cpu->hvf_fd, HV_X86_RAX, rax);
            //     wreg(cpu->hvf_fd, HV_X86_RBX, rbx);
            //     wreg(cpu->hvf_fd, HV_X86_RCX, rcx);
            //     wreg(cpu->hvf_fd, HV_X86_RDX, rdx);

            //     macvm_set_rip(cpu, rip + ins_len);
            //     break;
            // }
            // TODO-convert-to-arm64
            // case EXIT_REASON_XSETBV: {
            //     X86CPU *x86_cpu = X86_CPU(cpu);
            //     CPUX86State *env = &x86_cpu->env;
            //     uint32_t eax = (uint32_t)rreg(cpu->hvf_fd, HV_X86_RAX);
            //     uint32_t ecx = (uint32_t)rreg(cpu->hvf_fd, HV_X86_RCX);
            //     uint32_t edx = (uint32_t)rreg(cpu->hvf_fd, HV_X86_RDX);

            //     if (ecx) {
            //         DPRINTF("xsetbv: invalid index %d\n", ecx);
            //         macvm_set_rip(cpu, rip + ins_len);
            //         break;
            //     }
            //     env->xcr0 = ((uint64_t)edx << 32) | eax;
            //     wreg(cpu->hvf_fd, HV_X86_XCR0, env->xcr0 | 1);
            //     macvm_set_rip(cpu, rip + ins_len);
            //     break;
            // }
            // case EXIT_REASON_INTR_WINDOW:
            //     // TODO-convert-to-arm64
            //     // vmx_clear_int_window_exiting(cpu);
            //     ret = EXCP_INTERRUPT;
            //     break;
            // case EXIT_REASON_NMI_WINDOW:
            //     // TODO-convert-to-arm64
            //     // vmx_clear_nmi_window_exiting(cpu);
            //     ret = EXCP_INTERRUPT;
            //     break;
            // case EXIT_REASON_EXT_INTR:
            //     /* force exit and allow io handling */
            //     ret = EXCP_INTERRUPT;
            //     break;
            // TODO-convert-to-arm64
            // case EXIT_REASON_RDMSR:
            // case EXIT_REASON_WRMSR:
            // {
            //     load_regs(cpu);
            //     if (exit_reason == EXIT_REASON_RDMSR)
            //         simulate_rdmsr(cpu);
            //     else
            //         simulate_wrmsr(cpu);
            //     RIP(cpu) += rvmcs(cpu->hvf_fd, VMCS_EXIT_INSTRUCTION_LENGTH);
            //     store_regs(cpu);
            //     break;
            // }
            // case EXIT_REASON_CR_ACCESS: {
            //     int cr;
            //     int reg;

            //     load_regs(cpu);
            //     cr = exit_qual & 15;
            //     reg = (exit_qual >> 8) & 15;

            //     DPRINTF("%lx: mov cr %d from %d %llx\n", rip, cr, reg, RRX(cpu, reg));
            //     switch (cr) {
            //         case 0x0: {
            //             macvm_set_cr0(cpu->hvf_fd, RRX(cpu, reg));
            //             break;
            //         }
            //         case 4: {
            //             macvm_set_cr4(cpu->hvf_fd, RRX(cpu, reg));
            //             break;
            //         }
            //         case 8: {
            //             X86CPU *x86_cpu = X86_CPU(cpu);
            //             if (exit_qual & 0x10) {
            //                 RRX(cpu, reg) = cpu_get_apic_tpr(x86_cpu->apic_state);
            //             }
            //             else {
            //                 int tpr = RRX(cpu, reg);
            //                 cpu_set_apic_tpr(x86_cpu->apic_state, tpr);
            //                 ret = EXCP_INTERRUPT;
            //             }
            //             break;
            //         }
            //         default:
            //             qemu_abort("%s: Unrecognized CR %d\n", __func__, cr);
            //     }
            //     RIP(cpu) += ins_len;
            //     store_regs(cpu);
            //     break;
            // }
            // case EXIT_REASON_APIC_ACCESS: { // TODO
            //     struct x86_decode decode;

            //     load_regs(cpu);
            //     cpu->hvf_x86->fetch_rip = rip;

            //     decode_instruction(cpu, &decode);
            //     DPRINTF("apic fetched %s, %x %x len %d\n", decode_cmd_to_string(decode.cmd), decode.opcode[0], decode.opcode[1], decode.len);
            //     exec_instruction(cpu, &decode);
            //     store_regs(cpu);
            //     break;
            // }
            // case EXIT_REASON_TPR: {
            //     ret = 1;
            //     break;
            // }
            // case EXIT_REASON_TASK_SWITCH: {
            //                                   // TODO-convert-to-arm64
            //     // uint64_t vinfo = rvmcs(cpu->hvf_fd, VMCS_IDT_VECTORING_INFO);
            //     // DPRINTF("%llx: task switch %lld, vector %lld, gate %lld\n", RIP(cpu), (exit_qual >> 30) & 0x3, vinfo & VECTORING_INFO_VECTOR_MASK, vinfo & VMCS_INTR_T_MASK);
            //     // x68_segment_selector sel = {.sel = exit_qual & 0xffff};
            //     // vmx_handle_task_switch(cpu, sel, (exit_qual >> 30) & 0x3, vinfo & VMCS_INTR_VALID, vinfo & VECTORING_INFO_VECTOR_MASK, vinfo & VMCS_INTR_T_MASK);
            //     break;
            // }
            // case EXIT_REASON_TRIPLE_FAULT: {
            //     // TODO-convert-to-arm64
            //     // addr_t gpa = rvmcs(cpu->hvf_fd, VMCS_GUEST_PHYSICAL_ADDRESS);

            //     // DPRINTF("triple fault at %llx (%llx, %llx), cr0 %llx, qual %llx, gpa %llx, ins len %d, cpu %p\n",
            //     //         linear_rip(cpu, rip), RIP(cpu), linear_addr(cpu, rip, REG_SEG_CS),
            //     //         rvmcs(cpu->hvf_fd, VMCS_GUEST_CR0), exit_qual, gpa, ins_len, cpu);
            //     // qemu_system_reset_request(SHUTDOWN_CAUSE_GUEST_RESET);
            //     // usleep(1000 * 100);
            //     ret = EXCP_INTERRUPT;
            //     break;
            // }
            // TODO-convert-to-arm64
            // case EXIT_REASON_RDPMC:
            //     wreg(cpu->hvf_fd, HV_X86_RAX, 0);
            //     wreg(cpu->hvf_fd, HV_X86_RDX, 0);
            //     macvm_set_rip(cpu, rip + ins_len);
            //     break;
            // case VMX_REASON_VMCALL:
            //     // TODO: maybe just take this out?
            //     // if (g_hypervisor_iface) {
            //     //     load_regs(cpu);
            //     //     g_hypervisor_iface->hypercall_handler(cpu);
            //     //     RIP(cpu) += rvmcs(cpu->hvf_fd, VMCS_EXIT_INSTRUCTION_LENGTH);
            //     //     store_regs(cpu);
            //     // }
            //     break;
            // case EXIT_REASON_DR_ACCESS:
            //     fprintf(stderr, "%llx: unhandled dr access %llx\n", rip, exit_reason);
            //     load_regs(cpu);
            //     {
            //         // Just skip the whole thing for now
            //         uint32_t insnLen = rvmcs(cpu->hvf_fd, VMCS_EXIT_INSTRUCTION_LENGTH);
            //         RIP(cpu) += insnLen;
            //     }
            //     store_regs(cpu);
            //     break;
            default:
                fprintf(stderr, "unhandled exit %llx\n", (unsigned long long)cpu->hvf_vcpu_exit_info->reason);
                abort();
                qemu_system_reset_request(SHUTDOWN_CAUSE_GUEST_RESET);
        }
    } while (ret == 0);

    return ret;
}

int hvf_smp_cpu_exec(CPUState * cpu)
{
    CPUArchState *env = (CPUArchState *) (cpu->env_ptr);
    int why;
    int ret;

    while (1) {
        if (cpu->exception_index >= EXCP_INTERRUPT) {
            ret = cpu->exception_index;
            cpu->exception_index = -1;
            break;
        }

        why = hvf_vcpu_exec(cpu);
    }

    return ret;
}

static int hvf_accel_init(MachineState *ms) {
    int x;
    DPRINTF("%s: call. hv vm create?\n", __func__);
    int r = hv_vm_create(0);

    if (!check_hvf_ok(r)) {
        hv_vm_destroy();
        return -EINVAL;
    }

    struct hvf_accel_state* s =
        (struct hvf_accel_state*)g_malloc0(sizeof(struct hvf_accel_state));

    s->num_slots = HVF_MAX_SLOTS;
    for (x = 0; x < s->num_slots; ++x) {
        s->slots[x].size = 0;
        s->slots[x].slot_id = x;
    }

    hvf_state = s;
    cpu_interrupt_handler = hvf_handle_interrupt;
    memory_listener_register(&hvf_memory_listener, &address_space_memory);
    memory_listener_register(&hvf_io_listener, &address_space_io);
    qemu_set_user_backed_mapping_funcs(
        hvf_user_backed_ram_map,
        hvf_user_backed_ram_unmap);
    return 0;
}

bool hvf_allowed;

static void hvf_accel_class_init(ObjectClass *oc, void *data)
{
    DPRINTF("%s: call\n", __func__);
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
