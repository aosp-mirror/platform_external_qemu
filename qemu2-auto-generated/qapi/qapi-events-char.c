/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI/QMP events
 *
 * Copyright (c) 2014 Wenchao Xia
 * Copyright (c) 2015-2018 Red Hat Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qapi-events.h"
#include "qapi-visit-char.h"
#include "qapi/error.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qobject-output-visitor.h"
#include "qapi/qmp-event.h"


void qapi_event_send_vserport_change(const char *id, bool open, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_VSERPORT_CHANGE_arg param = {
        (char *)id, open
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("VSERPORT_CHANGE");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "VSERPORT_CHANGE", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_VSERPORT_CHANGE_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_VSERPORT_CHANGE, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_events_char_c;
