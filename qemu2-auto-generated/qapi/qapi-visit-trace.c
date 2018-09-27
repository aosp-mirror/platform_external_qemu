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
#include "qapi-visit-trace.h"

void visit_type_TraceEventState(Visitor *v, const char *name, TraceEventState *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &TraceEventState_lookup, errp);
    *obj = value;
}

void visit_type_TraceEventInfo_members(Visitor *v, TraceEventInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_TraceEventState(v, "state", &obj->state, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "vcpu", &obj->vcpu, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_TraceEventInfo(Visitor *v, const char *name, TraceEventInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(TraceEventInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_TraceEventInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TraceEventInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_trace_event_get_state_arg_members(Visitor *v, q_obj_trace_event_get_state_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "vcpu", &obj->has_vcpu)) {
        visit_type_int(v, "vcpu", &obj->vcpu, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_TraceEventInfoList(Visitor *v, const char *name, TraceEventInfoList **obj, Error **errp)
{
    Error *err = NULL;
    TraceEventInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (TraceEventInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_TraceEventInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TraceEventInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_trace_event_set_state_arg_members(Visitor *v, q_obj_trace_event_set_state_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "enable", &obj->enable, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "ignore-unavailable", &obj->has_ignore_unavailable)) {
        visit_type_bool(v, "ignore-unavailable", &obj->ignore_unavailable, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "vcpu", &obj->has_vcpu)) {
        visit_type_int(v, "vcpu", &obj->vcpu, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_trace_c;
