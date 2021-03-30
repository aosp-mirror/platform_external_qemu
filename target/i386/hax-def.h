#pragma once

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

enum component_index_t {
    VMX_PIN_CONTROLS                            = 0x00004000,
    VMX_PRIMARY_PROCESSOR_CONTROLS              = 0x00004002,
    VMX_SECONDARY_PROCESSOR_CONTROLS            = 0x0000401e,
    VMX_EXCEPTION_BITMAP                        = 0x00004004,
    VMX_PAGE_FAULT_ERROR_CODE_MASK              = 0x00004006,
    VMX_PAGE_FAULT_ERROR_CODE_MATCH             = 0x00004008,
    VMX_EXIT_CONTROLS                           = 0x0000400c,
    VMX_EXIT_MSR_STORE_COUNT                    = 0x0000400e,
    VMX_EXIT_MSR_LOAD_COUNT                     = 0x00004010,
    VMX_ENTRY_CONTROLS                          = 0x00004012,
    VMX_ENTRY_MSR_LOAD_COUNT                    = 0x00004014,
    VMX_ENTRY_INTERRUPT_INFO                    = 0x00004016,
    VMX_ENTRY_EXCEPTION_ERROR_CODE              = 0x00004018,
    VMX_ENTRY_INSTRUCTION_LENGTH                = 0x0000401a,
    VMX_TPR_THRESHOLD                           = 0x0000401c,

    VMX_CR0_MASK                                = 0x00006000,
    VMX_CR4_MASK                                = 0x00006002,
    VMX_CR0_READ_SHADOW                         = 0x00006004,
    VMX_CR4_READ_SHADOW                         = 0x00006006,
    VMX_CR3_TARGET_COUNT                        = 0x0000400a,
    VMX_CR3_TARGET_VAL_BASE                     = 0x00006008, // x6008-x6206

    VMX_VPID                                    = 0x00000000,
    VMX_IO_BITMAP_A                             = 0x00002000,
    VMX_IO_BITMAP_B                             = 0x00002002,
    VMX_MSR_BITMAP                              = 0x00002004,
    VMX_EXIT_MSR_STORE_ADDRESS                  = 0x00002006,
    VMX_EXIT_MSR_LOAD_ADDRESS                   = 0x00002008,
    VMX_ENTRY_MSR_LOAD_ADDRESS                  = 0x0000200a,
    VMX_TSC_OFFSET                              = 0x00002010,
    VMX_VAPIC_PAGE                              = 0x00002012,
    VMX_APIC_ACCESS_PAGE                        = 0x00002014,
    VMX_EPTP                                    = 0x0000201a,
    VMX_PREEMPTION_TIMER                        = 0x0000482e,

    VMX_INSTRUCTION_ERROR_CODE                  = 0x00004400,

    VM_EXIT_INFO_REASON                         = 0x00004402,
    VM_EXIT_INFO_INTERRUPT_INFO                 = 0x00004404,
    VM_EXIT_INFO_EXCEPTION_ERROR_CODE           = 0x00004406,
    VM_EXIT_INFO_IDT_VECTORING                  = 0x00004408,
    VM_EXIT_INFO_IDT_VECTORING_ERROR_CODE       = 0x0000440a,
    VM_EXIT_INFO_INSTRUCTION_LENGTH             = 0x0000440c,
    VM_EXIT_INFO_INSTRUCTION_INFO               = 0x0000440e,
    VM_EXIT_INFO_QUALIFICATION                  = 0x00006400,
    VM_EXIT_INFO_IO_ECX                         = 0x00006402,
    VM_EXIT_INFO_IO_ESI                         = 0x00006404,
    VM_EXIT_INFO_IO_EDI                         = 0x00006406,
    VM_EXIT_INFO_IO_EIP                         = 0x00006408,
    VM_EXIT_INFO_GUEST_LINEAR_ADDRESS           = 0x0000640a,
    VM_EXIT_INFO_GUEST_PHYSICAL_ADDRESS         = 0x00002400,

