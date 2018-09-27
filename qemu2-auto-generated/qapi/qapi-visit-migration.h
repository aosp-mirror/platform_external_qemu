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

#ifndef QAPI_VISIT_MIGRATION_H
#define QAPI_VISIT_MIGRATION_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-migration.h"

#include "qapi-visit-common.h"

void visit_type_MigrationStats_members(Visitor *v, MigrationStats *obj, Error **errp);
void visit_type_MigrationStats(Visitor *v, const char *name, MigrationStats **obj, Error **errp);

void visit_type_XBZRLECacheStats_members(Visitor *v, XBZRLECacheStats *obj, Error **errp);
void visit_type_XBZRLECacheStats(Visitor *v, const char *name, XBZRLECacheStats **obj, Error **errp);
void visit_type_MigrationStatus(Visitor *v, const char *name, MigrationStatus *obj, Error **errp);

void visit_type_MigrationInfo_members(Visitor *v, MigrationInfo *obj, Error **errp);
void visit_type_MigrationInfo(Visitor *v, const char *name, MigrationInfo **obj, Error **errp);
void visit_type_MigrationCapability(Visitor *v, const char *name, MigrationCapability *obj, Error **errp);

void visit_type_MigrationCapabilityStatus_members(Visitor *v, MigrationCapabilityStatus *obj, Error **errp);
void visit_type_MigrationCapabilityStatus(Visitor *v, const char *name, MigrationCapabilityStatus **obj, Error **errp);
void visit_type_MigrationCapabilityStatusList(Visitor *v, const char *name, MigrationCapabilityStatusList **obj, Error **errp);

void visit_type_q_obj_migrate_set_capabilities_arg_members(Visitor *v, q_obj_migrate_set_capabilities_arg *obj, Error **errp);
void visit_type_MigrationParameter(Visitor *v, const char *name, MigrationParameter *obj, Error **errp);

void visit_type_MigrateSetParameters_members(Visitor *v, MigrateSetParameters *obj, Error **errp);
void visit_type_MigrateSetParameters(Visitor *v, const char *name, MigrateSetParameters **obj, Error **errp);

void visit_type_MigrationParameters_members(Visitor *v, MigrationParameters *obj, Error **errp);
void visit_type_MigrationParameters(Visitor *v, const char *name, MigrationParameters **obj, Error **errp);

void visit_type_q_obj_client_migrate_info_arg_members(Visitor *v, q_obj_client_migrate_info_arg *obj, Error **errp);

void visit_type_q_obj_MIGRATION_arg_members(Visitor *v, q_obj_MIGRATION_arg *obj, Error **errp);

void visit_type_q_obj_MIGRATION_PASS_arg_members(Visitor *v, q_obj_MIGRATION_PASS_arg *obj, Error **errp);
void visit_type_COLOMessage(Visitor *v, const char *name, COLOMessage *obj, Error **errp);
void visit_type_COLOMode(Visitor *v, const char *name, COLOMode *obj, Error **errp);
void visit_type_FailoverStatus(Visitor *v, const char *name, FailoverStatus *obj, Error **errp);

void visit_type_q_obj_migrate_continue_arg_members(Visitor *v, q_obj_migrate_continue_arg *obj, Error **errp);

void visit_type_q_obj_migrate_set_downtime_arg_members(Visitor *v, q_obj_migrate_set_downtime_arg *obj, Error **errp);

void visit_type_q_obj_migrate_set_speed_arg_members(Visitor *v, q_obj_migrate_set_speed_arg *obj, Error **errp);

void visit_type_q_obj_migrate_set_cache_size_arg_members(Visitor *v, q_obj_migrate_set_cache_size_arg *obj, Error **errp);

void visit_type_q_obj_migrate_arg_members(Visitor *v, q_obj_migrate_arg *obj, Error **errp);

void visit_type_q_obj_migrate_incoming_arg_members(Visitor *v, q_obj_migrate_incoming_arg *obj, Error **errp);

void visit_type_q_obj_xen_save_devices_state_arg_members(Visitor *v, q_obj_xen_save_devices_state_arg *obj, Error **errp);

void visit_type_q_obj_xen_set_replication_arg_members(Visitor *v, q_obj_xen_set_replication_arg *obj, Error **errp);

void visit_type_ReplicationStatus_members(Visitor *v, ReplicationStatus *obj, Error **errp);
void visit_type_ReplicationStatus(Visitor *v, const char *name, ReplicationStatus **obj, Error **errp);

#endif /* QAPI_VISIT_MIGRATION_H */
