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

#ifndef QAPI_COMMANDS_UI_H
#define QAPI_COMMANDS_UI_H

#include "qapi-commands-sockets.h"
#include "qapi-types-ui.h"
#include "qapi/qmp/dispatch.h"

void qmp_set_password(const char *protocol, const char *password, bool has_connected, const char *connected, Error **errp);
void qmp_marshal_set_password(QDict *args, QObject **ret, Error **errp);
void qmp_expire_password(const char *protocol, const char *time, Error **errp);
void qmp_marshal_expire_password(QDict *args, QObject **ret, Error **errp);
void qmp_screendump(const char *filename, bool has_device, const char *device, bool has_head, int64_t head, Error **errp);
void qmp_marshal_screendump(QDict *args, QObject **ret, Error **errp);
SpiceInfo *qmp_query_spice(Error **errp);
void qmp_marshal_query_spice(QDict *args, QObject **ret, Error **errp);
VncInfo *qmp_query_vnc(Error **errp);
void qmp_marshal_query_vnc(QDict *args, QObject **ret, Error **errp);
VncInfo2List *qmp_query_vnc_servers(Error **errp);
void qmp_marshal_query_vnc_servers(QDict *args, QObject **ret, Error **errp);
void qmp_change_vnc_password(const char *password, Error **errp);
void qmp_marshal_change_vnc_password(QDict *args, QObject **ret, Error **errp);
MouseInfoList *qmp_query_mice(Error **errp);
void qmp_marshal_query_mice(QDict *args, QObject **ret, Error **errp);
void qmp_send_key(KeyValueList *keys, bool has_hold_time, int64_t hold_time, Error **errp);
void qmp_marshal_send_key(QDict *args, QObject **ret, Error **errp);
void qmp_input_send_event(bool has_device, const char *device, bool has_head, int64_t head, InputEventList *events, Error **errp);
void qmp_marshal_input_send_event(QDict *args, QObject **ret, Error **errp);

#endif /* QAPI_COMMANDS_UI_H */
