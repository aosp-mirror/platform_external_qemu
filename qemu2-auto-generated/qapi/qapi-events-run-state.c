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
#include "qapi-visit-run-state.h"
#include "qapi/error.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qobject-output-visitor.h"
#include "qapi/qmp-event.h"


void qapi_event_send_shutdown(bool guest, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_SHUTDOWN_arg param = {
        guest
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("SHUTDOWN");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "SHUTDOWN", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_SHUTDOWN_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_SHUTDOWN, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_powerdown(Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("POWERDOWN");

    emit(QAPI_EVENT_POWERDOWN, qmp, &err);

    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_reset(bool guest, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_RESET_arg param = {
        guest
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("RESET");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "RESET", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_RESET_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_RESET, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_stop(Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("STOP");

    emit(QAPI_EVENT_STOP, qmp, &err);

    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_resume(Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("RESUME");

    emit(QAPI_EVENT_RESUME, qmp, &err);

    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_suspend(Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("SUSPEND");

    emit(QAPI_EVENT_SUSPEND, qmp, &err);

    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_suspend_disk(Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("SUSPEND_DISK");

    emit(QAPI_EVENT_SUSPEND_DISK, qmp, &err);

    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_wakeup(Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("WAKEUP");

    emit(QAPI_EVENT_WAKEUP, qmp, &err);

    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_watchdog(WatchdogAction action, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_WATCHDOG_arg param = {
        action
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("WATCHDOG");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "WATCHDOG", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_WATCHDOG_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_WATCHDOG, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_guest_panicked(GuestPanicAction action, bool has_info, GuestPanicInformation *info, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_GUEST_PANICKED_arg param = {
        action, has_info, info
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("GUEST_PANICKED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "GUEST_PANICKED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_GUEST_PANICKED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_GUEST_PANICKED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_events_run_state_c;
