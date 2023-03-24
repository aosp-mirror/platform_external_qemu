/*
 * QEMU AEHD support
 *
 * Copyright IBM, Corp. 2008
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * Copyright (c) 2017 Intel Corporation
 *  Written by:
 *  Haitao Shan <hshan@google.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef AEHD_INTERFACE_H
#define AEHD_INTERFACE_H

#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#include <windef.h>
#endif
#include <memory.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#define __u8 uint8_t
#define __u16 uint16_t
#define __u32 uint32_t
#define __u64 uint64_t
#define __s8 int8_t
#define __s16 int16_t
#define __s32 int32_t
#define __s64 int64_t

/*
 * AEHD x86 specific structures and definitions
 *
 */

#define DE_VECTOR 0
#define DB_VECTOR 1
#define BP_VECTOR 3
#define OF_VECTOR 4
#define BR_VECTOR 5
#define UD_VECTOR 6
#define NM_VECTOR 7
#define DF_VECTOR 8
#define TS_VECTOR 10
#define NP_VECTOR 11
#define SS_VECTOR 12
#define GP_VECTOR 13
#define PF_VECTOR 14
#define MF_VECTOR 16
#define AC_VECTOR 17
#define MC_VECTOR 18
#define XM_VECTOR 19
#define VE_VECTOR 20

/* Architectural interrupt line count. */
#define AEHD_NR_INTERRUPTS 256

struct aehd_memory_alias {
	__u32 slot;  /* this has a different namespace than memory slots */
	__u32 flags;
	__u64 guest_phys_addr;
	__u64 memory_size;
	__u64 target_phys_addr;
};

/* for AEHD_GET_IRQCHIP and AEHD_SET_IRQCHIP */
struct aehd_pic_state {
	__u8 last_irr;	/* edge detection */
	__u8 irr;		/* interrupt request register */
	__u8 imr;		/* interrupt mask register */
	__u8 isr;		/* interrupt service register */
	__u8 priority_add;	/* highest irq priority */
	__u8 irq_base;
	__u8 read_reg_select;
	__u8 poll;
	__u8 special_mask;
	__u8 init_state;
	__u8 auto_eoi;
	__u8 rotate_on_auto_eoi;
	__u8 special_fully_nested_mode;
	__u8 init4;		/* true if 4 byte init */
	__u8 elcr;		/* PIIX edge/trigger selection */
	__u8 elcr_mask;
};

#define AEHD_IOAPIC_NUM_PINS  24
struct aehd_ioapic_state {
	__u64 base_address;
	__u32 ioregsel;
	__u32 id;
	__u32 irr;
	__u32 pad;
	union {
		__u64 bits;
		struct {
			__u8 vector;
			__u8 delivery_mode:3;
			__u8 dest_mode:1;
			__u8 delivery_status:1;
			__u8 polarity:1;
			__u8 remote_irr:1;
			__u8 trig_mode:1;
			__u8 mask:1;
			__u8 reserve:7;
			__u8 reserved[4];
			__u8 dest_id;
		} fields;
	} redirtbl[AEHD_IOAPIC_NUM_PINS];
};

#define AEHD_IRQCHIP_PIC_MASTER   0
#define AEHD_IRQCHIP_PIC_SLAVE    1
#define AEHD_IRQCHIP_IOAPIC       2
#define AEHD_NR_IRQCHIPS          3

#define AEHD_RUN_X86_SMM		 (1 << 0)

/* for AEHD_GET_REGS and AEHD_SET_REGS */
struct aehd_regs {
	/* out (AEHD_GET_REGS) / in (AEHD_SET_REGS) */
	__u64 rax, rbx, rcx, rdx;
	__u64 rsi, rdi, rsp, rbp;
	__u64 r8,  r9,  r10, r11;
	__u64 r12, r13, r14, r15;
	__u64 rip, rflags;
};

/* for AEHD_GET_LAPIC and AEHD_SET_LAPIC */
#define AEHD_APIC_REG_SIZE 0x400
struct aehd_lapic_state {
	char regs[AEHD_APIC_REG_SIZE];
};

