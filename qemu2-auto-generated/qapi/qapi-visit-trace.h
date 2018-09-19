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

#ifndef QAPI_VISIT_TRACE_H
#define QAPI_VISIT_TRACE_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-trace.h"

void visit_type_TraceEventState(Visitor *v, const char *name, TraceEventState *obj, Error **errp);

void visit_type_TraceEventInfo_members(Visitor *v, TraceEventInfo *obj, Error **errp);
void visit_type_TraceEventInfo(Visitor *v, const char *name, TraceEventInfo **obj, Error **errp);

void visit_type_q_obj_trace_event_get_state_arg_members(Visitor *v, q_obj_trace_event_get_state_arg *obj, Error **errp);
void visit_type_TraceEventInfoList(Visitor *v, const char *name, TraceEventInfoList **obj, Error **errp);

void visit_type_q_obj_trace_event_set_state_arg_members(Visitor *v, q_obj_trace_event_set_state_arg *obj, Error **errp);

#endif /* QAPI_VISIT_TRACE_H */
