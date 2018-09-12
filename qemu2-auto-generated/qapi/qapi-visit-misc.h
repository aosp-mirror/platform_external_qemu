/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI visitors
 *
 * Copyright IBM, Corp. 2011
 * Copyright (C) 2014-2018 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#ifndef QAPI_VISIT_MISC_H
#define QAPI_VISIT_MISC_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-misc.h"

void visit_type_QMPCapabilityList(Visitor *v, const char *name, QMPCapabilityList **obj, Error **errp);

void visit_type_q_obj_qmp_capabilities_arg_members(Visitor *v, q_obj_qmp_capabilities_arg *obj, Error **errp);
void visit_type_QMPCapability(Visitor *v, const char *name, QMPCapability *obj, Error **errp);

void visit_type_VersionTriple_members(Visitor *v, VersionTriple *obj, Error **errp);
void visit_type_VersionTriple(Visitor *v, const char *name, VersionTriple **obj, Error **errp);

void visit_type_VersionInfo_members(Visitor *v, VersionInfo *obj, Error **errp);
void visit_type_VersionInfo(Visitor *v, const char *name, VersionInfo **obj, Error **errp);

void visit_type_CommandInfo_members(Visitor *v, CommandInfo *obj, Error **errp);
void visit_type_CommandInfo(Visitor *v, const char *name, CommandInfo **obj, Error **errp);
void visit_type_CommandInfoList(Visitor *v, const char *name, CommandInfoList **obj, Error **errp);
void visit_type_LostTickPolicy(Visitor *v, const char *name, LostTickPolicy *obj, Error **errp);

void visit_type_q_obj_add_client_arg_members(Visitor *v, q_obj_add_client_arg *obj, Error **errp);

void visit_type_NameInfo_members(Visitor *v, NameInfo *obj, Error **errp);
void visit_type_NameInfo(Visitor *v, const char *name, NameInfo **obj, Error **errp);

void visit_type_KvmInfo_members(Visitor *v, KvmInfo *obj, Error **errp);
void visit_type_KvmInfo(Visitor *v, const char *name, KvmInfo **obj, Error **errp);

void visit_type_UuidInfo_members(Visitor *v, UuidInfo *obj, Error **errp);
void visit_type_UuidInfo(Visitor *v, const char *name, UuidInfo **obj, Error **errp);

void visit_type_EventInfo_members(Visitor *v, EventInfo *obj, Error **errp);
void visit_type_EventInfo(Visitor *v, const char *name, EventInfo **obj, Error **errp);
void visit_type_EventInfoList(Visitor *v, const char *name, EventInfoList **obj, Error **errp);
void visit_type_CpuInfoArch(Visitor *v, const char *name, CpuInfoArch *obj, Error **errp);

void visit_type_q_obj_CpuInfo_base_members(Visitor *v, q_obj_CpuInfo_base *obj, Error **errp);

void visit_type_CpuInfo_members(Visitor *v, CpuInfo *obj, Error **errp);
void visit_type_CpuInfo(Visitor *v, const char *name, CpuInfo **obj, Error **errp);

void visit_type_CpuInfoX86_members(Visitor *v, CpuInfoX86 *obj, Error **errp);
void visit_type_CpuInfoX86(Visitor *v, const char *name, CpuInfoX86 **obj, Error **errp);

void visit_type_CpuInfoSPARC_members(Visitor *v, CpuInfoSPARC *obj, Error **errp);
void visit_type_CpuInfoSPARC(Visitor *v, const char *name, CpuInfoSPARC **obj, Error **errp);

void visit_type_CpuInfoPPC_members(Visitor *v, CpuInfoPPC *obj, Error **errp);
void visit_type_CpuInfoPPC(Visitor *v, const char *name, CpuInfoPPC **obj, Error **errp);

void visit_type_CpuInfoMIPS_members(Visitor *v, CpuInfoMIPS *obj, Error **errp);
void visit_type_CpuInfoMIPS(Visitor *v, const char *name, CpuInfoMIPS **obj, Error **errp);

void visit_type_CpuInfoTricore_members(Visitor *v, CpuInfoTricore *obj, Error **errp);
void visit_type_CpuInfoTricore(Visitor *v, const char *name, CpuInfoTricore **obj, Error **errp);

