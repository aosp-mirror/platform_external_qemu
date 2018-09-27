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

#include "qemu/osdep.h"
#include "qapi/dealloc-visitor.h"
#include "qapi-types-misc.h"
#include "qapi-visit-misc.h"

void qapi_free_QMPCapabilityList(QMPCapabilityList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QMPCapabilityList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup QMPCapability_lookup = {
    .array = (const char *const[]) {
        [QMP_CAPABILITY_OOB] = "oob",
    },
    .size = QMP_CAPABILITY__MAX
};

void qapi_free_VersionTriple(VersionTriple *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VersionTriple(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_VersionInfo(VersionInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VersionInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CommandInfo(CommandInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CommandInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CommandInfoList(CommandInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CommandInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup LostTickPolicy_lookup = {
    .array = (const char *const[]) {
        [LOST_TICK_POLICY_DISCARD] = "discard",
        [LOST_TICK_POLICY_DELAY] = "delay",
        [LOST_TICK_POLICY_MERGE] = "merge",
        [LOST_TICK_POLICY_SLEW] = "slew",
    },
    .size = LOST_TICK_POLICY__MAX
};

void qapi_free_NameInfo(NameInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_NameInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_KvmInfo(KvmInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_KvmInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UuidInfo(UuidInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UuidInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_EventInfo(EventInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_EventInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_EventInfoList(EventInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_EventInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup CpuInfoArch_lookup = {
    .array = (const char *const[]) {
        [CPU_INFO_ARCH_X86] = "x86",
        [CPU_INFO_ARCH_SPARC] = "sparc",
        [CPU_INFO_ARCH_PPC] = "ppc",
        [CPU_INFO_ARCH_MIPS] = "mips",
        [CPU_INFO_ARCH_TRICORE] = "tricore",
        [CPU_INFO_ARCH_S390] = "s390",
        [CPU_INFO_ARCH_RISCV] = "riscv",
        [CPU_INFO_ARCH_OTHER] = "other",
    },
    .size = CPU_INFO_ARCH__MAX
};

void qapi_free_CpuInfo(CpuInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInfoX86(CpuInfoX86 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoX86(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInfoSPARC(CpuInfoSPARC *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoSPARC(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInfoPPC(CpuInfoPPC *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoPPC(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInfoMIPS(CpuInfoMIPS *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoMIPS(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInfoTricore(CpuInfoTricore *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoTricore(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInfoRISCV(CpuInfoRISCV *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoRISCV(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInfoOther(CpuInfoOther *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoOther(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup CpuS390State_lookup = {
    .array = (const char *const[]) {
        [S390_CPU_STATE_UNINITIALIZED] = "uninitialized",
        [S390_CPU_STATE_STOPPED] = "stopped",
        [S390_CPU_STATE_CHECK_STOP] = "check-stop",
        [S390_CPU_STATE_OPERATING] = "operating",
        [S390_CPU_STATE_LOAD] = "load",
    },
    .size = S390_CPU_STATE__MAX
};

void qapi_free_CpuInfoS390(CpuInfoS390 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoS390(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInfoList(CpuInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInfoFast(CpuInfoFast *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoFast(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInfoFastList(CpuInfoFastList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInfoFastList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_IOThreadInfo(IOThreadInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_IOThreadInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_IOThreadInfoList(IOThreadInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_IOThreadInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BalloonInfo(BalloonInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BalloonInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciMemoryRange(PciMemoryRange *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciMemoryRange(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciMemoryRegion(PciMemoryRegion *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciMemoryRegion(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciBusInfo(PciBusInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciBusInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciDeviceInfoList(PciDeviceInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciDeviceInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciBridgeInfo(PciBridgeInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciBridgeInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciDeviceClass(PciDeviceClass *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciDeviceClass(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciDeviceId(PciDeviceId *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciDeviceId(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciMemoryRegionList(PciMemoryRegionList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciMemoryRegionList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciDeviceInfo(PciDeviceInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciDeviceInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciInfo(PciInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PciInfoList(PciInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PciInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ObjectPropertyInfo(ObjectPropertyInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ObjectPropertyInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ObjectPropertyInfoList(ObjectPropertyInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ObjectPropertyInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ObjectTypeInfo(ObjectTypeInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ObjectTypeInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ObjectTypeInfoList(ObjectTypeInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ObjectTypeInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup DumpGuestMemoryFormat_lookup = {
    .array = (const char *const[]) {
        [DUMP_GUEST_MEMORY_FORMAT_ELF] = "elf",
        [DUMP_GUEST_MEMORY_FORMAT_KDUMP_ZLIB] = "kdump-zlib",
        [DUMP_GUEST_MEMORY_FORMAT_KDUMP_LZO] = "kdump-lzo",
        [DUMP_GUEST_MEMORY_FORMAT_KDUMP_SNAPPY] = "kdump-snappy",
    },
    .size = DUMP_GUEST_MEMORY_FORMAT__MAX
};

const QEnumLookup DumpStatus_lookup = {
    .array = (const char *const[]) {
        [DUMP_STATUS_NONE] = "none",
        [DUMP_STATUS_ACTIVE] = "active",
        [DUMP_STATUS_COMPLETED] = "completed",
        [DUMP_STATUS_FAILED] = "failed",
    },
    .size = DUMP_STATUS__MAX
};

void qapi_free_DumpQueryResult(DumpQueryResult *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_DumpQueryResult(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_DumpGuestMemoryFormatList(DumpGuestMemoryFormatList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_DumpGuestMemoryFormatList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_DumpGuestMemoryCapability(DumpGuestMemoryCapability *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_DumpGuestMemoryCapability(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_MachineInfo(MachineInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MachineInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_MachineInfoList(MachineInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MachineInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuDefinitionInfo(CpuDefinitionInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuDefinitionInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_MemoryInfo(MemoryInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MemoryInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuDefinitionInfoList(CpuDefinitionInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuDefinitionInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuModelInfo(CpuModelInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuModelInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup CpuModelExpansionType_lookup = {
    .array = (const char *const[]) {
        [CPU_MODEL_EXPANSION_TYPE_STATIC] = "static",
        [CPU_MODEL_EXPANSION_TYPE_FULL] = "full",
    },
    .size = CPU_MODEL_EXPANSION_TYPE__MAX
};

void qapi_free_CpuModelExpansionInfo(CpuModelExpansionInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuModelExpansionInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup CpuModelCompareResult_lookup = {
    .array = (const char *const[]) {
        [CPU_MODEL_COMPARE_RESULT_INCOMPATIBLE] = "incompatible",
        [CPU_MODEL_COMPARE_RESULT_IDENTICAL] = "identical",
        [CPU_MODEL_COMPARE_RESULT_SUPERSET] = "superset",
        [CPU_MODEL_COMPARE_RESULT_SUBSET] = "subset",
    },
    .size = CPU_MODEL_COMPARE_RESULT__MAX
};

void qapi_free_CpuModelCompareInfo(CpuModelCompareInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuModelCompareInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuModelBaselineInfo(CpuModelBaselineInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuModelBaselineInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_AddfdInfo(AddfdInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_AddfdInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_FdsetFdInfo(FdsetFdInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_FdsetFdInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_FdsetFdInfoList(FdsetFdInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_FdsetFdInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_FdsetInfo(FdsetInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_FdsetInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_FdsetInfoList(FdsetInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_FdsetInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_TargetInfo(TargetInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TargetInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_AcpiTableOptions(AcpiTableOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_AcpiTableOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup CommandLineParameterType_lookup = {
    .array = (const char *const[]) {
        [COMMAND_LINE_PARAMETER_TYPE_STRING] = "string",
        [COMMAND_LINE_PARAMETER_TYPE_BOOLEAN] = "boolean",
        [COMMAND_LINE_PARAMETER_TYPE_NUMBER] = "number",
        [COMMAND_LINE_PARAMETER_TYPE_SIZE] = "size",
    },
    .size = COMMAND_LINE_PARAMETER_TYPE__MAX
};

void qapi_free_CommandLineParameterInfo(CommandLineParameterInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CommandLineParameterInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CommandLineParameterInfoList(CommandLineParameterInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CommandLineParameterInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CommandLineOptionInfo(CommandLineOptionInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CommandLineOptionInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CommandLineOptionInfoList(CommandLineOptionInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CommandLineOptionInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup X86CPURegister32_lookup = {
    .array = (const char *const[]) {
        [X86_CPU_REGISTER32_EAX] = "EAX",
        [X86_CPU_REGISTER32_EBX] = "EBX",
        [X86_CPU_REGISTER32_ECX] = "ECX",
        [X86_CPU_REGISTER32_EDX] = "EDX",
        [X86_CPU_REGISTER32_ESP] = "ESP",
        [X86_CPU_REGISTER32_EBP] = "EBP",
        [X86_CPU_REGISTER32_ESI] = "ESI",
        [X86_CPU_REGISTER32_EDI] = "EDI",
    },
    .size = X86_CPU_REGISTER32__MAX
};

void qapi_free_X86CPUFeatureWordInfo(X86CPUFeatureWordInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_X86CPUFeatureWordInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_X86CPUFeatureWordInfoList(X86CPUFeatureWordInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_X86CPUFeatureWordInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_DummyForceArrays(DummyForceArrays *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_DummyForceArrays(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup NumaOptionsType_lookup = {
    .array = (const char *const[]) {
        [NUMA_OPTIONS_TYPE_NODE] = "node",
        [NUMA_OPTIONS_TYPE_DIST] = "dist",
        [NUMA_OPTIONS_TYPE_CPU] = "cpu",
    },
    .size = NUMA_OPTIONS_TYPE__MAX
};

void qapi_free_NumaOptions(NumaOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_NumaOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_NumaNodeOptions(NumaNodeOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_NumaNodeOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_NumaDistOptions(NumaDistOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_NumaDistOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_NumaCpuOptions(NumaCpuOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_NumaCpuOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup HostMemPolicy_lookup = {
    .array = (const char *const[]) {
        [HOST_MEM_POLICY_DEFAULT] = "default",
        [HOST_MEM_POLICY_PREFERRED] = "preferred",
        [HOST_MEM_POLICY_BIND] = "bind",
        [HOST_MEM_POLICY_INTERLEAVE] = "interleave",
    },
    .size = HOST_MEM_POLICY__MAX
};

void qapi_free_Memdev(Memdev *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_Memdev(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_MemdevList(MemdevList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MemdevList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_PCDIMMDeviceInfo(PCDIMMDeviceInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_PCDIMMDeviceInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup MemoryDeviceInfoKind_lookup = {
    .array = (const char *const[]) {
        [MEMORY_DEVICE_INFO_KIND_DIMM] = "dimm",
        [MEMORY_DEVICE_INFO_KIND_NVDIMM] = "nvdimm",
    },
    .size = MEMORY_DEVICE_INFO_KIND__MAX
};

void qapi_free_MemoryDeviceInfo(MemoryDeviceInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MemoryDeviceInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_MemoryDeviceInfoList(MemoryDeviceInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MemoryDeviceInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup ACPISlotType_lookup = {
    .array = (const char *const[]) {
        [ACPI_SLOT_TYPE_DIMM] = "DIMM",
        [ACPI_SLOT_TYPE_CPU] = "CPU",
    },
    .size = ACPI_SLOT_TYPE__MAX
};

void qapi_free_ACPIOSTInfo(ACPIOSTInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ACPIOSTInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ACPIOSTInfoList(ACPIOSTInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ACPIOSTInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup ReplayMode_lookup = {
    .array = (const char *const[]) {
        [REPLAY_MODE_NONE] = "none",
        [REPLAY_MODE_RECORD] = "record",
        [REPLAY_MODE_PLAY] = "play",
    },
    .size = REPLAY_MODE__MAX
};

void qapi_free_GICCapability(GICCapability *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_GICCapability(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_GICCapabilityList(GICCapabilityList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_GICCapabilityList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_CpuInstanceProperties(CpuInstanceProperties *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_CpuInstanceProperties(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_HotpluggableCPU(HotpluggableCPU *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_HotpluggableCPU(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_HotpluggableCPUList(HotpluggableCPUList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_HotpluggableCPUList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_GuidInfo(GuidInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_GuidInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup SevState_lookup = {
    .array = (const char *const[]) {
        [SEV_STATE_UNINIT] = "uninit",
        [SEV_STATE_LAUNCH_UPDATE] = "launch-update",
        [SEV_STATE_LAUNCH_SECRET] = "launch-secret",
        [SEV_STATE_RUNNING] = "running",
        [SEV_STATE_SEND_UPDATE] = "send-update",
        [SEV_STATE_RECEIVE_UPDATE] = "receive-update",
    },
    .size = SEV_STATE__MAX
};

void qapi_free_SevInfo(SevInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SevInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SevLaunchMeasureInfo(SevLaunchMeasureInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SevLaunchMeasureInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SevCapability(SevCapability *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SevCapability(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup CommandDropReason_lookup = {
    .array = (const char *const[]) {
        [COMMAND_DROP_REASON_QUEUE_FULL] = "queue-full",
    },
    .size = COMMAND_DROP_REASON__MAX
};
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_misc_c;
