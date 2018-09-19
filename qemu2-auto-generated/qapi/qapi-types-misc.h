/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI types
 *
 * Copyright IBM, Corp. 2011
 * Copyright (c) 2013-2018 Red Hat Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#ifndef QAPI_TYPES_MISC_H
#define QAPI_TYPES_MISC_H

#include "qapi/qapi-builtin-types.h"

typedef struct QMPCapabilityList QMPCapabilityList;

typedef struct q_obj_qmp_capabilities_arg q_obj_qmp_capabilities_arg;

typedef enum QMPCapability {
    QMP_CAPABILITY_OOB = 0,
    QMP_CAPABILITY__MAX = 1,
} QMPCapability;

#define QMPCapability_str(val) \
    qapi_enum_lookup(&QMPCapability_lookup, (val))

extern const QEnumLookup QMPCapability_lookup;

typedef struct VersionTriple VersionTriple;

typedef struct VersionInfo VersionInfo;

typedef struct CommandInfo CommandInfo;

typedef struct CommandInfoList CommandInfoList;

typedef enum LostTickPolicy {
    LOST_TICK_POLICY_DISCARD = 0,
    LOST_TICK_POLICY_DELAY = 1,
    LOST_TICK_POLICY_MERGE = 2,
    LOST_TICK_POLICY_SLEW = 3,
    LOST_TICK_POLICY__MAX = 4,
} LostTickPolicy;

#define LostTickPolicy_str(val) \
    qapi_enum_lookup(&LostTickPolicy_lookup, (val))

extern const QEnumLookup LostTickPolicy_lookup;

typedef struct q_obj_add_client_arg q_obj_add_client_arg;

typedef struct NameInfo NameInfo;

typedef struct KvmInfo KvmInfo;

typedef struct UuidInfo UuidInfo;

typedef struct EventInfo EventInfo;

typedef struct EventInfoList EventInfoList;

typedef enum CpuInfoArch {
    CPU_INFO_ARCH_X86 = 0,
    CPU_INFO_ARCH_SPARC = 1,
    CPU_INFO_ARCH_PPC = 2,
    CPU_INFO_ARCH_MIPS = 3,
    CPU_INFO_ARCH_TRICORE = 4,
    CPU_INFO_ARCH_S390 = 5,
    CPU_INFO_ARCH_RISCV = 6,
    CPU_INFO_ARCH_OTHER = 7,
    CPU_INFO_ARCH__MAX = 8,
} CpuInfoArch;

#define CpuInfoArch_str(val) \
    qapi_enum_lookup(&CpuInfoArch_lookup, (val))

extern const QEnumLookup CpuInfoArch_lookup;

typedef struct q_obj_CpuInfo_base q_obj_CpuInfo_base;

typedef struct CpuInfo CpuInfo;

typedef struct CpuInfoX86 CpuInfoX86;

typedef struct CpuInfoSPARC CpuInfoSPARC;

typedef struct CpuInfoPPC CpuInfoPPC;

typedef struct CpuInfoMIPS CpuInfoMIPS;

typedef struct CpuInfoTricore CpuInfoTricore;

typedef struct CpuInfoRISCV CpuInfoRISCV;

typedef struct CpuInfoOther CpuInfoOther;

typedef enum CpuS390State {
    S390_CPU_STATE_UNINITIALIZED = 0,
    S390_CPU_STATE_STOPPED = 1,
    S390_CPU_STATE_CHECK_STOP = 2,
    S390_CPU_STATE_OPERATING = 3,
    S390_CPU_STATE_LOAD = 4,
    S390_CPU_STATE__MAX = 5,
} CpuS390State;

#define CpuS390State_str(val) \
    qapi_enum_lookup(&CpuS390State_lookup, (val))

extern const QEnumLookup CpuS390State_lookup;

typedef struct CpuInfoS390 CpuInfoS390;

typedef struct CpuInfoList CpuInfoList;

typedef struct q_obj_CpuInfoFast_base q_obj_CpuInfoFast_base;

typedef struct CpuInfoFast CpuInfoFast;

typedef struct CpuInfoFastList CpuInfoFastList;

typedef struct IOThreadInfo IOThreadInfo;

typedef struct IOThreadInfoList IOThreadInfoList;

typedef struct BalloonInfo BalloonInfo;

typedef struct q_obj_BALLOON_CHANGE_arg q_obj_BALLOON_CHANGE_arg;

typedef struct PciMemoryRange PciMemoryRange;

typedef struct PciMemoryRegion PciMemoryRegion;

typedef struct PciBusInfo PciBusInfo;

typedef struct PciDeviceInfoList PciDeviceInfoList;

typedef struct PciBridgeInfo PciBridgeInfo;

typedef struct PciDeviceClass PciDeviceClass;

typedef struct PciDeviceId PciDeviceId;

typedef struct PciMemoryRegionList PciMemoryRegionList;

typedef struct PciDeviceInfo PciDeviceInfo;

typedef struct PciInfo PciInfo;

typedef struct PciInfoList PciInfoList;

typedef struct q_obj_cpu_add_arg q_obj_cpu_add_arg;

typedef struct q_obj_memsave_arg q_obj_memsave_arg;

typedef struct q_obj_pmemsave_arg q_obj_pmemsave_arg;