struct aehd_segment {
	__u64 base;
	__u32 limit;
	__u16 selector;
	__u8  type;
	__u8  present, dpl, db, s, l, g, avl;
	__u8  unusable;
	__u8  padding;
};

struct aehd_dtable {
	__u64 base;
	__u16 limit;
	__u16 padding[3];
};


/* for AEHD_GET_SREGS and AEHD_SET_SREGS */
struct aehd_sregs {
	/* out (AEHD_GET_SREGS) / in (AEHD_SET_SREGS) */
	struct aehd_segment cs, ds, es, fs, gs, ss;
	struct aehd_segment tr, ldt;
	struct aehd_dtable gdt, idt;
	__u64 cr0, cr2, cr3, cr4, cr8;
	__u64 efer;
	__u64 apic_base;
	__u64 interrupt_bitmap[(AEHD_NR_INTERRUPTS + 63) / 64];
};

/* for AEHD_GET_FPU and AEHD_SET_FPU */
struct aehd_fpu {
	__u8  fpr[8][16];
	__u16 fcw;
	__u16 fsw;
	__u8  ftwx;  /* in fxsave format */
	__u8  pad1;
	__u16 last_opcode;
	__u64 last_ip;
	__u64 last_dp;
	__u8  xmm[16][16];
	__u32 mxcsr;
	__u32 pad2;
};

struct aehd_msr_entry {
	__u32 index;
	__u32 reserved;
	__u64 data;
};

/* for AEHD_GET_MSRS and AEHD_SET_MSRS */
struct aehd_msrs {
	__u32 nmsrs; /* number of msrs in entries */
	__u32 pad;

	struct aehd_msr_entry entries[0];
};

/* for AEHD_GET_MSR_INDEX_LIST */
struct aehd_msr_list {
	__u32 nmsrs; /* number of msrs in entries */
	__u32 indices[0];
};

struct aehd_cpuid_entry {
	__u32 function;
	__u32 index;
	__u32 flags;
	__u32 eax;
	__u32 ebx;
	__u32 ecx;
	__u32 edx;
	__u32 padding[3];
};

#define AEHD_CPUID_FLAG_SIGNIFCANT_INDEX		(1 << 0)
#define AEHD_CPUID_FLAG_STATEFUL_FUNC		(1 << 1)
#define AEHD_CPUID_FLAG_STATE_READ_NEXT		(1 << 2)

/* for AEHD_SET_CPUID */
struct aehd_cpuid {
	__u32 nent;
	__u32 padding;
	struct aehd_cpuid_entry entries[0];
};

struct aehd_debug_exit_arch {
	__u32 exception;
	__u32 pad;
	__u64 pc;
	__u64 dr6;
	__u64 dr7;
};

#define AEHD_GUESTDBG_USE_SW_BP		0x00010000
#define AEHD_GUESTDBG_USE_HW_BP		0x00020000
#define AEHD_GUESTDBG_INJECT_DB		0x00040000
#define AEHD_GUESTDBG_INJECT_BP		0x00080000

/* for AEHD_SET_GUEST_DEBUG */
struct aehd_guest_debug_arch {
	__u64 debugreg[8];
};

/* When set in flags, include corresponding fields on AEHD_SET_VCPU_EVENTS */
#define AEHD_VCPUEVENT_VALID_NMI_PENDING	0x00000001
#define AEHD_VCPUEVENT_VALID_SIPI_VECTOR	0x00000002
#define AEHD_VCPUEVENT_VALID_SHADOW	0x00000004
#define AEHD_VCPUEVENT_VALID_SMM		0x00000008

/* Interrupt shadow states */
#define AEHD_X86_SHADOW_INT_MOV_SS	0x01
#define AEHD_X86_SHADOW_INT_STI		0x02

/* for AEHD_GET/SET_VCPU_EVENTS */
struct aehd_vcpu_events {
	struct {
		__u8 injected;
		__u8 nr;
		__u8 has_error_code;
		__u8 pad;
		__u32 error_code;
	} exception;
	struct {
		__u8 injected;
		__u8 nr;
		__u8 soft;
		__u8 shadow;
	} interrupt;
	struct {
		__u8 injected;
		__u8 pending;
		__u8 masked;
		__u8 pad;
	} nmi;
	__u32 sipi_vector;
	__u32 flags;
	struct {
		__u8 smm;
		__u8 pending;
		__u8 smm_inside_nmi;
		__u8 latched_init;
	} smi;
	__u32 reserved[9];
};

