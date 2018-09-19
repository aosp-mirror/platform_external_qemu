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

#ifndef QAPI_COMMANDS_TRACE_H
#define QAPI_COMMANDS_TRACE_H

#include "qapi-types-trace.h"
#include "qapi/qmp/dispatch.h"

TraceEventInfoList *qmp_trace_event_get_state(const char *name, bool has_vcpu, int64_t vcpu, Error **errp);
void qmp_marshal_trace_event_get_state(QDict *args, QObject **ret, Error **errp);
void qmp_trace_event_set_state(const char *name, bool enable, bool has_ignore_unavailable, bool ignore_unavailable, bool has_vcpu, int64_t vcpu, Error **errp);
void qmp_marshal_trace_event_set_state(QDict *args, QObject **ret, Error **errp);

#endif /* QAPI_COMMANDS_TRACE_H */