typedef struct q_obj_balloon_arg q_obj_balloon_arg;

typedef struct q_obj_human_monitor_command_arg q_obj_human_monitor_command_arg;

typedef struct ObjectPropertyInfo ObjectPropertyInfo;

typedef struct q_obj_qom_list_arg q_obj_qom_list_arg;

typedef struct ObjectPropertyInfoList ObjectPropertyInfoList;

typedef struct q_obj_qom_get_arg q_obj_qom_get_arg;

typedef struct q_obj_qom_set_arg q_obj_qom_set_arg;

typedef struct q_obj_change_arg q_obj_change_arg;

typedef struct ObjectTypeInfo ObjectTypeInfo;

typedef struct q_obj_qom_list_types_arg q_obj_qom_list_types_arg;

typedef struct ObjectTypeInfoList ObjectTypeInfoList;

typedef struct q_obj_device_list_properties_arg q_obj_device_list_properties_arg;

typedef struct q_obj_qom_list_properties_arg q_obj_qom_list_properties_arg;

typedef struct q_obj_xen_set_global_dirty_log_arg q_obj_xen_set_global_dirty_log_arg;

typedef struct q_obj_device_add_arg q_obj_device_add_arg;

typedef struct q_obj_device_del_arg q_obj_device_del_arg;

typedef struct q_obj_DEVICE_DELETED_arg q_obj_DEVICE_DELETED_arg;

typedef enum DumpGuestMemoryFormat {
    DUMP_GUEST_MEMORY_FORMAT_ELF = 0,
    DUMP_GUEST_MEMORY_FORMAT_KDUMP_ZLIB = 1,
    DUMP_GUEST_MEMORY_FORMAT_KDUMP_LZO = 2,
    DUMP_GUEST_MEMORY_FORMAT_KDUMP_SNAPPY = 3,
    DUMP_GUEST_MEMORY_FORMAT__MAX = 4,
} DumpGuestMemoryFormat;

#define DumpGuestMemoryFormat_str(val) \
    qapi_enum_lookup(&DumpGuestMemoryFormat_lookup, (val))

extern const QEnumLookup DumpGuestMemoryFormat_lookup;

typedef struct q_obj_dump_guest_memory_arg q_obj_dump_guest_memory_arg;

typedef enum DumpStatus {
    DUMP_STATUS_NONE = 0,
    DUMP_STATUS_ACTIVE = 1,
    DUMP_STATUS_COMPLETED = 2,
    DUMP_STATUS_FAILED = 3,
    DUMP_STATUS__MAX = 4,
} DumpStatus;

#define DumpStatus_str(val) \
    qapi_enum_lookup(&DumpStatus_lookup, (val))

extern const QEnumLookup DumpStatus_lookup;

typedef struct DumpQueryResult DumpQueryResult;

typedef struct q_obj_DUMP_COMPLETED_arg q_obj_DUMP_COMPLETED_arg;

typedef struct DumpGuestMemoryFormatList DumpGuestMemoryFormatList;

typedef struct DumpGuestMemoryCapability DumpGuestMemoryCapability;

typedef struct q_obj_dump_skeys_arg q_obj_dump_skeys_arg;

typedef struct q_obj_object_add_arg q_obj_object_add_arg;

typedef struct q_obj_object_del_arg q_obj_object_del_arg;

typedef struct q_obj_getfd_arg q_obj_getfd_arg;

typedef struct q_obj_closefd_arg q_obj_closefd_arg;

typedef struct MachineInfo MachineInfo;

typedef struct MachineInfoList MachineInfoList;

typedef struct CpuDefinitionInfo CpuDefinitionInfo;

typedef struct MemoryInfo MemoryInfo;

typedef struct CpuDefinitionInfoList CpuDefinitionInfoList;

typedef struct CpuModelInfo CpuModelInfo;

typedef enum CpuModelExpansionType {
    CPU_MODEL_EXPANSION_TYPE_STATIC = 0,
    CPU_MODEL_EXPANSION_TYPE_FULL = 1,
    CPU_MODEL_EXPANSION_TYPE__MAX = 2,
} CpuModelExpansionType;

#define CpuModelExpansionType_str(val) \
    qapi_enum_lookup(&CpuModelExpansionType_lookup, (val))

extern const QEnumLookup CpuModelExpansionType_lookup;

typedef struct CpuModelExpansionInfo CpuModelExpansionInfo;

typedef struct q_obj_query_cpu_model_expansion_arg q_obj_query_cpu_model_expansion_arg;

typedef enum CpuModelCompareResult {
    CPU_MODEL_COMPARE_RESULT_INCOMPATIBLE = 0,
    CPU_MODEL_COMPARE_RESULT_IDENTICAL = 1,
    CPU_MODEL_COMPARE_RESULT_SUPERSET = 2,
    CPU_MODEL_COMPARE_RESULT_SUBSET = 3,
    CPU_MODEL_COMPARE_RESULT__MAX = 4,
} CpuModelCompareResult;

#define CpuModelCompareResult_str(val) \
    qapi_enum_lookup(&CpuModelCompareResult_lookup, (val))

extern const QEnumLookup CpuModelCompareResult_lookup;

typedef struct CpuModelCompareInfo CpuModelCompareInfo;