/* for AEHD_GET/SET_DEBUGREGS */
struct aehd_debugregs {
	__u64 db[4];
	__u64 dr6;
	__u64 dr7;
	__u64 flags;
	__u64 reserved[9];
};

/* for AEHD_CAP_XSAVE */
struct aehd_xsave {
	__u32 region[1024];
};

#define AEHD_MAX_XCRS	16

struct aehd_xcr {
	__u32 xcr;
	__u32 reserved;
	__u64 value;
};

struct aehd_xcrs {
	__u32 nr_xcrs;
	__u32 flags;
	struct aehd_xcr xcrs[AEHD_MAX_XCRS];
	__u64 padding[16];
};

/* definition of registers in aehd_run */
struct aehd_sync_regs {
};

#define AEHD_X86_QUIRK_LINT0_REENABLED	(1 << 0)
#define AEHD_X86_QUIRK_CD_NW_CLEARED	(1 << 1)

#define FILE_DEVICE_AEHD 0xE3E3

/* Macros to convert Linux style ioctl to Windows */
#define __IO(a,b) CTL_CODE(FILE_DEVICE_AEHD, b, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define __IOR(a,b,c) __IO(a,b)
#define __IOW(a,b,c) __IO(a,b)
#define __IOWR(a,b,c) __IO(a,b)

#define AEHD_API_VERSION 1

/* for AEHD_CREATE_MEMORY_REGION */
struct aehd_memory_region {
	__u32 slot;
	__u32 flags;
	__u64 guest_phys_addr;
	__u64 memory_size; /* bytes */
};

/* for AEHD_SET_USER_MEMORY_REGION */
struct aehd_userspace_memory_region {
	__u32 slot;
	__u32 flags;
	__u64 guest_phys_addr;
	__u64 memory_size; /* bytes */
	__u64 userspace_addr; /* start of the userspace allocated memory */
};

/*
 * The bit 0 ~ bit 15 of aehd_memory_region::flags are visible for userspace,
 * other bits are reserved for aehd internal use which are defined in
 * include/linux/aehd_host.h.
 */
#define AEHD_MEM_LOG_DIRTY_PAGES	(1UL << 0)
#define AEHD_MEM_READONLY	(1UL << 1)

/* for AEHD_IRQ_LINE */
struct aehd_irq_level {
	/*
	 * ACPI gsi notion of irq.
	 * For IA-64 (APIC model) IOAPIC0: irq 0-23; IOAPIC1: irq 24-47..
	 * For X86 (standard AT mode) PIC0/1: irq 0-15. IOAPIC0: 0-23..
	 * For ARM: See Documentation/virtual/aehd/api.txt
	 */
	union {
		__u32 irq;
		__s32 status;
	};
	__u32 level;
};


struct aehd_irqchip {
	__u32 chip_id;
	__u32 pad;
        union {
		char dummy[512];  /* reserving space */
                struct aehd_pic_state pic;
		struct aehd_ioapic_state ioapic;
	} chip;
};

#define AEHD_EXIT_UNKNOWN          0
#define AEHD_EXIT_EXCEPTION        1
#define AEHD_EXIT_IO               2
#define AEHD_EXIT_HYPERCALL        3
#define AEHD_EXIT_DEBUG            4
#define AEHD_EXIT_HLT              5
#define AEHD_EXIT_MMIO             6
#define AEHD_EXIT_IRQ_WINDOW_OPEN  7
#define AEHD_EXIT_SHUTDOWN         8
#define AEHD_EXIT_FAIL_ENTRY       9
#define AEHD_EXIT_INTR             10
#define AEHD_EXIT_SET_TPR          11
#define AEHD_EXIT_TPR_ACCESS       12
#define AEHD_EXIT_NMI              16
#define AEHD_EXIT_INTERNAL_ERROR   17
#define AEHD_EXIT_OSI              18
#define AEHD_EXIT_PAPR_HCALL	  19
#define AEHD_EXIT_WATCHDOG         21
#define AEHD_EXIT_EPR              23
#define AEHD_EXIT_SYSTEM_EVENT     24
#define AEHD_EXIT_IOAPIC_EOI       26

