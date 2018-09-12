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

#ifndef QAPI_VISIT_BLOCK_H
#define QAPI_VISIT_BLOCK_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-block.h"

#include "qapi-visit-block-core.h"
void visit_type_BiosAtaTranslation(Visitor *v, const char *name, BiosAtaTranslation *obj, Error **errp);
void visit_type_FloppyDriveType(Visitor *v, const char *name, FloppyDriveType *obj, Error **errp);

void visit_type_BlockdevSnapshotInternal_members(Visitor *v, BlockdevSnapshotInternal *obj, Error **errp);
void visit_type_BlockdevSnapshotInternal(Visitor *v, const char *name, BlockdevSnapshotInternal **obj, Error **errp);

void visit_type_q_obj_blockdev_snapshot_delete_internal_sync_arg_members(Visitor *v, q_obj_blockdev_snapshot_delete_internal_sync_arg *obj, Error **errp);

void visit_type_q_obj_eject_arg_members(Visitor *v, q_obj_eject_arg *obj, Error **errp);

void visit_type_q_obj_nbd_server_start_arg_members(Visitor *v, q_obj_nbd_server_start_arg *obj, Error **errp);

void visit_type_q_obj_nbd_server_add_arg_members(Visitor *v, q_obj_nbd_server_add_arg *obj, Error **errp);
void visit_type_NbdServerRemoveMode(Visitor *v, const char *name, NbdServerRemoveMode *obj, Error **errp);

void visit_type_q_obj_nbd_server_remove_arg_members(Visitor *v, q_obj_nbd_server_remove_arg *obj, Error **errp);

void visit_type_q_obj_DEVICE_TRAY_MOVED_arg_members(Visitor *v, q_obj_DEVICE_TRAY_MOVED_arg *obj, Error **errp);
void visit_type_QuorumOpType(Visitor *v, const char *name, QuorumOpType *obj, Error **errp);

void visit_type_q_obj_QUORUM_FAILURE_arg_members(Visitor *v, q_obj_QUORUM_FAILURE_arg *obj, Error **errp);

void visit_type_q_obj_QUORUM_REPORT_BAD_arg_members(Visitor *v, q_obj_QUORUM_REPORT_BAD_arg *obj, Error **errp);

#endif /* QAPI_VISIT_BLOCK_H */
