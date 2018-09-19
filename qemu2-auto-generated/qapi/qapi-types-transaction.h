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

#ifndef QAPI_TYPES_TRANSACTION_H
#define QAPI_TYPES_TRANSACTION_H

#include "qapi/qapi-builtin-types.h"
#include "qapi-types-block.h"

typedef struct Abort Abort;

typedef enum ActionCompletionMode {
    ACTION_COMPLETION_MODE_INDIVIDUAL = 0,
    ACTION_COMPLETION_MODE_GROUPED = 1,
    ACTION_COMPLETION_MODE__MAX = 2,
} ActionCompletionMode;

#define ActionCompletionMode_str(val) \
    qapi_enum_lookup(&ActionCompletionMode_lookup, (val))

extern const QEnumLookup ActionCompletionMode_lookup;

typedef struct q_obj_Abort_wrapper q_obj_Abort_wrapper;

typedef struct q_obj_BlockDirtyBitmapAdd_wrapper q_obj_BlockDirtyBitmapAdd_wrapper;

typedef struct q_obj_BlockDirtyBitmap_wrapper q_obj_BlockDirtyBitmap_wrapper;

typedef struct q_obj_BlockdevBackup_wrapper q_obj_BlockdevBackup_wrapper;

typedef struct q_obj_BlockdevSnapshot_wrapper q_obj_BlockdevSnapshot_wrapper;

typedef struct q_obj_BlockdevSnapshotInternal_wrapper q_obj_BlockdevSnapshotInternal_wrapper;

typedef struct q_obj_BlockdevSnapshotSync_wrapper q_obj_BlockdevSnapshotSync_wrapper;

typedef struct q_obj_DriveBackup_wrapper q_obj_DriveBackup_wrapper;

typedef enum TransactionActionKind {
    TRANSACTION_ACTION_KIND_ABORT = 0,
    TRANSACTION_ACTION_KIND_BLOCK_DIRTY_BITMAP_ADD = 1,
    TRANSACTION_ACTION_KIND_BLOCK_DIRTY_BITMAP_CLEAR = 2,
    TRANSACTION_ACTION_KIND_BLOCKDEV_BACKUP = 3,
    TRANSACTION_ACTION_KIND_BLOCKDEV_SNAPSHOT = 4,
    TRANSACTION_ACTION_KIND_BLOCKDEV_SNAPSHOT_INTERNAL_SYNC = 5,
    TRANSACTION_ACTION_KIND_BLOCKDEV_SNAPSHOT_SYNC = 6,
    TRANSACTION_ACTION_KIND_DRIVE_BACKUP = 7,
    TRANSACTION_ACTION_KIND__MAX = 8,
} TransactionActionKind;

#define TransactionActionKind_str(val) \
    qapi_enum_lookup(&TransactionActionKind_lookup, (val))

extern const QEnumLookup TransactionActionKind_lookup;

typedef struct TransactionAction TransactionAction;

typedef struct TransactionProperties TransactionProperties;

typedef struct TransactionActionList TransactionActionList;

typedef struct q_obj_transaction_arg q_obj_transaction_arg;

struct Abort {
    char qapi_dummy_for_empty_struct;
};

void qapi_free_Abort(Abort *obj);

struct q_obj_Abort_wrapper {
    Abort *data;
};

struct q_obj_BlockDirtyBitmapAdd_wrapper {
    BlockDirtyBitmapAdd *data;
};

struct q_obj_BlockDirtyBitmap_wrapper {
    BlockDirtyBitmap *data;
};

struct q_obj_BlockdevBackup_wrapper {
    BlockdevBackup *data;
};

struct q_obj_BlockdevSnapshot_wrapper {
    BlockdevSnapshot *data;
};

struct q_obj_BlockdevSnapshotInternal_wrapper {
    BlockdevSnapshotInternal *data;
};

struct q_obj_BlockdevSnapshotSync_wrapper {
    BlockdevSnapshotSync *data;
};

struct q_obj_DriveBackup_wrapper {
    DriveBackup *data;
};

struct TransactionAction {
    TransactionActionKind type;
    union { /* union tag is @type */
        q_obj_Abort_wrapper abort;
        q_obj_BlockDirtyBitmapAdd_wrapper block_dirty_bitmap_add;
        q_obj_BlockDirtyBitmap_wrapper block_dirty_bitmap_clear;
        q_obj_BlockdevBackup_wrapper blockdev_backup;
        q_obj_BlockdevSnapshot_wrapper blockdev_snapshot;
        q_obj_BlockdevSnapshotInternal_wrapper blockdev_snapshot_internal_sync;
        q_obj_BlockdevSnapshotSync_wrapper blockdev_snapshot_sync;
        q_obj_DriveBackup_wrapper drive_backup;
    } u;
};

void qapi_free_TransactionAction(TransactionAction *obj);

struct TransactionProperties {
    bool has_completion_mode;
    ActionCompletionMode completion_mode;
};

void qapi_free_TransactionProperties(TransactionProperties *obj);

struct TransactionActionList {
    TransactionActionList *next;
    TransactionAction *value;
};

void qapi_free_TransactionActionList(TransactionActionList *obj);

struct q_obj_transaction_arg {
    TransactionActionList *actions;
    bool has_properties;
    TransactionProperties *properties;
};

#endif /* QAPI_TYPES_TRANSACTION_H */