/* For AEHD_EXIT_INTERNAL_ERROR */
/* Emulate instruction failed. */
#define AEHD_INTERNAL_ERROR_EMULATION	1
/* Encounter unexpected simultaneous exceptions. */
#define AEHD_INTERNAL_ERROR_SIMUL_EX	2
/* Encounter unexpected vm-exit due to delivery event. */
#define AEHD_INTERNAL_ERROR_DELIVERY_EV	3

/* for AEHD_RUN, returned by mmap(vcpu_fd, offset=0) */
struct aehd_run {
	/* in */
	__u8 request_interrupt_window;
	__u8 user_event_pending;
	__u8 padding1[6];

	/* out */
	__u32 exit_reason;
	__u8 ready_for_interrupt_injection;
	__u8 if_flag;
	__u16 flags;

	/* in (pre_aehd_run), out (post_aehd_run) */
	__u64 cr8;
	__u64 apic_base;

	union {
		/* AEHD_EXIT_UNKNOWN */
		struct {
			__u64 hardware_exit_reason;
		} hw;
		/* AEHD_EXIT_FAIL_ENTRY */
		struct {
			__u64 hardware_entry_failure_reason;
		} fail_entry;
		/* AEHD_EXIT_EXCEPTION */
		struct {
			__u32 exception;
			__u32 error_code;
		} ex;
		/* AEHD_EXIT_IO */
		struct {
#define AEHD_EXIT_IO_IN  0
#define AEHD_EXIT_IO_OUT 1
			__u8 direction;
			__u8 size; /* bytes */
			__u16 port;
			__u32 count;
			__u64 data_offset; /* relative to aehd_run start */
		} io;
		/* AEHD_EXIT_DEBUG */
		struct {
			struct aehd_debug_exit_arch arch;
		} debug;
		/* AEHD_EXIT_MMIO */
		struct {
			__u64 phys_addr;
			__u8  data[8];
			__u32 len;
			__u8  is_write;
		} mmio;
		/* AEHD_EXIT_HYPERCALL */
		struct {
			__u64 nr;
			__u64 args[6];
			__u64 ret;
			__u32 longmode;
			__u32 pad;
		} hypercall;
		/* AEHD_EXIT_TPR_ACCESS */
		struct {
			__u64 rip;
			__u32 is_write;
			__u32 pad;
		} tpr_access;
		/* AEHD_EXIT_INTERNAL_ERROR */
		struct {
			__u32 suberror;
			/* Available with AEHD_CAP_INTERNAL_ERROR_DATA: */
			__u32 ndata;
			__u64 data[16];
		} internal;
		/* AEHD_EXIT_OSI */
		struct {
			__u64 gprs[32];
		} osi;
		/* AEHD_EXIT_PAPR_HCALL */
		struct {
			__u64 nr;
			__u64 ret;
			__u64 args[9];
		} papr_hcall;
		/* AEHD_EXIT_EPR */
		struct {
			__u32 epr;
		} epr;
		/* AEHD_EXIT_SYSTEM_EVENT */
		struct {
#define AEHD_SYSTEM_EVENT_SHUTDOWN       1
#define AEHD_SYSTEM_EVENT_RESET          2
#define AEHD_SYSTEM_EVENT_CRASH          3
			__u32 type;
			__u64 flags;
		} system_event;
		/* AEHD_EXIT_IOAPIC_EOI */
		struct {
			__u8 vector;
		} eoi;
		/* Fix the size of the union. */
		char padding[256];
	};

	/*
	 * shared registers between aehd and userspace.
	 * aehd_valid_regs specifies the register classes set by the host
	 * aehd_dirty_regs specified the register classes dirtied by userspace
	 * struct aehd_sync_regs is architecture specific, as well as the
	 * bits for aehd_valid_regs and aehd_dirty_regs
	 */
	__u64 aehd_valid_regs;
	__u64 aehd_dirty_regs;
	union {
		struct aehd_sync_regs regs;
		char padding[2048];
	} s;
};