void visit_type_CpuInfoRISCV_members(Visitor *v, CpuInfoRISCV *obj, Error **errp);
void visit_type_CpuInfoRISCV(Visitor *v, const char *name, CpuInfoRISCV **obj, Error **errp);

void visit_type_CpuInfoOther_members(Visitor *v, CpuInfoOther *obj, Error **errp);
void visit_type_CpuInfoOther(Visitor *v, const char *name, CpuInfoOther **obj, Error **errp);
void visit_type_CpuS390State(Visitor *v, const char *name, CpuS390State *obj, Error **errp);

void visit_type_CpuInfoS390_members(Visitor *v, CpuInfoS390 *obj, Error **errp);
void visit_type_CpuInfoS390(Visitor *v, const char *name, CpuInfoS390 **obj, Error **errp);
void visit_type_CpuInfoList(Visitor *v, const char *name, CpuInfoList **obj, Error **errp);

void visit_type_q_obj_CpuInfoFast_base_members(Visitor *v, q_obj_CpuInfoFast_base *obj, Error **errp);

void visit_type_CpuInfoFast_members(Visitor *v, CpuInfoFast *obj, Error **errp);
void visit_type_CpuInfoFast(Visitor *v, const char *name, CpuInfoFast **obj, Error **errp);
void visit_type_CpuInfoFastList(Visitor *v, const char *name, CpuInfoFastList **obj, Error **errp);

void visit_type_IOThreadInfo_members(Visitor *v, IOThreadInfo *obj, Error **errp);
void visit_type_IOThreadInfo(Visitor *v, const char *name, IOThreadInfo **obj, Error **errp);
void visit_type_IOThreadInfoList(Visitor *v, const char *name, IOThreadInfoList **obj, Error **errp);

void visit_type_BalloonInfo_members(Visitor *v, BalloonInfo *obj, Error **errp);
void visit_type_BalloonInfo(Visitor *v, const char *name, BalloonInfo **obj, Error **errp);

void visit_type_q_obj_BALLOON_CHANGE_arg_members(Visitor *v, q_obj_BALLOON_CHANGE_arg *obj, Error **errp);

void visit_type_PciMemoryRange_members(Visitor *v, PciMemoryRange *obj, Error **errp);
void visit_type_PciMemoryRange(Visitor *v, const char *name, PciMemoryRange **obj, Error **errp);

void visit_type_PciMemoryRegion_members(Visitor *v, PciMemoryRegion *obj, Error **errp);
void visit_type_PciMemoryRegion(Visitor *v, const char *name, PciMemoryRegion **obj, Error **errp);

void visit_type_PciBusInfo_members(Visitor *v, PciBusInfo *obj, Error **errp);
void visit_type_PciBusInfo(Visitor *v, const char *name, PciBusInfo **obj, Error **errp);
void visit_type_PciDeviceInfoList(Visitor *v, const char *name, PciDeviceInfoList **obj, Error **errp);

void visit_type_PciBridgeInfo_members(Visitor *v, PciBridgeInfo *obj, Error **errp);
void visit_type_PciBridgeInfo(Visitor *v, const char *name, PciBridgeInfo **obj, Error **errp);

void visit_type_PciDeviceClass_members(Visitor *v, PciDeviceClass *obj, Error **errp);
void visit_type_PciDeviceClass(Visitor *v, const char *name, PciDeviceClass **obj, Error **errp);

void visit_type_PciDeviceId_members(Visitor *v, PciDeviceId *obj, Error **errp);
void visit_type_PciDeviceId(Visitor *v, const char *name, PciDeviceId **obj, Error **errp);
void visit_type_PciMemoryRegionList(Visitor *v, const char *name, PciMemoryRegionList **obj, Error **errp);

void visit_type_PciDeviceInfo_members(Visitor *v, PciDeviceInfo *obj, Error **errp);
void visit_type_PciDeviceInfo(Visitor *v, const char *name, PciDeviceInfo **obj, Error **errp);

void visit_type_PciInfo_members(Visitor *v, PciInfo *obj, Error **errp);
void visit_type_PciInfo(Visitor *v, const char *name, PciInfo **obj, Error **errp);
void visit_type_PciInfoList(Visitor *v, const char *name, PciInfoList **obj, Error **errp);

