/*
 * Block driver for joining multiple images.
 *
 * Copyright 2017 Google Inc.
 *
 * Authors:
 *  Tao Wu <lepton@google.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2 or later.
 * See the COPYING.LIB file in the top-level directory.
 *
 */
#ifdef _MSC_VER
#define USE_QEMU_DIRENT
#endif
#include "qemu/osdep.h"
#ifndef _MSC_VER
#include <dirent.h>
#endif
#include "qapi/error.h"
#include "block/block_int.h"
#include "qemu/module.h"
#include "qemu/bswap.h"
#include "migration/misc.h"
#include "migration/migration.h"
#include "qapi/qmp/qstring.h"
#include "qapi/qmp/qdict.h"
#include "qemu/cutils.h"


#define MAX_BACKING_COUNT 32

typedef struct BDRVCATState {
    BdrvChild* backings[MAX_BACKING_COUNT];
    uint64_t ends[MAX_BACKING_COUNT];
    uint32_t count;
} BDRVCATState;

static QemuOptsList runtime_opts = {
    .name = "cat",
    .head = QTAILQ_HEAD_INITIALIZER(runtime_opts.head),
    .desc = {
        {
            .name = "backings",
            .type = QEMU_OPT_STRING,
            .help = "images back the cat device, separated with '|'",
        },
        { /* end of list */ }
    },
};

static void cat_parse_filename(const char *filename, QDict *options,
                                 Error **errp)
{
    if (!strstart(filename, "cat:", NULL)) {
        error_setg(errp, "File name string must start with 'cat:'");
        return;
    }
    qdict_put(options, "backings", qstring_from_str(filename + 4));
}

static int cat_open(BlockDriverState *bs, QDict *options, int flags,
                    Error **errp)
{
    BDRVCATState *s = bs->opaque;
    QemuOpts *opts;
    Error *local_err = NULL;
    char *ptr = NULL;
    int ret;

    s->count = 0;

    opts = qemu_opts_create(&runtime_opts, NULL, 0, &error_abort);
    qemu_opts_absorb_qdict(opts, options, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        ret = -EINVAL;
        goto fail;
    }

    const char * backings = qemu_opt_get(opts, "backings");
    if (!backings) {
        error_setg(errp, "cat driver requires backings option");
        ret = -EINVAL;
        goto fail;
    }

    ptr = g_strdup(backings);
    char *file = ptr;

    bs->total_sectors = 0;
    for (s->count = 0;s->count < MAX_BACKING_COUNT;) {
        char * end = strchr(file, '|');
        if (end) *end = 0;
        BlockDriverState* backing = bdrv_open(file, NULL, NULL, flags, &local_err);
        if (!backing) {
            error_setg(errp, "cat driver can't open backing file %s", file);
            ret = -EINVAL;
            goto fail;
        }
        bs->total_sectors += backing->total_sectors;
        s->ends[s->count] = bs->total_sectors;
        s->backings[s->count] = bdrv_attach_child(bs, backing, "cat", &child_file, &local_err);
        s->count++;
        if (end)
            file = end + 1;
        else
            break;
    }
    ret = 0;
fail:
    g_free(ptr);
    qemu_opts_del(opts);
    if (ret < 0) {
        int i;
        for (i = 0; i < s->count; i++) {
            bdrv_unref_child(bs, s->backings[i]);
        }
    }
    return ret;
}

static void cat_refresh_limits(BlockDriverState *bs, Error **errp)
{
    bs->bl.request_alignment = BDRV_SECTOR_SIZE; /* No sub-sector I/O */
}

int find_backing(BDRVCATState *s, uint64_t sec_num) {
    int i;
    for (i=0; i < s->count; i++) {
        if (sec_num < s->ends[i]) {
            return i;
        }
    }
    return -1;
}

static int coroutine_fn
cat_co_rw(BlockDriverState *bs, int64_t sector_num, int nb_sectors, QEMUIOVector *qiov,
          bool write)
{
    int ret;
    BDRVCATState *s = bs->opaque;

    if (sector_num  < 0 || nb_sectors < 0 || sector_num + nb_sectors > bs->total_sectors) {
        return -EINVAL;
    }
    int i = find_backing(s, sector_num);
    assert(i >= 0);
    uint64_t start = sector_num;
    if (i > 0) {
        start -= s->ends[i-1];
    }
    QEMUIOVector *c_iov = qiov;
    QEMUIOVector tmp;
    qemu_iovec_init(&tmp, qiov->niov);
    uint64_t done = 0, left = nb_sectors;
    while (left > 0) {
        uint64_t cnt = left;
        if (start +  cnt > s->backings[i]->bs->total_sectors) {
                cnt = s->backings[i]->bs->total_sectors - start;
        }
        if (write) {
          ret = bdrv_co_writev(s->backings[i],  start, cnt, c_iov);
        } else {
          ret = bdrv_co_readv(s->backings[i],  start, cnt, c_iov);
        }
        if (ret < 0) {
            qemu_iovec_destroy(&tmp);
            return ret;
        }
        i++;
        start = 0;
        done += cnt;
        left -= cnt;
        if (left > 0) {
            qemu_iovec_reset(&tmp);
            qemu_iovec_concat(&tmp, qiov, done << BDRV_SECTOR_BITS, left << BDRV_SECTOR_BITS);
            c_iov = &tmp;
        }
    }
    qemu_iovec_destroy(&tmp);
    return 0;
}