typedef struct q_obj_query_cpu_model_comparison_arg q_obj_query_cpu_model_comparison_arg;

typedef struct CpuModelBaselineInfo CpuModelBaselineInfo;

typedef struct q_obj_query_cpu_model_baseline_arg q_obj_query_cpu_model_baseline_arg;

typedef struct AddfdInfo AddfdInfo;

typedef struct q_obj_add_fd_arg q_obj_add_fd_arg;

typedef struct q_obj_remove_fd_arg q_obj_remove_fd_arg;

typedef struct FdsetFdInfo FdsetFdInfo;

typedef struct FdsetFdInfoList FdsetFdInfoList;

typedef struct FdsetInfo FdsetInfo;

typedef struct FdsetInfoList FdsetInfoList;

typedef struct TargetInfo TargetInfo;

typedef struct AcpiTableOptions AcpiTableOptions;

typedef enum CommandLineParameterType {
    COMMAND_LINE_PARAMETER_TYPE_STRING = 0,
    COMMAND_LINE_PARAMETER_TYPE_BOOLEAN = 1,
    COMMAND_LINE_PARAMETER_TYPE_NUMBER = 2,
    COMMAND_LINE_PARAMETER_TYPE_SIZE = 3,
    COMMAND_LINE_PARAMETER_TYPE__MAX = 4,
} CommandLineParameterType;

#define CommandLineParameterType_str(val) \
    qapi_enum_lookup(&CommandLineParameterType_lookup, (val))

extern const QEnumLookup CommandLineParameterType_lookup;

typedef struct CommandLineParameterInfo CommandLineParameterInfo;

typedef struct CommandLineParameterInfoList CommandLineParameterInfoList;

typedef struct CommandLineOptionInfo CommandLineOptionInfo;

typedef struct q_obj_query_command_line_options_arg q_obj_query_command_line_options_arg;

typedef struct CommandLineOptionInfoList CommandLineOptionInfoList;

typedef enum X86CPURegister32 {
    X86_CPU_REGISTER32_EAX = 0,
    X86_CPU_REGISTER32_EBX = 1,
    X86_CPU_REGISTER32_ECX = 2,
    X86_CPU_REGISTER32_EDX = 3,
    X86_CPU_REGISTER32_ESP = 4,
    X86_CPU_REGISTER32_EBP = 5,
    X86_CPU_REGISTER32_ESI = 6,
    X86_CPU_REGISTER32_EDI = 7,
    X86_CPU_REGISTER32__MAX = 8,
} X86CPURegister32;

#define X86CPURegister32_str(val) \
    qapi_enum_lookup(&X86CPURegister32_lookup, (val))

extern const QEnumLookup X86CPURegister32_lookup;

typedef struct X86CPUFeatureWordInfo X86CPUFeatureWordInfo;

typedef struct X86CPUFeatureWordInfoList X86CPUFeatureWordInfoList;

typedef struct DummyForceArrays DummyForceArrays;

typedef enum NumaOptionsType {
    NUMA_OPTIONS_TYPE_NODE = 0,
    NUMA_OPTIONS_TYPE_DIST = 1,
    NUMA_OPTIONS_TYPE_CPU = 2,
    NUMA_OPTIONS_TYPE__MAX = 3,
} NumaOptionsType;

#define NumaOptionsType_str(val) \
    qapi_enum_lookup(&NumaOptionsType_lookup, (val))

extern const QEnumLookup NumaOptionsType_lookup;

typedef struct q_obj_NumaOptions_base q_obj_NumaOptions_base;

typedef struct NumaOptions NumaOptions;

typedef struct NumaNodeOptions NumaNodeOptions;

typedef struct NumaDistOptions NumaDistOptions;

typedef struct NumaCpuOptions NumaCpuOptions;

typedef enum HostMemPolicy {
    HOST_MEM_POLICY_DEFAULT = 0,
    HOST_MEM_POLICY_PREFERRED = 1,
    HOST_MEM_POLICY_BIND = 2,
    HOST_MEM_POLICY_INTERLEAVE = 3,
    HOST_MEM_POLICY__MAX = 4,
} HostMemPolicy;

#define HostMemPolicy_str(val) \
    qapi_enum_lookup(&HostMemPolicy_lookup, (val))

extern const QEnumLookup HostMemPolicy_lookup;

typedef struct Memdev Memdev;

typedef struct MemdevList MemdevList;

typedef struct PCDIMMDeviceInfo PCDIMMDeviceInfo;

typedef struct q_obj_PCDIMMDeviceInfo_wrapper q_obj_PCDIMMDeviceInfo_wrapper;

typedef enum MemoryDeviceInfoKind {
    MEMORY_DEVICE_INFO_KIND_DIMM = 0,
    MEMORY_DEVICE_INFO_KIND_NVDIMM = 1,
    MEMORY_DEVICE_INFO_KIND__MAX = 2,
} MemoryDeviceInfoKind;

#define MemoryDeviceInfoKind_str(val) \
    qapi_enum_lookup(&MemoryDeviceInfoKind_lookup, (val))

extern const QEnumLookup MemoryDeviceInfoKind_lookup;

typedef struct MemoryDeviceInfo MemoryDeviceInfo;

typedef struct MemoryDeviceInfoList MemoryDeviceInfoList;

