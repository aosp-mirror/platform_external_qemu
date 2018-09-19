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

#ifndef QAPI_COMMANDS_BLOCK_CORE_H
#define QAPI_COMMANDS_BLOCK_CORE_H

#include "qapi-commands-common.h"
#include "qapi-commands-crypto.h"
#include "qapi-commands-sockets.h"
#include "qapi-types-block-core.h"
#include "qapi/qmp/dispatch.h"

void qmp_x_block_latency_histogram_set(const char *device, bool has_boundaries, uint64List *boundaries, bool has_boundaries_read, uint64List *boundaries_read, bool has_boundaries_write, uint64List *boundaries_write, bool has_boundaries_flush, uint64List *boundaries_flush, Error **errp);
void qmp_marshal_x_block_latency_histogram_set(QDict *args, QObject **ret, Error **errp);
BlockInfoList *qmp_query_block(Error **errp);
void qmp_marshal_query_block(QDict *args, QObject **ret, Error **errp);
BlockStatsList *qmp_query_blockstats(bool has_query_nodes, bool query_nodes, Error **errp);
void qmp_marshal_query_blockstats(QDict *args, QObject **ret, Error **errp);
BlockJobInfoList *qmp_query_block_jobs(Error **errp);
void qmp_marshal_query_block_jobs(QDict *args, QObject **ret, Error **errp);
void qmp_block_passwd(bool has_device, const char *device, bool has_node_name, const char *node_name, const char *password, Error **errp);
void qmp_marshal_block_passwd(QDict *args, QObject **ret, Error **errp);
void qmp_block_resize(bool has_device, const char *device, bool has_node_name, const char *node_name, int64_t size, Error **errp);
void qmp_marshal_block_resize(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_snapshot_sync(bool has_device, const char *device, bool has_node_name, const char *node_name, const char *snapshot_file, bool has_snapshot_node_name, const char *snapshot_node_name, bool has_format, const char *format, bool has_mode, NewImageMode mode, Error **errp);
void qmp_marshal_blockdev_snapshot_sync(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_snapshot(const char *node, const char *overlay, Error **errp);
void qmp_marshal_blockdev_snapshot(QDict *args, QObject **ret, Error **errp);
void qmp_change_backing_file(const char *device, const char *image_node_name, const char *backing_file, Error **errp);
void qmp_marshal_change_backing_file(QDict *args, QObject **ret, Error **errp);
void qmp_block_commit(bool has_job_id, const char *job_id, const char *device, bool has_base, const char *base, bool has_top, const char *top, bool has_backing_file, const char *backing_file, bool has_speed, int64_t speed, bool has_filter_node_name, const char *filter_node_name, Error **errp);
void qmp_marshal_block_commit(QDict *args, QObject **ret, Error **errp);
void qmp_drive_backup(DriveBackup *arg, Error **errp);
void qmp_marshal_drive_backup(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_backup(BlockdevBackup *arg, Error **errp);
void qmp_marshal_blockdev_backup(QDict *args, QObject **ret, Error **errp);
BlockDeviceInfoList *qmp_query_named_block_nodes(Error **errp);
void qmp_marshal_query_named_block_nodes(QDict *args, QObject **ret, Error **errp);
void qmp_drive_mirror(DriveMirror *arg, Error **errp);
void qmp_marshal_drive_mirror(QDict *args, QObject **ret, Error **errp);
void qmp_block_dirty_bitmap_add(const char *node, const char *name, bool has_granularity, uint32_t granularity, bool has_persistent, bool persistent, bool has_autoload, bool autoload, Error **errp);
void qmp_marshal_block_dirty_bitmap_add(QDict *args, QObject **ret, Error **errp);
void qmp_block_dirty_bitmap_remove(const char *node, const char *name, Error **errp);
void qmp_marshal_block_dirty_bitmap_remove(QDict *args, QObject **ret, Error **errp);
void qmp_block_dirty_bitmap_clear(const char *node, const char *name, Error **errp);
void qmp_marshal_block_dirty_bitmap_clear(QDict *args, QObject **ret, Error **errp);
BlockDirtyBitmapSha256 *qmp_x_debug_block_dirty_bitmap_sha256(const char *node, const char *name, Error **errp);
void qmp_marshal_x_debug_block_dirty_bitmap_sha256(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_mirror(bool has_job_id, const char *job_id, const char *device, const char *target, bool has_replaces, const char *replaces, MirrorSyncMode sync, bool has_speed, int64_t speed, bool has_granularity, uint32_t granularity, bool has_buf_size, int64_t buf_size, bool has_on_source_error, BlockdevOnError on_source_error, bool has_on_target_error, BlockdevOnError on_target_error, bool has_filter_node_name, const char *filter_node_name, Error **errp);
void qmp_marshal_blockdev_mirror(QDict *args, QObject **ret, Error **errp);
void qmp_block_set_io_throttle(BlockIOThrottle *arg, Error **errp);
void qmp_marshal_block_set_io_throttle(QDict *args, QObject **ret, Error **errp);
void qmp_block_stream(bool has_job_id, const char *job_id, const char *device, bool has_base, const char *base, bool has_base_node, const char *base_node, bool has_backing_file, const char *backing_file, bool has_speed, int64_t speed, bool has_on_error, BlockdevOnError on_error, Error **errp);
void qmp_marshal_block_stream(QDict *args, QObject **ret, Error **errp);
void qmp_block_job_set_speed(const char *device, int64_t speed, Error **errp);
void qmp_marshal_block_job_set_speed(QDict *args, QObject **ret, Error **errp);
void qmp_block_job_cancel(const char *device, bool has_force, bool force, Error **errp);
void qmp_marshal_block_job_cancel(QDict *args, QObject **ret, Error **errp);
void qmp_block_job_pause(const char *device, Error **errp);
void qmp_marshal_block_job_pause(QDict *args, QObject **ret, Error **errp);
void qmp_block_job_resume(const char *device, Error **errp);
void qmp_marshal_block_job_resume(QDict *args, QObject **ret, Error **errp);
void qmp_block_job_complete(const char *device, Error **errp);
void qmp_marshal_block_job_complete(QDict *args, QObject **ret, Error **errp);
void qmp_block_job_dismiss(const char *id, Error **errp);
void qmp_marshal_block_job_dismiss(QDict *args, QObject **ret, Error **errp);
void qmp_block_job_finalize(const char *id, Error **errp);
void qmp_marshal_block_job_finalize(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_add(BlockdevOptions *arg, Error **errp);
void qmp_marshal_blockdev_add(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_del(const char *node_name, Error **errp);
void qmp_marshal_blockdev_del(QDict *args, QObject **ret, Error **errp);
void qmp_x_blockdev_create(BlockdevCreateOptions *arg, Error **errp);
void qmp_marshal_x_blockdev_create(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_open_tray(bool has_device, const char *device, bool has_id, const char *id, bool has_force, bool force, Error **errp);
void qmp_marshal_blockdev_open_tray(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_close_tray(bool has_device, const char *device, bool has_id, const char *id, Error **errp);
void qmp_marshal_blockdev_close_tray(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_remove_medium(const char *id, Error **errp);
void qmp_marshal_blockdev_remove_medium(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_insert_medium(const char *id, const char *node_name, Error **errp);
void qmp_marshal_blockdev_insert_medium(QDict *args, QObject **ret, Error **errp);
void qmp_blockdev_change_medium(bool has_device, const char *device, bool has_id, const char *id, const char *filename, bool has_format, const char *format, bool has_read_only_mode, BlockdevChangeReadOnlyMode read_only_mode, Error **errp);
void qmp_marshal_blockdev_change_medium(QDict *args, QObject **ret, Error **errp);
void qmp_block_set_write_threshold(const char *node_name, uint64_t write_threshold, Error **errp);
void qmp_marshal_block_set_write_threshold(QDict *args, QObject **ret, Error **errp);
void qmp_x_blockdev_change(const char *parent, bool has_child, const char *child, bool has_node, const char *node, Error **errp);
void qmp_marshal_x_blockdev_change(QDict *args, QObject **ret, Error **errp);
void qmp_x_blockdev_set_iothread(const char *node_name, StrOrNull *iothread, bool has_force, bool force, Error **errp);
void qmp_marshal_x_blockdev_set_iothread(QDict *args, QObject **ret, Error **errp);

#endif /* QAPI_COMMANDS_BLOCK_CORE_H */
