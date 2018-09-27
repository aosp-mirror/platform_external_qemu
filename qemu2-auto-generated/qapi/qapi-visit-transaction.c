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

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qapi/error.h"
#include "qapi/qmp/qerror.h"
#include "qapi-visit-transaction.h"

void visit_type_Abort_members(Visitor *v, Abort *obj, Error **errp)
{
    Error *err = NULL;

    error_propagate(errp, err);
}

void visit_type_Abort(Visitor *v, const char *name, Abort **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(Abort), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_Abort_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_Abort(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ActionCompletionMode(Visitor *v, const char *name, ActionCompletionMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &ActionCompletionMode_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_Abort_wrapper_members(Visitor *v, q_obj_Abort_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_Abort(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BlockDirtyBitmapAdd_wrapper_members(Visitor *v, q_obj_BlockDirtyBitmapAdd_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockDirtyBitmapAdd(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BlockDirtyBitmap_wrapper_members(Visitor *v, q_obj_BlockDirtyBitmap_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockDirtyBitmap(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BlockdevBackup_wrapper_members(Visitor *v, q_obj_BlockdevBackup_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevBackup(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BlockdevSnapshot_wrapper_members(Visitor *v, q_obj_BlockdevSnapshot_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevSnapshot(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BlockdevSnapshotInternal_wrapper_members(Visitor *v, q_obj_BlockdevSnapshotInternal_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevSnapshotInternal(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BlockdevSnapshotSync_wrapper_members(Visitor *v, q_obj_BlockdevSnapshotSync_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevSnapshotSync(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_DriveBackup_wrapper_members(Visitor *v, q_obj_DriveBackup_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_DriveBackup(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_TransactionActionKind(Visitor *v, const char *name, TransactionActionKind *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &TransactionActionKind_lookup, errp);
    *obj = value;
}

void visit_type_TransactionAction_members(Visitor *v, TransactionAction *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_TransactionActionKind(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case TRANSACTION_ACTION_KIND_ABORT:
        visit_type_q_obj_Abort_wrapper_members(v, &obj->u.abort, &err);
        break;
    case TRANSACTION_ACTION_KIND_BLOCK_DIRTY_BITMAP_ADD:
        visit_type_q_obj_BlockDirtyBitmapAdd_wrapper_members(v, &obj->u.block_dirty_bitmap_add, &err);
        break;
    case TRANSACTION_ACTION_KIND_BLOCK_DIRTY_BITMAP_CLEAR:
        visit_type_q_obj_BlockDirtyBitmap_wrapper_members(v, &obj->u.block_dirty_bitmap_clear, &err);
        break;
    case TRANSACTION_ACTION_KIND_BLOCKDEV_BACKUP:
        visit_type_q_obj_BlockdevBackup_wrapper_members(v, &obj->u.blockdev_backup, &err);
        break;
    case TRANSACTION_ACTION_KIND_BLOCKDEV_SNAPSHOT:
        visit_type_q_obj_BlockdevSnapshot_wrapper_members(v, &obj->u.blockdev_snapshot, &err);
        break;
    case TRANSACTION_ACTION_KIND_BLOCKDEV_SNAPSHOT_INTERNAL_SYNC:
        visit_type_q_obj_BlockdevSnapshotInternal_wrapper_members(v, &obj->u.blockdev_snapshot_internal_sync, &err);
        break;
    case TRANSACTION_ACTION_KIND_BLOCKDEV_SNAPSHOT_SYNC:
        visit_type_q_obj_BlockdevSnapshotSync_wrapper_members(v, &obj->u.blockdev_snapshot_sync, &err);
        break;
    case TRANSACTION_ACTION_KIND_DRIVE_BACKUP:
        visit_type_q_obj_DriveBackup_wrapper_members(v, &obj->u.drive_backup, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_TransactionAction(Visitor *v, const char *name, TransactionAction **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(TransactionAction), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_TransactionAction_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TransactionAction(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_TransactionProperties_members(Visitor *v, TransactionProperties *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "completion-mode", &obj->has_completion_mode)) {
        visit_type_ActionCompletionMode(v, "completion-mode", &obj->completion_mode, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_TransactionProperties(Visitor *v, const char *name, TransactionProperties **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(TransactionProperties), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_TransactionProperties_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TransactionProperties(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_TransactionActionList(Visitor *v, const char *name, TransactionActionList **obj, Error **errp)
{
    Error *err = NULL;
    TransactionActionList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (TransactionActionList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_TransactionAction(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TransactionActionList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_transaction_arg_members(Visitor *v, q_obj_transaction_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_TransactionActionList(v, "actions", &obj->actions, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "properties", &obj->has_properties)) {
        visit_type_TransactionProperties(v, "properties", &obj->properties, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_transaction_c;
