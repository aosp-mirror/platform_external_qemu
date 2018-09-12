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

#ifndef QAPI_COMMANDS_RUN_STATE_H
#define QAPI_COMMANDS_RUN_STATE_H

#include "qapi-types-run-state.h"
#include "qapi/qmp/dispatch.h"

StatusInfo *qmp_query_status(Error **errp);
void qmp_marshal_query_status(QDict *args, QObject **ret, Error **errp);
void qmp_watchdog_set_action(WatchdogAction action, Error **errp);
void qmp_marshal_watchdog_set_action(QDict *args, QObject **ret, Error **errp);

#endif /* QAPI_COMMANDS_RUN_STATE_H */