    HOST_RIP                                    = 0x00006c16,
    HOST_RSP                                    = 0x00006c14,
    HOST_CR0                                    = 0x00006c00,
    HOST_CR3                                    = 0x00006c02,
    HOST_CR4                                    = 0x00006c04,

    HOST_CS_SELECTOR                            = 0x00000c02,
    HOST_DS_SELECTOR                            = 0x00000c06,
    HOST_ES_SELECTOR                            = 0x00000c00,
    HOST_FS_SELECTOR                            = 0x00000c08,
    HOST_GS_SELECTOR                            = 0x00000c0a,
    HOST_SS_SELECTOR                            = 0x00000c04,
    HOST_TR_SELECTOR                            = 0x00000c0c,
    HOST_FS_BASE                                = 0x00006c06,
    HOST_GS_BASE                                = 0x00006c08,
    HOST_TR_BASE                                = 0x00006c0a,
    HOST_GDTR_BASE                              = 0x00006c0c,
    HOST_IDTR_BASE                              = 0x00006c0e,

    HOST_SYSENTER_CS                            = 0x00004c00,
    HOST_SYSENTER_ESP                           = 0x00006c10,
    HOST_SYSENTER_EIP                           = 0x00006c12,

    HOST_PAT                                    = 0x00002c00,
    HOST_EFER                                   = 0x00002c02,
    HOST_PERF_GLOBAL_CTRL                       = 0x00002c04,


    GUEST_RIP                                   = 0x0000681e,
    GUEST_RFLAGS                                = 0x00006820,
    GUEST_RSP                                   = 0x0000681c,
    GUEST_CR0                                   = 0x00006800,
    GUEST_CR3                                   = 0x00006802,
    GUEST_CR4                                   = 0x00006804,

    GUEST_ES_SELECTOR                           = 0x00000800,
    GUEST_CS_SELECTOR                           = 0x00000802,
    GUEST_SS_SELECTOR                           = 0x00000804,
    GUEST_DS_SELECTOR                           = 0x00000806,
    GUEST_FS_SELECTOR                           = 0x00000808,
    GUEST_GS_SELECTOR                           = 0x0000080a,
    GUEST_LDTR_SELECTOR                         = 0x0000080c,
    GUEST_TR_SELECTOR                           = 0x0000080e,

    GUEST_ES_AR                                 = 0x00004814,
    GUEST_CS_AR                                 = 0x00004816,
    GUEST_SS_AR                                 = 0x00004818,
    GUEST_DS_AR                                 = 0x0000481a,
    GUEST_FS_AR                                 = 0x0000481c,
    GUEST_GS_AR                                 = 0x0000481e,
    GUEST_LDTR_AR                               = 0x00004820,
    GUEST_TR_AR                                 = 0x00004822,

    GUEST_ES_BASE                               = 0x00006806,
    GUEST_CS_BASE                               = 0x00006808,
    GUEST_SS_BASE                               = 0x0000680a,
    GUEST_DS_BASE                               = 0x0000680c,
    GUEST_FS_BASE                               = 0x0000680e,
    GUEST_GS_BASE                               = 0x00006810,
    GUEST_LDTR_BASE                             = 0x00006812,
    GUEST_TR_BASE                               = 0x00006814,
    GUEST_GDTR_BASE                             = 0x00006816,
    GUEST_IDTR_BASE                             = 0x00006818,

    GUEST_ES_LIMIT                              = 0x00004800,
    GUEST_CS_LIMIT                              = 0x00004802,
    GUEST_SS_LIMIT                              = 0x00004804,
    GUEST_DS_LIMIT                              = 0x00004806,
    GUEST_FS_LIMIT                              = 0x00004808,
    GUEST_GS_LIMIT                              = 0x0000480a,
    GUEST_LDTR_LIMIT                            = 0x0000480c,
    GUEST_TR_LIMIT                              = 0x0000480e,
    GUEST_GDTR_LIMIT                            = 0x00004810,
    GUEST_IDTR_LIMIT                            = 0x00004812,

