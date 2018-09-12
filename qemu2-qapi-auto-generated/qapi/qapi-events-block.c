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
#include "qapi-visit-block.h"
#include "qapi/error.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qobject-output-visitor.h"
#include "qapi/qmp-event.h"


void qapi_event_send_device_tray_moved(const char *device, const char *id, bool tray_open, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_DEVICE_TRAY_MOVED_arg param = {
        (char *)device, (char *)id, tray_open
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("DEVICE_TRAY_MOVED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "DEVICE_TRAY_MOVED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_DEVICE_TRAY_MOVED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_DEVICE_TRAY_MOVED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_quorum_failure(const char *reference, int64_t sector_num, int64_t sectors_count, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_QUORUM_FAILURE_arg param = {
        (char *)reference, sector_num, sectors_count
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("QUORUM_FAILURE");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "QUORUM_FAILURE", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_QUORUM_FAILURE_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_QUORUM_FAILURE, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_quorum_report_bad(QuorumOpType type, bool has_error, const char *error, const char *node_name, int64_t sector_num, int64_t sectors_count, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_QUORUM_REPORT_BAD_arg param = {
        type, has_error, (char *)error, (char *)node_name, sector_num, sectors_count
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("QUORUM_REPORT_BAD");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "QUORUM_REPORT_BAD", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_QUORUM_REPORT_BAD_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_QUORUM_REPORT_BAD, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_events_block_c;
