/*
 * QEMU AEHD support
 *
 * Copyright IBM, Corp. 2008
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef QEMU_AEHD_H
#define QEMU_AEHD_H

#include "qemu/queue.h"
#include "qom/cpu.h"
#include "exec/memattrs.h"
#include "hw/irq.h"

/* These constants must never be used at runtime if aehd_enabled() is false.
 * They exist so we don't need #ifdefs around AEHD-specific code that already
 * checks aehd_enabled() properly.
 */
#define AEHD_CPUID_SIGNATURE      0
#define AEHD_CPUID_FEATURES       0
#define AEHD_FEATURE_MMU_OP       0
#define AEHD_FEATURE_PV_EOI       0

extern bool aehd_allowed;
extern bool aehd_kernel_irqchip;

#if defined CONFIG_AEHD
#define aehd_enabled()           (aehd_allowed)
/**
 * aehd_irqchip_in_kernel:
 *
 * Returns: true if the user asked us to create an in-kernel
 * irqchip via the "kernel_irqchip=on" machine option.
 * What this actually means is architecture and machine model
 * specific: on PC, for instance, it means that the LAPIC,
 * IOAPIC and PIT are all in kernel. This function should never
 * be used from generic target-independent code: use one of the
 * following functions or some other specific check instead.
 */
#define aehd_irqchip_in_kernel() (aehd_kernel_irqchip)

#else
#define aehd_enabled()           (0)
#define aehd_irqchip_in_kernel() (false)
#endif

struct aehd_run;
struct aehd_lapic_state;
struct aehd_irq_routing_entry;

typedef struct AEHDCapabilityInfo {
    const char *name;
    int value;
} AEHDCapabilityInfo;

#define AEHD_CAP_INFO(CAP) { "AEHD_CAP_" stringify(CAP), AEHD_CAP_##CAP }
#define AEHD_CAP_LAST_INFO { NULL, 0 }

struct AEHDState;
typedef struct AEHDState AEHDState;
extern AEHDState *aehd_state;

/* external API */

void* aehd_gpa2hva(uint64_t gpa, bool* found);
int aehd_hva2gpa(void* hva, uint64_t length, int array_size,
                uint64_t* gpa, uint64_t* size);
bool aehd_has_free_slot(MachineState *ms);

int aehd_init_vcpu(CPUState *cpu);
int aehd_cpu_exec(CPUState *cpu);
int aehd_destroy_vcpu(CPUState *cpu);

#ifdef NEED_CPU_H
#include "cpu.h"

void aehd_flush_coalesced_mmio_buffer(void);

int aehd_insert_breakpoint(CPUState *cpu, target_ulong addr,
                          target_ulong len, int type);
int aehd_remove_breakpoint(CPUState *cpu, target_ulong addr,
                          target_ulong len, int type);
void aehd_remove_all_breakpoints(CPUState *cpu);
int aehd_update_guest_debug(CPUState *cpu, unsigned long reinject_trap);
#ifndef _WIN32
int aehd_set_signal_mask(CPUState *cpu, const sigset_t *sigset);
#endif

int aehd_on_sigbus_vcpu(CPUState *cpu, int code, void *addr);
int aehd_on_sigbus(int code, void *addr);

/* internal API */

int aehd_ioctl(AEHDState *s, int type,
        void *input, size_t input_size,
        void *output, size_t output_size);
int aehd_vm_ioctl(AEHDState *s, int type,
        void *input, size_t input_size,
        void *output, size_t output_size);
int aehd_vcpu_ioctl(CPUState *cpu, int type,
        void *input, size_t input_size,
        void *output, size_t output_size);

/* Arch specific hooks */

extern const AEHDCapabilityInfo aehd_arch_required_capabilities[];

void aehd_arch_pre_run(CPUState *cpu, struct aehd_run *run);
MemTxAttrs aehd_arch_post_run(CPUState *cpu, struct aehd_run *run);

int aehd_arch_handle_exit(CPUState *cpu, struct aehd_run *run);

int aehd_arch_handle_ioapic_eoi(CPUState *cpu, struct aehd_run *run);

