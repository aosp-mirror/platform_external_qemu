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

#ifndef QAPI_COMMANDS_NET_H
#define QAPI_COMMANDS_NET_H

#include "qapi-commands-common.h"
#include "qapi-types-net.h"
#include "qapi/qmp/dispatch.h"

void qmp_set_link(const char *name, bool up, Error **errp);
void qmp_marshal_set_link(QDict *args, QObject **ret, Error **errp);
void qmp_netdev_del(const char *id, Error **errp);
void qmp_marshal_netdev_del(QDict *args, QObject **ret, Error **errp);
RxFilterInfoList *qmp_query_rx_filter(bool has_name, const char *name, Error **errp);
void qmp_marshal_query_rx_filter(QDict *args, QObject **ret, Error **errp);

#endif /* QAPI_COMMANDS_NET_H */