void visit_type_q_obj_cpu_add_arg_members(Visitor *v, q_obj_cpu_add_arg *obj, Error **errp);

void visit_type_q_obj_memsave_arg_members(Visitor *v, q_obj_memsave_arg *obj, Error **errp);

void visit_type_q_obj_pmemsave_arg_members(Visitor *v, q_obj_pmemsave_arg *obj, Error **errp);

void visit_type_q_obj_balloon_arg_members(Visitor *v, q_obj_balloon_arg *obj, Error **errp);

void visit_type_q_obj_human_monitor_command_arg_members(Visitor *v, q_obj_human_monitor_command_arg *obj, Error **errp);

void visit_type_ObjectPropertyInfo_members(Visitor *v, ObjectPropertyInfo *obj, Error **errp);
void visit_type_ObjectPropertyInfo(Visitor *v, const char *name, ObjectPropertyInfo **obj, Error **errp);

void visit_type_q_obj_qom_list_arg_members(Visitor *v, q_obj_qom_list_arg *obj, Error **errp);
void visit_type_ObjectPropertyInfoList(Visitor *v, const char *name, ObjectPropertyInfoList **obj, Error **errp);

void visit_type_q_obj_qom_get_arg_members(Visitor *v, q_obj_qom_get_arg *obj, Error **errp);

void visit_type_q_obj_qom_set_arg_members(Visitor *v, q_obj_qom_set_arg *obj, Error **errp);

void visit_type_q_obj_change_arg_members(Visitor *v, q_obj_change_arg *obj, Error **errp);

void visit_type_ObjectTypeInfo_members(Visitor *v, ObjectTypeInfo *obj, Error **errp);
void visit_type_ObjectTypeInfo(Visitor *v, const char *name, ObjectTypeInfo **obj, Error **errp);

void visit_type_q_obj_qom_list_types_arg_members(Visitor *v, q_obj_qom_list_types_arg *obj, Error **errp);
void visit_type_ObjectTypeInfoList(Visitor *v, const char *name, ObjectTypeInfoList **obj, Error **errp);

void visit_type_q_obj_device_list_properties_arg_members(Visitor *v, q_obj_device_list_properties_arg *obj, Error **errp);

void visit_type_q_obj_qom_list_properties_arg_members(Visitor *v, q_obj_qom_list_properties_arg *obj, Error **errp);

void visit_type_q_obj_xen_set_global_dirty_log_arg_members(Visitor *v, q_obj_xen_set_global_dirty_log_arg *obj, Error **errp);

void visit_type_q_obj_device_add_arg_members(Visitor *v, q_obj_device_add_arg *obj, Error **errp);

void visit_type_q_obj_device_del_arg_members(Visitor *v, q_obj_device_del_arg *obj, Error **errp);

void visit_type_q_obj_DEVICE_DELETED_arg_members(Visitor *v, q_obj_DEVICE_DELETED_arg *obj, Error **errp);
void visit_type_DumpGuestMemoryFormat(Visitor *v, const char *name, DumpGuestMemoryFormat *obj, Error **errp);

void visit_type_q_obj_dump_guest_memory_arg_members(Visitor *v, q_obj_dump_guest_memory_arg *obj, Error **errp);
void visit_type_DumpStatus(Visitor *v, const char *name, DumpStatus *obj, Error **errp);

void visit_type_DumpQueryResult_members(Visitor *v, DumpQueryResult *obj, Error **errp);
void visit_type_DumpQueryResult(Visitor *v, const char *name, DumpQueryResult **obj, Error **errp);

void visit_type_q_obj_DUMP_COMPLETED_arg_members(Visitor *v, q_obj_DUMP_COMPLETED_arg *obj, Error **errp);
void visit_type_DumpGuestMemoryFormatList(Visitor *v, const char *name, DumpGuestMemoryFormatList **obj, Error **errp);

void visit_type_DumpGuestMemoryCapability_members(Visitor *v, DumpGuestMemoryCapability *obj, Error **errp);
void visit_type_DumpGuestMemoryCapability(Visitor *v, const char *name, DumpGuestMemoryCapability **obj, Error **errp);

void visit_type_q_obj_dump_skeys_arg_members(Visitor *v, q_obj_dump_skeys_arg *obj, Error **errp);

