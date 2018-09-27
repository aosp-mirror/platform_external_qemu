/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI/QMP commands
 *
 * Copyright IBM, Corp. 2011
 * Copyright (C) 2014-2018 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#ifndef QAPI_COMMANDS_MISC_H
#define QAPI_COMMANDS_MISC_H

#include "qapi-types-misc.h"
#include "qapi/qmp/dispatch.h"

void qmp_qmp_capabilities(bool has_enable, QMPCapabilityList *enable, Error **errp);
void qmp_marshal_qmp_capabilities(QDict *args, QObject **ret, Error **errp);
VersionInfo *qmp_query_version(Error **errp);
void qmp_marshal_query_version(QDict *args, QObject **ret, Error **errp);
CommandInfoList *qmp_query_commands(Error **errp);
void qmp_marshal_query_commands(QDict *args, QObject **ret, Error **errp);
void qmp_add_client(const char *protocol, const char *fdname, bool has_skipauth, bool skipauth, bool has_tls, bool tls, Error **errp);
void qmp_marshal_add_client(QDict *args, QObject **ret, Error **errp);
NameInfo *qmp_query_name(Error **errp);
void qmp_marshal_query_name(QDict *args, QObject **ret, Error **errp);
KvmInfo *qmp_query_kvm(Error **errp);
void qmp_marshal_query_kvm(QDict *args, QObject **ret, Error **errp);
UuidInfo *qmp_query_uuid(Error **errp);
void qmp_marshal_query_uuid(QDict *args, QObject **ret, Error **errp);
EventInfoList *qmp_query_events(Error **errp);
void qmp_marshal_query_events(QDict *args, QObject **ret, Error **errp);
CpuInfoList *qmp_query_cpus(Error **errp);
void qmp_marshal_query_cpus(QDict *args, QObject **ret, Error **errp);
CpuInfoFastList *qmp_query_cpus_fast(Error **errp);
void qmp_marshal_query_cpus_fast(QDict *args, QObject **ret, Error **errp);
IOThreadInfoList *qmp_query_iothreads(Error **errp);
void qmp_marshal_query_iothreads(QDict *args, QObject **ret, Error **errp);
BalloonInfo *qmp_query_balloon(Error **errp);
void qmp_marshal_query_balloon(QDict *args, QObject **ret, Error **errp);
PciInfoList *qmp_query_pci(Error **errp);
void qmp_marshal_query_pci(QDict *args, QObject **ret, Error **errp);
void qmp_quit(Error **errp);
void qmp_marshal_quit(QDict *args, QObject **ret, Error **errp);
void qmp_stop(Error **errp);
void qmp_marshal_stop(QDict *args, QObject **ret, Error **errp);
void qmp_system_reset(Error **errp);
void qmp_marshal_system_reset(QDict *args, QObject **ret, Error **errp);
void qmp_system_powerdown(Error **errp);
void qmp_marshal_system_powerdown(QDict *args, QObject **ret, Error **errp);
void qmp_cpu_add(int64_t id, Error **errp);
void qmp_marshal_cpu_add(QDict *args, QObject **ret, Error **errp);
void qmp_memsave(int64_t val, int64_t size, const char *filename, bool has_cpu_index, int64_t cpu_index, Error **errp);
void qmp_marshal_memsave(QDict *args, QObject **ret, Error **errp);
void qmp_pmemsave(int64_t val, int64_t size, const char *filename, Error **errp);
void qmp_marshal_pmemsave(QDict *args, QObject **ret, Error **errp);
void qmp_cont(Error **errp);
void qmp_marshal_cont(QDict *args, QObject **ret, Error **errp);
void qmp_system_wakeup(Error **errp);
void qmp_marshal_system_wakeup(QDict *args, QObject **ret, Error **errp);
void qmp_inject_nmi(Error **errp);
void qmp_marshal_inject_nmi(QDict *args, QObject **ret, Error **errp);
void qmp_balloon(int64_t value, Error **errp);
void qmp_marshal_balloon(QDict *args, QObject **ret, Error **errp);
char *qmp_human_monitor_command(const char *command_line, bool has_cpu_index, int64_t cpu_index, Error **errp);
void qmp_marshal_human_monitor_command(QDict *args, QObject **ret, Error **errp);
ObjectPropertyInfoList *qmp_qom_list(const char *path, Error **errp);
void qmp_marshal_qom_list(QDict *args, QObject **ret, Error **errp);
QObject *qmp_qom_get(const char *path, const char *property, Error **errp);
void qmp_marshal_qom_get(QDict *args, QObject **ret, Error **errp);
void qmp_qom_set(const char *path, const char *property, QObject *value, Error **errp);
void qmp_marshal_qom_set(QDict *args, QObject **ret, Error **errp);
void qmp_change(const char *device, const char *target, bool has_arg, const char *arg, Error **errp);
void qmp_marshal_change(QDict *args, QObject **ret, Error **errp);
ObjectTypeInfoList *qmp_qom_list_types(bool has_implements, const char *implements, bool has_abstract, bool abstract, Error **errp);
void qmp_marshal_qom_list_types(QDict *args, QObject **ret, Error **errp);
ObjectPropertyInfoList *qmp_device_list_properties(const char *q_typename, Error **errp);
void qmp_marshal_device_list_properties(QDict *args, QObject **ret, Error **errp);
ObjectPropertyInfoList *qmp_qom_list_properties(const char *q_typename, Error **errp);
void qmp_marshal_qom_list_properties(QDict *args, QObject **ret, Error **errp);
void qmp_xen_set_global_dirty_log(bool enable, Error **errp);
void qmp_marshal_xen_set_global_dirty_log(QDict *args, QObject **ret, Error **errp);
void qmp_device_del(const char *id, Error **errp);
void qmp_marshal_device_del(QDict *args, QObject **ret, Error **errp);
void qmp_dump_guest_memory(bool paging, const char *protocol, bool has_detach, bool detach, bool has_begin, int64_t begin, bool has_length, int64_t length, bool has_format, DumpGuestMemoryFormat format, Error **errp);
void qmp_marshal_dump_guest_memory(QDict *args, QObject **ret, Error **errp);
DumpQueryResult *qmp_query_dump(Error **errp);
void qmp_marshal_query_dump(QDict *args, QObject **ret, Error **errp);
DumpGuestMemoryCapability *qmp_query_dump_guest_memory_capability(Error **errp);
void qmp_marshal_query_dump_guest_memory_capability(QDict *args, QObject **ret, Error **errp);
void qmp_dump_skeys(const char *filename, Error **errp);
void qmp_marshal_dump_skeys(QDict *args, QObject **ret, Error **errp);
void qmp_object_add(const char *qom_type, const char *id, bool has_props, QObject *props, Error **errp);
void qmp_marshal_object_add(QDict *args, QObject **ret, Error **errp);
void qmp_object_del(const char *id, Error **errp);
void qmp_marshal_object_del(QDict *args, QObject **ret, Error **errp);
void qmp_getfd(const char *fdname, Error **errp);
void qmp_marshal_getfd(QDict *args, QObject **ret, Error **errp);
void qmp_closefd(const char *fdname, Error **errp);
void qmp_marshal_closefd(QDict *args, QObject **ret, Error **errp);
MachineInfoList *qmp_query_machines(Error **errp);
void qmp_marshal_query_machines(QDict *args, QObject **ret, Error **errp);
MemoryInfo *qmp_query_memory_size_summary(Error **errp);
void qmp_marshal_query_memory_size_summary(QDict *args, QObject **ret, Error **errp);
CpuDefinitionInfoList *qmp_query_cpu_definitions(Error **errp);
void qmp_marshal_query_cpu_definitions(QDict *args, QObject **ret, Error **errp);
CpuModelExpansionInfo *qmp_query_cpu_model_expansion(CpuModelExpansionType type, CpuModelInfo *model, Error **errp);
void qmp_marshal_query_cpu_model_expansion(QDict *args, QObject **ret, Error **errp);
CpuModelCompareInfo *qmp_query_cpu_model_comparison(CpuModelInfo *modela, CpuModelInfo *modelb, Error **errp);
void qmp_marshal_query_cpu_model_comparison(QDict *args, QObject **ret, Error **errp);
CpuModelBaselineInfo *qmp_query_cpu_model_baseline(CpuModelInfo *modela, CpuModelInfo *modelb, Error **errp);
void qmp_marshal_query_cpu_model_baseline(QDict *args, QObject **ret, Error **errp);
AddfdInfo *qmp_add_fd(bool has_fdset_id, int64_t fdset_id, bool has_opaque, const char *opaque, Error **errp);
void qmp_marshal_add_fd(QDict *args, QObject **ret, Error **errp);
void qmp_remove_fd(int64_t fdset_id, bool has_fd, int64_t fd, Error **errp);
void qmp_marshal_remove_fd(QDict *args, QObject **ret, Error **errp);
FdsetInfoList *qmp_query_fdsets(Error **errp);
void qmp_marshal_query_fdsets(QDict *args, QObject **ret, Error **errp);
TargetInfo *qmp_query_target(Error **errp);
void qmp_marshal_query_target(QDict *args, QObject **ret, Error **errp);
CommandLineOptionInfoList *qmp_query_command_line_options(bool has_option, const char *option, Error **errp);
void qmp_marshal_query_command_line_options(QDict *args, QObject **ret, Error **errp);
MemdevList *qmp_query_memdev(Error **errp);
void qmp_marshal_query_memdev(QDict *args, QObject **ret, Error **errp);
MemoryDeviceInfoList *qmp_query_memory_devices(Error **errp);
void qmp_marshal_query_memory_devices(QDict *args, QObject **ret, Error **errp);
ACPIOSTInfoList *qmp_query_acpi_ospm_status(Error **errp);
void qmp_marshal_query_acpi_ospm_status(QDict *args, QObject **ret, Error **errp);
void qmp_rtc_reset_reinjection(Error **errp);
void qmp_marshal_rtc_reset_reinjection(QDict *args, QObject **ret, Error **errp);
void qmp_xen_load_devices_state(const char *filename, Error **errp);
void qmp_marshal_xen_load_devices_state(QDict *args, QObject **ret, Error **errp);
GICCapabilityList *qmp_query_gic_capabilities(Error **errp);
void qmp_marshal_query_gic_capabilities(QDict *args, QObject **ret, Error **errp);
HotpluggableCPUList *qmp_query_hotpluggable_cpus(Error **errp);
void qmp_marshal_query_hotpluggable_cpus(QDict *args, QObject **ret, Error **errp);
GuidInfo *qmp_query_vm_generation_id(Error **errp);
void qmp_marshal_query_vm_generation_id(QDict *args, QObject **ret, Error **errp);
SevInfo *qmp_query_sev(Error **errp);
void qmp_marshal_query_sev(QDict *args, QObject **ret, Error **errp);
SevLaunchMeasureInfo *qmp_query_sev_launch_measure(Error **errp);
void qmp_marshal_query_sev_launch_measure(QDict *args, QObject **ret, Error **errp);
SevCapability *qmp_query_sev_capabilities(Error **errp);
void qmp_marshal_query_sev_capabilities(QDict *args, QObject **ret, Error **errp);
void qmp_x_oob_test(bool lock, Error **errp);
void qmp_marshal_x_oob_test(QDict *args, QObject **ret, Error **errp);

#endif /* QAPI_COMMANDS_MISC_H */