typedef struct q_obj_MEM_UNPLUG_ERROR_arg q_obj_MEM_UNPLUG_ERROR_arg;

typedef enum ACPISlotType {
    ACPI_SLOT_TYPE_DIMM = 0,
    ACPI_SLOT_TYPE_CPU = 1,
    ACPI_SLOT_TYPE__MAX = 2,
} ACPISlotType;

#define ACPISlotType_str(val) \
    qapi_enum_lookup(&ACPISlotType_lookup, (val))

extern const QEnumLookup ACPISlotType_lookup;

typedef struct ACPIOSTInfo ACPIOSTInfo;

typedef struct ACPIOSTInfoList ACPIOSTInfoList;

typedef struct q_obj_ACPI_DEVICE_OST_arg q_obj_ACPI_DEVICE_OST_arg;

typedef struct q_obj_RTC_CHANGE_arg q_obj_RTC_CHANGE_arg;

typedef enum ReplayMode {
    REPLAY_MODE_NONE = 0,
    REPLAY_MODE_RECORD = 1,
    REPLAY_MODE_PLAY = 2,
    REPLAY_MODE__MAX = 3,
} ReplayMode;

#define ReplayMode_str(val) \
    qapi_enum_lookup(&ReplayMode_lookup, (val))

extern const QEnumLookup ReplayMode_lookup;

typedef struct q_obj_xen_load_devices_state_arg q_obj_xen_load_devices_state_arg;

typedef struct GICCapability GICCapability;

typedef struct GICCapabilityList GICCapabilityList;

typedef struct CpuInstanceProperties CpuInstanceProperties;

typedef struct HotpluggableCPU HotpluggableCPU;

typedef struct HotpluggableCPUList HotpluggableCPUList;

typedef struct GuidInfo GuidInfo;

typedef enum SevState {
    SEV_STATE_UNINIT = 0,
    SEV_STATE_LAUNCH_UPDATE = 1,
    SEV_STATE_LAUNCH_SECRET = 2,
    SEV_STATE_RUNNING = 3,
    SEV_STATE_SEND_UPDATE = 4,
    SEV_STATE_RECEIVE_UPDATE = 5,
    SEV_STATE__MAX = 6,
} SevState;

#define SevState_str(val) \
    qapi_enum_lookup(&SevState_lookup, (val))

extern const QEnumLookup SevState_lookup;

typedef struct SevInfo SevInfo;

typedef struct SevLaunchMeasureInfo SevLaunchMeasureInfo;

typedef struct SevCapability SevCapability;

typedef enum CommandDropReason {
    COMMAND_DROP_REASON_QUEUE_FULL = 0,
    COMMAND_DROP_REASON__MAX = 1,
} CommandDropReason;

#define CommandDropReason_str(val) \
    qapi_enum_lookup(&CommandDropReason_lookup, (val))

extern const QEnumLookup CommandDropReason_lookup;

typedef struct q_obj_COMMAND_DROPPED_arg q_obj_COMMAND_DROPPED_arg;

typedef struct q_obj_x_oob_test_arg q_obj_x_oob_test_arg;

struct QMPCapabilityList {
    QMPCapabilityList *next;
    QMPCapability value;
};

void qapi_free_QMPCapabilityList(QMPCapabilityList *obj);

struct q_obj_qmp_capabilities_arg {
    bool has_enable;
    QMPCapabilityList *enable;
};

struct VersionTriple {
    int64_t major;
    int64_t minor;
    int64_t micro;
};

void qapi_free_VersionTriple(VersionTriple *obj);

struct VersionInfo {
    VersionTriple *qemu;
    char *package;
};

void qapi_free_VersionInfo(VersionInfo *obj);

struct CommandInfo {
    char *name;
};

void qapi_free_CommandInfo(CommandInfo *obj);

struct CommandInfoList {
    CommandInfoList *next;
    CommandInfo *value;
};

void qapi_free_CommandInfoList(CommandInfoList *obj);

struct q_obj_add_client_arg {
    char *protocol;
    char *fdname;
    bool has_skipauth;
    bool skipauth;
    bool has_tls;
    bool tls;
};

struct NameInfo {
    bool has_name;
    char *name;
};

void qapi_free_NameInfo(NameInfo *obj);

struct KvmInfo {
    bool enabled;
    bool present;
};

void qapi_free_KvmInfo(KvmInfo *obj);

struct UuidInfo {
    char *UUID;
};

void qapi_free_UuidInfo(UuidInfo *obj);

struct EventInfo {
    char *name;
};

void qapi_free_EventInfo(EventInfo *obj);

struct EventInfoList {
    EventInfoList *next;
    EventInfo *value;
};

void qapi_free_EventInfoList(EventInfoList *obj);

struct q_obj_CpuInfo_base {
    int64_t CPU;
    bool current;
    bool halted;
    char *qom_path;
    int64_t thread_id;
    bool has_props;
    CpuInstanceProperties *props;
    CpuInfoArch arch;
};

struct CpuInfoX86 {
    int64_t pc;
};

struct CpuInfoSPARC {
    int64_t pc;
    int64_t npc;
};

struct CpuInfoPPC {
    int64_t nip;
};

struct CpuInfoMIPS {
    int64_t PC;
};

struct CpuInfoTricore {
    int64_t PC;
};

