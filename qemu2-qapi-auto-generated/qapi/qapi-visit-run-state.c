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
#include "qapi-visit-run-state.h"

void visit_type_RunState(Visitor *v, const char *name, RunState *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &RunState_lookup, errp);
    *obj = value;
}

void visit_type_StatusInfo_members(Visitor *v, StatusInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "running", &obj->running, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "singlestep", &obj->singlestep, &err);
    if (err) {
        goto out;
    }
    visit_type_RunState(v, "status", &obj->status, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_StatusInfo(Visitor *v, const char *name, StatusInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(StatusInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_StatusInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_StatusInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_SHUTDOWN_arg_members(Visitor *v, q_obj_SHUTDOWN_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "guest", &obj->guest, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_RESET_arg_members(Visitor *v, q_obj_RESET_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "guest", &obj->guest, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_WATCHDOG_arg_members(Visitor *v, q_obj_WATCHDOG_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_WatchdogAction(v, "action", &obj->action, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_WatchdogAction(Visitor *v, const char *name, WatchdogAction *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &WatchdogAction_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_watchdog_set_action_arg_members(Visitor *v, q_obj_watchdog_set_action_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_WatchdogAction(v, "action", &obj->action, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_GUEST_PANICKED_arg_members(Visitor *v, q_obj_GUEST_PANICKED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_GuestPanicAction(v, "action", &obj->action, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "info", &obj->has_info)) {
        visit_type_GuestPanicInformation(v, "info", &obj->info, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_GuestPanicAction(Visitor *v, const char *name, GuestPanicAction *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &GuestPanicAction_lookup, errp);
    *obj = value;
}

void visit_type_GuestPanicInformationType(Visitor *v, const char *name, GuestPanicInformationType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &GuestPanicInformationType_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_GuestPanicInformation_base_members(Visitor *v, q_obj_GuestPanicInformation_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_GuestPanicInformationType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_GuestPanicInformation_members(Visitor *v, GuestPanicInformation *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_GuestPanicInformation_base_members(v, (q_obj_GuestPanicInformation_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case GUEST_PANIC_INFORMATION_TYPE_HYPER_V:
        visit_type_GuestPanicInformationHyperV_members(v, &obj->u.hyper_v, &err);
        break;
    case GUEST_PANIC_INFORMATION_TYPE_S390:
        visit_type_GuestPanicInformationS390_members(v, &obj->u.s390, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_GuestPanicInformation(Visitor *v, const char *name, GuestPanicInformation **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(GuestPanicInformation), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_GuestPanicInformation_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_GuestPanicInformation(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_GuestPanicInformationHyperV_members(Visitor *v, GuestPanicInformationHyperV *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint64(v, "arg1", &obj->arg1, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "arg2", &obj->arg2, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "arg3", &obj->arg3, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "arg4", &obj->arg4, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "arg5", &obj->arg5, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_GuestPanicInformationHyperV(Visitor *v, const char *name, GuestPanicInformationHyperV **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(GuestPanicInformationHyperV), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_GuestPanicInformationHyperV_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_GuestPanicInformationHyperV(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_S390CrashReason(Visitor *v, const char *name, S390CrashReason *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &S390CrashReason_lookup, errp);
    *obj = value;
}

void visit_type_GuestPanicInformationS390_members(Visitor *v, GuestPanicInformationS390 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint32(v, "core", &obj->core, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "psw-mask", &obj->psw_mask, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "psw-addr", &obj->psw_addr, &err);
    if (err) {
        goto out;
    }
    visit_type_S390CrashReason(v, "reason", &obj->reason, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_GuestPanicInformationS390(Visitor *v, const char *name, GuestPanicInformationS390 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(GuestPanicInformationS390), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_GuestPanicInformationS390_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_GuestPanicInformationS390(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_run_state_c;
