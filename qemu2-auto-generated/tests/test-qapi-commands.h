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

#ifndef TEST_QAPI_COMMANDS_H
#define TEST_QAPI_COMMANDS_H

#include "test-qapi-types.h"
#include "qapi/qmp/dispatch.h"

Empty2 *qmp_user_def_cmd0(Error **errp);
void qmp_marshal_user_def_cmd0(QDict *args, QObject **ret, Error **errp);
void qmp_user_def_cmd(Error **errp);
void qmp_marshal_user_def_cmd(QDict *args, QObject **ret, Error **errp);
void qmp_user_def_cmd1(UserDefOne *ud1a, Error **errp);
void qmp_marshal_user_def_cmd1(QDict *args, QObject **ret, Error **errp);
UserDefTwo *qmp_user_def_cmd2(UserDefOne *ud1a, bool has_ud1b, UserDefOne *ud1b, Error **errp);
void qmp_marshal_user_def_cmd2(QDict *args, QObject **ret, Error **errp);
int64_t qmp_guest_get_time(int64_t a, bool has_b, int64_t b, Error **errp);
void qmp_marshal_guest_get_time(QDict *args, QObject **ret, Error **errp);
QObject *qmp_guest_sync(QObject *arg, Error **errp);
void qmp_marshal_guest_sync(QDict *args, QObject **ret, Error **errp);
void qmp_boxed_struct(UserDefZero *arg, Error **errp);
void qmp_marshal_boxed_struct(QDict *args, QObject **ret, Error **errp);
void qmp_boxed_union(UserDefNativeListUnion *arg, Error **errp);
void qmp_marshal_boxed_union(QDict *args, QObject **ret, Error **errp);
void qmp_an_oob_command(Error **errp);
void qmp_marshal_an_oob_command(QDict *args, QObject **ret, Error **errp);
__org_qemu_x_Union1 *qmp___org_qemu_x_command(__org_qemu_x_EnumList *a, __org_qemu_x_StructList *b, __org_qemu_x_Union2 *c, __org_qemu_x_Alt *d, Error **errp);
void qmp_marshal___org_qemu_x_command(QDict *args, QObject **ret, Error **errp);
void test_qmp_init_marshal(QmpCommandList *cmds);

#endif /* TEST_QAPI_COMMANDS_H */