struct CpuInfoS390 {
    CpuS390State cpu_state;
};

struct CpuInfoRISCV {
    int64_t pc;
};

struct CpuInfoOther {
    char qapi_dummy_for_empty_struct;
};

struct CpuInfo {
    int64_t CPU;
    bool current;
    bool halted;
    char *qom_path;
    int64_t thread_id;
    bool has_props;
    CpuInstanceProperties *props;
    CpuInfoArch arch;
    union { /* union tag is @arch */
        CpuInfoX86 x86;
        CpuInfoSPARC q_sparc;
        CpuInfoPPC ppc;
        CpuInfoMIPS q_mips;
        CpuInfoTricore tricore;
        CpuInfoS390 s390;
        CpuInfoRISCV riscv;
        CpuInfoOther other;
    } u;
};

void qapi_free_CpuInfo(CpuInfo *obj);

void qapi_free_CpuInfoX86(CpuInfoX86 *obj);

void qapi_free_CpuInfoSPARC(CpuInfoSPARC *obj);

void qapi_free_CpuInfoPPC(CpuInfoPPC *obj);

void qapi_free_CpuInfoMIPS(CpuInfoMIPS *obj);

void qapi_free_CpuInfoTricore(CpuInfoTricore *obj);

void qapi_free_CpuInfoRISCV(CpuInfoRISCV *obj);

void qapi_free_CpuInfoOther(CpuInfoOther *obj);

void qapi_free_CpuInfoS390(CpuInfoS390 *obj);

struct CpuInfoList {
    CpuInfoList *next;
    CpuInfo *value;
};

void qapi_free_CpuInfoList(CpuInfoList *obj);

struct q_obj_CpuInfoFast_base {
    int64_t cpu_index;
    char *qom_path;
    int64_t thread_id;
    bool has_props;
    CpuInstanceProperties *props;
    CpuInfoArch arch;
};

struct CpuInfoFast {
    int64_t cpu_index;
    char *qom_path;
    int64_t thread_id;
    bool has_props;
    CpuInstanceProperties *props;
    CpuInfoArch arch;
    union { /* union tag is @arch */
        CpuInfoOther x86;
        CpuInfoOther q_sparc;
        CpuInfoOther ppc;
        CpuInfoOther q_mips;
        CpuInfoOther tricore;
        CpuInfoS390 s390;
        CpuInfoRISCV riscv;
        CpuInfoOther other;
    } u;
};

void qapi_free_CpuInfoFast(CpuInfoFast *obj);

struct CpuInfoFastList {
    CpuInfoFastList *next;
    CpuInfoFast *value;
};

void qapi_free_CpuInfoFastList(CpuInfoFastList *obj);

struct IOThreadInfo {
    char *id;
    int64_t thread_id;
    int64_t poll_max_ns;
    int64_t poll_grow;
    int64_t poll_shrink;
};

void qapi_free_IOThreadInfo(IOThreadInfo *obj);

struct IOThreadInfoList {
    IOThreadInfoList *next;
    IOThreadInfo *value;
};

void qapi_free_IOThreadInfoList(IOThreadInfoList *obj);

struct BalloonInfo {
    int64_t actual;
};

void qapi_free_BalloonInfo(BalloonInfo *obj);

struct q_obj_BALLOON_CHANGE_arg {
    int64_t actual;
};

struct PciMemoryRange {
    int64_t base;
    int64_t limit;
};

void qapi_free_PciMemoryRange(PciMemoryRange *obj);

struct PciMemoryRegion {
    int64_t bar;
    char *type;
    int64_t address;
    int64_t size;
    bool has_prefetch;
    bool prefetch;
    bool has_mem_type_64;
    bool mem_type_64;
};

void qapi_free_PciMemoryRegion(PciMemoryRegion *obj);

struct PciBusInfo {
    int64_t number;
    int64_t secondary;
    int64_t subordinate;
    PciMemoryRange *io_range;
    PciMemoryRange *memory_range;
    PciMemoryRange *prefetchable_range;
};

void qapi_free_PciBusInfo(PciBusInfo *obj);

struct PciDeviceInfoList {
    PciDeviceInfoList *next;
    PciDeviceInfo *value;
};

void qapi_free_PciDeviceInfoList(PciDeviceInfoList *obj);

struct PciBridgeInfo {
    PciBusInfo *bus;
    bool has_devices;
    PciDeviceInfoList *devices;
};

void qapi_free_PciBridgeInfo(PciBridgeInfo *obj);

struct PciDeviceClass {
    bool has_desc;
    char *desc;
    int64_t q_class;
};

void qapi_free_PciDeviceClass(PciDeviceClass *obj);

struct PciDeviceId {
    int64_t device;
    int64_t vendor;
};

void qapi_free_PciDeviceId(PciDeviceId *obj);

struct PciMemoryRegionList {
    PciMemoryRegionList *next;
    PciMemoryRegion *value;
};

void qapi_free_PciMemoryRegionList(PciMemoryRegionList *obj);

struct PciDeviceInfo {
    int64_t bus;
    int64_t slot;
    int64_t function;
    PciDeviceClass *class_info;
    PciDeviceId *id;
    bool has_irq;
    int64_t irq;
    char *qdev_id;
    bool has_pci_bridge;
    PciBridgeInfo *pci_bridge;
    PciMemoryRegionList *regions;
};