int aehd_arch_process_async_events(CPUState *cpu);

int aehd_arch_get_registers(CPUState *cpu);

/* state subset only touched by the VCPU itself during runtime */
#define AEHD_PUT_RUNTIME_STATE   1
/* state subset modified during VCPU reset */
#define AEHD_PUT_RESET_STATE     2
/* full state set, modified during initialization or on vmload */
#define AEHD_PUT_FULL_STATE      3

int aehd_arch_put_registers(CPUState *cpu, int level);

int aehd_arch_init(MachineState *ms, AEHDState *s);

int aehd_arch_init_vcpu(CPUState *cpu);

bool aehd_vcpu_id_is_valid(int vcpu_id);

/* Returns VCPU ID to be used on AEHD_CREATE_VCPU ioctl() */
unsigned long aehd_arch_vcpu_id(CPUState *cpu);

int aehd_arch_on_sigbus_vcpu(CPUState *cpu, int code, void *addr);
int aehd_arch_on_sigbus(int code, void *addr);

void aehd_arch_init_irq_routing(AEHDState *s);

int aehd_arch_fixup_msi_route(struct aehd_irq_routing_entry *route,
                             uint64_t address, uint32_t data, PCIDevice *dev);

/* Notify arch about newly added MSI routes */
int aehd_arch_add_msi_route_post(struct aehd_irq_routing_entry *route,
                                int vector, PCIDevice *dev);
/* Notify arch about released MSI routes */
int aehd_arch_release_virq_post(int virq);

int aehd_set_irq(AEHDState *s, int irq, int level);
int aehd_irqchip_send_msi(AEHDState *s, MSIMessage msg);

void aehd_irqchip_add_irq_route(AEHDState *s, int gsi, int irqchip, int pin);

void aehd_put_apic_state(DeviceState *d, struct aehd_lapic_state *kapic);
void aehd_get_apic_state(DeviceState *d, struct aehd_lapic_state *kapic);

struct aehd_guest_debug;
struct aehd_debug_exit_arch;

struct aehd_sw_breakpoint {
    target_ulong pc;
    target_ulong saved_insn;
    int use_count;
    QTAILQ_ENTRY(aehd_sw_breakpoint) entry;
};

QTAILQ_HEAD(aehd_sw_breakpoint_head, aehd_sw_breakpoint);

struct aehd_sw_breakpoint *aehd_find_sw_breakpoint(CPUState *cpu,
                                                 target_ulong pc);

int aehd_sw_breakpoints_active(CPUState *cpu);

int aehd_arch_insert_sw_breakpoint(CPUState *cpu,
                                  struct aehd_sw_breakpoint *bp);
int aehd_arch_remove_sw_breakpoint(CPUState *cpu,
                                  struct aehd_sw_breakpoint *bp);
int aehd_arch_insert_hw_breakpoint(target_ulong addr,
                                  target_ulong len, int type);
int aehd_arch_remove_hw_breakpoint(target_ulong addr,
                                  target_ulong len, int type);
void aehd_arch_remove_all_hw_breakpoints(void);

void aehd_arch_update_guest_debug(CPUState *cpu, struct aehd_guest_debug *dbg);

bool aehd_arch_stop_on_emulation_error(CPUState *cpu);

int aehd_check_extension(AEHDState *s, unsigned int extension);

int aehd_vm_check_extension(AEHDState *s, unsigned int extension);

#define aehd_vm_enable_cap(s, capability, cap_flags, ...)             \
    ({                                                               \
        struct aehd_enable_cap cap = {                                \
            .cap = capability,                                       \
            .flags = cap_flags,                                      \
        };                                                           \
        uint64_t args_tmp[] = { __VA_ARGS__ };                       \
        int i;                                                       \
        for (i = 0; i < (int)ARRAY_SIZE(args_tmp) &&                 \
                     i < ARRAY_SIZE(cap.args); i++) {                \
            cap.args[i] = args_tmp[i];                               \
        }                                                            \
        aehd_vm_ioctl(s, AEHD_ENABLE_CAP, &cap, sizeof(cap), NULL, 0); \
    })

