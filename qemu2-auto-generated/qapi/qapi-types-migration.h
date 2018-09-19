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

#ifndef QAPI_TYPES_MIGRATION_H
#define QAPI_TYPES_MIGRATION_H

#include "qapi/qapi-builtin-types.h"
#include "qapi-types-common.h"

typedef struct MigrationStats MigrationStats;

typedef struct XBZRLECacheStats XBZRLECacheStats;

typedef enum MigrationStatus {
    MIGRATION_STATUS_NONE = 0,
    MIGRATION_STATUS_SETUP = 1,
    MIGRATION_STATUS_CANCELLING = 2,
    MIGRATION_STATUS_CANCELLED = 3,
    MIGRATION_STATUS_ACTIVE = 4,
    MIGRATION_STATUS_POSTCOPY_ACTIVE = 5,
    MIGRATION_STATUS_COMPLETED = 6,
    MIGRATION_STATUS_FAILED = 7,
    MIGRATION_STATUS_COLO = 8,
    MIGRATION_STATUS_PRE_SWITCHOVER = 9,
    MIGRATION_STATUS_DEVICE = 10,
    MIGRATION_STATUS__MAX = 11,
} MigrationStatus;

#define MigrationStatus_str(val) \
    qapi_enum_lookup(&MigrationStatus_lookup, (val))

extern const QEnumLookup MigrationStatus_lookup;

typedef struct MigrationInfo MigrationInfo;

typedef enum MigrationCapability {
    MIGRATION_CAPABILITY_XBZRLE = 0,
    MIGRATION_CAPABILITY_RDMA_PIN_ALL = 1,
    MIGRATION_CAPABILITY_AUTO_CONVERGE = 2,
    MIGRATION_CAPABILITY_ZERO_BLOCKS = 3,
    MIGRATION_CAPABILITY_COMPRESS = 4,
    MIGRATION_CAPABILITY_EVENTS = 5,
    MIGRATION_CAPABILITY_POSTCOPY_RAM = 6,
    MIGRATION_CAPABILITY_X_COLO = 7,
    MIGRATION_CAPABILITY_RELEASE_RAM = 8,
    MIGRATION_CAPABILITY_BLOCK = 9,
    MIGRATION_CAPABILITY_RETURN_PATH = 10,
    MIGRATION_CAPABILITY_PAUSE_BEFORE_SWITCHOVER = 11,
    MIGRATION_CAPABILITY_X_MULTIFD = 12,
    MIGRATION_CAPABILITY_DIRTY_BITMAPS = 13,
    MIGRATION_CAPABILITY__MAX = 14,
} MigrationCapability;

#define MigrationCapability_str(val) \
    qapi_enum_lookup(&MigrationCapability_lookup, (val))

extern const QEnumLookup MigrationCapability_lookup;

typedef struct MigrationCapabilityStatus MigrationCapabilityStatus;

typedef struct MigrationCapabilityStatusList MigrationCapabilityStatusList;

typedef struct q_obj_migrate_set_capabilities_arg q_obj_migrate_set_capabilities_arg;

typedef enum MigrationParameter {
    MIGRATION_PARAMETER_COMPRESS_LEVEL = 0,
    MIGRATION_PARAMETER_COMPRESS_THREADS = 1,
    MIGRATION_PARAMETER_DECOMPRESS_THREADS = 2,
    MIGRATION_PARAMETER_CPU_THROTTLE_INITIAL = 3,
    MIGRATION_PARAMETER_CPU_THROTTLE_INCREMENT = 4,
    MIGRATION_PARAMETER_TLS_CREDS = 5,
    MIGRATION_PARAMETER_TLS_HOSTNAME = 6,
    MIGRATION_PARAMETER_MAX_BANDWIDTH = 7,
    MIGRATION_PARAMETER_DOWNTIME_LIMIT = 8,
    MIGRATION_PARAMETER_X_CHECKPOINT_DELAY = 9,
    MIGRATION_PARAMETER_BLOCK_INCREMENTAL = 10,
    MIGRATION_PARAMETER_X_MULTIFD_CHANNELS = 11,
    MIGRATION_PARAMETER_X_MULTIFD_PAGE_COUNT = 12,
    MIGRATION_PARAMETER_XBZRLE_CACHE_SIZE = 13,
    MIGRATION_PARAMETER__MAX = 14,
} MigrationParameter;