    GUEST_VMCS_LINK_PTR                         = 0x00002800,
    GUEST_DEBUGCTL                              = 0x00002802,
    GUEST_PAT                                   = 0x00002804,
    GUEST_EFER                                  = 0x00002806,
    GUEST_PERF_GLOBAL_CTRL                      = 0x00002808,
    GUEST_PDPTE0                                = 0x0000280a,
    GUEST_PDPTE1                                = 0x0000280c,
    GUEST_PDPTE2                                = 0x0000280e,
    GUEST_PDPTE3                                = 0x00002810,

    GUEST_DR7                                   = 0x0000681a,
    GUEST_PENDING_DBE                           = 0x00006822,
    GUEST_SYSENTER_CS                           = 0x0000482a,
    GUEST_SYSENTER_ESP                          = 0x00006824,
    GUEST_SYSENTER_EIP                          = 0x00006826,
    GUEST_SMBASE                                = 0x00004828,
    GUEST_INTERRUPTIBILITY                      = 0x00004824,
    GUEST_ACTIVITY_STATE                        = 0x00004826,

    /* Invalid encoding */
    VMCS_NO_COMPONENT                           = 0x0000ffff
};

static uint32_t dump_vmcs_list[] = {
    VMX_PIN_CONTROLS,
    VMX_PRIMARY_PROCESSOR_CONTROLS,
    VMX_SECONDARY_PROCESSOR_CONTROLS,
    VMX_EXCEPTION_BITMAP,
    VMX_PAGE_FAULT_ERROR_CODE_MASK,
    VMX_PAGE_FAULT_ERROR_CODE_MATCH,
    VMX_EXIT_CONTROLS,
    VMX_EXIT_MSR_STORE_COUNT,
    VMX_EXIT_MSR_LOAD_COUNT,
    VMX_ENTRY_CONTROLS,
    VMX_ENTRY_MSR_LOAD_COUNT,
    VMX_ENTRY_INTERRUPT_INFO,
    VMX_ENTRY_EXCEPTION_ERROR_CODE,
    VMX_ENTRY_INSTRUCTION_LENGTH,
    VMX_TPR_THRESHOLD,

    VMX_CR0_MASK,
    VMX_CR4_MASK,
    VMX_CR0_READ_SHADOW,
    VMX_CR4_READ_SHADOW,
    VMX_CR3_TARGET_COUNT,
    VMX_CR3_TARGET_VAL_BASE, // x6008-x6206

    VMX_VPID,
    VMX_IO_BITMAP_A,
    VMX_IO_BITMAP_B,
    VMX_MSR_BITMAP,
    VMX_EXIT_MSR_STORE_ADDRESS,
    VMX_EXIT_MSR_LOAD_ADDRESS,
    VMX_ENTRY_MSR_LOAD_ADDRESS,
    VMX_TSC_OFFSET,
    VMX_VAPIC_PAGE,
    VMX_APIC_ACCESS_PAGE,
    VMX_EPTP,
    VMX_PREEMPTION_TIMER,

    VMX_INSTRUCTION_ERROR_CODE,

    VM_EXIT_INFO_REASON,
    VM_EXIT_INFO_INTERRUPT_INFO,
    VM_EXIT_INFO_EXCEPTION_ERROR_CODE,
    VM_EXIT_INFO_IDT_VECTORING,
    VM_EXIT_INFO_IDT_VECTORING_ERROR_CODE,
    VM_EXIT_INFO_INSTRUCTION_LENGTH,
    VM_EXIT_INFO_INSTRUCTION_INFO,
    VM_EXIT_INFO_QUALIFICATION,
    VM_EXIT_INFO_IO_ECX,
    VM_EXIT_INFO_IO_ESI,
    VM_EXIT_INFO_IO_EDI,
    VM_EXIT_INFO_IO_EIP,
    VM_EXIT_INFO_GUEST_LINEAR_ADDRESS,
    VM_EXIT_INFO_GUEST_PHYSICAL_ADDRESS,

    HOST_RIP,
    HOST_RSP,
    HOST_CR0,
    HOST_CR3,
    HOST_CR4,

    HOST_CS_SELECTOR,
    HOST_DS_SELECTOR,
    HOST_ES_SELECTOR,
    HOST_FS_SELECTOR,
    HOST_GS_SELECTOR,
    HOST_SS_SELECTOR,
    HOST_TR_SELECTOR,
    HOST_FS_BASE,
    HOST_GS_BASE,
    HOST_TR_BASE,
    HOST_GDTR_BASE,
    HOST_IDTR_BASE,

    HOST_SYSENTER_CS,
    HOST_SYSENTER_ESP,
    HOST_SYSENTER_EIP,

    GUEST_RIP,
    GUEST_RFLAGS,
    GUEST_RSP,
    GUEST_CR0,
    GUEST_CR3,
    GUEST_CR4,

    GUEST_ES_SELECTOR,
    GUEST_CS_SELECTOR,
    GUEST_SS_SELECTOR,
    GUEST_DS_SELECTOR,
    GUEST_FS_SELECTOR,
    GUEST_GS_SELECTOR,
    GUEST_LDTR_SELECTOR,
    GUEST_TR_SELECTOR,

    GUEST_ES_AR,
    GUEST_CS_AR,
    GUEST_SS_AR,
    GUEST_DS_AR,
    GUEST_FS_AR,
    GUEST_GS_AR,
    GUEST_LDTR_AR,
    GUEST_TR_AR,

    GUEST_ES_BASE,
    GUEST_CS_BASE,
    GUEST_SS_BASE,
    GUEST_DS_BASE,
    GUEST_FS_BASE,
    GUEST_GS_BASE,
    GUEST_LDTR_BASE,
    GUEST_TR_BASE,
    GUEST_GDTR_BASE,
    GUEST_IDTR_BASE,

    GUEST_ES_LIMIT,
    GUEST_CS_LIMIT,
    GUEST_SS_LIMIT,
    GUEST_DS_LIMIT,
    GUEST_FS_LIMIT,
    GUEST_GS_LIMIT,
    GUEST_LDTR_LIMIT,
    GUEST_TR_LIMIT,
    GUEST_GDTR_LIMIT,
    GUEST_IDTR_LIMIT,

    GUEST_VMCS_LINK_PTR,
    GUEST_DEBUGCTL,
    GUEST_PAT,
    GUEST_EFER,
    GUEST_PERF_GLOBAL_CTRL,
    GUEST_PDPTE0,
    GUEST_PDPTE1,
    GUEST_PDPTE2,
    GUEST_PDPTE3,

    GUEST_DR7,
    GUEST_PENDING_DBE,
    GUEST_SYSENTER_CS,
    GUEST_SYSENTER_ESP,
    GUEST_SYSENTER_EIP,
    GUEST_SMBASE,
    GUEST_INTERRUPTIBILITY,
    GUEST_ACTIVITY_STATE,
};

