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

#ifndef QAPI_VISIT_TRANSACTION_H
#define QAPI_VISIT_TRANSACTION_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-transaction.h"

#include "qapi-visit-block.h"

void visit_type_Abort_members(Visitor *v, Abort *obj, Error **errp);
void visit_type_Abort(Visitor *v, const char *name, Abort **obj, Error **errp);
void visit_type_ActionCompletionMode(Visitor *v, const char *name, ActionCompletionMode *obj, Error **errp);

void visit_type_q_obj_Abort_wrapper_members(Visitor *v, q_obj_Abort_wrapper *obj, Error **errp);

void visit_type_q_obj_BlockDirtyBitmapAdd_wrapper_members(Visitor *v, q_obj_BlockDirtyBitmapAdd_wrapper *obj, Error **errp);

void visit_type_q_obj_BlockDirtyBitmap_wrapper_members(Visitor *v, q_obj_BlockDirtyBitmap_wrapper *obj, Error **errp);

void visit_type_q_obj_BlockdevBackup_wrapper_members(Visitor *v, q_obj_BlockdevBackup_wrapper *obj, Error **errp);

void visit_type_q_obj_BlockdevSnapshot_wrapper_members(Visitor *v, q_obj_BlockdevSnapshot_wrapper *obj, Error **errp);

void visit_type_q_obj_BlockdevSnapshotInternal_wrapper_members(Visitor *v, q_obj_BlockdevSnapshotInternal_wrapper *obj, Error **errp);

void visit_type_q_obj_BlockdevSnapshotSync_wrapper_members(Visitor *v, q_obj_BlockdevSnapshotSync_wrapper *obj, Error **errp);

void visit_type_q_obj_DriveBackup_wrapper_members(Visitor *v, q_obj_DriveBackup_wrapper *obj, Error **errp);
void visit_type_TransactionActionKind(Visitor *v, const char *name, TransactionActionKind *obj, Error **errp);

void visit_type_TransactionAction_members(Visitor *v, TransactionAction *obj, Error **errp);
void visit_type_TransactionAction(Visitor *v, const char *name, TransactionAction **obj, Error **errp);

void visit_type_TransactionProperties_members(Visitor *v, TransactionProperties *obj, Error **errp);
void visit_type_TransactionProperties(Visitor *v, const char *name, TransactionProperties **obj, Error **errp);
void visit_type_TransactionActionList(Visitor *v, const char *name, TransactionActionList **obj, Error **errp);

void visit_type_q_obj_transaction_arg_members(Visitor *v, q_obj_transaction_arg *obj, Error **errp);

#endif /* QAPI_VISIT_TRANSACTION_H */
