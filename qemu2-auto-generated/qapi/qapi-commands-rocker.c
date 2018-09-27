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

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/module.h"
#include "qapi/visitor.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qobject-output-visitor.h"
#include "qapi/qobject-input-visitor.h"
#include "qapi/dealloc-visitor.h"
#include "qapi/error.h"
#include "qapi-visit-rocker.h"
#include "qapi-commands-rocker.h"


static void qmp_marshal_output_RockerSwitch(RockerSwitch *ret_in, QObject **ret_out, Error **errp)
{
    Error *err = NULL;
    Visitor *v;

    v = qobject_output_visitor_new(ret_out);
    visit_type_RockerSwitch(v, "unused", &ret_in, &err);
    if (!err) {
        visit_complete(v, ret_out);
    }
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_type_RockerSwitch(v, "unused", &ret_in, NULL);
    visit_free(v);
}

void qmp_marshal_query_rocker(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    RockerSwitch *retval;
    Visitor *v;
    q_obj_query_rocker_arg arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_query_rocker_arg_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    retval = qmp_query_rocker(arg.name, &err);
    if (err) {
        goto out;
    }

    qmp_marshal_output_RockerSwitch(retval, ret, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_q_obj_query_rocker_arg_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}

static void qmp_marshal_output_RockerPortList(RockerPortList *ret_in, QObject **ret_out, Error **errp)
{
    Error *err = NULL;
    Visitor *v;

    v = qobject_output_visitor_new(ret_out);
    visit_type_RockerPortList(v, "unused", &ret_in, &err);
    if (!err) {
        visit_complete(v, ret_out);
    }
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_type_RockerPortList(v, "unused", &ret_in, NULL);
    visit_free(v);
}

void qmp_marshal_query_rocker_ports(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    RockerPortList *retval;
    Visitor *v;
    q_obj_query_rocker_ports_arg arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_query_rocker_ports_arg_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    retval = qmp_query_rocker_ports(arg.name, &err);
    if (err) {
        goto out;
    }

    qmp_marshal_output_RockerPortList(retval, ret, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_q_obj_query_rocker_ports_arg_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}

static void qmp_marshal_output_RockerOfDpaFlowList(RockerOfDpaFlowList *ret_in, QObject **ret_out, Error **errp)
{
    Error *err = NULL;
    Visitor *v;

    v = qobject_output_visitor_new(ret_out);
    visit_type_RockerOfDpaFlowList(v, "unused", &ret_in, &err);
    if (!err) {
        visit_complete(v, ret_out);
    }
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_type_RockerOfDpaFlowList(v, "unused", &ret_in, NULL);
    visit_free(v);
}

void qmp_marshal_query_rocker_of_dpa_flows(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    RockerOfDpaFlowList *retval;
    Visitor *v;
    q_obj_query_rocker_of_dpa_flows_arg arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_query_rocker_of_dpa_flows_arg_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    retval = qmp_query_rocker_of_dpa_flows(arg.name, arg.has_tbl_id, arg.tbl_id, &err);
    if (err) {
        goto out;
    }

    qmp_marshal_output_RockerOfDpaFlowList(retval, ret, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_q_obj_query_rocker_of_dpa_flows_arg_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}

static void qmp_marshal_output_RockerOfDpaGroupList(RockerOfDpaGroupList *ret_in, QObject **ret_out, Error **errp)
{
    Error *err = NULL;
    Visitor *v;

    v = qobject_output_visitor_new(ret_out);
    visit_type_RockerOfDpaGroupList(v, "unused", &ret_in, &err);
    if (!err) {
        visit_complete(v, ret_out);
    }
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_type_RockerOfDpaGroupList(v, "unused", &ret_in, NULL);
    visit_free(v);
}

void qmp_marshal_query_rocker_of_dpa_groups(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    RockerOfDpaGroupList *retval;
    Visitor *v;
    q_obj_query_rocker_of_dpa_groups_arg arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_query_rocker_of_dpa_groups_arg_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    retval = qmp_query_rocker_of_dpa_groups(arg.name, arg.has_type, arg.type, &err);
    if (err) {
        goto out;
    }

    qmp_marshal_output_RockerOfDpaGroupList(retval, ret, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_q_obj_query_rocker_of_dpa_groups_arg_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_commands_rocker_c;
