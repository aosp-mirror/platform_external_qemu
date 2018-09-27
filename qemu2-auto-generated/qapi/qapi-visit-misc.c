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

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qapi/error.h"
#include "qapi/qmp/qerror.h"
#include "qapi-visit-misc.h"

void visit_type_QMPCapabilityList(Visitor *v, const char *name, QMPCapabilityList **obj, Error **errp)
{
    Error *err = NULL;
    QMPCapabilityList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (QMPCapabilityList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_QMPCapability(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QMPCapabilityList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_qmp_capabilities_arg_members(Visitor *v, q_obj_qmp_capabilities_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "enable", &obj->has_enable)) {
        visit_type_QMPCapabilityList(v, "enable", &obj->enable, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_QMPCapability(Visitor *v, const char *name, QMPCapability *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QMPCapability_lookup, errp);
    *obj = value;
}

void visit_type_VersionTriple_members(Visitor *v, VersionTriple *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "major", &obj->major, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "minor", &obj->minor, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "micro", &obj->micro, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_VersionTriple(Visitor *v, const char *name, VersionTriple **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(VersionTriple), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_VersionTriple_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VersionTriple(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_VersionInfo_members(Visitor *v, VersionInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_VersionTriple(v, "qemu", &obj->qemu, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "package", &obj->package, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_VersionInfo(Visitor *v, const char *name, VersionInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(VersionInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_VersionInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VersionInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CommandInfo_members(Visitor *v, CommandInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CommandInfo(Visitor *v, const char *name, CommandInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CommandInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CommandInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CommandInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CommandInfoList(Visitor *v, const char *name, CommandInfoList **obj, Error **errp)
{
    Error *err = NULL;
    CommandInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (CommandInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_CommandInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CommandInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_LostTickPolicy(Visitor *v, const char *name, LostTickPolicy *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &LostTickPolicy_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_add_client_arg_members(Visitor *v, q_obj_add_client_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "protocol", &obj->protocol, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "fdname", &obj->fdname, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "skipauth", &obj->has_skipauth)) {
        visit_type_bool(v, "skipauth", &obj->skipauth, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tls", &obj->has_tls)) {
        visit_type_bool(v, "tls", &obj->tls, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NameInfo_members(Visitor *v, NameInfo *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "name", &obj->has_name)) {
        visit_type_str(v, "name", &obj->name, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NameInfo(Visitor *v, const char *name, NameInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NameInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NameInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NameInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_KvmInfo_members(Visitor *v, KvmInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "enabled", &obj->enabled, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "present", &obj->present, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_KvmInfo(Visitor *v, const char *name, KvmInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(KvmInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_KvmInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_KvmInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UuidInfo_members(Visitor *v, UuidInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "UUID", &obj->UUID, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_UuidInfo(Visitor *v, const char *name, UuidInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UuidInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UuidInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UuidInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_EventInfo_members(Visitor *v, EventInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_EventInfo(Visitor *v, const char *name, EventInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(EventInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_EventInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_EventInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_EventInfoList(Visitor *v, const char *name, EventInfoList **obj, Error **errp)
{
    Error *err = NULL;
    EventInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (EventInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_EventInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_EventInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoArch(Visitor *v, const char *name, CpuInfoArch *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &CpuInfoArch_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_CpuInfo_base_members(Visitor *v, q_obj_CpuInfo_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "CPU", &obj->CPU, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "current", &obj->current, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "halted", &obj->halted, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "qom_path", &obj->qom_path, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "thread_id", &obj->thread_id, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "props", &obj->has_props)) {
        visit_type_CpuInstanceProperties(v, "props", &obj->props, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_CpuInfoArch(v, "arch", &obj->arch, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfo_members(Visitor *v, CpuInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_CpuInfo_base_members(v, (q_obj_CpuInfo_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->arch) {
    case CPU_INFO_ARCH_X86:
        visit_type_CpuInfoX86_members(v, &obj->u.x86, &err);
        break;
    case CPU_INFO_ARCH_SPARC:
        visit_type_CpuInfoSPARC_members(v, &obj->u.q_sparc, &err);
        break;
    case CPU_INFO_ARCH_PPC:
        visit_type_CpuInfoPPC_members(v, &obj->u.ppc, &err);
        break;
    case CPU_INFO_ARCH_MIPS:
        visit_type_CpuInfoMIPS_members(v, &obj->u.q_mips, &err);
        break;
    case CPU_INFO_ARCH_TRICORE:
        visit_type_CpuInfoTricore_members(v, &obj->u.tricore, &err);
        break;
    case CPU_INFO_ARCH_S390:
        visit_type_CpuInfoS390_members(v, &obj->u.s390, &err);
        break;
    case CPU_INFO_ARCH_RISCV:
        visit_type_CpuInfoRISCV_members(v, &obj->u.riscv, &err);
        break;
    case CPU_INFO_ARCH_OTHER:
        visit_type_CpuInfoOther_members(v, &obj->u.other, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfo(Visitor *v, const char *name, CpuInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoX86_members(Visitor *v, CpuInfoX86 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "pc", &obj->pc, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoX86(Visitor *v, const char *name, CpuInfoX86 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInfoX86), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInfoX86_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoX86(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoSPARC_members(Visitor *v, CpuInfoSPARC *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "pc", &obj->pc, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "npc", &obj->npc, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoSPARC(Visitor *v, const char *name, CpuInfoSPARC **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInfoSPARC), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInfoSPARC_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoSPARC(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoPPC_members(Visitor *v, CpuInfoPPC *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "nip", &obj->nip, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoPPC(Visitor *v, const char *name, CpuInfoPPC **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInfoPPC), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInfoPPC_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoPPC(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoMIPS_members(Visitor *v, CpuInfoMIPS *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "PC", &obj->PC, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoMIPS(Visitor *v, const char *name, CpuInfoMIPS **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInfoMIPS), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInfoMIPS_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoMIPS(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoTricore_members(Visitor *v, CpuInfoTricore *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "PC", &obj->PC, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoTricore(Visitor *v, const char *name, CpuInfoTricore **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInfoTricore), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInfoTricore_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoTricore(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoRISCV_members(Visitor *v, CpuInfoRISCV *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "pc", &obj->pc, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoRISCV(Visitor *v, const char *name, CpuInfoRISCV **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInfoRISCV), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInfoRISCV_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoRISCV(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoOther_members(Visitor *v, CpuInfoOther *obj, Error **errp)
{
    Error *err = NULL;

    error_propagate(errp, err);
}

void visit_type_CpuInfoOther(Visitor *v, const char *name, CpuInfoOther **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInfoOther), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInfoOther_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoOther(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuS390State(Visitor *v, const char *name, CpuS390State *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &CpuS390State_lookup, errp);
    *obj = value;
}

void visit_type_CpuInfoS390_members(Visitor *v, CpuInfoS390 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_CpuS390State(v, "cpu-state", &obj->cpu_state, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoS390(Visitor *v, const char *name, CpuInfoS390 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInfoS390), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInfoS390_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoS390(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoList(Visitor *v, const char *name, CpuInfoList **obj, Error **errp)
{
    Error *err = NULL;
    CpuInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (CpuInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_CpuInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_CpuInfoFast_base_members(Visitor *v, q_obj_CpuInfoFast_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "cpu-index", &obj->cpu_index, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "qom-path", &obj->qom_path, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "thread-id", &obj->thread_id, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "props", &obj->has_props)) {
        visit_type_CpuInstanceProperties(v, "props", &obj->props, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_CpuInfoArch(v, "arch", &obj->arch, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoFast_members(Visitor *v, CpuInfoFast *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_CpuInfoFast_base_members(v, (q_obj_CpuInfoFast_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->arch) {
    case CPU_INFO_ARCH_X86:
        visit_type_CpuInfoOther_members(v, &obj->u.x86, &err);
        break;
    case CPU_INFO_ARCH_SPARC:
        visit_type_CpuInfoOther_members(v, &obj->u.q_sparc, &err);
        break;
    case CPU_INFO_ARCH_PPC:
        visit_type_CpuInfoOther_members(v, &obj->u.ppc, &err);
        break;
    case CPU_INFO_ARCH_MIPS:
        visit_type_CpuInfoOther_members(v, &obj->u.q_mips, &err);
        break;
    case CPU_INFO_ARCH_TRICORE:
        visit_type_CpuInfoOther_members(v, &obj->u.tricore, &err);
        break;
    case CPU_INFO_ARCH_S390:
        visit_type_CpuInfoS390_members(v, &obj->u.s390, &err);
        break;
    case CPU_INFO_ARCH_RISCV:
        visit_type_CpuInfoRISCV_members(v, &obj->u.riscv, &err);
        break;
    case CPU_INFO_ARCH_OTHER:
        visit_type_CpuInfoOther_members(v, &obj->u.other, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoFast(Visitor *v, const char *name, CpuInfoFast **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInfoFast), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInfoFast_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoFast(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInfoFastList(Visitor *v, const char *name, CpuInfoFastList **obj, Error **errp)
{
    Error *err = NULL;
    CpuInfoFastList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (CpuInfoFastList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_CpuInfoFast(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInfoFastList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_IOThreadInfo_members(Visitor *v, IOThreadInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "thread-id", &obj->thread_id, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "poll-max-ns", &obj->poll_max_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "poll-grow", &obj->poll_grow, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "poll-shrink", &obj->poll_shrink, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_IOThreadInfo(Visitor *v, const char *name, IOThreadInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(IOThreadInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_IOThreadInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_IOThreadInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_IOThreadInfoList(Visitor *v, const char *name, IOThreadInfoList **obj, Error **errp)
{
    Error *err = NULL;
    IOThreadInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (IOThreadInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_IOThreadInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_IOThreadInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BalloonInfo_members(Visitor *v, BalloonInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "actual", &obj->actual, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BalloonInfo(Visitor *v, const char *name, BalloonInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BalloonInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BalloonInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BalloonInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BALLOON_CHANGE_arg_members(Visitor *v, q_obj_BALLOON_CHANGE_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "actual", &obj->actual, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_PciMemoryRange_members(Visitor *v, PciMemoryRange *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "base", &obj->base, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "limit", &obj->limit, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_PciMemoryRange(Visitor *v, const char *name, PciMemoryRange **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(PciMemoryRange), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_PciMemoryRange_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciMemoryRange(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PciMemoryRegion_members(Visitor *v, PciMemoryRegion *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "bar", &obj->bar, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "address", &obj->address, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "prefetch", &obj->has_prefetch)) {
        visit_type_bool(v, "prefetch", &obj->prefetch, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "mem_type_64", &obj->has_mem_type_64)) {
        visit_type_bool(v, "mem_type_64", &obj->mem_type_64, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_PciMemoryRegion(Visitor *v, const char *name, PciMemoryRegion **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(PciMemoryRegion), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_PciMemoryRegion_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciMemoryRegion(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PciBusInfo_members(Visitor *v, PciBusInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "number", &obj->number, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "secondary", &obj->secondary, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "subordinate", &obj->subordinate, &err);
    if (err) {
        goto out;
    }
    visit_type_PciMemoryRange(v, "io_range", &obj->io_range, &err);
    if (err) {
        goto out;
    }
    visit_type_PciMemoryRange(v, "memory_range", &obj->memory_range, &err);
    if (err) {
        goto out;
    }
    visit_type_PciMemoryRange(v, "prefetchable_range", &obj->prefetchable_range, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_PciBusInfo(Visitor *v, const char *name, PciBusInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(PciBusInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_PciBusInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciBusInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PciDeviceInfoList(Visitor *v, const char *name, PciDeviceInfoList **obj, Error **errp)
{
    Error *err = NULL;
    PciDeviceInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (PciDeviceInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_PciDeviceInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciDeviceInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PciBridgeInfo_members(Visitor *v, PciBridgeInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_PciBusInfo(v, "bus", &obj->bus, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "devices", &obj->has_devices)) {
        visit_type_PciDeviceInfoList(v, "devices", &obj->devices, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_PciBridgeInfo(Visitor *v, const char *name, PciBridgeInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(PciBridgeInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_PciBridgeInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciBridgeInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PciDeviceClass_members(Visitor *v, PciDeviceClass *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "desc", &obj->has_desc)) {
        visit_type_str(v, "desc", &obj->desc, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "class", &obj->q_class, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_PciDeviceClass(Visitor *v, const char *name, PciDeviceClass **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(PciDeviceClass), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_PciDeviceClass_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciDeviceClass(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PciDeviceId_members(Visitor *v, PciDeviceId *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "vendor", &obj->vendor, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_PciDeviceId(Visitor *v, const char *name, PciDeviceId **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(PciDeviceId), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_PciDeviceId_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciDeviceId(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PciMemoryRegionList(Visitor *v, const char *name, PciMemoryRegionList **obj, Error **errp)
{
    Error *err = NULL;
    PciMemoryRegionList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (PciMemoryRegionList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_PciMemoryRegion(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciMemoryRegionList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PciDeviceInfo_members(Visitor *v, PciDeviceInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "bus", &obj->bus, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "slot", &obj->slot, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "function", &obj->function, &err);
    if (err) {
        goto out;
    }
    visit_type_PciDeviceClass(v, "class_info", &obj->class_info, &err);
    if (err) {
        goto out;
    }
    visit_type_PciDeviceId(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "irq", &obj->has_irq)) {
        visit_type_int(v, "irq", &obj->irq, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "qdev_id", &obj->qdev_id, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "pci_bridge", &obj->has_pci_bridge)) {
        visit_type_PciBridgeInfo(v, "pci_bridge", &obj->pci_bridge, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_PciMemoryRegionList(v, "regions", &obj->regions, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_PciDeviceInfo(Visitor *v, const char *name, PciDeviceInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(PciDeviceInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_PciDeviceInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciDeviceInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PciInfo_members(Visitor *v, PciInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "bus", &obj->bus, &err);
    if (err) {
        goto out;
    }
    visit_type_PciDeviceInfoList(v, "devices", &obj->devices, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_PciInfo(Visitor *v, const char *name, PciInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(PciInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_PciInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PciInfoList(Visitor *v, const char *name, PciInfoList **obj, Error **errp)
{
    Error *err = NULL;
    PciInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (PciInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_PciInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PciInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_cpu_add_arg_members(Visitor *v, q_obj_cpu_add_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_memsave_arg_members(Visitor *v, q_obj_memsave_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "val", &obj->val, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "cpu-index", &obj->has_cpu_index)) {
        visit_type_int(v, "cpu-index", &obj->cpu_index, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_pmemsave_arg_members(Visitor *v, q_obj_pmemsave_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "val", &obj->val, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_balloon_arg_members(Visitor *v, q_obj_balloon_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "value", &obj->value, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_human_monitor_command_arg_members(Visitor *v, q_obj_human_monitor_command_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "command-line", &obj->command_line, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "cpu-index", &obj->has_cpu_index)) {
        visit_type_int(v, "cpu-index", &obj->cpu_index, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ObjectPropertyInfo_members(Visitor *v, ObjectPropertyInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "description", &obj->has_description)) {
        visit_type_str(v, "description", &obj->description, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ObjectPropertyInfo(Visitor *v, const char *name, ObjectPropertyInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ObjectPropertyInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ObjectPropertyInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ObjectPropertyInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_qom_list_arg_members(Visitor *v, q_obj_qom_list_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "path", &obj->path, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ObjectPropertyInfoList(Visitor *v, const char *name, ObjectPropertyInfoList **obj, Error **errp)
{
    Error *err = NULL;
    ObjectPropertyInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (ObjectPropertyInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_ObjectPropertyInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ObjectPropertyInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_qom_get_arg_members(Visitor *v, q_obj_qom_get_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "path", &obj->path, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "property", &obj->property, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_qom_set_arg_members(Visitor *v, q_obj_qom_set_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "path", &obj->path, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "property", &obj->property, &err);
    if (err) {
        goto out;
    }
    visit_type_any(v, "value", &obj->value, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_change_arg_members(Visitor *v, q_obj_change_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "target", &obj->target, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "arg", &obj->has_arg)) {
        visit_type_str(v, "arg", &obj->arg, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ObjectTypeInfo_members(Visitor *v, ObjectTypeInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "abstract", &obj->has_abstract)) {
        visit_type_bool(v, "abstract", &obj->abstract, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "parent", &obj->has_parent)) {
        visit_type_str(v, "parent", &obj->parent, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ObjectTypeInfo(Visitor *v, const char *name, ObjectTypeInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ObjectTypeInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ObjectTypeInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ObjectTypeInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_qom_list_types_arg_members(Visitor *v, q_obj_qom_list_types_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "implements", &obj->has_implements)) {
        visit_type_str(v, "implements", &obj->implements, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "abstract", &obj->has_abstract)) {
        visit_type_bool(v, "abstract", &obj->abstract, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ObjectTypeInfoList(Visitor *v, const char *name, ObjectTypeInfoList **obj, Error **errp)
{
    Error *err = NULL;
    ObjectTypeInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (ObjectTypeInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_ObjectTypeInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ObjectTypeInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_device_list_properties_arg_members(Visitor *v, q_obj_device_list_properties_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "typename", &obj->q_typename, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_qom_list_properties_arg_members(Visitor *v, q_obj_qom_list_properties_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "typename", &obj->q_typename, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_xen_set_global_dirty_log_arg_members(Visitor *v, q_obj_xen_set_global_dirty_log_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "enable", &obj->enable, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_device_add_arg_members(Visitor *v, q_obj_device_add_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "driver", &obj->driver, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "bus", &obj->has_bus)) {
        visit_type_str(v, "bus", &obj->bus, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "id", &obj->has_id)) {
        visit_type_str(v, "id", &obj->id, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_device_del_arg_members(Visitor *v, q_obj_device_del_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_DEVICE_DELETED_arg_members(Visitor *v, q_obj_DEVICE_DELETED_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "path", &obj->path, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_DumpGuestMemoryFormat(Visitor *v, const char *name, DumpGuestMemoryFormat *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &DumpGuestMemoryFormat_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_dump_guest_memory_arg_members(Visitor *v, q_obj_dump_guest_memory_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "paging", &obj->paging, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "protocol", &obj->protocol, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "detach", &obj->has_detach)) {
        visit_type_bool(v, "detach", &obj->detach, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "begin", &obj->has_begin)) {
        visit_type_int(v, "begin", &obj->begin, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "length", &obj->has_length)) {
        visit_type_int(v, "length", &obj->length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "format", &obj->has_format)) {
        visit_type_DumpGuestMemoryFormat(v, "format", &obj->format, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_DumpStatus(Visitor *v, const char *name, DumpStatus *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &DumpStatus_lookup, errp);
    *obj = value;
}

void visit_type_DumpQueryResult_members(Visitor *v, DumpQueryResult *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_DumpStatus(v, "status", &obj->status, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "completed", &obj->completed, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "total", &obj->total, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_DumpQueryResult(Visitor *v, const char *name, DumpQueryResult **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(DumpQueryResult), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_DumpQueryResult_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_DumpQueryResult(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_DUMP_COMPLETED_arg_members(Visitor *v, q_obj_DUMP_COMPLETED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_DumpQueryResult(v, "result", &obj->result, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "error", &obj->has_error)) {
        visit_type_str(v, "error", &obj->error, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_DumpGuestMemoryFormatList(Visitor *v, const char *name, DumpGuestMemoryFormatList **obj, Error **errp)
{
    Error *err = NULL;
    DumpGuestMemoryFormatList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (DumpGuestMemoryFormatList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_DumpGuestMemoryFormat(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_DumpGuestMemoryFormatList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_DumpGuestMemoryCapability_members(Visitor *v, DumpGuestMemoryCapability *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_DumpGuestMemoryFormatList(v, "formats", &obj->formats, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_DumpGuestMemoryCapability(Visitor *v, const char *name, DumpGuestMemoryCapability **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(DumpGuestMemoryCapability), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_DumpGuestMemoryCapability_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_DumpGuestMemoryCapability(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_dump_skeys_arg_members(Visitor *v, q_obj_dump_skeys_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_object_add_arg_members(Visitor *v, q_obj_object_add_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "qom-type", &obj->qom_type, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "props", &obj->has_props)) {
        visit_type_any(v, "props", &obj->props, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_object_del_arg_members(Visitor *v, q_obj_object_del_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_getfd_arg_members(Visitor *v, q_obj_getfd_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "fdname", &obj->fdname, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_closefd_arg_members(Visitor *v, q_obj_closefd_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "fdname", &obj->fdname, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_MachineInfo_members(Visitor *v, MachineInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "alias", &obj->has_alias)) {
        visit_type_str(v, "alias", &obj->alias, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "is-default", &obj->has_is_default)) {
        visit_type_bool(v, "is-default", &obj->is_default, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "cpu-max", &obj->cpu_max, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "hotpluggable-cpus", &obj->hotpluggable_cpus, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_MachineInfo(Visitor *v, const char *name, MachineInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(MachineInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_MachineInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_MachineInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_MachineInfoList(Visitor *v, const char *name, MachineInfoList **obj, Error **errp)
{
    Error *err = NULL;
    MachineInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (MachineInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_MachineInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_MachineInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuDefinitionInfo_members(Visitor *v, CpuDefinitionInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "migration-safe", &obj->has_migration_safe)) {
        visit_type_bool(v, "migration-safe", &obj->migration_safe, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_bool(v, "static", &obj->q_static, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "unavailable-features", &obj->has_unavailable_features)) {
        visit_type_strList(v, "unavailable-features", &obj->unavailable_features, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "typename", &obj->q_typename, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuDefinitionInfo(Visitor *v, const char *name, CpuDefinitionInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuDefinitionInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuDefinitionInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuDefinitionInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_MemoryInfo_members(Visitor *v, MemoryInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_size(v, "base-memory", &obj->base_memory, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "plugged-memory", &obj->has_plugged_memory)) {
        visit_type_size(v, "plugged-memory", &obj->plugged_memory, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_MemoryInfo(Visitor *v, const char *name, MemoryInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(MemoryInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_MemoryInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_MemoryInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuDefinitionInfoList(Visitor *v, const char *name, CpuDefinitionInfoList **obj, Error **errp)
{
    Error *err = NULL;
    CpuDefinitionInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (CpuDefinitionInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_CpuDefinitionInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuDefinitionInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuModelInfo_members(Visitor *v, CpuModelInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "props", &obj->has_props)) {
        visit_type_any(v, "props", &obj->props, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuModelInfo(Visitor *v, const char *name, CpuModelInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuModelInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuModelInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuModelInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuModelExpansionType(Visitor *v, const char *name, CpuModelExpansionType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &CpuModelExpansionType_lookup, errp);
    *obj = value;
}

void visit_type_CpuModelExpansionInfo_members(Visitor *v, CpuModelExpansionInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_CpuModelInfo(v, "model", &obj->model, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuModelExpansionInfo(Visitor *v, const char *name, CpuModelExpansionInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuModelExpansionInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuModelExpansionInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuModelExpansionInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_query_cpu_model_expansion_arg_members(Visitor *v, q_obj_query_cpu_model_expansion_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_CpuModelExpansionType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_CpuModelInfo(v, "model", &obj->model, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuModelCompareResult(Visitor *v, const char *name, CpuModelCompareResult *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &CpuModelCompareResult_lookup, errp);
    *obj = value;
}

void visit_type_CpuModelCompareInfo_members(Visitor *v, CpuModelCompareInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_CpuModelCompareResult(v, "result", &obj->result, &err);
    if (err) {
        goto out;
    }
    visit_type_strList(v, "responsible-properties", &obj->responsible_properties, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuModelCompareInfo(Visitor *v, const char *name, CpuModelCompareInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuModelCompareInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuModelCompareInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuModelCompareInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_query_cpu_model_comparison_arg_members(Visitor *v, q_obj_query_cpu_model_comparison_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_CpuModelInfo(v, "modela", &obj->modela, &err);
    if (err) {
        goto out;
    }
    visit_type_CpuModelInfo(v, "modelb", &obj->modelb, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuModelBaselineInfo_members(Visitor *v, CpuModelBaselineInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_CpuModelInfo(v, "model", &obj->model, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuModelBaselineInfo(Visitor *v, const char *name, CpuModelBaselineInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuModelBaselineInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuModelBaselineInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuModelBaselineInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_query_cpu_model_baseline_arg_members(Visitor *v, q_obj_query_cpu_model_baseline_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_CpuModelInfo(v, "modela", &obj->modela, &err);
    if (err) {
        goto out;
    }
    visit_type_CpuModelInfo(v, "modelb", &obj->modelb, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_AddfdInfo_members(Visitor *v, AddfdInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "fdset-id", &obj->fdset_id, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "fd", &obj->fd, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_AddfdInfo(Visitor *v, const char *name, AddfdInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(AddfdInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_AddfdInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_AddfdInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_add_fd_arg_members(Visitor *v, q_obj_add_fd_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "fdset-id", &obj->has_fdset_id)) {
        visit_type_int(v, "fdset-id", &obj->fdset_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "opaque", &obj->has_opaque)) {
        visit_type_str(v, "opaque", &obj->opaque, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_remove_fd_arg_members(Visitor *v, q_obj_remove_fd_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "fdset-id", &obj->fdset_id, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "fd", &obj->has_fd)) {
        visit_type_int(v, "fd", &obj->fd, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_FdsetFdInfo_members(Visitor *v, FdsetFdInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "fd", &obj->fd, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "opaque", &obj->has_opaque)) {
        visit_type_str(v, "opaque", &obj->opaque, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_FdsetFdInfo(Visitor *v, const char *name, FdsetFdInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(FdsetFdInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_FdsetFdInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_FdsetFdInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_FdsetFdInfoList(Visitor *v, const char *name, FdsetFdInfoList **obj, Error **errp)
{
    Error *err = NULL;
    FdsetFdInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (FdsetFdInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_FdsetFdInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_FdsetFdInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_FdsetInfo_members(Visitor *v, FdsetInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "fdset-id", &obj->fdset_id, &err);
    if (err) {
        goto out;
    }
    visit_type_FdsetFdInfoList(v, "fds", &obj->fds, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_FdsetInfo(Visitor *v, const char *name, FdsetInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(FdsetInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_FdsetInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_FdsetInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_FdsetInfoList(Visitor *v, const char *name, FdsetInfoList **obj, Error **errp)
{
    Error *err = NULL;
    FdsetInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (FdsetInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_FdsetInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_FdsetInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_TargetInfo_members(Visitor *v, TargetInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "arch", &obj->arch, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_TargetInfo(Visitor *v, const char *name, TargetInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(TargetInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_TargetInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TargetInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_AcpiTableOptions_members(Visitor *v, AcpiTableOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "sig", &obj->has_sig)) {
        visit_type_str(v, "sig", &obj->sig, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "rev", &obj->has_rev)) {
        visit_type_uint8(v, "rev", &obj->rev, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "oem_id", &obj->has_oem_id)) {
        visit_type_str(v, "oem_id", &obj->oem_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "oem_table_id", &obj->has_oem_table_id)) {
        visit_type_str(v, "oem_table_id", &obj->oem_table_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "oem_rev", &obj->has_oem_rev)) {
        visit_type_uint32(v, "oem_rev", &obj->oem_rev, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "asl_compiler_id", &obj->has_asl_compiler_id)) {
        visit_type_str(v, "asl_compiler_id", &obj->asl_compiler_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "asl_compiler_rev", &obj->has_asl_compiler_rev)) {
        visit_type_uint32(v, "asl_compiler_rev", &obj->asl_compiler_rev, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "file", &obj->has_file)) {
        visit_type_str(v, "file", &obj->file, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "data", &obj->has_data)) {
        visit_type_str(v, "data", &obj->data, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_AcpiTableOptions(Visitor *v, const char *name, AcpiTableOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(AcpiTableOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_AcpiTableOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_AcpiTableOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CommandLineParameterType(Visitor *v, const char *name, CommandLineParameterType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &CommandLineParameterType_lookup, errp);
    *obj = value;
}

void visit_type_CommandLineParameterInfo_members(Visitor *v, CommandLineParameterInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_CommandLineParameterType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "help", &obj->has_help)) {
        visit_type_str(v, "help", &obj->help, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "default", &obj->has_q_default)) {
        visit_type_str(v, "default", &obj->q_default, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_CommandLineParameterInfo(Visitor *v, const char *name, CommandLineParameterInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CommandLineParameterInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CommandLineParameterInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CommandLineParameterInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CommandLineParameterInfoList(Visitor *v, const char *name, CommandLineParameterInfoList **obj, Error **errp)
{
    Error *err = NULL;
    CommandLineParameterInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (CommandLineParameterInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_CommandLineParameterInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CommandLineParameterInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CommandLineOptionInfo_members(Visitor *v, CommandLineOptionInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "option", &obj->option, &err);
    if (err) {
        goto out;
    }
    visit_type_CommandLineParameterInfoList(v, "parameters", &obj->parameters, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_CommandLineOptionInfo(Visitor *v, const char *name, CommandLineOptionInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CommandLineOptionInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CommandLineOptionInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CommandLineOptionInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_query_command_line_options_arg_members(Visitor *v, q_obj_query_command_line_options_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "option", &obj->has_option)) {
        visit_type_str(v, "option", &obj->option, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_CommandLineOptionInfoList(Visitor *v, const char *name, CommandLineOptionInfoList **obj, Error **errp)
{
    Error *err = NULL;
    CommandLineOptionInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (CommandLineOptionInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_CommandLineOptionInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CommandLineOptionInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_X86CPURegister32(Visitor *v, const char *name, X86CPURegister32 *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &X86CPURegister32_lookup, errp);
    *obj = value;
}

void visit_type_X86CPUFeatureWordInfo_members(Visitor *v, X86CPUFeatureWordInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "cpuid-input-eax", &obj->cpuid_input_eax, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "cpuid-input-ecx", &obj->has_cpuid_input_ecx)) {
        visit_type_int(v, "cpuid-input-ecx", &obj->cpuid_input_ecx, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_X86CPURegister32(v, "cpuid-register", &obj->cpuid_register, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "features", &obj->features, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_X86CPUFeatureWordInfo(Visitor *v, const char *name, X86CPUFeatureWordInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(X86CPUFeatureWordInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_X86CPUFeatureWordInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_X86CPUFeatureWordInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_X86CPUFeatureWordInfoList(Visitor *v, const char *name, X86CPUFeatureWordInfoList **obj, Error **errp)
{
    Error *err = NULL;
    X86CPUFeatureWordInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (X86CPUFeatureWordInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_X86CPUFeatureWordInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_X86CPUFeatureWordInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_DummyForceArrays_members(Visitor *v, DummyForceArrays *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_X86CPUFeatureWordInfoList(v, "unused", &obj->unused, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_DummyForceArrays(Visitor *v, const char *name, DummyForceArrays **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(DummyForceArrays), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_DummyForceArrays_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_DummyForceArrays(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NumaOptionsType(Visitor *v, const char *name, NumaOptionsType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &NumaOptionsType_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_NumaOptions_base_members(Visitor *v, q_obj_NumaOptions_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_NumaOptionsType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_NumaOptions_members(Visitor *v, NumaOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_NumaOptions_base_members(v, (q_obj_NumaOptions_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case NUMA_OPTIONS_TYPE_NODE:
        visit_type_NumaNodeOptions_members(v, &obj->u.node, &err);
        break;
    case NUMA_OPTIONS_TYPE_DIST:
        visit_type_NumaDistOptions_members(v, &obj->u.dist, &err);
        break;
    case NUMA_OPTIONS_TYPE_CPU:
        visit_type_NumaCpuOptions_members(v, &obj->u.cpu, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_NumaOptions(Visitor *v, const char *name, NumaOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NumaOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NumaOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NumaOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NumaNodeOptions_members(Visitor *v, NumaNodeOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "nodeid", &obj->has_nodeid)) {
        visit_type_uint16(v, "nodeid", &obj->nodeid, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cpus", &obj->has_cpus)) {
        visit_type_uint16List(v, "cpus", &obj->cpus, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "mem", &obj->has_mem)) {
        visit_type_size(v, "mem", &obj->mem, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "memdev", &obj->has_memdev)) {
        visit_type_str(v, "memdev", &obj->memdev, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NumaNodeOptions(Visitor *v, const char *name, NumaNodeOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NumaNodeOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NumaNodeOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NumaNodeOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NumaDistOptions_members(Visitor *v, NumaDistOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint16(v, "src", &obj->src, &err);
    if (err) {
        goto out;
    }
    visit_type_uint16(v, "dst", &obj->dst, &err);
    if (err) {
        goto out;
    }
    visit_type_uint8(v, "val", &obj->val, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_NumaDistOptions(Visitor *v, const char *name, NumaDistOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NumaDistOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NumaDistOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NumaDistOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NumaCpuOptions_members(Visitor *v, NumaCpuOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_CpuInstanceProperties_members(v, (CpuInstanceProperties *)obj, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_NumaCpuOptions(Visitor *v, const char *name, NumaCpuOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NumaCpuOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NumaCpuOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NumaCpuOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_HostMemPolicy(Visitor *v, const char *name, HostMemPolicy *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &HostMemPolicy_lookup, errp);
    *obj = value;
}

void visit_type_Memdev_members(Visitor *v, Memdev *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "id", &obj->has_id)) {
        visit_type_str(v, "id", &obj->id, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "merge", &obj->merge, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "dump", &obj->dump, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "prealloc", &obj->prealloc, &err);
    if (err) {
        goto out;
    }
    visit_type_uint16List(v, "host-nodes", &obj->host_nodes, &err);
    if (err) {
        goto out;
    }
    visit_type_HostMemPolicy(v, "policy", &obj->policy, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_Memdev(Visitor *v, const char *name, Memdev **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(Memdev), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_Memdev_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_Memdev(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_MemdevList(Visitor *v, const char *name, MemdevList **obj, Error **errp)
{
    Error *err = NULL;
    MemdevList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (MemdevList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_Memdev(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_MemdevList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_PCDIMMDeviceInfo_members(Visitor *v, PCDIMMDeviceInfo *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "id", &obj->has_id)) {
        visit_type_str(v, "id", &obj->id, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "addr", &obj->addr, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "slot", &obj->slot, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "node", &obj->node, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "memdev", &obj->memdev, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "hotplugged", &obj->hotplugged, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "hotpluggable", &obj->hotpluggable, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_PCDIMMDeviceInfo(Visitor *v, const char *name, PCDIMMDeviceInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(PCDIMMDeviceInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_PCDIMMDeviceInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_PCDIMMDeviceInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_PCDIMMDeviceInfo_wrapper_members(Visitor *v, q_obj_PCDIMMDeviceInfo_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_PCDIMMDeviceInfo(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_MemoryDeviceInfoKind(Visitor *v, const char *name, MemoryDeviceInfoKind *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &MemoryDeviceInfoKind_lookup, errp);
    *obj = value;
}

void visit_type_MemoryDeviceInfo_members(Visitor *v, MemoryDeviceInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_MemoryDeviceInfoKind(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case MEMORY_DEVICE_INFO_KIND_DIMM:
        visit_type_q_obj_PCDIMMDeviceInfo_wrapper_members(v, &obj->u.dimm, &err);
        break;
    case MEMORY_DEVICE_INFO_KIND_NVDIMM:
        visit_type_q_obj_PCDIMMDeviceInfo_wrapper_members(v, &obj->u.nvdimm, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_MemoryDeviceInfo(Visitor *v, const char *name, MemoryDeviceInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(MemoryDeviceInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_MemoryDeviceInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_MemoryDeviceInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_MemoryDeviceInfoList(Visitor *v, const char *name, MemoryDeviceInfoList **obj, Error **errp)
{
    Error *err = NULL;
    MemoryDeviceInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (MemoryDeviceInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_MemoryDeviceInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_MemoryDeviceInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_MEM_UNPLUG_ERROR_arg_members(Visitor *v, q_obj_MEM_UNPLUG_ERROR_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "msg", &obj->msg, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ACPISlotType(Visitor *v, const char *name, ACPISlotType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &ACPISlotType_lookup, errp);
    *obj = value;
}

void visit_type_ACPIOSTInfo_members(Visitor *v, ACPIOSTInfo *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "slot", &obj->slot, &err);
    if (err) {
        goto out;
    }
    visit_type_ACPISlotType(v, "slot-type", &obj->slot_type, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "source", &obj->source, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "status", &obj->status, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ACPIOSTInfo(Visitor *v, const char *name, ACPIOSTInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ACPIOSTInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ACPIOSTInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ACPIOSTInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ACPIOSTInfoList(Visitor *v, const char *name, ACPIOSTInfoList **obj, Error **errp)
{
    Error *err = NULL;
    ACPIOSTInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (ACPIOSTInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_ACPIOSTInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ACPIOSTInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ACPI_DEVICE_OST_arg_members(Visitor *v, q_obj_ACPI_DEVICE_OST_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ACPIOSTInfo(v, "info", &obj->info, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_RTC_CHANGE_arg_members(Visitor *v, q_obj_RTC_CHANGE_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "offset", &obj->offset, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ReplayMode(Visitor *v, const char *name, ReplayMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &ReplayMode_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_xen_load_devices_state_arg_members(Visitor *v, q_obj_xen_load_devices_state_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_GICCapability_members(Visitor *v, GICCapability *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "version", &obj->version, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "emulated", &obj->emulated, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "kernel", &obj->kernel, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_GICCapability(Visitor *v, const char *name, GICCapability **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(GICCapability), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_GICCapability_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_GICCapability(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_GICCapabilityList(Visitor *v, const char *name, GICCapabilityList **obj, Error **errp)
{
    Error *err = NULL;
    GICCapabilityList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (GICCapabilityList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_GICCapability(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_GICCapabilityList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CpuInstanceProperties_members(Visitor *v, CpuInstanceProperties *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "node-id", &obj->has_node_id)) {
        visit_type_int(v, "node-id", &obj->node_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "socket-id", &obj->has_socket_id)) {
        visit_type_int(v, "socket-id", &obj->socket_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "core-id", &obj->has_core_id)) {
        visit_type_int(v, "core-id", &obj->core_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "thread-id", &obj->has_thread_id)) {
        visit_type_int(v, "thread-id", &obj->thread_id, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_CpuInstanceProperties(Visitor *v, const char *name, CpuInstanceProperties **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(CpuInstanceProperties), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_CpuInstanceProperties_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_CpuInstanceProperties(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_HotpluggableCPU_members(Visitor *v, HotpluggableCPU *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "vcpus-count", &obj->vcpus_count, &err);
    if (err) {
        goto out;
    }
    visit_type_CpuInstanceProperties(v, "props", &obj->props, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "qom-path", &obj->has_qom_path)) {
        visit_type_str(v, "qom-path", &obj->qom_path, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_HotpluggableCPU(Visitor *v, const char *name, HotpluggableCPU **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(HotpluggableCPU), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_HotpluggableCPU_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_HotpluggableCPU(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_HotpluggableCPUList(Visitor *v, const char *name, HotpluggableCPUList **obj, Error **errp)
{
    Error *err = NULL;
    HotpluggableCPUList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (HotpluggableCPUList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_HotpluggableCPU(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_HotpluggableCPUList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_GuidInfo_members(Visitor *v, GuidInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "guid", &obj->guid, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_GuidInfo(Visitor *v, const char *name, GuidInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(GuidInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_GuidInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_GuidInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SevState(Visitor *v, const char *name, SevState *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &SevState_lookup, errp);
    *obj = value;
}

void visit_type_SevInfo_members(Visitor *v, SevInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "enabled", &obj->enabled, &err);
    if (err) {
        goto out;
    }
    visit_type_uint8(v, "api-major", &obj->api_major, &err);
    if (err) {
        goto out;
    }
    visit_type_uint8(v, "api-minor", &obj->api_minor, &err);
    if (err) {
        goto out;
    }
    visit_type_uint8(v, "build-id", &obj->build_id, &err);
    if (err) {
        goto out;
    }
    visit_type_uint32(v, "policy", &obj->policy, &err);
    if (err) {
        goto out;
    }
    visit_type_SevState(v, "state", &obj->state, &err);
    if (err) {
        goto out;
    }
    visit_type_uint32(v, "handle", &obj->handle, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SevInfo(Visitor *v, const char *name, SevInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SevInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SevInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SevInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SevLaunchMeasureInfo_members(Visitor *v, SevLaunchMeasureInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SevLaunchMeasureInfo(Visitor *v, const char *name, SevLaunchMeasureInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SevLaunchMeasureInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SevLaunchMeasureInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SevLaunchMeasureInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SevCapability_members(Visitor *v, SevCapability *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "pdh", &obj->pdh, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "cert-chain", &obj->cert_chain, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "cbitpos", &obj->cbitpos, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "reduced-phys-bits", &obj->reduced_phys_bits, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SevCapability(Visitor *v, const char *name, SevCapability **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SevCapability), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SevCapability_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SevCapability(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_CommandDropReason(Visitor *v, const char *name, CommandDropReason *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &CommandDropReason_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_COMMAND_DROPPED_arg_members(Visitor *v, q_obj_COMMAND_DROPPED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_any(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_CommandDropReason(v, "reason", &obj->reason, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_x_oob_test_arg_members(Visitor *v, q_obj_x_oob_test_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "lock", &obj->lock, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_misc_c;