void qapi_free_PciDeviceInfo(PciDeviceInfo *obj);

struct PciInfo {
    int64_t bus;
    PciDeviceInfoList *devices;
};

void qapi_free_PciInfo(PciInfo *obj);

struct PciInfoList {
    PciInfoList *next;
    PciInfo *value;
};

void qapi_free_PciInfoList(PciInfoList *obj);

struct q_obj_cpu_add_arg {
    int64_t id;
};

struct q_obj_memsave_arg {
    int64_t val;
    int64_t size;
    char *filename;
    bool has_cpu_index;
    int64_t cpu_index;
};

struct q_obj_pmemsave_arg {
    int64_t val;
    int64_t size;
    char *filename;
};

struct q_obj_balloon_arg {
    int64_t value;
};

struct q_obj_human_monitor_command_arg {
    char *command_line;
    bool has_cpu_index;
    int64_t cpu_index;
};

struct ObjectPropertyInfo {
    char *name;
    char *type;
    bool has_description;
    char *description;
};

void qapi_free_ObjectPropertyInfo(ObjectPropertyInfo *obj);

struct q_obj_qom_list_arg {
    char *path;
};

struct ObjectPropertyInfoList {
    ObjectPropertyInfoList *next;
    ObjectPropertyInfo *value;
};

void qapi_free_ObjectPropertyInfoList(ObjectPropertyInfoList *obj);

struct q_obj_qom_get_arg {
    char *path;
    char *property;
};

struct q_obj_qom_set_arg {
    char *path;
    char *property;
    QObject *value;
};

struct q_obj_change_arg {
    char *device;
    char *target;
    bool has_arg;
    char *arg;
};

struct ObjectTypeInfo {
    char *name;
    bool has_abstract;
    bool abstract;
    bool has_parent;
    char *parent;
};

void qapi_free_ObjectTypeInfo(ObjectTypeInfo *obj);

struct q_obj_qom_list_types_arg {
    bool has_implements;
    char *implements;
    bool has_abstract;
    bool abstract;
};

struct ObjectTypeInfoList {
    ObjectTypeInfoList *next;
    ObjectTypeInfo *value;
};

void qapi_free_ObjectTypeInfoList(ObjectTypeInfoList *obj);

struct q_obj_device_list_properties_arg {
    char *q_typename;
};

struct q_obj_qom_list_properties_arg {
    char *q_typename;
};

struct q_obj_xen_set_global_dirty_log_arg {
    bool enable;
};

struct q_obj_device_add_arg {
    char *driver;
    bool has_bus;
    char *bus;
    bool has_id;
    char *id;
};

struct q_obj_device_del_arg {
    char *id;
};

struct q_obj_DEVICE_DELETED_arg {
    bool has_device;
    char *device;
    char *path;
};

struct q_obj_dump_guest_memory_arg {
    bool paging;
    char *protocol;
    bool has_detach;
    bool detach;
    bool has_begin;
    int64_t begin;
    bool has_length;
    int64_t length;
    bool has_format;
    DumpGuestMemoryFormat format;
};

struct DumpQueryResult {
    DumpStatus status;
    int64_t completed;
    int64_t total;
};

void qapi_free_DumpQueryResult(DumpQueryResult *obj);

struct q_obj_DUMP_COMPLETED_arg {
    DumpQueryResult *result;
    bool has_error;
    char *error;
};

struct DumpGuestMemoryFormatList {
    DumpGuestMemoryFormatList *next;
    DumpGuestMemoryFormat value;
};

void qapi_free_DumpGuestMemoryFormatList(DumpGuestMemoryFormatList *obj);

struct DumpGuestMemoryCapability {
    DumpGuestMemoryFormatList *formats;
};

void qapi_free_DumpGuestMemoryCapability(DumpGuestMemoryCapability *obj);

struct q_obj_dump_skeys_arg {
    char *filename;
};

struct q_obj_object_add_arg {
    char *qom_type;
    char *id;
    bool has_props;
    QObject *props;
};

struct q_obj_object_del_arg {
    char *id;
};

struct q_obj_getfd_arg {
    char *fdname;
};

struct q_obj_closefd_arg {
    char *fdname;
};

struct MachineInfo {
    char *name;
    bool has_alias;
    char *alias;
    bool has_is_default;
    bool is_default;
    int64_t cpu_max;
    bool hotpluggable_cpus;
};

void qapi_free_MachineInfo(MachineInfo *obj);

struct MachineInfoList {
    MachineInfoList *next;
    MachineInfo *value;
};

void qapi_free_MachineInfoList(MachineInfoList *obj);

struct CpuDefinitionInfo {
    char *name;
    bool has_migration_safe;
    bool migration_safe;
    bool q_static;
    bool has_unavailable_features;
    strList *unavailable_features;
    char *q_typename;
};

void qapi_free_CpuDefinitionInfo(CpuDefinitionInfo *obj);

struct MemoryInfo {
    uint64_t base_memory;
    bool has_plugged_memory;
    uint64_t plugged_memory;
};

void qapi_free_MemoryInfo(MemoryInfo *obj);

struct CpuDefinitionInfoList {
    CpuDefinitionInfoList *next;
    CpuDefinitionInfo *value;
};