void visit_type_q_obj_object_add_arg_members(Visitor *v, q_obj_object_add_arg *obj, Error **errp);

void visit_type_q_obj_object_del_arg_members(Visitor *v, q_obj_object_del_arg *obj, Error **errp);

void visit_type_q_obj_getfd_arg_members(Visitor *v, q_obj_getfd_arg *obj, Error **errp);

void visit_type_q_obj_closefd_arg_members(Visitor *v, q_obj_closefd_arg *obj, Error **errp);

void visit_type_MachineInfo_members(Visitor *v, MachineInfo *obj, Error **errp);
void visit_type_MachineInfo(Visitor *v, const char *name, MachineInfo **obj, Error **errp);
void visit_type_MachineInfoList(Visitor *v, const char *name, MachineInfoList **obj, Error **errp);

void visit_type_CpuDefinitionInfo_members(Visitor *v, CpuDefinitionInfo *obj, Error **errp);
void visit_type_CpuDefinitionInfo(Visitor *v, const char *name, CpuDefinitionInfo **obj, Error **errp);

void visit_type_MemoryInfo_members(Visitor *v, MemoryInfo *obj, Error **errp);
void visit_type_MemoryInfo(Visitor *v, const char *name, MemoryInfo **obj, Error **errp);
void visit_type_CpuDefinitionInfoList(Visitor *v, const char *name, CpuDefinitionInfoList **obj, Error **errp);

void visit_type_CpuModelInfo_members(Visitor *v, CpuModelInfo *obj, Error **errp);
void visit_type_CpuModelInfo(Visitor *v, const char *name, CpuModelInfo **obj, Error **errp);
void visit_type_CpuModelExpansionType(Visitor *v, const char *name, CpuModelExpansionType *obj, Error **errp);

void visit_type_CpuModelExpansionInfo_members(Visitor *v, CpuModelExpansionInfo *obj, Error **errp);
void visit_type_CpuModelExpansionInfo(Visitor *v, const char *name, CpuModelExpansionInfo **obj, Error **errp);

void visit_type_q_obj_query_cpu_model_expansion_arg_members(Visitor *v, q_obj_query_cpu_model_expansion_arg *obj, Error **errp);
void visit_type_CpuModelCompareResult(Visitor *v, const char *name, CpuModelCompareResult *obj, Error **errp);

void visit_type_CpuModelCompareInfo_members(Visitor *v, CpuModelCompareInfo *obj, Error **errp);
void visit_type_CpuModelCompareInfo(Visitor *v, const char *name, CpuModelCompareInfo **obj, Error **errp);

void visit_type_q_obj_query_cpu_model_comparison_arg_members(Visitor *v, q_obj_query_cpu_model_comparison_arg *obj, Error **errp);

void visit_type_CpuModelBaselineInfo_members(Visitor *v, CpuModelBaselineInfo *obj, Error **errp);
void visit_type_CpuModelBaselineInfo(Visitor *v, const char *name, CpuModelBaselineInfo **obj, Error **errp);

void visit_type_q_obj_query_cpu_model_baseline_arg_members(Visitor *v, q_obj_query_cpu_model_baseline_arg *obj, Error **errp);

void visit_type_AddfdInfo_members(Visitor *v, AddfdInfo *obj, Error **errp);
void visit_type_AddfdInfo(Visitor *v, const char *name, AddfdInfo **obj, Error **errp);

void visit_type_q_obj_add_fd_arg_members(Visitor *v, q_obj_add_fd_arg *obj, Error **errp);

void visit_type_q_obj_remove_fd_arg_members(Visitor *v, q_obj_remove_fd_arg *obj, Error **errp);

void visit_type_FdsetFdInfo_members(Visitor *v, FdsetFdInfo *obj, Error **errp);
void visit_type_FdsetFdInfo(Visitor *v, const char *name, FdsetFdInfo **obj, Error **errp);
void visit_type_FdsetFdInfoList(Visitor *v, const char *name, FdsetFdInfoList **obj, Error **errp);

void visit_type_FdsetInfo_members(Visitor *v, FdsetInfo *obj, Error **errp);
void visit_type_FdsetInfo(Visitor *v, const char *name, FdsetInfo **obj, Error **errp);
void visit_type_FdsetInfoList(Visitor *v, const char *name, FdsetInfoList **obj, Error **errp);

