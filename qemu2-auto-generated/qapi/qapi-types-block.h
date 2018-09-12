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

#ifndef QAPI_TYPES_BLOCK_H
#define QAPI_TYPES_BLOCK_H

#include "qapi/qapi-builtin-types.h"
#include "qapi-types-block-core.h"

typedef enum BiosAtaTranslation {
    BIOS_ATA_TRANSLATION_AUTO = 0,
    BIOS_ATA_TRANSLATION_NONE = 1,
    BIOS_ATA_TRANSLATION_LBA = 2,
    BIOS_ATA_TRANSLATION_LARGE = 3,
    BIOS_ATA_TRANSLATION_RECHS = 4,
    BIOS_ATA_TRANSLATION__MAX = 5,
} BiosAtaTranslation;

#define BiosAtaTranslation_str(val) \
    qapi_enum_lookup(&BiosAtaTranslation_lookup, (val))

extern const QEnumLookup BiosAtaTranslation_lookup;

typedef enum FloppyDriveType {
    FLOPPY_DRIVE_TYPE_144 = 0,
    FLOPPY_DRIVE_TYPE_288 = 1,
    FLOPPY_DRIVE_TYPE_120 = 2,
    FLOPPY_DRIVE_TYPE_NONE = 3,
    FLOPPY_DRIVE_TYPE_AUTO = 4,
    FLOPPY_DRIVE_TYPE__MAX = 5,
} FloppyDriveType;

#define FloppyDriveType_str(val) \
    qapi_enum_lookup(&FloppyDriveType_lookup, (val))

extern const QEnumLookup FloppyDriveType_lookup;

typedef struct BlockdevSnapshotInternal BlockdevSnapshotInternal;

typedef struct q_obj_blockdev_snapshot_delete_internal_sync_arg q_obj_blockdev_snapshot_delete_internal_sync_arg;

typedef struct q_obj_eject_arg q_obj_eject_arg;

typedef struct q_obj_nbd_server_start_arg q_obj_nbd_server_start_arg;

typedef struct q_obj_nbd_server_add_arg q_obj_nbd_server_add_arg;

typedef enum NbdServerRemoveMode {
    NBD_SERVER_REMOVE_MODE_SAFE = 0,
    NBD_SERVER_REMOVE_MODE_HARD = 1,
    NBD_SERVER_REMOVE_MODE__MAX = 2,
} NbdServerRemoveMode;

#define NbdServerRemoveMode_str(val) \
    qapi_enum_lookup(&NbdServerRemoveMode_lookup, (val))

extern const QEnumLookup NbdServerRemoveMode_lookup;

typedef struct q_obj_nbd_server_remove_arg q_obj_nbd_server_remove_arg;

typedef struct q_obj_DEVICE_TRAY_MOVED_arg q_obj_DEVICE_TRAY_MOVED_arg;

typedef enum QuorumOpType {
    QUORUM_OP_TYPE_READ = 0,
    QUORUM_OP_TYPE_WRITE = 1,
    QUORUM_OP_TYPE_FLUSH = 2,
    QUORUM_OP_TYPE__MAX = 3,
} QuorumOpType;

#define QuorumOpType_str(val) \
    qapi_enum_lookup(&QuorumOpType_lookup, (val))

extern const QEnumLookup QuorumOpType_lookup;

typedef struct q_obj_QUORUM_FAILURE_arg q_obj_QUORUM_FAILURE_arg;

typedef struct q_obj_QUORUM_REPORT_BAD_arg q_obj_QUORUM_REPORT_BAD_arg;

struct BlockdevSnapshotInternal {
    char *device;
    char *name;
};

void qapi_free_BlockdevSnapshotInternal(BlockdevSnapshotInternal *obj);

struct q_obj_blockdev_snapshot_delete_internal_sync_arg {
    char *device;
    bool has_id;
    char *id;
    bool has_name;
    char *name;
};

struct q_obj_eject_arg {
    bool has_device;
    char *device;
    bool has_id;
    char *id;
    bool has_force;
    bool force;
};

struct q_obj_nbd_server_start_arg {
    SocketAddressLegacy *addr;
    bool has_tls_creds;
    char *tls_creds;
};

struct q_obj_nbd_server_add_arg {
    char *device;
    bool has_name;
    char *name;
    bool has_writable;
    bool writable;
};

struct q_obj_nbd_server_remove_arg {
    char *name;
    bool has_mode;
    NbdServerRemoveMode mode;
};

struct q_obj_DEVICE_TRAY_MOVED_arg {
    char *device;
    char *id;
    bool tray_open;
};

struct q_obj_QUORUM_FAILURE_arg {
    char *reference;
    int64_t sector_num;
    int64_t sectors_count;
};

struct q_obj_QUORUM_REPORT_BAD_arg {
    QuorumOpType type;
    bool has_error;
    char *error;
    char *node_name;
    int64_t sector_num;
    int64_t sectors_count;
};

#endif /* QAPI_TYPES_BLOCK_H */
