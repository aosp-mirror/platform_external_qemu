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

#ifndef QAPI_COMMANDS_MIGRATION_H
#define QAPI_COMMANDS_MIGRATION_H

#include "qapi-commands-common.h"
#include "qapi-types-migration.h"
#include "qapi/qmp/dispatch.h"

MigrationInfo *qmp_query_migrate(Error **errp);
void qmp_marshal_query_migrate(QDict *args, QObject **ret, Error **errp);
void qmp_migrate_set_capabilities(MigrationCapabilityStatusList *capabilities, Error **errp);
void qmp_marshal_migrate_set_capabilities(QDict *args, QObject **ret, Error **errp);
MigrationCapabilityStatusList *qmp_query_migrate_capabilities(Error **errp);
void qmp_marshal_query_migrate_capabilities(QDict *args, QObject **ret, Error **errp);
void qmp_migrate_set_parameters(MigrateSetParameters *arg, Error **errp);
void qmp_marshal_migrate_set_parameters(QDict *args, QObject **ret, Error **errp);
MigrationParameters *qmp_query_migrate_parameters(Error **errp);
void qmp_marshal_query_migrate_parameters(QDict *args, QObject **ret, Error **errp);
void qmp_client_migrate_info(const char *protocol, const char *hostname, bool has_port, int64_t port, bool has_tls_port, int64_t tls_port, bool has_cert_subject, const char *cert_subject, Error **errp);
void qmp_marshal_client_migrate_info(QDict *args, QObject **ret, Error **errp);
void qmp_migrate_start_postcopy(Error **errp);
void qmp_marshal_migrate_start_postcopy(QDict *args, QObject **ret, Error **errp);
void qmp_x_colo_lost_heartbeat(Error **errp);
void qmp_marshal_x_colo_lost_heartbeat(QDict *args, QObject **ret, Error **errp);
void qmp_migrate_cancel(Error **errp);
void qmp_marshal_migrate_cancel(QDict *args, QObject **ret, Error **errp);
void qmp_migrate_continue(MigrationStatus state, Error **errp);
void qmp_marshal_migrate_continue(QDict *args, QObject **ret, Error **errp);
void qmp_migrate_set_downtime(double value, Error **errp);
void qmp_marshal_migrate_set_downtime(QDict *args, QObject **ret, Error **errp);
void qmp_migrate_set_speed(int64_t value, Error **errp);
void qmp_marshal_migrate_set_speed(QDict *args, QObject **ret, Error **errp);
void qmp_migrate_set_cache_size(int64_t value, Error **errp);
void qmp_marshal_migrate_set_cache_size(QDict *args, QObject **ret, Error **errp);
int64_t qmp_query_migrate_cache_size(Error **errp);
void qmp_marshal_query_migrate_cache_size(QDict *args, QObject **ret, Error **errp);
void qmp_migrate(const char *uri, bool has_blk, bool blk, bool has_inc, bool inc, bool has_detach, bool detach, Error **errp);
void qmp_marshal_migrate(QDict *args, QObject **ret, Error **errp);
void qmp_migrate_incoming(const char *uri, Error **errp);
void qmp_marshal_migrate_incoming(QDict *args, QObject **ret, Error **errp);
void qmp_xen_save_devices_state(const char *filename, bool has_live, bool live, Error **errp);
void qmp_marshal_xen_save_devices_state(QDict *args, QObject **ret, Error **errp);
void qmp_xen_set_replication(bool enable, bool primary, bool has_failover, bool failover, Error **errp);
void qmp_marshal_xen_set_replication(QDict *args, QObject **ret, Error **errp);
ReplicationStatus *qmp_query_xen_replication_status(Error **errp);
void qmp_marshal_query_xen_replication_status(QDict *args, QObject **ret, Error **errp);
void qmp_xen_colo_do_checkpoint(Error **errp);
void qmp_marshal_xen_colo_do_checkpoint(QDict *args, QObject **ret, Error **errp);

#endif /* QAPI_COMMANDS_MIGRATION_H */
