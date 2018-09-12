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
#include "qapi-visit-ui.h"
#include "qapi/error.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qobject-output-visitor.h"
#include "qapi/qmp-event.h"


void qapi_event_send_spice_connected(SpiceBasicInfo *server, SpiceBasicInfo *client, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_SPICE_CONNECTED_arg param = {
        server, client
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("SPICE_CONNECTED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "SPICE_CONNECTED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_SPICE_CONNECTED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_SPICE_CONNECTED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_spice_initialized(SpiceServerInfo *server, SpiceChannel *client, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_SPICE_INITIALIZED_arg param = {
        server, client
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("SPICE_INITIALIZED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "SPICE_INITIALIZED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_SPICE_INITIALIZED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_SPICE_INITIALIZED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_spice_disconnected(SpiceBasicInfo *server, SpiceBasicInfo *client, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_SPICE_DISCONNECTED_arg param = {
        server, client
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("SPICE_DISCONNECTED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "SPICE_DISCONNECTED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_SPICE_DISCONNECTED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_SPICE_DISCONNECTED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_spice_migrate_completed(Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("SPICE_MIGRATE_COMPLETED");

    emit(QAPI_EVENT_SPICE_MIGRATE_COMPLETED, qmp, &err);

    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_vnc_connected(VncServerInfo *server, VncBasicInfo *client, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_VNC_CONNECTED_arg param = {
        server, client
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("VNC_CONNECTED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "VNC_CONNECTED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_VNC_CONNECTED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_VNC_CONNECTED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_vnc_initialized(VncServerInfo *server, VncClientInfo *client, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_VNC_INITIALIZED_arg param = {
        server, client
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("VNC_INITIALIZED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "VNC_INITIALIZED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_VNC_INITIALIZED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_VNC_INITIALIZED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_vnc_disconnected(VncServerInfo *server, VncClientInfo *client, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_VNC_DISCONNECTED_arg param = {
        server, client
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("VNC_DISCONNECTED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "VNC_DISCONNECTED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_VNC_DISCONNECTED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_VNC_DISCONNECTED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_events_ui_c;
