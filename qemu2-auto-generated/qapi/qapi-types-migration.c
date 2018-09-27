/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI types
 *
 * Copyright IBM, Corp. 2011
 * Copyright (c) 2013-2018 Red Hat Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qapi/dealloc-visitor.h"
#include "qapi-types-migration.h"
#include "qapi-visit-migration.h"

void qapi_free_MigrationStats(MigrationStats *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MigrationStats(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_XBZRLECacheStats(XBZRLECacheStats *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_XBZRLECacheStats(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup MigrationStatus_lookup = {
    .array = (const char *const[]) {
        [MIGRATION_STATUS_NONE] = "none",
        [MIGRATION_STATUS_SETUP] = "setup",
        [MIGRATION_STATUS_CANCELLING] = "cancelling",
        [MIGRATION_STATUS_CANCELLED] = "cancelled",
        [MIGRATION_STATUS_ACTIVE] = "active",
        [MIGRATION_STATUS_POSTCOPY_ACTIVE] = "postcopy-active",
        [MIGRATION_STATUS_COMPLETED] = "completed",
        [MIGRATION_STATUS_FAILED] = "failed",
        [MIGRATION_STATUS_COLO] = "colo",
        [MIGRATION_STATUS_PRE_SWITCHOVER] = "pre-switchover",
        [MIGRATION_STATUS_DEVICE] = "device",
    },
    .size = MIGRATION_STATUS__MAX
};

void qapi_free_MigrationInfo(MigrationInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MigrationInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup MigrationCapability_lookup = {
    .array = (const char *const[]) {
        [MIGRATION_CAPABILITY_XBZRLE] = "xbzrle",
        [MIGRATION_CAPABILITY_RDMA_PIN_ALL] = "rdma-pin-all",
        [MIGRATION_CAPABILITY_AUTO_CONVERGE] = "auto-converge",
        [MIGRATION_CAPABILITY_ZERO_BLOCKS] = "zero-blocks",
        [MIGRATION_CAPABILITY_COMPRESS] = "compress",
        [MIGRATION_CAPABILITY_EVENTS] = "events",
        [MIGRATION_CAPABILITY_POSTCOPY_RAM] = "postcopy-ram",
        [MIGRATION_CAPABILITY_X_COLO] = "x-colo",
        [MIGRATION_CAPABILITY_RELEASE_RAM] = "release-ram",
        [MIGRATION_CAPABILITY_BLOCK] = "block",
        [MIGRATION_CAPABILITY_RETURN_PATH] = "return-path",
        [MIGRATION_CAPABILITY_PAUSE_BEFORE_SWITCHOVER] = "pause-before-switchover",
        [MIGRATION_CAPABILITY_X_MULTIFD] = "x-multifd",
        [MIGRATION_CAPABILITY_DIRTY_BITMAPS] = "dirty-bitmaps",
    },
    .size = MIGRATION_CAPABILITY__MAX
};

void qapi_free_MigrationCapabilityStatus(MigrationCapabilityStatus *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MigrationCapabilityStatus(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_MigrationCapabilityStatusList(MigrationCapabilityStatusList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MigrationCapabilityStatusList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup MigrationParameter_lookup = {
    .array = (const char *const[]) {
        [MIGRATION_PARAMETER_COMPRESS_LEVEL] = "compress-level",
        [MIGRATION_PARAMETER_COMPRESS_THREADS] = "compress-threads",
        [MIGRATION_PARAMETER_DECOMPRESS_THREADS] = "decompress-threads",
        [MIGRATION_PARAMETER_CPU_THROTTLE_INITIAL] = "cpu-throttle-initial",
        [MIGRATION_PARAMETER_CPU_THROTTLE_INCREMENT] = "cpu-throttle-increment",
        [MIGRATION_PARAMETER_TLS_CREDS] = "tls-creds",
        [MIGRATION_PARAMETER_TLS_HOSTNAME] = "tls-hostname",
        [MIGRATION_PARAMETER_MAX_BANDWIDTH] = "max-bandwidth",
        [MIGRATION_PARAMETER_DOWNTIME_LIMIT] = "downtime-limit",
        [MIGRATION_PARAMETER_X_CHECKPOINT_DELAY] = "x-checkpoint-delay",
        [MIGRATION_PARAMETER_BLOCK_INCREMENTAL] = "block-incremental",
        [MIGRATION_PARAMETER_X_MULTIFD_CHANNELS] = "x-multifd-channels",
        [MIGRATION_PARAMETER_X_MULTIFD_PAGE_COUNT] = "x-multifd-page-count",
        [MIGRATION_PARAMETER_XBZRLE_CACHE_SIZE] = "xbzrle-cache-size",
    },
    .size = MIGRATION_PARAMETER__MAX
};

void qapi_free_MigrateSetParameters(MigrateSetParameters *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MigrateSetParameters(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_MigrationParameters(MigrationParameters *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MigrationParameters(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup COLOMessage_lookup = {
    .array = (const char *const[]) {
        [COLO_MESSAGE_CHECKPOINT_READY] = "checkpoint-ready",
        [COLO_MESSAGE_CHECKPOINT_REQUEST] = "checkpoint-request",
        [COLO_MESSAGE_CHECKPOINT_REPLY] = "checkpoint-reply",
        [COLO_MESSAGE_VMSTATE_SEND] = "vmstate-send",
        [COLO_MESSAGE_VMSTATE_SIZE] = "vmstate-size",
        [COLO_MESSAGE_VMSTATE_RECEIVED] = "vmstate-received",
        [COLO_MESSAGE_VMSTATE_LOADED] = "vmstate-loaded",
    },
    .size = COLO_MESSAGE__MAX
};

const QEnumLookup COLOMode_lookup = {
    .array = (const char *const[]) {
        [COLO_MODE_UNKNOWN] = "unknown",
        [COLO_MODE_PRIMARY] = "primary",
        [COLO_MODE_SECONDARY] = "secondary",
    },
    .size = COLO_MODE__MAX
};

const QEnumLookup FailoverStatus_lookup = {
    .array = (const char *const[]) {
        [FAILOVER_STATUS_NONE] = "none",
        [FAILOVER_STATUS_REQUIRE] = "require",
        [FAILOVER_STATUS_ACTIVE] = "active",
        [FAILOVER_STATUS_COMPLETED] = "completed",
        [FAILOVER_STATUS_RELAUNCH] = "relaunch",
    },
    .size = FAILOVER_STATUS__MAX
};

void qapi_free_ReplicationStatus(ReplicationStatus *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ReplicationStatus(v, NULL, &obj, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_migration_c;
