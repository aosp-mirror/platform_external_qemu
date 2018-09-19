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
#include "qapi-visit-migration.h"
#include "qapi/error.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qobject-output-visitor.h"
#include "qapi/qmp-event.h"


void qapi_event_send_migration(MigrationStatus status, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_MIGRATION_arg param = {
        status
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("MIGRATION");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "MIGRATION", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_MIGRATION_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_MIGRATION, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_migration_pass(int64_t pass, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_MIGRATION_PASS_arg param = {
        pass
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("MIGRATION_PASS");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "MIGRATION_PASS", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_MIGRATION_PASS_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_MIGRATION_PASS, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_events_migration_c;