/* for AEHD_TRANSLATE */
struct aehd_translation {
	/* in */
	__u64 linear_address;

	/* out */
	__u64 physical_address;
	__u8  valid;
	__u8  writeable;
	__u8  usermode;
	__u8  pad[5];
};

/* for AEHD_INTERRUPT */
struct aehd_interrupt {
	/* in */
	__u32 irq;
};

/* for AEHD_GET_DIRTY_LOG */
struct aehd_dirty_log {
	__u32 slot;
	__u32 padding1;
	union {
		void *dirty_bitmap; /* one bit per page */
		__u64 padding2;
	};
};

/* for AEHD_TPR_ACCESS_REPORTING */
struct aehd_tpr_access_ctl {
	__u32 enabled;
	__u32 flags;
	__u32 reserved[8];
};

/* for AEHD_SET_VAPIC_ADDR */
struct aehd_vapic_addr {
	__u64 vapic_addr;
};

/* for AEHD_SET_MP_STATE */

/* not all states are valid on all architectures */
#define AEHD_MP_STATE_RUNNABLE          0
#define AEHD_MP_STATE_UNINITIALIZED     1
#define AEHD_MP_STATE_INIT_RECEIVED     2
#define AEHD_MP_STATE_HALTED            3
#define AEHD_MP_STATE_SIPI_RECEIVED     4
#define AEHD_MP_STATE_STOPPED           5
#define AEHD_MP_STATE_CHECK_STOP        6
#define AEHD_MP_STATE_OPERATING         7
#define AEHD_MP_STATE_LOAD              8

struct aehd_mp_state {
	__u32 mp_state;
};

/* for AEHD_SET_GUEST_DEBUG */

#define AEHD_GUESTDBG_ENABLE		0x00000001
#define AEHD_GUESTDBG_SINGLESTEP		0x00000002

struct aehd_guest_debug {
	__u32 control;
	__u32 pad;
	struct aehd_guest_debug_arch arch;
};

/* for AEHD_ENABLE_CAP */
struct aehd_enable_cap {
	/* in */
	__u32 cap;
	__u32 flags;
	__u64 args[4];
	__u8  pad[64];
};

/*
 * ioctls for /dev/aehd fds:
 */
#define AEHD_GET_API_VERSION       __IO(AEHDIO,   0x00)
#define AEHD_CREATE_VM             __IO(AEHDIO,   0x01) /* returns a VM fd */
#define AEHD_GET_MSR_INDEX_LIST    __IOWR(AEHDIO, 0x02, struct aehd_msr_list)
/*
 * Check if a aehd extension is available.  Argument is extension number,
 * return is 1 (yes) or 0 (no, sorry).
 */
#define AEHD_CHECK_EXTENSION       __IO(AEHDIO,   0x03)
/*
 * Get size for mmap(vcpu_fd)
 */
#define AEHD_GET_VCPU_MMAP_SIZE    __IO(AEHDIO,   0x04) /* in bytes */
#define AEHD_GET_SUPPORTED_CPUID   __IOWR(AEHDIO, 0x05, struct aehd_cpuid)
#define AEHD_GET_EMULATED_CPUID	  __IOWR(AEHDIO, 0x09, struct aehd_cpuid)
/*
 * Extension capability list.
 */