#define ARRAY_ELEMENTS(x)       (sizeof(x)/sizeof((x)[0]))

const char *name_vmcs_component(int value)
{
    // The following macro compactifies the VMCS component encoding to 9 bits,
    // thus all major compilers will optimize the switch-case via jump tables.
    // Compilers will ensure this is a perfect hash function, by preventing
    // the two identical cases to exist.
    // The original encoding is described by:
    // Intel SDM Vol. 3C: Table 24-17. Structure of VMCS Component Encoding
    // Our compact encoding is described by:
    // [W WTTI IIII]  W=Width, T=Type, I=Index

    // As of this writing, no VMCS component is encoded with an Index of more
    // than 5 bits (Intel SDM Vol. 3D, Appendix B: Field Encoding in VMCS),
    // even though 9 bits are allocated to Index.
#define HASH(x) \
    (((x) & 0x003E) >> 1) /* Index */ | \
    (((x) & 0x0C00) >> 5) /* Type  */ | \
    (((x) & 0x6000) >> 6) /* Width */
#define CASE(x) \
    case HASH(x): \
        return #x

    switch (HASH(value)) {
    CASE(VMX_PIN_CONTROLS);
    CASE(VMX_PRIMARY_PROCESSOR_CONTROLS);
    CASE(VMX_SECONDARY_PROCESSOR_CONTROLS);
    CASE(VMX_EXCEPTION_BITMAP);
    CASE(VMX_PAGE_FAULT_ERROR_CODE_MASK);
    CASE(VMX_PAGE_FAULT_ERROR_CODE_MATCH);
    CASE(VMX_EXIT_CONTROLS);
    CASE(VMX_EXIT_MSR_STORE_COUNT);
    CASE(VMX_EXIT_MSR_LOAD_COUNT);
    CASE(VMX_ENTRY_CONTROLS);
    CASE(VMX_ENTRY_MSR_LOAD_COUNT);
    CASE(VMX_ENTRY_INTERRUPT_INFO);
    CASE(VMX_ENTRY_EXCEPTION_ERROR_CODE);
    CASE(VMX_ENTRY_INSTRUCTION_LENGTH);
    CASE(VMX_TPR_THRESHOLD);
    CASE(VMX_CR0_MASK);
    CASE(VMX_CR4_MASK);
    CASE(VMX_CR0_READ_SHADOW);
    CASE(VMX_CR4_READ_SHADOW);
    CASE(VMX_CR3_TARGET_COUNT);
    CASE(VMX_CR3_TARGET_VAL_BASE);
    CASE(VMX_VPID);
    CASE(VMX_IO_BITMAP_A);
    CASE(VMX_IO_BITMAP_B);
    CASE(VMX_MSR_BITMAP);
    CASE(VMX_EXIT_MSR_STORE_ADDRESS);
    CASE(VMX_EXIT_MSR_LOAD_ADDRESS);
    CASE(VMX_ENTRY_MSR_LOAD_ADDRESS);
    CASE(VMX_TSC_OFFSET);
    CASE(VMX_VAPIC_PAGE);
    CASE(VMX_APIC_ACCESS_PAGE);
    CASE(VMX_EPTP);
    CASE(VMX_PREEMPTION_TIMER);
    CASE(VMX_INSTRUCTION_ERROR_CODE);
    CASE(VM_EXIT_INFO_REASON);
    CASE(VM_EXIT_INFO_INTERRUPT_INFO);
    CASE(VM_EXIT_INFO_EXCEPTION_ERROR_CODE);
    CASE(VM_EXIT_INFO_IDT_VECTORING);
    CASE(VM_EXIT_INFO_IDT_VECTORING_ERROR_CODE);
    CASE(VM_EXIT_INFO_INSTRUCTION_LENGTH);
    CASE(VM_EXIT_INFO_INSTRUCTION_INFO);
    CASE(VM_EXIT_INFO_QUALIFICATION);
    CASE(VM_EXIT_INFO_IO_ECX);
    CASE(VM_EXIT_INFO_IO_ESI);
    CASE(VM_EXIT_INFO_IO_EDI);
    CASE(VM_EXIT_INFO_IO_EIP);
    CASE(VM_EXIT_INFO_GUEST_LINEAR_ADDRESS);
    CASE(VM_EXIT_INFO_GUEST_PHYSICAL_ADDRESS);
    CASE(HOST_RIP);
    CASE(HOST_RSP);
    CASE(HOST_CR0);
    CASE(HOST_CR3);
    CASE(HOST_CR4);
    CASE(HOST_CS_SELECTOR);
    CASE(HOST_DS_SELECTOR);
    CASE(HOST_ES_SELECTOR);
    CASE(HOST_FS_SELECTOR);
    CASE(HOST_GS_SELECTOR);
    CASE(HOST_SS_SELECTOR);
    CASE(HOST_TR_SELECTOR);
    CASE(HOST_FS_BASE);
    CASE(HOST_GS_BASE);
    CASE(HOST_TR_BASE);
    CASE(HOST_GDTR_BASE);
    CASE(HOST_IDTR_BASE);
    CASE(HOST_SYSENTER_CS);
    CASE(HOST_SYSENTER_ESP);
    CASE(HOST_SYSENTER_EIP);
    CASE(HOST_PAT);
    CASE(HOST_EFER);
    CASE(HOST_PERF_GLOBAL_CTRL);
    CASE(GUEST_RIP);
    CASE(GUEST_RFLAGS);
    CASE(GUEST_RSP);
    CASE(GUEST_CR0);
    CASE(GUEST_CR3);
    CASE(GUEST_CR4);
    CASE(GUEST_ES_SELECTOR);
    CASE(GUEST_CS_SELECTOR);
    CASE(GUEST_SS_SELECTOR);
    CASE(GUEST_DS_SELECTOR);
    CASE(GUEST_FS_SELECTOR);
    CASE(GUEST_GS_SELECTOR);
    CASE(GUEST_LDTR_SELECTOR);
    CASE(GUEST_TR_SELECTOR);
    CASE(GUEST_ES_AR);
    CASE(GUEST_CS_AR);
    CASE(GUEST_SS_AR);
    CASE(GUEST_DS_AR);
    CASE(GUEST_FS_AR);
    CASE(GUEST_GS_AR);
    CASE(GUEST_LDTR_AR);
    CASE(GUEST_TR_AR);
    CASE(GUEST_ES_BASE);
    CASE(GUEST_CS_BASE);
    CASE(GUEST_SS_BASE);
    CASE(GUEST_DS_BASE);
    CASE(GUEST_FS_BASE);
    CASE(GUEST_GS_BASE);
    CASE(GUEST_LDTR_BASE);
    CASE(GUEST_TR_BASE);
    CASE(GUEST_GDTR_BASE);
    CASE(GUEST_IDTR_BASE);
    CASE(GUEST_ES_LIMIT);
    CASE(GUEST_CS_LIMIT);
    CASE(GUEST_SS_LIMIT);
    CASE(GUEST_DS_LIMIT);
    CASE(GUEST_FS_LIMIT);
    CASE(GUEST_GS_LIMIT);
    CASE(GUEST_LDTR_LIMIT);
    CASE(GUEST_TR_LIMIT);
    CASE(GUEST_GDTR_LIMIT);
    CASE(GUEST_IDTR_LIMIT);
    CASE(GUEST_VMCS_LINK_PTR);
    CASE(GUEST_DEBUGCTL);
    CASE(GUEST_PAT);
    CASE(GUEST_EFER);
    CASE(GUEST_PERF_GLOBAL_CTRL);
    CASE(GUEST_PDPTE0);
    CASE(GUEST_PDPTE1);
    CASE(GUEST_PDPTE2);
    CASE(GUEST_PDPTE3);
    CASE(GUEST_DR7);
    CASE(GUEST_PENDING_DBE);
    CASE(GUEST_SYSENTER_CS);
    CASE(GUEST_SYSENTER_ESP);
    CASE(GUEST_SYSENTER_EIP);
    CASE(GUEST_SMBASE);
    CASE(GUEST_INTERRUPTIBILITY);
    CASE(GUEST_ACTIVITY_STATE);
    default:
        return "";
    }
#undef HASH
#undef CASE
}

static void dump_vmcs(void *opaque, uint64_t (*vmread)(void* opaque, uint32_t enc))
{
    uint32_t i, enc, n;
    const char *name;

    uint32_t *list = dump_vmcs_list;
    n = ARRAY_ELEMENTS(dump_vmcs_list);

    for (i = 0; i < n; i++) {
        enc = list[i];
        name = name_vmcs_component(enc);
        uint64_t val = vmread(opaque, enc);
        printf("%04x %s: %"PRIx64"\n", enc, name, val);
    }
}
