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
#include "qapi-visit-misc.h"
#include "qapi/error.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qobject-output-visitor.h"
#include "qapi/qmp-event.h"


void qapi_event_send_balloon_change(int64_t actual, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_BALLOON_CHANGE_arg param = {
        actual
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("BALLOON_CHANGE");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "BALLOON_CHANGE", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_BALLOON_CHANGE_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_BALLOON_CHANGE, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_device_deleted(bool has_device, const char *device, const char *path, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_DEVICE_DELETED_arg param = {
        has_device, (char *)device, (char *)path
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("DEVICE_DELETED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "DEVICE_DELETED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_DEVICE_DELETED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_DEVICE_DELETED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_dump_completed(DumpQueryResult *result, bool has_error, const char *error, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_DUMP_COMPLETED_arg param = {
        result, has_error, (char *)error
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("DUMP_COMPLETED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "DUMP_COMPLETED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_DUMP_COMPLETED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_DUMP_COMPLETED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_mem_unplug_error(const char *device, const char *msg, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_MEM_UNPLUG_ERROR_arg param = {
        (char *)device, (char *)msg
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("MEM_UNPLUG_ERROR");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "MEM_UNPLUG_ERROR", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_MEM_UNPLUG_ERROR_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_MEM_UNPLUG_ERROR, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_acpi_device_ost(ACPIOSTInfo *info, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_ACPI_DEVICE_OST_arg param = {
        info
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("ACPI_DEVICE_OST");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "ACPI_DEVICE_OST", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_ACPI_DEVICE_OST_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_ACPI_DEVICE_OST, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_rtc_change(int64_t offset, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_RTC_CHANGE_arg param = {
        offset
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("RTC_CHANGE");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "RTC_CHANGE", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_RTC_CHANGE_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_RTC_CHANGE, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

void qapi_event_send_command_dropped(QObject *id, CommandDropReason reason, Error **errp)
{
    QDict *qmp;
    Error *err = NULL;
    QMPEventFuncEmit emit;
    QObject *obj;
    Visitor *v;
    q_obj_COMMAND_DROPPED_arg param = {
        id, reason
    };

    emit = qmp_event_get_func_emit();
    if (!emit) {
        return;
    }

    qmp = qmp_event_build_dict("COMMAND_DROPPED");

    v = qobject_output_visitor_new(&obj);

    visit_start_struct(v, "COMMAND_DROPPED", NULL, 0, &err);
    if (err) {
        goto out;
    }
    visit_type_q_obj_COMMAND_DROPPED_arg_members(v, &param, &err);
    if (!err) {
        visit_check_struct(v, &err);
    }
    visit_end_struct(v, NULL);
    if (err) {
        goto out;
    }

    visit_complete(v, &obj);
    qdict_put_obj(qmp, "data", obj);
    emit(QAPI_EVENT_COMMAND_DROPPED, qmp, &err);

out:
    visit_free(v);
    error_propagate(errp, err);
    QDECREF(qmp);
}

const QEnumLookup QAPIEvent_lookup = {
    .array = (const char *const[]) {
        [QAPI_EVENT_SHUTDOWN] = "SHUTDOWN",
        [QAPI_EVENT_POWERDOWN] = "POWERDOWN",
        [QAPI_EVENT_RESET] = "RESET",
        [QAPI_EVENT_STOP] = "STOP",
        [QAPI_EVENT_RESUME] = "RESUME",
        [QAPI_EVENT_SUSPEND] = "SUSPEND",
        [QAPI_EVENT_SUSPEND_DISK] = "SUSPEND_DISK",
        [QAPI_EVENT_WAKEUP] = "WAKEUP",
        [QAPI_EVENT_WATCHDOG] = "WATCHDOG",
        [QAPI_EVENT_GUEST_PANICKED] = "GUEST_PANICKED",
        [QAPI_EVENT_BLOCK_IMAGE_CORRUPTED] = "BLOCK_IMAGE_CORRUPTED",
        [QAPI_EVENT_BLOCK_IO_ERROR] = "BLOCK_IO_ERROR",
        [QAPI_EVENT_BLOCK_JOB_COMPLETED] = "BLOCK_JOB_COMPLETED",
        [QAPI_EVENT_BLOCK_JOB_CANCELLED] = "BLOCK_JOB_CANCELLED",
        [QAPI_EVENT_BLOCK_JOB_ERROR] = "BLOCK_JOB_ERROR",
        [QAPI_EVENT_BLOCK_JOB_READY] = "BLOCK_JOB_READY",
        [QAPI_EVENT_BLOCK_JOB_PENDING] = "BLOCK_JOB_PENDING",
        [QAPI_EVENT_BLOCK_WRITE_THRESHOLD] = "BLOCK_WRITE_THRESHOLD",
        [QAPI_EVENT_DEVICE_TRAY_MOVED] = "DEVICE_TRAY_MOVED",
        [QAPI_EVENT_QUORUM_FAILURE] = "QUORUM_FAILURE",
        [QAPI_EVENT_QUORUM_REPORT_BAD] = "QUORUM_REPORT_BAD",
        [QAPI_EVENT_VSERPORT_CHANGE] = "VSERPORT_CHANGE",
        [QAPI_EVENT_NIC_RX_FILTER_CHANGED] = "NIC_RX_FILTER_CHANGED",
        [QAPI_EVENT_SPICE_CONNECTED] = "SPICE_CONNECTED",
        [QAPI_EVENT_SPICE_INITIALIZED] = "SPICE_INITIALIZED",
        [QAPI_EVENT_SPICE_DISCONNECTED] = "SPICE_DISCONNECTED",
        [QAPI_EVENT_SPICE_MIGRATE_COMPLETED] = "SPICE_MIGRATE_COMPLETED",
        [QAPI_EVENT_VNC_CONNECTED] = "VNC_CONNECTED",
        [QAPI_EVENT_VNC_INITIALIZED] = "VNC_INITIALIZED",
        [QAPI_EVENT_VNC_DISCONNECTED] = "VNC_DISCONNECTED",
        [QAPI_EVENT_MIGRATION] = "MIGRATION",
        [QAPI_EVENT_MIGRATION_PASS] = "MIGRATION_PASS",
        [QAPI_EVENT_BALLOON_CHANGE] = "BALLOON_CHANGE",
        [QAPI_EVENT_DEVICE_DELETED] = "DEVICE_DELETED",
        [QAPI_EVENT_DUMP_COMPLETED] = "DUMP_COMPLETED",
        [QAPI_EVENT_MEM_UNPLUG_ERROR] = "MEM_UNPLUG_ERROR",
        [QAPI_EVENT_ACPI_DEVICE_OST] = "ACPI_DEVICE_OST",
        [QAPI_EVENT_RTC_CHANGE] = "RTC_CHANGE",
        [QAPI_EVENT_COMMAND_DROPPED] = "COMMAND_DROPPED",
    },
    .size = QAPI_EVENT__MAX
};
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_events_misc_c;