void visit_type_TargetInfo_members(Visitor *v, TargetInfo *obj, Error **errp);
void visit_type_TargetInfo(Visitor *v, const char *name, TargetInfo **obj, Error **errp);

void visit_type_AcpiTableOptions_members(Visitor *v, AcpiTableOptions *obj, Error **errp);
void visit_type_AcpiTableOptions(Visitor *v, const char *name, AcpiTableOptions **obj, Error **errp);
void visit_type_CommandLineParameterType(Visitor *v, const char *name, CommandLineParameterType *obj, Error **errp);

void visit_type_CommandLineParameterInfo_members(Visitor *v, CommandLineParameterInfo *obj, Error **errp);
void visit_type_CommandLineParameterInfo(Visitor *v, const char *name, CommandLineParameterInfo **obj, Error **errp);
void visit_type_CommandLineParameterInfoList(Visitor *v, const char *name, CommandLineParameterInfoList **obj, Error **errp);

void visit_type_CommandLineOptionInfo_members(Visitor *v, CommandLineOptionInfo *obj, Error **errp);
void visit_type_CommandLineOptionInfo(Visitor *v, const char *name, CommandLineOptionInfo **obj, Error **errp);

void visit_type_q_obj_query_command_line_options_arg_members(Visitor *v, q_obj_query_command_line_options_arg *obj, Error **errp);
void visit_type_CommandLineOptionInfoList(Visitor *v, const char *name, CommandLineOptionInfoList **obj, Error **errp);
void visit_type_X86CPURegister32(Visitor *v, const char *name, X86CPURegister32 *obj, Error **errp);

void visit_type_X86CPUFeatureWordInfo_members(Visitor *v, X86CPUFeatureWordInfo *obj, Error **errp);
void visit_type_X86CPUFeatureWordInfo(Visitor *v, const char *name, X86CPUFeatureWordInfo **obj, Error **errp);
void visit_type_X86CPUFeatureWordInfoList(Visitor *v, const char *name, X86CPUFeatureWordInfoList **obj, Error **errp);

void visit_type_DummyForceArrays_members(Visitor *v, DummyForceArrays *obj, Error **errp);
void visit_type_DummyForceArrays(Visitor *v, const char *name, DummyForceArrays **obj, Error **errp);
void visit_type_NumaOptionsType(Visitor *v, const char *name, NumaOptionsType *obj, Error **errp);

void visit_type_q_obj_NumaOptions_base_members(Visitor *v, q_obj_NumaOptions_base *obj, Error **errp);

void visit_type_NumaOptions_members(Visitor *v, NumaOptions *obj, Error **errp);
void visit_type_NumaOptions(Visitor *v, const char *name, NumaOptions **obj, Error **errp);

void visit_type_NumaNodeOptions_members(Visitor *v, NumaNodeOptions *obj, Error **errp);
void visit_type_NumaNodeOptions(Visitor *v, const char *name, NumaNodeOptions **obj, Error **errp);

void visit_type_NumaDistOptions_members(Visitor *v, NumaDistOptions *obj, Error **errp);
void visit_type_NumaDistOptions(Visitor *v, const char *name, NumaDistOptions **obj, Error **errp);

void visit_type_NumaCpuOptions_members(Visitor *v, NumaCpuOptions *obj, Error **errp);
void visit_type_NumaCpuOptions(Visitor *v, const char *name, NumaCpuOptions **obj, Error **errp);
void visit_type_HostMemPolicy(Visitor *v, const char *name, HostMemPolicy *obj, Error **errp);

void visit_type_Memdev_members(Visitor *v, Memdev *obj, Error **errp);
void visit_type_Memdev(Visitor *v, const char *name, Memdev **obj, Error **errp);
void visit_type_MemdevList(Visitor *v, const char *name, MemdevList **obj, Error **errp);

void visit_type_PCDIMMDeviceInfo_members(Visitor *v, PCDIMMDeviceInfo *obj, Error **errp);
void visit_type_PCDIMMDeviceInfo(Visitor *v, const char *name, PCDIMMDeviceInfo **obj, Error **errp);