void qapi_free_CpuDefinitionInfoList(CpuDefinitionInfoList *obj);

struct CpuModelInfo {
    char *name;
    bool has_props;
    QObject *props;
};

void qapi_free_CpuModelInfo(CpuModelInfo *obj);

struct CpuModelExpansionInfo {
    CpuModelInfo *model;
};

void qapi_free_CpuModelExpansionInfo(CpuModelExpansionInfo *obj);

struct q_obj_query_cpu_model_expansion_arg {
    CpuModelExpansionType type;
    CpuModelInfo *model;
};

struct CpuModelCompareInfo {
    CpuModelCompareResult result;
    strList *responsible_properties;
};

void qapi_free_CpuModelCompareInfo(CpuModelCompareInfo *obj);

struct q_obj_query_cpu_model_comparison_arg {
    CpuModelInfo *modela;
    CpuModelInfo *modelb;
};

struct CpuModelBaselineInfo {
    CpuModelInfo *model;
};

void qapi_free_CpuModelBaselineInfo(CpuModelBaselineInfo *obj);

struct q_obj_query_cpu_model_baseline_arg {
    CpuModelInfo *modela;
    CpuModelInfo *modelb;
};

struct AddfdInfo {
    int64_t fdset_id;
    int64_t fd;
};

void qapi_free_AddfdInfo(AddfdInfo *obj);

struct q_obj_add_fd_arg {
    bool has_fdset_id;
    int64_t fdset_id;
    bool has_opaque;
    char *opaque;
};

struct q_obj_remove_fd_arg {
    int64_t fdset_id;
    bool has_fd;
    int64_t fd;
};

struct FdsetFdInfo {
    int64_t fd;
    bool has_opaque;
    char *opaque;
};

void qapi_free_FdsetFdInfo(FdsetFdInfo *obj);

struct FdsetFdInfoList {
    FdsetFdInfoList *next;
    FdsetFdInfo *value;
};

void qapi_free_FdsetFdInfoList(FdsetFdInfoList *obj);

struct FdsetInfo {
    int64_t fdset_id;
    FdsetFdInfoList *fds;
};

void qapi_free_FdsetInfo(FdsetInfo *obj);

struct FdsetInfoList {
    FdsetInfoList *next;
    FdsetInfo *value;
};

void qapi_free_FdsetInfoList(FdsetInfoList *obj);

struct TargetInfo {
    char *arch;
};

void qapi_free_TargetInfo(TargetInfo *obj);

struct AcpiTableOptions {
    bool has_sig;
    char *sig;
    bool has_rev;
    uint8_t rev;
    bool has_oem_id;
    char *oem_id;
    bool has_oem_table_id;
    char *oem_table_id;
    bool has_oem_rev;
    uint32_t oem_rev;
    bool has_asl_compiler_id;
    char *asl_compiler_id;
    bool has_asl_compiler_rev;
    uint32_t asl_compiler_rev;
    bool has_file;
    char *file;
    bool has_data;
    char *data;
};

void qapi_free_AcpiTableOptions(AcpiTableOptions *obj);

struct CommandLineParameterInfo {
    char *name;
    CommandLineParameterType type;
    bool has_help;
    char *help;
    bool has_q_default;
    char *q_default;
};

void qapi_free_CommandLineParameterInfo(CommandLineParameterInfo *obj);

struct CommandLineParameterInfoList {
    CommandLineParameterInfoList *next;
    CommandLineParameterInfo *value;
};

void qapi_free_CommandLineParameterInfoList(CommandLineParameterInfoList *obj);

struct CommandLineOptionInfo {
    char *option;
    CommandLineParameterInfoList *parameters;
};

void qapi_free_CommandLineOptionInfo(CommandLineOptionInfo *obj);

struct q_obj_query_command_line_options_arg {
    bool has_option;
    char *option;
};

struct CommandLineOptionInfoList {
    CommandLineOptionInfoList *next;
    CommandLineOptionInfo *value;
};

void qapi_free_CommandLineOptionInfoList(CommandLineOptionInfoList *obj);

struct X86CPUFeatureWordInfo {
    int64_t cpuid_input_eax;
    bool has_cpuid_input_ecx;
    int64_t cpuid_input_ecx;
    X86CPURegister32 cpuid_register;
    int64_t features;
};

void qapi_free_X86CPUFeatureWordInfo(X86CPUFeatureWordInfo *obj);

struct X86CPUFeatureWordInfoList {
    X86CPUFeatureWordInfoList *next;
    X86CPUFeatureWordInfo *value;
};

void qapi_free_X86CPUFeatureWordInfoList(X86CPUFeatureWordInfoList *obj);

struct DummyForceArrays {
    X86CPUFeatureWordInfoList *unused;
};

void qapi_free_DummyForceArrays(DummyForceArrays *obj);

struct q_obj_NumaOptions_base {
    NumaOptionsType type;
};

struct NumaNodeOptions {
    bool has_nodeid;
    uint16_t nodeid;
    bool has_cpus;
    uint16List *cpus;
    bool has_mem;
    uint64_t mem;
    bool has_memdev;
    char *memdev;
};

struct NumaDistOptions {
    uint16_t src;
    uint16_t dst;
    uint8_t val;
};