static int coroutine_fn
cat_co_readv(BlockDriverState *bs, int64_t sector_num, int nb_sectors, QEMUIOVector *qiov)
{
    return cat_co_rw(bs, sector_num, nb_sectors, qiov, 0);
}

static int coroutine_fn
cat_co_writev(BlockDriverState *bs, int64_t sector_num, int nb_sectors,
              QEMUIOVector *qiov)
{
    return cat_co_rw(bs, sector_num, nb_sectors, qiov, 1);
}

static void cat_close(BlockDriverState *bs)
{
    BDRVCATState *s = bs->opaque;
    int i = 0;
    for (i = 0; i < s->count; i++) {
        bdrv_flush(s->backings[i]->bs);
        bdrv_unref_child(bs, s->backings[i]);
    }
}

static int cat_snapshot_create(BlockDriverState *bs, QEMUSnapshotInfo *sn_info)
{
     BDRVCATState* s = bs->opaque;
     int i, ret;
     for (i = 0; i < s->count; i++) {
         ret = bdrv_snapshot_create(s->backings[i]->bs, sn_info);
         if (ret) return ret;
     }
     return 0;
}

static int cat_snapshot_goto(BlockDriverState *bs, const char *snapshot_id)
{
     BDRVCATState* s = bs->opaque;
    Error *local_err = NULL;
     int i, ret;
     for (i = 0; i < s->count; i++) {
         ret = bdrv_snapshot_goto(s->backings[i]->bs, snapshot_id, &local_err);
         if (ret) return ret;
     }
     return 0;
}

static int cat_snapshot_delete(BlockDriverState *bs, const char *snapshot_id,
                               const char*name, Error **errp)
{
     BDRVCATState* s = bs->opaque;
     int i, ret;
     for (i = 0; i < s->count; i++) {
         ret = bdrv_snapshot_delete(s->backings[i]->bs, snapshot_id, name, errp);
         // We have to finish all deletes even error happens.
     }
     return ret;
}

static int cat_snapshot_list(BlockDriverState *bs, QEMUSnapshotInfo **psn_tab)
{
     BDRVCATState* s = bs->opaque;
     // Use snapshot list from last backing file.
     return bdrv_snapshot_list(s->backings[s->count - 1]->bs, psn_tab);
}

static int cat_save_vmstate(BlockDriverState *bs, QEMUIOVector *qiov,
                            int64_t pos)
{
    BDRVCATState *s = bs->opaque;
    // Use first backing file to save.
    return bdrv_writev_vmstate(s->backings[0]->bs, qiov, pos);
}

static int cat_load_vmstate(BlockDriverState *bs, QEMUIOVector *qiov,
                            int64_t pos)
{
    BDRVCATState *s = bs->opaque;
    return bdrv_readv_vmstate(s->backings[0]->bs, qiov, pos);
}

static BlockDriver bdrv_cat = {
    .format_name            = "cat",
    .protocol_name          = "cat",
    .instance_size          = sizeof(BDRVCATState),

    .bdrv_parse_filename    = cat_parse_filename,
    .bdrv_file_open         = cat_open,
    .bdrv_refresh_limits    = cat_refresh_limits,
    .bdrv_close             = cat_close,
    .bdrv_child_perm        = bdrv_format_default_perms,

    .bdrv_co_readv          = cat_co_readv,
    .bdrv_co_writev         = cat_co_writev,

    .bdrv_snapshot_create   = cat_snapshot_create,
    .bdrv_snapshot_goto     = cat_snapshot_goto,
    .bdrv_snapshot_delete   = cat_snapshot_delete,
    .bdrv_snapshot_list     = cat_snapshot_list,

    .bdrv_save_vmstate      = cat_save_vmstate,
    .bdrv_load_vmstate      = cat_load_vmstate,
};

static void bdrv_cat_init(void)
{
    bdrv_register(&bdrv_cat);
}

block_init(bdrv_cat_init);
