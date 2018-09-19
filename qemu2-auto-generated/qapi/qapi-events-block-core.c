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
#include "qapi-visit-block-core.h"
#include "qapi/error.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qobject-output-visitor.h"
#include "qapi/qmp-event.h"


void qapi_event_send_block_image_corrupted(const char *device, bool has_node_name, const char *node_name, const char *msg, bool has_offset, int64_t offset, bool has_size, int64_t size, bool fatal, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_BLOCK_IMAGE_CORRUPTED_arg param = {
        (char *)device, has_node_name, (char *)node_name, (char *)msg, has_offset, offset, has_size, size, fatal
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("BLOCK_IMAGE_CORRUPTED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "BLOCK_IMAGE_CORRUPTED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_BLOCK_IMAGE_CORRUPTED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_BLOCK_IMAGE_CORRUPTED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_block_io_error(const char *device, bool has_node_name, const char *node_name, IoOperationType operation, BlockErrorAction action, bool has_nospace, bool nospace, const char *reason, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_BLOCK_IO_ERROR_arg param = {
        (char *)device, has_node_name, (char *)node_name, operation, action, has_nospace, nospace, (char *)reason
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("BLOCK_IO_ERROR");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "BLOCK_IO_ERROR", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_BLOCK_IO_ERROR_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_BLOCK_IO_ERROR, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_block_job_completed(BlockJobType type, const char *device, int64_t len, int64_t offset, int64_t speed, bool has_error, const char *error, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_BLOCK_JOB_COMPLETED_arg param = {
        type, (char *)device, len, offset, speed, has_error, (char *)error
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("BLOCK_JOB_COMPLETED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "BLOCK_JOB_COMPLETED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_BLOCK_JOB_COMPLETED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_BLOCK_JOB_COMPLETED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_block_job_cancelled(BlockJobType type, const char *device, int64_t len, int64_t offset, int64_t speed, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_BLOCK_JOB_CANCELLED_arg param = {
        type, (char *)device, len, offset, speed
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("BLOCK_JOB_CANCELLED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "BLOCK_JOB_CANCELLED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_BLOCK_JOB_CANCELLED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_BLOCK_JOB_CANCELLED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_block_job_error(const char *device, IoOperationType operation, BlockErrorAction action, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_BLOCK_JOB_ERROR_arg param = {
        (char *)device, operation, action
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("BLOCK_JOB_ERROR");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "BLOCK_JOB_ERROR", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_BLOCK_JOB_ERROR_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_BLOCK_JOB_ERROR, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_block_job_ready(BlockJobType type, const char *device, int64_t len, int64_t offset, int64_t speed, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_BLOCK_JOB_READY_arg param = {
        type, (char *)device, len, offset, speed
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("BLOCK_JOB_READY");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "BLOCK_JOB_READY", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_BLOCK_JOB_READY_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_BLOCK_JOB_READY, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_block_job_pending(BlockJobType type, const char *id, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_BLOCK_JOB_PENDING_arg param = {
        type, (char *)id
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("BLOCK_JOB_PENDING");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "BLOCK_JOB_PENDING", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_BLOCK_JOB_PENDING_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_BLOCK_JOB_PENDING, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_block_write_threshold(const char *node_name, uint64_t amount_exceeded, uint64_t write_threshold, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_BLOCK_WRITE_THRESHOLD_arg param = {
        (char *)node_name, amount_exceeded, write_threshold
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("BLOCK_WRITE_THRESHOLD");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "BLOCK_WRITE_THRESHOLD", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_BLOCK_WRITE_THRESHOLD_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_BLOCK_WRITE_THRESHOLD, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_events_block_core_c;