#define AEHD_CAP_IRQCHIP	  0
#define AEHD_CAP_HLT	  1
#define AEHD_CAP_MMU_SHADOW_CACHE_CONTROL 2
#define AEHD_CAP_VAPIC 6
#define AEHD_CAP_NR_VCPUS 9       /* returns recommended max vcpus per vm */
#define AEHD_CAP_NR_MEMSLOTS 10   /* returns max memory slots per vm */
#define AEHD_CAP_SYNC_MMU 16  /* Changes to host mmap are reflected in guest */
#define AEHD_CAP_IOMMU 18
#define AEHD_CAP_USER_NMI 22
#define AEHD_CAP_IRQ_ROUTING 25
#define AEHD_CAP_SET_BOOT_CPU_ID 34
#define AEHD_CAP_SET_IDENTITY_MAP_ADDR 37
#define AEHD_CAP_PCI_SEGMENT 47
#define AEHD_CAP_INTR_SHADOW 49
#define AEHD_CAP_ENABLE_CAP 54
#define AEHD_CAP_XSAVE 55
#define AEHD_CAP_XCRS 56
#define AEHD_CAP_MAX_VCPUS 66       /* returns max vcpus per vm */
#define AEHD_CAP_SW_TLB 69
#define AEHD_CAP_SYNC_REGS 74
#define AEHD_CAP_READONLY_MEM 81
#define AEHD_CAP_EXT_EMUL_CPUID 95
#define AEHD_CAP_IOAPIC_POLARITY_IGNORED 97
#define AEHD_CAP_ENABLE_CAP_VM 98
#define AEHD_CAP_VM_ATTRIBUTES 101
#define AEHD_CAP_CHECK_EXTENSION_VM 105
#define AEHD_CAP_DISABLE_QUIRKS 116
#define AEHD_CAP_X86_SMM 117
#define AEHD_CAP_MULTI_ADDRESS_SPACE 118
#define AEHD_CAP_GUEST_DEBUG_HW_BPS 119
#define AEHD_CAP_GUEST_DEBUG_HW_WPS 120
#define AEHD_CAP_VCPU_ATTRIBUTES 127
#define AEHD_CAP_MAX_VCPU_ID 128
#define AEHD_CAP_X2APIC_API 129
#define AEHD_CAP_MSI_DEVID 131

struct aehd_irq_routing_irqchip {
	__u32 irqchip;
	__u32 pin;
};

struct aehd_irq_routing_msi {
	__u32 address_lo;
	__u32 address_hi;
	__u32 data;
	union {
		__u32 pad;
		__u32 devid;
	};
};

struct aehd_irq_routing_hv_sint {
	__u32 vcpu;
	__u32 sint;
};

/* gsi routing entry types */
#define AEHD_IRQ_ROUTING_IRQCHIP 1
#define AEHD_IRQ_ROUTING_MSI 2
#define AEHD_IRQ_ROUTING_HV_SINT 4

struct aehd_irq_routing_entry {
	__u32 gsi;
	__u32 type;
	__u32 flags;
	__u32 pad;
	union {
		struct aehd_irq_routing_irqchip irqchip;
		struct aehd_irq_routing_msi msi;
		struct aehd_irq_routing_hv_sint hv_sint;
		__u32 pad[8];
	} u;
};

struct aehd_irq_routing {
	__u32 nr;
	__u32 flags;
	struct aehd_irq_routing_entry entries[0];
};

/* For AEHD_CAP_SW_TLB */

#define AEHD_MMU_FSL_BOOKE_NOHV		0
#define AEHD_MMU_FSL_BOOKE_HV		1

struct aehd_config_tlb {
	__u64 params;
	__u64 array;
	__u32 mmu_type;
	__u32 array_len;
};

struct aehd_dirty_tlb {
	__u64 bitmap;
	__u32 num_dirty;
};

/* Available with AEHD_CAP_ONE_REG */

#define AEHD_REG_ARCH_MASK	0xff00000000000000ULL
#define AEHD_REG_GENERIC		0x0000000000000000ULL

/*
 * Architecture specific registers are to be defined in arch headers and
 * ORed with the arch identifier.
 */
#define AEHD_REG_PPC		0x1000000000000000ULL
#define AEHD_REG_X86		0x2000000000000000ULL
#define AEHD_REG_IA64		0x3000000000000000ULL
#define AEHD_REG_ARM		0x4000000000000000ULL
#define AEHD_REG_S390		0x5000000000000000ULL
#define AEHD_REG_ARM64		0x6000000000000000ULL
#define AEHD_REG_MIPS		0x7000000000000000ULL

