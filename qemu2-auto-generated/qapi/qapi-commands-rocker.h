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

#ifndef QAPI_COMMANDS_ROCKER_H
#define QAPI_COMMANDS_ROCKER_H

#include "qapi-types-rocker.h"
#include "qapi/qmp/dispatch.h"

RockerSwitch *qmp_query_rocker(const char *name, Error **errp);
void qmp_marshal_query_rocker(QDict *args, QObject **ret, Error **errp);
RockerPortList *qmp_query_rocker_ports(const char *name, Error **errp);
void qmp_marshal_query_rocker_ports(QDict *args, QObject **ret, Error **errp);
RockerOfDpaFlowList *qmp_query_rocker_of_dpa_flows(const char *name, bool has_tbl_id, uint32_t tbl_id, Error **errp);
void qmp_marshal_query_rocker_of_dpa_flows(QDict *args, QObject **ret, Error **errp);
RockerOfDpaGroupList *qmp_query_rocker_of_dpa_groups(const char *name, bool has_type, uint8_t type, Error **errp);
void qmp_marshal_query_rocker_of_dpa_groups(QDict *args, QObject **ret, Error **errp);

#endif /* QAPI_COMMANDS_ROCKER_H */