struct NumaCpuOptions {
    /* Members inherited from CpuInstanceProperties: */
    bool has_node_id;
    int64_t node_id;
    bool has_socket_id;
    int64_t socket_id;
    bool has_core_id;
    int64_t core_id;
    bool has_thread_id;
    int64_t thread_id;
    /* Own members: */
};

struct NumaOptions {
    NumaOptionsType type;
    union { /* union tag is @type */
        NumaNodeOptions node;
        NumaDistOptions dist;
        NumaCpuOptions cpu;
    } u;
};

void qapi_free_NumaOptions(NumaOptions *obj);

void qapi_free_NumaNodeOptions(NumaNodeOptions *obj);

void qapi_free_NumaDistOptions(NumaDistOptions *obj);

static inline CpuInstanceProperties *qapi_NumaCpuOptions_base(const NumaCpuOptions *obj)
{
    return (CpuInstanceProperties *)obj;
}

void qapi_free_NumaCpuOptions(NumaCpuOptions *obj);

struct Memdev {
    bool has_id;
    char *id;
    uint64_t size;
    bool merge;
    bool dump;
    bool prealloc;
    uint16List *host_nodes;
    HostMemPolicy policy;
};

void qapi_free_Memdev(Memdev *obj);

struct MemdevList {
    MemdevList *next;
    Memdev *value;
};

void qapi_free_MemdevList(MemdevList *obj);

struct PCDIMMDeviceInfo {
    bool has_id;
    char *id;
    int64_t addr;
    int64_t size;
    int64_t slot;
    int64_t node;
    char *memdev;
    bool hotplugged;
    bool hotpluggable;
};

void qapi_free_PCDIMMDeviceInfo(PCDIMMDeviceInfo *obj);

struct q_obj_PCDIMMDeviceInfo_wrapper {
    PCDIMMDeviceInfo *data;
};

struct MemoryDeviceInfo {
    MemoryDeviceInfoKind type;
    union { /* union tag is @type */
        q_obj_PCDIMMDeviceInfo_wrapper dimm;
        q_obj_PCDIMMDeviceInfo_wrapper nvdimm;
    } u;
};

void qapi_free_MemoryDeviceInfo(MemoryDeviceInfo *obj);

struct MemoryDeviceInfoList {
    MemoryDeviceInfoList *next;
    MemoryDeviceInfo *value;
};

void qapi_free_MemoryDeviceInfoList(MemoryDeviceInfoList *obj);

struct q_obj_MEM_UNPLUG_ERROR_arg {
    char *device;
    char *msg;
};

struct ACPIOSTInfo {
    bool has_device;
    char *device;
    char *slot;
    ACPISlotType slot_type;
    int64_t source;
    int64_t status;
};

void qapi_free_ACPIOSTInfo(ACPIOSTInfo *obj);

struct ACPIOSTInfoList {
    ACPIOSTInfoList *next;
    ACPIOSTInfo *value;
};

void qapi_free_ACPIOSTInfoList(ACPIOSTInfoList *obj);

struct q_obj_ACPI_DEVICE_OST_arg {
    ACPIOSTInfo *info;
};

struct q_obj_RTC_CHANGE_arg {
    int64_t offset;
};

struct q_obj_xen_load_devices_state_arg {
    char *filename;
};

struct GICCapability {
    int64_t version;
    bool emulated;
    bool kernel;
};

void qapi_free_GICCapability(GICCapability *obj);

struct GICCapabilityList {
    GICCapabilityList *next;
    GICCapability *value;
};

void qapi_free_GICCapabilityList(GICCapabilityList *obj);

struct CpuInstanceProperties {
    bool has_node_id;
    int64_t node_id;
    bool has_socket_id;
    int64_t socket_id;
    bool has_core_id;
    int64_t core_id;
    bool has_thread_id;
    int64_t thread_id;
};

void qapi_free_CpuInstanceProperties(CpuInstanceProperties *obj);

struct HotpluggableCPU {
    char *type;
    int64_t vcpus_count;
    CpuInstanceProperties *props;
    bool has_qom_path;
    char *qom_path;
};

void qapi_free_HotpluggableCPU(HotpluggableCPU *obj);

struct HotpluggableCPUList {
    HotpluggableCPUList *next;
    HotpluggableCPU *value;
};

void qapi_free_HotpluggableCPUList(HotpluggableCPUList *obj);

struct GuidInfo {
    char *guid;
};

void qapi_free_GuidInfo(GuidInfo *obj);

struct SevInfo {
    bool enabled;
    uint8_t api_major;
    uint8_t api_minor;
    uint8_t build_id;
    uint32_t policy;
    SevState state;
    uint32_t handle;
};

void qapi_free_SevInfo(SevInfo *obj);

struct SevLaunchMeasureInfo {
    char *data;
};

void qapi_free_SevLaunchMeasureInfo(SevLaunchMeasureInfo *obj);

struct SevCapability {
    char *pdh;
    char *cert_chain;
    int64_t cbitpos;
    int64_t reduced_phys_bits;
};

void qapi_free_SevCapability(SevCapability *obj);

struct q_obj_COMMAND_DROPPED_arg {
    QObject *id;
    CommandDropReason reason;
};

struct q_obj_x_oob_test_arg {
    bool lock;
};

#endif /* QAPI_TYPES_MISC_H */