#define AEHD_REG_SIZE_SHIFT	52
#define AEHD_REG_SIZE_MASK	0x00f0000000000000ULL
#define AEHD_REG_SIZE_U8		0x0000000000000000ULL
#define AEHD_REG_SIZE_U16	0x0010000000000000ULL
#define AEHD_REG_SIZE_U32	0x0020000000000000ULL
#define AEHD_REG_SIZE_U64	0x0030000000000000ULL
#define AEHD_REG_SIZE_U128	0x0040000000000000ULL
#define AEHD_REG_SIZE_U256	0x0050000000000000ULL
#define AEHD_REG_SIZE_U512	0x0060000000000000ULL
#define AEHD_REG_SIZE_U1024	0x0070000000000000ULL

struct aehd_reg_list {
	__u64 n; /* number of regs */
	__u64 reg[0];
};

struct aehd_one_reg {
	__u64 id;
	__u64 addr;
};

#define AEHD_MSI_VALID_DEVID	(1U << 0)
struct aehd_msi {
	__u32 address_lo;
	__u32 address_hi;
	__u32 data;
	__u32 flags;
	__u32 devid;
	__u8  pad[12];
};

/*
 * ioctls for VM fds
 */
#define AEHD_SET_MEMORY_REGION     __IOW(AEHDIO,  0x40, struct aehd_memory_region)
/*
 * AEHD_CREATE_VCPU receives as a parameter the vcpu slot, and returns
 * a vcpu fd.
 */
#define AEHD_CREATE_VCPU           __IO(AEHDIO,   0x41)
#define AEHD_GET_DIRTY_LOG         __IOW(AEHDIO,  0x42, struct aehd_dirty_log)
/* AEHD_SET_MEMORY_ALIAS is obsolete: */
#define AEHD_SET_MEMORY_ALIAS      __IOW(AEHDIO,  0x43, struct aehd_memory_alias)
#define AEHD_SET_NR_MMU_PAGES      __IO(AEHDIO,   0x44)
#define AEHD_GET_NR_MMU_PAGES      __IO(AEHDIO,   0x45)
#define AEHD_SET_USER_MEMORY_REGION __IOW(AEHDIO, 0x46, \
					struct aehd_userspace_memory_region)
#define AEHD_SET_TSS_ADDR          __IO(AEHDIO,   0x47)
#define AEHD_SET_IDENTITY_MAP_ADDR __IOW(AEHDIO,  0x48, __u64)
#define AEHD_KICK_VCPU             __IO(AEHDIO,   0x49)

/* Device model IOC */
#define AEHD_CREATE_IRQCHIP        __IO(AEHDIO,   0x60)
#define AEHD_GET_IRQCHIP           __IOWR(AEHDIO, 0x62, struct aehd_irqchip)
#define AEHD_SET_IRQCHIP           __IOR(AEHDIO,  0x63, struct aehd_irqchip)
#define AEHD_IRQ_LINE_STATUS       __IOWR(AEHDIO, 0x67, struct aehd_irq_level)
#define AEHD_SET_GSI_ROUTING       __IOW(AEHDIO,  0x6a, struct aehd_irq_routing)
/* deprecated, replaced by AEHD_ASSIGN_DEV_IRQ */
#define AEHD_ASSIGN_IRQ            __AEHD_DEPRECATED_VM_R_0x70
#define AEHD_ASSIGN_DEV_IRQ        __IOW(AEHDIO,  0x70, struct aehd_assigned_irq)
#define AEHD_REINJECT_CONTROL      __IO(AEHDIO,   0x71)
#define AEHD_SET_BOOT_CPU_ID       __IO(AEHDIO,   0x78)

/*
 * ioctls for vcpu fds
 */