#define MigrationParameter_str(val) \
    qapi_enum_lookup(&MigrationParameter_lookup, (val))

extern const QEnumLookup MigrationParameter_lookup;

typedef struct MigrateSetParameters MigrateSetParameters;

typedef struct MigrationParameters MigrationParameters;

typedef struct q_obj_client_migrate_info_arg q_obj_client_migrate_info_arg;

typedef struct q_obj_MIGRATION_arg q_obj_MIGRATION_arg;

typedef struct q_obj_MIGRATION_PASS_arg q_obj_MIGRATION_PASS_arg;

typedef enum COLOMessage {
    COLO_MESSAGE_CHECKPOINT_READY = 0,
    COLO_MESSAGE_CHECKPOINT_REQUEST = 1,
    COLO_MESSAGE_CHECKPOINT_REPLY = 2,
    COLO_MESSAGE_VMSTATE_SEND = 3,
    COLO_MESSAGE_VMSTATE_SIZE = 4,
    COLO_MESSAGE_VMSTATE_RECEIVED = 5,
    COLO_MESSAGE_VMSTATE_LOADED = 6,
    COLO_MESSAGE__MAX = 7,
} COLOMessage;

#define COLOMessage_str(val) \
    qapi_enum_lookup(&COLOMessage_lookup, (val))

extern const QEnumLookup COLOMessage_lookup;

typedef enum COLOMode {
    COLO_MODE_UNKNOWN = 0,
    COLO_MODE_PRIMARY = 1,
    COLO_MODE_SECONDARY = 2,
    COLO_MODE__MAX = 3,
} COLOMode;

#define COLOMode_str(val) \
    qapi_enum_lookup(&COLOMode_lookup, (val))

extern const QEnumLookup COLOMode_lookup;

typedef enum FailoverStatus {
    FAILOVER_STATUS_NONE = 0,
    FAILOVER_STATUS_REQUIRE = 1,
    FAILOVER_STATUS_ACTIVE = 2,
    FAILOVER_STATUS_COMPLETED = 3,
    FAILOVER_STATUS_RELAUNCH = 4,
    FAILOVER_STATUS__MAX = 5,
} FailoverStatus;

#define FailoverStatus_str(val) \
    qapi_enum_lookup(&FailoverStatus_lookup, (val))

extern const QEnumLookup FailoverStatus_lookup;

typedef struct q_obj_migrate_continue_arg q_obj_migrate_continue_arg;

typedef struct q_obj_migrate_set_downtime_arg q_obj_migrate_set_downtime_arg;

typedef struct q_obj_migrate_set_speed_arg q_obj_migrate_set_speed_arg;

typedef struct q_obj_migrate_set_cache_size_arg q_obj_migrate_set_cache_size_arg;

typedef struct q_obj_migrate_arg q_obj_migrate_arg;

typedef struct q_obj_migrate_incoming_arg q_obj_migrate_incoming_arg;

typedef struct q_obj_xen_save_devices_state_arg q_obj_xen_save_devices_state_arg;

typedef struct q_obj_xen_set_replication_arg q_obj_xen_set_replication_arg;

typedef struct ReplicationStatus ReplicationStatus;

struct MigrationStats {
    int64_t transferred;
    int64_t remaining;
    int64_t total;
    int64_t duplicate;
    int64_t skipped;
    int64_t normal;
    int64_t normal_bytes;
    int64_t dirty_pages_rate;
    double mbps;
    int64_t dirty_sync_count;
    int64_t postcopy_requests;
    int64_t page_size;
};

void qapi_free_MigrationStats(MigrationStats *obj);

struct XBZRLECacheStats {
    int64_t cache_size;
    int64_t bytes;
    int64_t pages;
    int64_t cache_miss;
    double cache_miss_rate;
    int64_t overflow;
};

void qapi_free_XBZRLECacheStats(XBZRLECacheStats *obj);

struct MigrationInfo {
    bool has_status;
    MigrationStatus status;
    bool has_ram;
    MigrationStats *ram;
    bool has_disk;
    MigrationStats *disk;
    bool has_xbzrle_cache;
    XBZRLECacheStats *xbzrle_cache;
    bool has_total_time;
    int64_t total_time;
    bool has_expected_downtime;
    int64_t expected_downtime;
    bool has_downtime;
    int64_t downtime;
    bool has_setup_time;
    int64_t setup_time;
    bool has_cpu_throttle_percentage;
    int64_t cpu_throttle_percentage;
    bool has_error_desc;
    char *error_desc;
};