#define aehd_vcpu_enable_cap(cpu, capability, cap_flags, ...)         \
    ({                                                               \
        struct aehd_enable_cap cap = {                                \
            .cap = capability,                                       \
            .flags = cap_flags,                                      \
        };                                                           \
        uint64_t args_tmp[] = { __VA_ARGS__ };                       \
        int i;                                                       \
        for (i = 0; i < (int)ARRAY_SIZE(args_tmp) &&                 \
                     i < ARRAY_SIZE(cap.args); i++) {                \
            cap.args[i] = args_tmp[i];                               \
        }                                                            \
        aehd_vcpu_ioctl(cpu, AEHD_ENABLE_CAP,                          \
                &cap, sizeof(cap), NULL, 0);                         \
    })

uint32_t aehd_arch_get_supported_cpuid(AEHDState *env, uint32_t function,
                                      uint32_t index, int reg);

void aehd_set_sigmask_len(AEHDState *s, unsigned int sigmask_len);

#if !defined(CONFIG_USER_ONLY)
int aehd_physical_memory_addr_from_host(AEHDState *s, void *ram_addr,
                                       hwaddr *phys_addr);
#endif

#endif /* NEED_CPU_H */

void aehd_raise_event(CPUState *cpu);
void aehd_cpu_synchronize_state(CPUState *cpu);
void aehd_cpu_synchronize_post_reset(CPUState *cpu);
void aehd_cpu_synchronize_post_init(CPUState *cpu);
void aehd_cpu_synchronize_pre_loadvm(CPUState *cpu);

/**
 * aehd_irqchip_add_msi_route - Add MSI route for specific vector
 * @s:      AEHD state
 * @vector: which vector to add. This can be either MSI/MSIX
 *          vector. The function will automatically detect whether
 *          MSI/MSIX is enabled, and fetch corresponding MSI
 *          message.
 * @dev:    Owner PCI device to add the route. If @dev is specified
 *          as @NULL, an empty MSI message will be inited.
 * @return: virq (>=0) when success, errno (<0) when failed.
 */
int aehd_irqchip_add_msi_route(AEHDState *s, int vector, PCIDevice *dev);
int aehd_irqchip_update_msi_route(AEHDState *s, int virq, MSIMessage msg,
                                 PCIDevice *dev);
void aehd_irqchip_commit_routes(AEHDState *s);
void aehd_irqchip_release_virq(AEHDState *s, int virq);

int aehd_irqchip_add_adapter_route(AEHDState *s, AdapterInfo *adapter);

void aehd_irqchip_set_qemuirq_gsi(AEHDState *s, qemu_irq irq, int gsi);
void aehd_pc_gsi_handler(void *opaque, int n, int level);
void aehd_pc_setup_irq_routing(bool pci_enabled);
void aehd_init_irq_routing(AEHDState *s);

/**
 * aehd_arch_irqchip_create:
 * @AEHDState: The AEHDState pointer
 * @MachineState: The MachineState pointer
 *
 * Allow architectures to create an in-kernel irq chip themselves.
 *
 * Returns: < 0: error
 *            0: irq chip was not created
 *          > 0: irq chip was created
 */
int aehd_arch_irqchip_create(MachineState *ms, AEHDState *s);

/**
 * aehd_set_one_reg - set a register value in AEHD via AEHD_SET_ONE_REG ioctl
 * @id: The register ID
 * @source: The pointer to the value to be set. It must point to a variable
 *          of the correct type/size for the register being accessed.
 *
 * Returns: 0 on success, or a negative errno on failure.
 */
int aehd_set_one_reg(CPUState *cs, uint64_t id, void *source);

/**
 * aehd_get_one_reg - get a register value from AEHD via AEHD_GET_ONE_REG ioctl
 * @id: The register ID
 * @target: The pointer where the value is to be stored. It must point to a
 *          variable of the correct type/size for the register being accessed.
 *
 * Returns: 0 on success, or a negative errno on failure.
 */
int aehd_get_one_reg(CPUState *cs, uint64_t id, void *target);
int aehd_get_max_memslots(void);
#endif