#define AEHD_RUN                   __IO(AEHDIO,   0x80)
#define AEHD_VCPU_MMAP             __IO(AEHDIO,   0x87)
#define AEHD_GET_REGS              __IOR(AEHDIO,  0x81, struct aehd_regs)
#define AEHD_SET_REGS              __IOW(AEHDIO,  0x82, struct aehd_regs)
#define AEHD_GET_SREGS             __IOR(AEHDIO,  0x83, struct aehd_sregs)
#define AEHD_SET_SREGS             __IOW(AEHDIO,  0x84, struct aehd_sregs)
#define AEHD_TRANSLATE             __IOWR(AEHDIO, 0x85, struct aehd_translation)
#define AEHD_INTERRUPT             __IOW(AEHDIO,  0x86, struct aehd_interrupt)
#define AEHD_GET_MSRS              __IOWR(AEHDIO, 0x88, struct aehd_msrs)
#define AEHD_SET_MSRS              __IOW(AEHDIO,  0x89, struct aehd_msrs)
#define AEHD_GET_FPU               __IOR(AEHDIO,  0x8c, struct aehd_fpu)
#define AEHD_SET_FPU               __IOW(AEHDIO,  0x8d, struct aehd_fpu)
#define AEHD_GET_LAPIC             __IOR(AEHDIO,  0x8e, struct aehd_lapic_state)
#define AEHD_SET_LAPIC             __IOW(AEHDIO,  0x8f, struct aehd_lapic_state)
#define AEHD_SET_CPUID             __IOW(AEHDIO,  0x90, struct aehd_cpuid)
#define AEHD_GET_CPUID             __IOWR(AEHDIO, 0x91, struct aehd_cpuid)
/* Available with AEHD_CAP_VAPIC */
#define AEHD_TPR_ACCESS_REPORTING  __IOWR(AEHDIO, 0x92, struct aehd_tpr_access_ctl)
/* Available with AEHD_CAP_VAPIC */
#define AEHD_SET_VAPIC_ADDR        __IOW(AEHDIO,  0x93, struct aehd_vapic_addr)
#define AEHD_GET_MP_STATE          __IOR(AEHDIO,  0x98, struct aehd_mp_state)
#define AEHD_SET_MP_STATE          __IOW(AEHDIO,  0x99, struct aehd_mp_state)
/* Available with AEHD_CAP_USER_NMI */
#define AEHD_NMI                   __IO(AEHDIO,   0x9a)
/* Available with AEHD_CAP_SET_GUEST_DEBUG */
#define AEHD_SET_GUEST_DEBUG       __IOW(AEHDIO,  0x9b, struct aehd_guest_debug)
/* Available with AEHD_CAP_VCPU_EVENTS */
#define AEHD_GET_VCPU_EVENTS       __IOR(AEHDIO,  0x9f, struct aehd_vcpu_events)
#define AEHD_SET_VCPU_EVENTS       __IOW(AEHDIO,  0xa0, struct aehd_vcpu_events)
/* Available with AEHD_CAP_DEBUGREGS */
#define AEHD_GET_DEBUGREGS         __IOR(AEHDIO,  0xa1, struct aehd_debugregs)
#define AEHD_SET_DEBUGREGS         __IOW(AEHDIO,  0xa2, struct aehd_debugregs)
/*
 * vcpu version available with AEHD_ENABLE_CAP
 * vm version available with AEHD_CAP_ENABLE_CAP_VM
 */
#define AEHD_ENABLE_CAP            __IOW(AEHDIO,  0xa3, struct aehd_enable_cap)
/* Available with AEHD_CAP_XSAVE */
#define AEHD_GET_XSAVE		  __IOR(AEHDIO,  0xa4, struct aehd_xsave)
#define AEHD_SET_XSAVE		  __IOW(AEHDIO,  0xa5, struct aehd_xsave)
/* Available with AEHD_CAP_XCRS */
#define AEHD_GET_XCRS		  __IOR(AEHDIO,  0xa6, struct aehd_xcrs)
#define AEHD_SET_XCRS		  __IOW(AEHDIO,  0xa7, struct aehd_xcrs)
/* Available with AEHD_CAP_SW_TLB */
#define AEHD_DIRTY_TLB		  __IOW(AEHDIO,  0xaa, struct aehd_dirty_tlb)
/* Available with AEHD_CAP_X86_SMM */
#define AEHD_SMI                   __IO(AEHDIO,   0xb7)

#define AEHD_X2APIC_API_USE_32BIT_IDS            (1ULL << 0)
#define AEHD_X2APIC_API_DISABLE_BROADCAST_QUIRK  (1ULL << 1)

#endif /*AEHD_INTERFACE_H */