void qapi_free_MigrationInfo(MigrationInfo *obj);

struct MigrationCapabilityStatus {
    MigrationCapability capability;
    bool state;
};

void qapi_free_MigrationCapabilityStatus(MigrationCapabilityStatus *obj);

struct MigrationCapabilityStatusList {
    MigrationCapabilityStatusList *next;
    MigrationCapabilityStatus *value;
};

void qapi_free_MigrationCapabilityStatusList(MigrationCapabilityStatusList *obj);

struct q_obj_migrate_set_capabilities_arg {
    MigrationCapabilityStatusList *capabilities;
};

struct MigrateSetParameters {
    bool has_compress_level;
    int64_t compress_level;
    bool has_compress_threads;
    int64_t compress_threads;
    bool has_decompress_threads;
    int64_t decompress_threads;
    bool has_cpu_throttle_initial;
    int64_t cpu_throttle_initial;
    bool has_cpu_throttle_increment;
    int64_t cpu_throttle_increment;
    bool has_tls_creds;
    StrOrNull *tls_creds;
    bool has_tls_hostname;
    StrOrNull *tls_hostname;
    bool has_max_bandwidth;
    int64_t max_bandwidth;
    bool has_downtime_limit;
    int64_t downtime_limit;
    bool has_x_checkpoint_delay;
    int64_t x_checkpoint_delay;
    bool has_block_incremental;
    bool block_incremental;
    bool has_x_multifd_channels;
    int64_t x_multifd_channels;
    bool has_x_multifd_page_count;
    int64_t x_multifd_page_count;
    bool has_xbzrle_cache_size;
    uint64_t xbzrle_cache_size;
};

void qapi_free_MigrateSetParameters(MigrateSetParameters *obj);

struct MigrationParameters {
    bool has_compress_level;
    uint8_t compress_level;
    bool has_compress_threads;
    uint8_t compress_threads;
    bool has_decompress_threads;
    uint8_t decompress_threads;
    bool has_cpu_throttle_initial;
    uint8_t cpu_throttle_initial;
    bool has_cpu_throttle_increment;
    uint8_t cpu_throttle_increment;
    bool has_tls_creds;
    char *tls_creds;
    bool has_tls_hostname;
    char *tls_hostname;
    bool has_max_bandwidth;
    uint64_t max_bandwidth;
    bool has_downtime_limit;
    uint64_t downtime_limit;
    bool has_x_checkpoint_delay;
    uint32_t x_checkpoint_delay;
    bool has_block_incremental;
    bool block_incremental;
    bool has_x_multifd_channels;
    uint8_t x_multifd_channels;
    bool has_x_multifd_page_count;
    uint32_t x_multifd_page_count;
    bool has_xbzrle_cache_size;
    uint64_t xbzrle_cache_size;
};

void qapi_free_MigrationParameters(MigrationParameters *obj);

struct q_obj_client_migrate_info_arg {
    char *protocol;
    char *hostname;
    bool has_port;
    int64_t port;
    bool has_tls_port;
    int64_t tls_port;
    bool has_cert_subject;
    char *cert_subject;
};

struct q_obj_MIGRATION_arg {
    MigrationStatus status;
};

struct q_obj_MIGRATION_PASS_arg {
    int64_t pass;
};

struct q_obj_migrate_continue_arg {
    MigrationStatus state;
};

struct q_obj_migrate_set_downtime_arg {
    double value;
};

struct q_obj_migrate_set_speed_arg {
    int64_t value;
};

struct q_obj_migrate_set_cache_size_arg {
    int64_t value;
};

struct q_obj_migrate_arg {
    char *uri;
    bool has_blk;
    bool blk;
    bool has_inc;
    bool inc;
    bool has_detach;
    bool detach;
};

struct q_obj_migrate_incoming_arg {
    char *uri;
};

struct q_obj_xen_save_devices_state_arg {
    char *filename;
    bool has_live;
    bool live;
};

struct q_obj_xen_set_replication_arg {
    bool enable;
    bool primary;
    bool has_failover;
    bool failover;
};

struct ReplicationStatus {
    bool error;
    bool has_desc;
    char *desc;
};

void qapi_free_ReplicationStatus(ReplicationStatus *obj);

#endif /* QAPI_TYPES_MIGRATION_H */
