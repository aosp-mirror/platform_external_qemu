/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI types
 *
 * Copyright IBM, Corp. 2011
 * Copyright (c) 2013-2018 Red Hat Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qapi/dealloc-visitor.h"
#include "qapi-types-trace.h"
#include "qapi-visit-trace.h"

const QEnumLookup TraceEventState_lookup = {
    .array = (const char *const[]) {
        [TRACE_EVENT_STATE_UNAVAILABLE] = "unavailable",
        [TRACE_EVENT_STATE_DISABLED] = "disabled",
        [TRACE_EVENT_STATE_ENABLED] = "enabled",
    },
    .size = TRACE_EVENT_STATE__MAX
};

void qapi_free_TraceEventInfo(TraceEventInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TraceEventInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_TraceEventInfoList(TraceEventInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TraceEventInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_trace_c;
