/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI/QMP commands
 *
 * Copyright IBM, Corp. 2011
 * Copyright (C) 2014-2018 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#ifndef QAPI_COMMANDS_BLOCK_H
#define QAPI_COMMANDS_BLOCK_H

#include "qapi-commands-block-core.h"
#include "qapi-types-block.h"
#include "qapi/qmp/dispatch.h"

void qmp_blockdev_snapshot_internal_sync(const char *device, const char *name, Error **errp);
void qmp_marshal_blockdev_snapshot_internal_sync(QDict *args, QObject **ret, Error **errp);
SnapshotInfo *qmp_blockdev_snapshot_delete_internal_sync(const char *device, bool has_id, const char *id, bool has_name, const char *name, Error **errp);
void qmp_marshal_blockdev_snapshot_delete_internal_sync(QDict *args, QObject **ret, Error **errp);
void qmp_eject(bool has_device, const char *device, bool has_id, const char *id, bool has_force, bool force, Error **errp);
void qmp_marshal_eject(QDict *args, QObject **ret, Error **errp);
void qmp_nbd_server_start(SocketAddressLegacy *addr, bool has_tls_creds, const char *tls_creds, Error **errp);
void qmp_marshal_nbd_server_start(QDict *args, QObject **ret, Error **errp);
void qmp_nbd_server_add(const char *device, bool has_name, const char *name, bool has_writable, bool writable, Error **errp);
void qmp_marshal_nbd_server_add(QDict *args, QObject **ret, Error **errp);
void qmp_nbd_server_remove(const char *name, bool has_mode, NbdServerRemoveMode mode, Error **errp);
void qmp_marshal_nbd_server_remove(QDict *args, QObject **ret, Error **errp);
void qmp_nbd_server_stop(Error **errp);
void qmp_marshal_nbd_server_stop(QDict *args, QObject **ret, Error **errp);

#endif /* QAPI_COMMANDS_BLOCK_H */