void visit_type_q_obj_PCDIMMDeviceInfo_wrapper_members(Visitor *v, q_obj_PCDIMMDeviceInfo_wrapper *obj, Error **errp);
void visit_type_MemoryDeviceInfoKind(Visitor *v, const char *name, MemoryDeviceInfoKind *obj, Error **errp);

void visit_type_MemoryDeviceInfo_members(Visitor *v, MemoryDeviceInfo *obj, Error **errp);
void visit_type_MemoryDeviceInfo(Visitor *v, const char *name, MemoryDeviceInfo **obj, Error **errp);
void visit_type_MemoryDeviceInfoList(Visitor *v, const char *name, MemoryDeviceInfoList **obj, Error **errp);

void visit_type_q_obj_MEM_UNPLUG_ERROR_arg_members(Visitor *v, q_obj_MEM_UNPLUG_ERROR_arg *obj, Error **errp);
void visit_type_ACPISlotType(Visitor *v, const char *name, ACPISlotType *obj, Error **errp);

void visit_type_ACPIOSTInfo_members(Visitor *v, ACPIOSTInfo *obj, Error **errp);
void visit_type_ACPIOSTInfo(Visitor *v, const char *name, ACPIOSTInfo **obj, Error **errp);
void visit_type_ACPIOSTInfoList(Visitor *v, const char *name, ACPIOSTInfoList **obj, Error **errp);

void visit_type_q_obj_ACPI_DEVICE_OST_arg_members(Visitor *v, q_obj_ACPI_DEVICE_OST_arg *obj, Error **errp);

void visit_type_q_obj_RTC_CHANGE_arg_members(Visitor *v, q_obj_RTC_CHANGE_arg *obj, Error **errp);
void visit_type_ReplayMode(Visitor *v, const char *name, ReplayMode *obj, Error **errp);

void visit_type_q_obj_xen_load_devices_state_arg_members(Visitor *v, q_obj_xen_load_devices_state_arg *obj, Error **errp);

void visit_type_GICCapability_members(Visitor *v, GICCapability *obj, Error **errp);
void visit_type_GICCapability(Visitor *v, const char *name, GICCapability **obj, Error **errp);
void visit_type_GICCapabilityList(Visitor *v, const char *name, GICCapabilityList **obj, Error **errp);

void visit_type_CpuInstanceProperties_members(Visitor *v, CpuInstanceProperties *obj, Error **errp);
void visit_type_CpuInstanceProperties(Visitor *v, const char *name, CpuInstanceProperties **obj, Error **errp);

void visit_type_HotpluggableCPU_members(Visitor *v, HotpluggableCPU *obj, Error **errp);
void visit_type_HotpluggableCPU(Visitor *v, const char *name, HotpluggableCPU **obj, Error **errp);
void visit_type_HotpluggableCPUList(Visitor *v, const char *name, HotpluggableCPUList **obj, Error **errp);

void visit_type_GuidInfo_members(Visitor *v, GuidInfo *obj, Error **errp);
void visit_type_GuidInfo(Visitor *v, const char *name, GuidInfo **obj, Error **errp);
void visit_type_SevState(Visitor *v, const char *name, SevState *obj, Error **errp);

void visit_type_SevInfo_members(Visitor *v, SevInfo *obj, Error **errp);
void visit_type_SevInfo(Visitor *v, const char *name, SevInfo **obj, Error **errp);

void visit_type_SevLaunchMeasureInfo_members(Visitor *v, SevLaunchMeasureInfo *obj, Error **errp);
void visit_type_SevLaunchMeasureInfo(Visitor *v, const char *name, SevLaunchMeasureInfo **obj, Error **errp);

void visit_type_SevCapability_members(Visitor *v, SevCapability *obj, Error **errp);
void visit_type_SevCapability(Visitor *v, const char *name, SevCapability **obj, Error **errp);
void visit_type_CommandDropReason(Visitor *v, const char *name, CommandDropReason *obj, Error **errp);

void visit_type_q_obj_COMMAND_DROPPED_arg_members(Visitor *v, q_obj_COMMAND_DROPPED_arg *obj, Error **errp);

void visit_type_q_obj_x_oob_test_arg_members(Visitor *v, q_obj_x_oob_test_arg *obj, Error **errp);

#endif /* QAPI_VISIT_MISC_H */
