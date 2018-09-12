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
#include "test-qapi-visit.h"
#include "test-qapi-commands.h"


static void qmp_marshal_output_Empty2(Empty2 *ret_in, QObject **ret_out, Error **errp)
{
    Error *err = NULL;
    Visitor *v;

    v = qobject_output_visitor_new(ret_out);
    visit_type_Empty2(v, "unused", &ret_in, &err);
    if (!err) {
        visit_complete(v, ret_out);
    }
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_type_Empty2(v, "unused", &ret_in, NULL);
    visit_free(v);
}

void qmp_marshal_user_def_cmd0(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    Empty2 *retval;
    Visitor *v = NULL;

    if (args) {
        v = qobject_input_visitor_new(QOBJECT(args));
        visit_start_struct(v, NULL, NULL, 0, &err);
        if (err) {
            goto out;
        }
        
        if (!err) {
            visit_check_struct(v, &err);
        }
        visit_end_struct(v, NULL);
        if (err) {
            goto out;
        }
    }

    retval = qmp_user_def_cmd0(&err);
    if (err) {
        goto out;
    }

    qmp_marshal_output_Empty2(retval, ret, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    if (args) {
        v = qapi_dealloc_visitor_new();
        visit_start_struct(v, NULL, NULL, 0, NULL);
        
        visit_end_struct(v, NULL);
        visit_free(v);
    }
}

void qmp_marshal_user_def_cmd(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    Visitor *v = NULL;

    if (args) {
        v = qobject_input_visitor_new(QOBJECT(args));
        visit_start_struct(v, NULL, NULL, 0, &err);
        if (err) {
            goto out;
        }
        
        if (!err) {
            visit_check_struct(v, &err);
        }
        visit_end_struct(v, NULL);
        if (err) {
            goto out;
        }
    }

    qmp_user_def_cmd(&err);

out:
    error_propagate(errp, err);
    visit_free(v);
    if (args) {
        v = qapi_dealloc_visitor_new();
        visit_start_struct(v, NULL, NULL, 0, NULL);
        
        visit_end_struct(v, NULL);
        visit_free(v);
    }
}

void qmp_marshal_user_def_cmd1(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    Visitor *v;
    q_obj_user_def_cmd1_arg arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_user_def_cmd1_arg_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    qmp_user_def_cmd1(arg.ud1a, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_q_obj_user_def_cmd1_arg_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}

static void qmp_marshal_output_UserDefTwo(UserDefTwo *ret_in, QObject **ret_out, Error **errp)
{
    Error *err = NULL;
    Visitor *v;

    v = qobject_output_visitor_new(ret_out);
    visit_type_UserDefTwo(v, "unused", &ret_in, &err);
    if (!err) {
        visit_complete(v, ret_out);
    }
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_type_UserDefTwo(v, "unused", &ret_in, NULL);
    visit_free(v);
}

void qmp_marshal_user_def_cmd2(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    UserDefTwo *retval;
    Visitor *v;
    q_obj_user_def_cmd2_arg arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_user_def_cmd2_arg_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    retval = qmp_user_def_cmd2(arg.ud1a, arg.has_ud1b, arg.ud1b, &err);
    if (err) {
        goto out;
    }

    qmp_marshal_output_UserDefTwo(retval, ret, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_q_obj_user_def_cmd2_arg_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}

static void qmp_marshal_output_int(int64_t ret_in, QObject **ret_out, Error **errp)
{
    Error *err = NULL;
    Visitor *v;

    v = qobject_output_visitor_new(ret_out);
    visit_type_int(v, "unused", &ret_in, &err);
    if (!err) {
        visit_complete(v, ret_out);
    }
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_type_int(v, "unused", &ret_in, NULL);
    visit_free(v);
}

void qmp_marshal_guest_get_time(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    int64_t retval;
    Visitor *v;
    q_obj_guest_get_time_arg arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_guest_get_time_arg_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    retval = qmp_guest_get_time(arg.a, arg.has_b, arg.b, &err);
    if (err) {
        goto out;
    }

    qmp_marshal_output_int(retval, ret, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_q_obj_guest_get_time_arg_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}

static void qmp_marshal_output_any(QObject *ret_in, QObject **ret_out, Error **errp)
{
    Error *err = NULL;
    Visitor *v;

    v = qobject_output_visitor_new(ret_out);
    visit_type_any(v, "unused", &ret_in, &err);
    if (!err) {
        visit_complete(v, ret_out);
    }
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_type_any(v, "unused", &ret_in, NULL);
    visit_free(v);
}

void qmp_marshal_guest_sync(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    QObject *retval;
    Visitor *v;
    q_obj_guest_sync_arg arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_guest_sync_arg_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    retval = qmp_guest_sync(arg.arg, &err);
    if (err) {
        goto out;
    }

    qmp_marshal_output_any(retval, ret, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_q_obj_guest_sync_arg_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}

void qmp_marshal_boxed_struct(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    Visitor *v;
    UserDefZero arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_UserDefZero_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    qmp_boxed_struct(&arg, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_UserDefZero_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}

void qmp_marshal_boxed_union(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    Visitor *v;
    UserDefNativeListUnion arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_UserDefNativeListUnion_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    qmp_boxed_union(&arg, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_UserDefNativeListUnion_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}

void qmp_marshal_an_oob_command(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    Visitor *v = NULL;

    if (args) {
        v = qobject_input_visitor_new(QOBJECT(args));
        visit_start_struct(v, NULL, NULL, 0, &err);
        if (err) {
            goto out;
        }
        
        if (!err) {
            visit_check_struct(v, &err);
        }
        visit_end_struct(v, NULL);
        if (err) {
            goto out;
        }
    }

    qmp_an_oob_command(&err);

out:
    error_propagate(errp, err);
    visit_free(v);
    if (args) {
        v = qapi_dealloc_visitor_new();
        visit_start_struct(v, NULL, NULL, 0, NULL);
        
        visit_end_struct(v, NULL);
        visit_free(v);
    }
}

static void qmp_marshal_output___org_qemu_x_Union1(__org_qemu_x_Union1 *ret_in, QObject **ret_out, Error **errp)
{
    Error *err = NULL;
    Visitor *v;

    v = qobject_output_visitor_new(ret_out);
    visit_type___org_qemu_x_Union1(v, "unused", &ret_in, &err);
    if (!err) {
        visit_complete(v, ret_out);
    }
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_type___org_qemu_x_Union1(v, "unused", &ret_in, NULL);
    visit_free(v);
}

void qmp_marshal___org_qemu_x_command(QDict *args, QObject **ret, Error **errp)
{
    Error *err = NULL;
    __org_qemu_x_Union1 *retval;
    Visitor *v;
    q_obj___org_qemu_x_command_arg arg = {0};

    v = qobject_input_visitor_new(QOBJECT(args));
    visit_start_struct(v, NULL, NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj___org_qemu_x_command_arg_members(v, &arg, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    retval = qmp___org_qemu_x_command(arg.a, arg.b, arg.c, arg.d, &err);
    if (err) {
        goto out;
    }

    qmp_marshal_output___org_qemu_x_Union1(retval, ret, &err);

out:
    error_propagate(errp, err);
    visit_free(v);
    v = qapi_dealloc_visitor_new();
    visit_start_struct(v, NULL, NULL, 0, NULL);
    visit_type_q_obj___org_qemu_x_command_arg_members(v, &arg, NULL);
    visit_end_struct(v, NULL);
    visit_free(v);
}

void test_qmp_init_marshal(QmpCommandList *cmds)
{
    QTAILQ_INIT(cmds);

    qmp_register_command(cmds, "user_def_cmd0",
                         qmp_marshal_user_def_cmd0, QCO_NO_OPTIONS);
    qmp_register_command(cmds, "user_def_cmd",
                         qmp_marshal_user_def_cmd, QCO_NO_OPTIONS);
    qmp_register_command(cmds, "user_def_cmd1",
                         qmp_marshal_user_def_cmd1, QCO_NO_OPTIONS);
    qmp_register_command(cmds, "user_def_cmd2",
                         qmp_marshal_user_def_cmd2, QCO_NO_OPTIONS);
    qmp_register_command(cmds, "guest-get-time",
                         qmp_marshal_guest_get_time, QCO_NO_OPTIONS);
    qmp_register_command(cmds, "guest-sync",
                         qmp_marshal_guest_sync, QCO_NO_OPTIONS);
    qmp_register_command(cmds, "boxed-struct",
                         qmp_marshal_boxed_struct, QCO_NO_OPTIONS);
    qmp_register_command(cmds, "boxed-union",
                         qmp_marshal_boxed_union, QCO_NO_OPTIONS);
    qmp_register_command(cmds, "an-oob-command",
                         qmp_marshal_an_oob_command, QCO_ALLOW_OOB);
    qmp_register_command(cmds, "__org.qemu_x-command",
                         qmp_marshal___org_qemu_x_command, QCO_NO_OPTIONS);
}
/* Dummy declaration to prevent empty .o file */
char dummy_test_qapi_commands_c;
