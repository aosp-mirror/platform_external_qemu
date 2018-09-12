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

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qapi/error.h"
#include "qapi/qmp/qerror.h"
#include "qapi-visit-block-core.h"

void visit_type_SnapshotInfo_members(Visitor *v, SnapshotInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "vm-state-size", &obj->vm_state_size, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "date-sec", &obj->date_sec, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "date-nsec", &obj->date_nsec, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "vm-clock-sec", &obj->vm_clock_sec, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "vm-clock-nsec", &obj->vm_clock_nsec, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SnapshotInfo(Visitor *v, const char *name, SnapshotInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SnapshotInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SnapshotInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SnapshotInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoSpecificQCow2EncryptionBase_members(Visitor *v, ImageInfoSpecificQCow2EncryptionBase *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevQcow2EncryptionFormat(v, "format", &obj->format, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoSpecificQCow2EncryptionBase(Visitor *v, const char *name, ImageInfoSpecificQCow2EncryptionBase **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ImageInfoSpecificQCow2EncryptionBase), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ImageInfoSpecificQCow2EncryptionBase_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ImageInfoSpecificQCow2EncryptionBase(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoSpecificQCow2Encryption_members(Visitor *v, ImageInfoSpecificQCow2Encryption *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ImageInfoSpecificQCow2EncryptionBase_members(v, (ImageInfoSpecificQCow2EncryptionBase *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->format) {
    case BLOCKDEV_QCOW2_ENCRYPTION_FORMAT_AES:
        visit_type_QCryptoBlockInfoQCow_members(v, &obj->u.aes, &err);
        break;
    case BLOCKDEV_QCOW2_ENCRYPTION_FORMAT_LUKS:
        visit_type_QCryptoBlockInfoLUKS_members(v, &obj->u.luks, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoSpecificQCow2Encryption(Visitor *v, const char *name, ImageInfoSpecificQCow2Encryption **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ImageInfoSpecificQCow2Encryption), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ImageInfoSpecificQCow2Encryption_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ImageInfoSpecificQCow2Encryption(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoSpecificQCow2_members(Visitor *v, ImageInfoSpecificQCow2 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "compat", &obj->compat, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "lazy-refcounts", &obj->has_lazy_refcounts)) {
        visit_type_bool(v, "lazy-refcounts", &obj->lazy_refcounts, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "corrupt", &obj->has_corrupt)) {
        visit_type_bool(v, "corrupt", &obj->corrupt, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "refcount-bits", &obj->refcount_bits, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "encrypt", &obj->has_encrypt)) {
        visit_type_ImageInfoSpecificQCow2Encryption(v, "encrypt", &obj->encrypt, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoSpecificQCow2(Visitor *v, const char *name, ImageInfoSpecificQCow2 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ImageInfoSpecificQCow2), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ImageInfoSpecificQCow2_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ImageInfoSpecificQCow2(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoList(Visitor *v, const char *name, ImageInfoList **obj, Error **errp)
{
    Error *err = NULL;
    ImageInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (ImageInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_ImageInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ImageInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoSpecificVmdk_members(Visitor *v, ImageInfoSpecificVmdk *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "create-type", &obj->create_type, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "cid", &obj->cid, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "parent-cid", &obj->parent_cid, &err);
    if (err) {
        goto out;
    }
    visit_type_ImageInfoList(v, "extents", &obj->extents, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoSpecificVmdk(Visitor *v, const char *name, ImageInfoSpecificVmdk **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ImageInfoSpecificVmdk), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ImageInfoSpecificVmdk_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ImageInfoSpecificVmdk(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ImageInfoSpecificQCow2_wrapper_members(Visitor *v, q_obj_ImageInfoSpecificQCow2_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ImageInfoSpecificQCow2(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ImageInfoSpecificVmdk_wrapper_members(Visitor *v, q_obj_ImageInfoSpecificVmdk_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ImageInfoSpecificVmdk(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_QCryptoBlockInfoLUKS_wrapper_members(Visitor *v, q_obj_QCryptoBlockInfoLUKS_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_QCryptoBlockInfoLUKS(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoSpecificKind(Visitor *v, const char *name, ImageInfoSpecificKind *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &ImageInfoSpecificKind_lookup, errp);
    *obj = value;
}

void visit_type_ImageInfoSpecific_members(Visitor *v, ImageInfoSpecific *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ImageInfoSpecificKind(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case IMAGE_INFO_SPECIFIC_KIND_QCOW2:
        visit_type_q_obj_ImageInfoSpecificQCow2_wrapper_members(v, &obj->u.qcow2, &err);
        break;
    case IMAGE_INFO_SPECIFIC_KIND_VMDK:
        visit_type_q_obj_ImageInfoSpecificVmdk_wrapper_members(v, &obj->u.vmdk, &err);
        break;
    case IMAGE_INFO_SPECIFIC_KIND_LUKS:
        visit_type_q_obj_QCryptoBlockInfoLUKS_wrapper_members(v, &obj->u.luks, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_ImageInfoSpecific(Visitor *v, const char *name, ImageInfoSpecific **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ImageInfoSpecific), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ImageInfoSpecific_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ImageInfoSpecific(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SnapshotInfoList(Visitor *v, const char *name, SnapshotInfoList **obj, Error **errp)
{
    Error *err = NULL;
    SnapshotInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (SnapshotInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_SnapshotInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SnapshotInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ImageInfo_members(Visitor *v, ImageInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "format", &obj->format, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "dirty-flag", &obj->has_dirty_flag)) {
        visit_type_bool(v, "dirty-flag", &obj->dirty_flag, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "actual-size", &obj->has_actual_size)) {
        visit_type_int(v, "actual-size", &obj->actual_size, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "virtual-size", &obj->virtual_size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "cluster-size", &obj->has_cluster_size)) {
        visit_type_int(v, "cluster-size", &obj->cluster_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "encrypted", &obj->has_encrypted)) {
        visit_type_bool(v, "encrypted", &obj->encrypted, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "compressed", &obj->has_compressed)) {
        visit_type_bool(v, "compressed", &obj->compressed, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "backing-filename", &obj->has_backing_filename)) {
        visit_type_str(v, "backing-filename", &obj->backing_filename, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "full-backing-filename", &obj->has_full_backing_filename)) {
        visit_type_str(v, "full-backing-filename", &obj->full_backing_filename, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "backing-filename-format", &obj->has_backing_filename_format)) {
        visit_type_str(v, "backing-filename-format", &obj->backing_filename_format, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "snapshots", &obj->has_snapshots)) {
        visit_type_SnapshotInfoList(v, "snapshots", &obj->snapshots, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "backing-image", &obj->has_backing_image)) {
        visit_type_ImageInfo(v, "backing-image", &obj->backing_image, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "format-specific", &obj->has_format_specific)) {
        visit_type_ImageInfoSpecific(v, "format-specific", &obj->format_specific, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ImageInfo(Visitor *v, const char *name, ImageInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ImageInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ImageInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ImageInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ImageCheck_members(Visitor *v, ImageCheck *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "format", &obj->format, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "check-errors", &obj->check_errors, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "image-end-offset", &obj->has_image_end_offset)) {
        visit_type_int(v, "image-end-offset", &obj->image_end_offset, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "corruptions", &obj->has_corruptions)) {
        visit_type_int(v, "corruptions", &obj->corruptions, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "leaks", &obj->has_leaks)) {
        visit_type_int(v, "leaks", &obj->leaks, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "corruptions-fixed", &obj->has_corruptions_fixed)) {
        visit_type_int(v, "corruptions-fixed", &obj->corruptions_fixed, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "leaks-fixed", &obj->has_leaks_fixed)) {
        visit_type_int(v, "leaks-fixed", &obj->leaks_fixed, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "total-clusters", &obj->has_total_clusters)) {
        visit_type_int(v, "total-clusters", &obj->total_clusters, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "allocated-clusters", &obj->has_allocated_clusters)) {
        visit_type_int(v, "allocated-clusters", &obj->allocated_clusters, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "fragmented-clusters", &obj->has_fragmented_clusters)) {
        visit_type_int(v, "fragmented-clusters", &obj->fragmented_clusters, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "compressed-clusters", &obj->has_compressed_clusters)) {
        visit_type_int(v, "compressed-clusters", &obj->compressed_clusters, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ImageCheck(Visitor *v, const char *name, ImageCheck **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ImageCheck), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ImageCheck_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ImageCheck(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_MapEntry_members(Visitor *v, MapEntry *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "start", &obj->start, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "length", &obj->length, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "zero", &obj->zero, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "depth", &obj->depth, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "offset", &obj->has_offset)) {
        visit_type_int(v, "offset", &obj->offset, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "filename", &obj->has_filename)) {
        visit_type_str(v, "filename", &obj->filename, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_MapEntry(Visitor *v, const char *name, MapEntry **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(MapEntry), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_MapEntry_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_MapEntry(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCacheInfo_members(Visitor *v, BlockdevCacheInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "writeback", &obj->writeback, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "direct", &obj->direct, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "no-flush", &obj->no_flush, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCacheInfo(Visitor *v, const char *name, BlockdevCacheInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCacheInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCacheInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCacheInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockDeviceInfo_members(Visitor *v, BlockDeviceInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "node-name", &obj->has_node_name)) {
        visit_type_str(v, "node-name", &obj->node_name, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_bool(v, "ro", &obj->ro, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "drv", &obj->drv, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "backing_file", &obj->has_backing_file)) {
        visit_type_str(v, "backing_file", &obj->backing_file, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "backing_file_depth", &obj->backing_file_depth, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "encrypted", &obj->encrypted, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "encryption_key_missing", &obj->encryption_key_missing, &err);
    if (err) {
        goto out;
    }
    visit_type_BlockdevDetectZeroesOptions(v, "detect_zeroes", &obj->detect_zeroes, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "bps", &obj->bps, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "bps_rd", &obj->bps_rd, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "bps_wr", &obj->bps_wr, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "iops", &obj->iops, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "iops_rd", &obj->iops_rd, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "iops_wr", &obj->iops_wr, &err);
    if (err) {
        goto out;
    }
    visit_type_ImageInfo(v, "image", &obj->image, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "bps_max", &obj->has_bps_max)) {
        visit_type_int(v, "bps_max", &obj->bps_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps_rd_max", &obj->has_bps_rd_max)) {
        visit_type_int(v, "bps_rd_max", &obj->bps_rd_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps_wr_max", &obj->has_bps_wr_max)) {
        visit_type_int(v, "bps_wr_max", &obj->bps_wr_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_max", &obj->has_iops_max)) {
        visit_type_int(v, "iops_max", &obj->iops_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_rd_max", &obj->has_iops_rd_max)) {
        visit_type_int(v, "iops_rd_max", &obj->iops_rd_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_wr_max", &obj->has_iops_wr_max)) {
        visit_type_int(v, "iops_wr_max", &obj->iops_wr_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps_max_length", &obj->has_bps_max_length)) {
        visit_type_int(v, "bps_max_length", &obj->bps_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps_rd_max_length", &obj->has_bps_rd_max_length)) {
        visit_type_int(v, "bps_rd_max_length", &obj->bps_rd_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps_wr_max_length", &obj->has_bps_wr_max_length)) {
        visit_type_int(v, "bps_wr_max_length", &obj->bps_wr_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_max_length", &obj->has_iops_max_length)) {
        visit_type_int(v, "iops_max_length", &obj->iops_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_rd_max_length", &obj->has_iops_rd_max_length)) {
        visit_type_int(v, "iops_rd_max_length", &obj->iops_rd_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_wr_max_length", &obj->has_iops_wr_max_length)) {
        visit_type_int(v, "iops_wr_max_length", &obj->iops_wr_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_size", &obj->has_iops_size)) {
        visit_type_int(v, "iops_size", &obj->iops_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "group", &obj->has_group)) {
        visit_type_str(v, "group", &obj->group, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_BlockdevCacheInfo(v, "cache", &obj->cache, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "write_threshold", &obj->write_threshold, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockDeviceInfo(Visitor *v, const char *name, BlockDeviceInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockDeviceInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockDeviceInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDeviceInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockDeviceIoStatus(Visitor *v, const char *name, BlockDeviceIoStatus *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockDeviceIoStatus_lookup, errp);
    *obj = value;
}

void visit_type_BlockDeviceMapEntry_members(Visitor *v, BlockDeviceMapEntry *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "start", &obj->start, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "length", &obj->length, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "depth", &obj->depth, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "zero", &obj->zero, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "offset", &obj->has_offset)) {
        visit_type_int(v, "offset", &obj->offset, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockDeviceMapEntry(Visitor *v, const char *name, BlockDeviceMapEntry **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockDeviceMapEntry), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockDeviceMapEntry_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDeviceMapEntry(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_DirtyBitmapStatus(Visitor *v, const char *name, DirtyBitmapStatus *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &DirtyBitmapStatus_lookup, errp);
    *obj = value;
}

void visit_type_BlockDirtyInfo_members(Visitor *v, BlockDirtyInfo *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "name", &obj->has_name)) {
        visit_type_str(v, "name", &obj->name, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "count", &obj->count, &err);
    if (err) {
        goto out;
    }
    visit_type_uint32(v, "granularity", &obj->granularity, &err);
    if (err) {
        goto out;
    }
    visit_type_DirtyBitmapStatus(v, "status", &obj->status, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockDirtyInfo(Visitor *v, const char *name, BlockDirtyInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockDirtyInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockDirtyInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDirtyInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockLatencyHistogramInfo_members(Visitor *v, BlockLatencyHistogramInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint64List(v, "boundaries", &obj->boundaries, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64List(v, "bins", &obj->bins, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockLatencyHistogramInfo(Visitor *v, const char *name, BlockLatencyHistogramInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockLatencyHistogramInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockLatencyHistogramInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockLatencyHistogramInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_x_block_latency_histogram_set_arg_members(Visitor *v, q_obj_x_block_latency_histogram_set_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "boundaries", &obj->has_boundaries)) {
        visit_type_uint64List(v, "boundaries", &obj->boundaries, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "boundaries-read", &obj->has_boundaries_read)) {
        visit_type_uint64List(v, "boundaries-read", &obj->boundaries_read, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "boundaries-write", &obj->has_boundaries_write)) {
        visit_type_uint64List(v, "boundaries-write", &obj->boundaries_write, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "boundaries-flush", &obj->has_boundaries_flush)) {
        visit_type_uint64List(v, "boundaries-flush", &obj->boundaries_flush, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockDirtyInfoList(Visitor *v, const char *name, BlockDirtyInfoList **obj, Error **errp)
{
    Error *err = NULL;
    BlockDirtyInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (BlockDirtyInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_BlockDirtyInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDirtyInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockInfo_members(Visitor *v, BlockInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "qdev", &obj->has_qdev)) {
        visit_type_str(v, "qdev", &obj->qdev, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "removable", &obj->removable, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "locked", &obj->locked, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "inserted", &obj->has_inserted)) {
        visit_type_BlockDeviceInfo(v, "inserted", &obj->inserted, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tray_open", &obj->has_tray_open)) {
        visit_type_bool(v, "tray_open", &obj->tray_open, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "io-status", &obj->has_io_status)) {
        visit_type_BlockDeviceIoStatus(v, "io-status", &obj->io_status, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "dirty-bitmaps", &obj->has_dirty_bitmaps)) {
        visit_type_BlockDirtyInfoList(v, "dirty-bitmaps", &obj->dirty_bitmaps, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockInfo(Visitor *v, const char *name, BlockInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockMeasureInfo_members(Visitor *v, BlockMeasureInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "required", &obj->required, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "fully-allocated", &obj->fully_allocated, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockMeasureInfo(Visitor *v, const char *name, BlockMeasureInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockMeasureInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockMeasureInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockMeasureInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockInfoList(Visitor *v, const char *name, BlockInfoList **obj, Error **errp)
{
    Error *err = NULL;
    BlockInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (BlockInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_BlockInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockDeviceTimedStats_members(Visitor *v, BlockDeviceTimedStats *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "interval_length", &obj->interval_length, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "min_rd_latency_ns", &obj->min_rd_latency_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "max_rd_latency_ns", &obj->max_rd_latency_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "avg_rd_latency_ns", &obj->avg_rd_latency_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "min_wr_latency_ns", &obj->min_wr_latency_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "max_wr_latency_ns", &obj->max_wr_latency_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "avg_wr_latency_ns", &obj->avg_wr_latency_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "min_flush_latency_ns", &obj->min_flush_latency_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "max_flush_latency_ns", &obj->max_flush_latency_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "avg_flush_latency_ns", &obj->avg_flush_latency_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_number(v, "avg_rd_queue_depth", &obj->avg_rd_queue_depth, &err);
    if (err) {
        goto out;
    }
    visit_type_number(v, "avg_wr_queue_depth", &obj->avg_wr_queue_depth, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockDeviceTimedStats(Visitor *v, const char *name, BlockDeviceTimedStats **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockDeviceTimedStats), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockDeviceTimedStats_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDeviceTimedStats(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockDeviceTimedStatsList(Visitor *v, const char *name, BlockDeviceTimedStatsList **obj, Error **errp)
{
    Error *err = NULL;
    BlockDeviceTimedStatsList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (BlockDeviceTimedStatsList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_BlockDeviceTimedStats(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDeviceTimedStatsList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockDeviceStats_members(Visitor *v, BlockDeviceStats *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "rd_bytes", &obj->rd_bytes, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "wr_bytes", &obj->wr_bytes, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "rd_operations", &obj->rd_operations, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "wr_operations", &obj->wr_operations, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "flush_operations", &obj->flush_operations, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "flush_total_time_ns", &obj->flush_total_time_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "wr_total_time_ns", &obj->wr_total_time_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "rd_total_time_ns", &obj->rd_total_time_ns, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "wr_highest_offset", &obj->wr_highest_offset, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "rd_merged", &obj->rd_merged, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "wr_merged", &obj->wr_merged, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "idle_time_ns", &obj->has_idle_time_ns)) {
        visit_type_int(v, "idle_time_ns", &obj->idle_time_ns, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "failed_rd_operations", &obj->failed_rd_operations, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "failed_wr_operations", &obj->failed_wr_operations, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "failed_flush_operations", &obj->failed_flush_operations, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "invalid_rd_operations", &obj->invalid_rd_operations, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "invalid_wr_operations", &obj->invalid_wr_operations, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "invalid_flush_operations", &obj->invalid_flush_operations, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "account_invalid", &obj->account_invalid, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "account_failed", &obj->account_failed, &err);
    if (err) {
        goto out;
    }
    visit_type_BlockDeviceTimedStatsList(v, "timed_stats", &obj->timed_stats, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "x_rd_latency_histogram", &obj->has_x_rd_latency_histogram)) {
        visit_type_BlockLatencyHistogramInfo(v, "x_rd_latency_histogram", &obj->x_rd_latency_histogram, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "x_wr_latency_histogram", &obj->has_x_wr_latency_histogram)) {
        visit_type_BlockLatencyHistogramInfo(v, "x_wr_latency_histogram", &obj->x_wr_latency_histogram, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "x_flush_latency_histogram", &obj->has_x_flush_latency_histogram)) {
        visit_type_BlockLatencyHistogramInfo(v, "x_flush_latency_histogram", &obj->x_flush_latency_histogram, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockDeviceStats(Visitor *v, const char *name, BlockDeviceStats **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockDeviceStats), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockDeviceStats_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDeviceStats(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockStats_members(Visitor *v, BlockStats *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "node-name", &obj->has_node_name)) {
        visit_type_str(v, "node-name", &obj->node_name, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_BlockDeviceStats(v, "stats", &obj->stats, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "parent", &obj->has_parent)) {
        visit_type_BlockStats(v, "parent", &obj->parent, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "backing", &obj->has_backing)) {
        visit_type_BlockStats(v, "backing", &obj->backing, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockStats(Visitor *v, const char *name, BlockStats **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockStats), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockStats_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockStats(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_query_blockstats_arg_members(Visitor *v, q_obj_query_blockstats_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "query-nodes", &obj->has_query_nodes)) {
        visit_type_bool(v, "query-nodes", &obj->query_nodes, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockStatsList(Visitor *v, const char *name, BlockStatsList **obj, Error **errp)
{
    Error *err = NULL;
    BlockStatsList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (BlockStatsList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_BlockStats(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockStatsList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOnError(Visitor *v, const char *name, BlockdevOnError *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevOnError_lookup, errp);
    *obj = value;
}

void visit_type_MirrorSyncMode(Visitor *v, const char *name, MirrorSyncMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &MirrorSyncMode_lookup, errp);
    *obj = value;
}

void visit_type_BlockJobType(Visitor *v, const char *name, BlockJobType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockJobType_lookup, errp);
    *obj = value;
}

void visit_type_BlockJobVerb(Visitor *v, const char *name, BlockJobVerb *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockJobVerb_lookup, errp);
    *obj = value;
}

void visit_type_BlockJobStatus(Visitor *v, const char *name, BlockJobStatus *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockJobStatus_lookup, errp);
    *obj = value;
}

void visit_type_BlockJobInfo_members(Visitor *v, BlockJobInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "len", &obj->len, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "offset", &obj->offset, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "busy", &obj->busy, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "paused", &obj->paused, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "speed", &obj->speed, &err);
    if (err) {
        goto out;
    }
    visit_type_BlockDeviceIoStatus(v, "io-status", &obj->io_status, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "ready", &obj->ready, &err);
    if (err) {
        goto out;
    }
    visit_type_BlockJobStatus(v, "status", &obj->status, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "auto-finalize", &obj->auto_finalize, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "auto-dismiss", &obj->auto_dismiss, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockJobInfo(Visitor *v, const char *name, BlockJobInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockJobInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockJobInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockJobInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockJobInfoList(Visitor *v, const char *name, BlockJobInfoList **obj, Error **errp)
{
    Error *err = NULL;
    BlockJobInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (BlockJobInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_BlockJobInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockJobInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_passwd_arg_members(Visitor *v, q_obj_block_passwd_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "node-name", &obj->has_node_name)) {
        visit_type_str(v, "node-name", &obj->node_name, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "password", &obj->password, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_resize_arg_members(Visitor *v, q_obj_block_resize_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "node-name", &obj->has_node_name)) {
        visit_type_str(v, "node-name", &obj->node_name, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_NewImageMode(Visitor *v, const char *name, NewImageMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &NewImageMode_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevSnapshotSync_members(Visitor *v, BlockdevSnapshotSync *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "node-name", &obj->has_node_name)) {
        visit_type_str(v, "node-name", &obj->node_name, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "snapshot-file", &obj->snapshot_file, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "snapshot-node-name", &obj->has_snapshot_node_name)) {
        visit_type_str(v, "snapshot-node-name", &obj->snapshot_node_name, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "format", &obj->has_format)) {
        visit_type_str(v, "format", &obj->format, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "mode", &obj->has_mode)) {
        visit_type_NewImageMode(v, "mode", &obj->mode, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevSnapshotSync(Visitor *v, const char *name, BlockdevSnapshotSync **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevSnapshotSync), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevSnapshotSync_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevSnapshotSync(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevSnapshot_members(Visitor *v, BlockdevSnapshot *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "node", &obj->node, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "overlay", &obj->overlay, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevSnapshot(Visitor *v, const char *name, BlockdevSnapshot **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevSnapshot), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevSnapshot_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevSnapshot(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_DriveBackup_members(Visitor *v, DriveBackup *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "job-id", &obj->has_job_id)) {
        visit_type_str(v, "job-id", &obj->job_id, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "target", &obj->target, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "format", &obj->has_format)) {
        visit_type_str(v, "format", &obj->format, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_MirrorSyncMode(v, "sync", &obj->sync, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "mode", &obj->has_mode)) {
        visit_type_NewImageMode(v, "mode", &obj->mode, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "speed", &obj->has_speed)) {
        visit_type_int(v, "speed", &obj->speed, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bitmap", &obj->has_bitmap)) {
        visit_type_str(v, "bitmap", &obj->bitmap, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "compress", &obj->has_compress)) {
        visit_type_bool(v, "compress", &obj->compress, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "on-source-error", &obj->has_on_source_error)) {
        visit_type_BlockdevOnError(v, "on-source-error", &obj->on_source_error, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "on-target-error", &obj->has_on_target_error)) {
        visit_type_BlockdevOnError(v, "on-target-error", &obj->on_target_error, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "auto-finalize", &obj->has_auto_finalize)) {
        visit_type_bool(v, "auto-finalize", &obj->auto_finalize, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "auto-dismiss", &obj->has_auto_dismiss)) {
        visit_type_bool(v, "auto-dismiss", &obj->auto_dismiss, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_DriveBackup(Visitor *v, const char *name, DriveBackup **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(DriveBackup), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_DriveBackup_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_DriveBackup(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevBackup_members(Visitor *v, BlockdevBackup *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "job-id", &obj->has_job_id)) {
        visit_type_str(v, "job-id", &obj->job_id, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "target", &obj->target, &err);
    if (err) {
        goto out;
    }
    visit_type_MirrorSyncMode(v, "sync", &obj->sync, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "speed", &obj->has_speed)) {
        visit_type_int(v, "speed", &obj->speed, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "compress", &obj->has_compress)) {
        visit_type_bool(v, "compress", &obj->compress, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "on-source-error", &obj->has_on_source_error)) {
        visit_type_BlockdevOnError(v, "on-source-error", &obj->on_source_error, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "on-target-error", &obj->has_on_target_error)) {
        visit_type_BlockdevOnError(v, "on-target-error", &obj->on_target_error, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "auto-finalize", &obj->has_auto_finalize)) {
        visit_type_bool(v, "auto-finalize", &obj->auto_finalize, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "auto-dismiss", &obj->has_auto_dismiss)) {
        visit_type_bool(v, "auto-dismiss", &obj->auto_dismiss, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevBackup(Visitor *v, const char *name, BlockdevBackup **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevBackup), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevBackup_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevBackup(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_change_backing_file_arg_members(Visitor *v, q_obj_change_backing_file_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "image-node-name", &obj->image_node_name, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "backing-file", &obj->backing_file, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_commit_arg_members(Visitor *v, q_obj_block_commit_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "job-id", &obj->has_job_id)) {
        visit_type_str(v, "job-id", &obj->job_id, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "base", &obj->has_base)) {
        visit_type_str(v, "base", &obj->base, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "top", &obj->has_top)) {
        visit_type_str(v, "top", &obj->top, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "backing-file", &obj->has_backing_file)) {
        visit_type_str(v, "backing-file", &obj->backing_file, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "speed", &obj->has_speed)) {
        visit_type_int(v, "speed", &obj->speed, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "filter-node-name", &obj->has_filter_node_name)) {
        visit_type_str(v, "filter-node-name", &obj->filter_node_name, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockDeviceInfoList(Visitor *v, const char *name, BlockDeviceInfoList **obj, Error **errp)
{
    Error *err = NULL;
    BlockDeviceInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (BlockDeviceInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_BlockDeviceInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDeviceInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_DriveMirror_members(Visitor *v, DriveMirror *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "job-id", &obj->has_job_id)) {
        visit_type_str(v, "job-id", &obj->job_id, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "target", &obj->target, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "format", &obj->has_format)) {
        visit_type_str(v, "format", &obj->format, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "node-name", &obj->has_node_name)) {
        visit_type_str(v, "node-name", &obj->node_name, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "replaces", &obj->has_replaces)) {
        visit_type_str(v, "replaces", &obj->replaces, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_MirrorSyncMode(v, "sync", &obj->sync, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "mode", &obj->has_mode)) {
        visit_type_NewImageMode(v, "mode", &obj->mode, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "speed", &obj->has_speed)) {
        visit_type_int(v, "speed", &obj->speed, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "granularity", &obj->has_granularity)) {
        visit_type_uint32(v, "granularity", &obj->granularity, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "buf-size", &obj->has_buf_size)) {
        visit_type_int(v, "buf-size", &obj->buf_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "on-source-error", &obj->has_on_source_error)) {
        visit_type_BlockdevOnError(v, "on-source-error", &obj->on_source_error, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "on-target-error", &obj->has_on_target_error)) {
        visit_type_BlockdevOnError(v, "on-target-error", &obj->on_target_error, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "unmap", &obj->has_unmap)) {
        visit_type_bool(v, "unmap", &obj->unmap, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_DriveMirror(Visitor *v, const char *name, DriveMirror **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(DriveMirror), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_DriveMirror_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_DriveMirror(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockDirtyBitmap_members(Visitor *v, BlockDirtyBitmap *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "node", &obj->node, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockDirtyBitmap(Visitor *v, const char *name, BlockDirtyBitmap **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockDirtyBitmap), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockDirtyBitmap_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDirtyBitmap(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockDirtyBitmapAdd_members(Visitor *v, BlockDirtyBitmapAdd *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "node", &obj->node, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "granularity", &obj->has_granularity)) {
        visit_type_uint32(v, "granularity", &obj->granularity, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "persistent", &obj->has_persistent)) {
        visit_type_bool(v, "persistent", &obj->persistent, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "autoload", &obj->has_autoload)) {
        visit_type_bool(v, "autoload", &obj->autoload, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockDirtyBitmapAdd(Visitor *v, const char *name, BlockDirtyBitmapAdd **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockDirtyBitmapAdd), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockDirtyBitmapAdd_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDirtyBitmapAdd(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockDirtyBitmapSha256_members(Visitor *v, BlockDirtyBitmapSha256 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "sha256", &obj->sha256, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockDirtyBitmapSha256(Visitor *v, const char *name, BlockDirtyBitmapSha256 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockDirtyBitmapSha256), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockDirtyBitmapSha256_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockDirtyBitmapSha256(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_blockdev_mirror_arg_members(Visitor *v, q_obj_blockdev_mirror_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "job-id", &obj->has_job_id)) {
        visit_type_str(v, "job-id", &obj->job_id, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "target", &obj->target, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "replaces", &obj->has_replaces)) {
        visit_type_str(v, "replaces", &obj->replaces, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_MirrorSyncMode(v, "sync", &obj->sync, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "speed", &obj->has_speed)) {
        visit_type_int(v, "speed", &obj->speed, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "granularity", &obj->has_granularity)) {
        visit_type_uint32(v, "granularity", &obj->granularity, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "buf-size", &obj->has_buf_size)) {
        visit_type_int(v, "buf-size", &obj->buf_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "on-source-error", &obj->has_on_source_error)) {
        visit_type_BlockdevOnError(v, "on-source-error", &obj->on_source_error, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "on-target-error", &obj->has_on_target_error)) {
        visit_type_BlockdevOnError(v, "on-target-error", &obj->on_target_error, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "filter-node-name", &obj->has_filter_node_name)) {
        visit_type_str(v, "filter-node-name", &obj->filter_node_name, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockIOThrottle_members(Visitor *v, BlockIOThrottle *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "id", &obj->has_id)) {
        visit_type_str(v, "id", &obj->id, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "bps", &obj->bps, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "bps_rd", &obj->bps_rd, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "bps_wr", &obj->bps_wr, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "iops", &obj->iops, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "iops_rd", &obj->iops_rd, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "iops_wr", &obj->iops_wr, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "bps_max", &obj->has_bps_max)) {
        visit_type_int(v, "bps_max", &obj->bps_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps_rd_max", &obj->has_bps_rd_max)) {
        visit_type_int(v, "bps_rd_max", &obj->bps_rd_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps_wr_max", &obj->has_bps_wr_max)) {
        visit_type_int(v, "bps_wr_max", &obj->bps_wr_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_max", &obj->has_iops_max)) {
        visit_type_int(v, "iops_max", &obj->iops_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_rd_max", &obj->has_iops_rd_max)) {
        visit_type_int(v, "iops_rd_max", &obj->iops_rd_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_wr_max", &obj->has_iops_wr_max)) {
        visit_type_int(v, "iops_wr_max", &obj->iops_wr_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps_max_length", &obj->has_bps_max_length)) {
        visit_type_int(v, "bps_max_length", &obj->bps_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps_rd_max_length", &obj->has_bps_rd_max_length)) {
        visit_type_int(v, "bps_rd_max_length", &obj->bps_rd_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps_wr_max_length", &obj->has_bps_wr_max_length)) {
        visit_type_int(v, "bps_wr_max_length", &obj->bps_wr_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_max_length", &obj->has_iops_max_length)) {
        visit_type_int(v, "iops_max_length", &obj->iops_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_rd_max_length", &obj->has_iops_rd_max_length)) {
        visit_type_int(v, "iops_rd_max_length", &obj->iops_rd_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_wr_max_length", &obj->has_iops_wr_max_length)) {
        visit_type_int(v, "iops_wr_max_length", &obj->iops_wr_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops_size", &obj->has_iops_size)) {
        visit_type_int(v, "iops_size", &obj->iops_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "group", &obj->has_group)) {
        visit_type_str(v, "group", &obj->group, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockIOThrottle(Visitor *v, const char *name, BlockIOThrottle **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockIOThrottle), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockIOThrottle_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockIOThrottle(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ThrottleLimits_members(Visitor *v, ThrottleLimits *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "iops-total", &obj->has_iops_total)) {
        visit_type_int(v, "iops-total", &obj->iops_total, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops-total-max", &obj->has_iops_total_max)) {
        visit_type_int(v, "iops-total-max", &obj->iops_total_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops-total-max-length", &obj->has_iops_total_max_length)) {
        visit_type_int(v, "iops-total-max-length", &obj->iops_total_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops-read", &obj->has_iops_read)) {
        visit_type_int(v, "iops-read", &obj->iops_read, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops-read-max", &obj->has_iops_read_max)) {
        visit_type_int(v, "iops-read-max", &obj->iops_read_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops-read-max-length", &obj->has_iops_read_max_length)) {
        visit_type_int(v, "iops-read-max-length", &obj->iops_read_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops-write", &obj->has_iops_write)) {
        visit_type_int(v, "iops-write", &obj->iops_write, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops-write-max", &obj->has_iops_write_max)) {
        visit_type_int(v, "iops-write-max", &obj->iops_write_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops-write-max-length", &obj->has_iops_write_max_length)) {
        visit_type_int(v, "iops-write-max-length", &obj->iops_write_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps-total", &obj->has_bps_total)) {
        visit_type_int(v, "bps-total", &obj->bps_total, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps-total-max", &obj->has_bps_total_max)) {
        visit_type_int(v, "bps-total-max", &obj->bps_total_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps-total-max-length", &obj->has_bps_total_max_length)) {
        visit_type_int(v, "bps-total-max-length", &obj->bps_total_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps-read", &obj->has_bps_read)) {
        visit_type_int(v, "bps-read", &obj->bps_read, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps-read-max", &obj->has_bps_read_max)) {
        visit_type_int(v, "bps-read-max", &obj->bps_read_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps-read-max-length", &obj->has_bps_read_max_length)) {
        visit_type_int(v, "bps-read-max-length", &obj->bps_read_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps-write", &obj->has_bps_write)) {
        visit_type_int(v, "bps-write", &obj->bps_write, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps-write-max", &obj->has_bps_write_max)) {
        visit_type_int(v, "bps-write-max", &obj->bps_write_max, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bps-write-max-length", &obj->has_bps_write_max_length)) {
        visit_type_int(v, "bps-write-max-length", &obj->bps_write_max_length, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iops-size", &obj->has_iops_size)) {
        visit_type_int(v, "iops-size", &obj->iops_size, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ThrottleLimits(Visitor *v, const char *name, ThrottleLimits **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ThrottleLimits), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ThrottleLimits_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ThrottleLimits(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_stream_arg_members(Visitor *v, q_obj_block_stream_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "job-id", &obj->has_job_id)) {
        visit_type_str(v, "job-id", &obj->job_id, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "base", &obj->has_base)) {
        visit_type_str(v, "base", &obj->base, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "base-node", &obj->has_base_node)) {
        visit_type_str(v, "base-node", &obj->base_node, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "backing-file", &obj->has_backing_file)) {
        visit_type_str(v, "backing-file", &obj->backing_file, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "speed", &obj->has_speed)) {
        visit_type_int(v, "speed", &obj->speed, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "on-error", &obj->has_on_error)) {
        visit_type_BlockdevOnError(v, "on-error", &obj->on_error, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_job_set_speed_arg_members(Visitor *v, q_obj_block_job_set_speed_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "speed", &obj->speed, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_job_cancel_arg_members(Visitor *v, q_obj_block_job_cancel_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "force", &obj->has_force)) {
        visit_type_bool(v, "force", &obj->force, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_job_pause_arg_members(Visitor *v, q_obj_block_job_pause_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_job_resume_arg_members(Visitor *v, q_obj_block_job_resume_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_job_complete_arg_members(Visitor *v, q_obj_block_job_complete_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_job_dismiss_arg_members(Visitor *v, q_obj_block_job_dismiss_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_job_finalize_arg_members(Visitor *v, q_obj_block_job_finalize_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevDiscardOptions(Visitor *v, const char *name, BlockdevDiscardOptions *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevDiscardOptions_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevDetectZeroesOptions(Visitor *v, const char *name, BlockdevDetectZeroesOptions *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevDetectZeroesOptions_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevAioOptions(Visitor *v, const char *name, BlockdevAioOptions *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevAioOptions_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevCacheOptions_members(Visitor *v, BlockdevCacheOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "direct", &obj->has_direct)) {
        visit_type_bool(v, "direct", &obj->direct, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "no-flush", &obj->has_no_flush)) {
        visit_type_bool(v, "no-flush", &obj->no_flush, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCacheOptions(Visitor *v, const char *name, BlockdevCacheOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCacheOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCacheOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCacheOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevDriver(Visitor *v, const char *name, BlockdevDriver *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevDriver_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevOptionsFile_members(Visitor *v, BlockdevOptionsFile *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "pr-manager", &obj->has_pr_manager)) {
        visit_type_str(v, "pr-manager", &obj->pr_manager, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "locking", &obj->has_locking)) {
        visit_type_OnOffAuto(v, "locking", &obj->locking, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "aio", &obj->has_aio)) {
        visit_type_BlockdevAioOptions(v, "aio", &obj->aio, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsFile(Visitor *v, const char *name, BlockdevOptionsFile **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsFile), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsFile_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsFile(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsNull_members(Visitor *v, BlockdevOptionsNull *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "size", &obj->has_size)) {
        visit_type_int(v, "size", &obj->size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "latency-ns", &obj->has_latency_ns)) {
        visit_type_uint64(v, "latency-ns", &obj->latency_ns, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsNull(Visitor *v, const char *name, BlockdevOptionsNull **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsNull), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsNull_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsNull(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsNVMe_members(Visitor *v, BlockdevOptionsNVMe *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "namespace", &obj->q_namespace, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsNVMe(Visitor *v, const char *name, BlockdevOptionsNVMe **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsNVMe), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsNVMe_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsNVMe(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsVVFAT_members(Visitor *v, BlockdevOptionsVVFAT *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "dir", &obj->dir, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "fat-type", &obj->has_fat_type)) {
        visit_type_int(v, "fat-type", &obj->fat_type, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "floppy", &obj->has_floppy)) {
        visit_type_bool(v, "floppy", &obj->floppy, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "label", &obj->has_label)) {
        visit_type_str(v, "label", &obj->label, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "rw", &obj->has_rw)) {
        visit_type_bool(v, "rw", &obj->rw, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsVVFAT(Visitor *v, const char *name, BlockdevOptionsVVFAT **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsVVFAT), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsVVFAT_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsVVFAT(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsGenericFormat_members(Visitor *v, BlockdevOptionsGenericFormat *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevRef(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsGenericFormat(Visitor *v, const char *name, BlockdevOptionsGenericFormat **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsGenericFormat), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsGenericFormat_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsGenericFormat(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsLUKS_members(Visitor *v, BlockdevOptionsLUKS *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsGenericFormat_members(v, (BlockdevOptionsGenericFormat *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "key-secret", &obj->has_key_secret)) {
        visit_type_str(v, "key-secret", &obj->key_secret, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsLUKS(Visitor *v, const char *name, BlockdevOptionsLUKS **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsLUKS), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsLUKS_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsLUKS(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsGenericCOWFormat_members(Visitor *v, BlockdevOptionsGenericCOWFormat *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsGenericFormat_members(v, (BlockdevOptionsGenericFormat *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "backing", &obj->has_backing)) {
        visit_type_BlockdevRefOrNull(v, "backing", &obj->backing, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsGenericCOWFormat(Visitor *v, const char *name, BlockdevOptionsGenericCOWFormat **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsGenericCOWFormat), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsGenericCOWFormat_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsGenericCOWFormat(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_Qcow2OverlapCheckMode(Visitor *v, const char *name, Qcow2OverlapCheckMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &Qcow2OverlapCheckMode_lookup, errp);
    *obj = value;
}

void visit_type_Qcow2OverlapCheckFlags_members(Visitor *v, Qcow2OverlapCheckFlags *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "template", &obj->has_q_template)) {
        visit_type_Qcow2OverlapCheckMode(v, "template", &obj->q_template, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "main-header", &obj->has_main_header)) {
        visit_type_bool(v, "main-header", &obj->main_header, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "active-l1", &obj->has_active_l1)) {
        visit_type_bool(v, "active-l1", &obj->active_l1, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "active-l2", &obj->has_active_l2)) {
        visit_type_bool(v, "active-l2", &obj->active_l2, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "refcount-table", &obj->has_refcount_table)) {
        visit_type_bool(v, "refcount-table", &obj->refcount_table, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "refcount-block", &obj->has_refcount_block)) {
        visit_type_bool(v, "refcount-block", &obj->refcount_block, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "snapshot-table", &obj->has_snapshot_table)) {
        visit_type_bool(v, "snapshot-table", &obj->snapshot_table, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "inactive-l1", &obj->has_inactive_l1)) {
        visit_type_bool(v, "inactive-l1", &obj->inactive_l1, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "inactive-l2", &obj->has_inactive_l2)) {
        visit_type_bool(v, "inactive-l2", &obj->inactive_l2, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_Qcow2OverlapCheckFlags(Visitor *v, const char *name, Qcow2OverlapCheckFlags **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(Qcow2OverlapCheckFlags), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_Qcow2OverlapCheckFlags_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_Qcow2OverlapCheckFlags(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_Qcow2OverlapChecks(Visitor *v, const char *name, Qcow2OverlapChecks **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_alternate(v, name, (GenericAlternate **)obj, sizeof(**obj),
                          &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    switch ((*obj)->type) {
    case QTYPE_QDICT:
        visit_start_struct(v, name, NULL, 0, &err);
        if (err) {
            break;
        }
        visit_type_Qcow2OverlapCheckFlags_members(v, &(*obj)->u.flags, &err);
        if (!err) {
            visit_check_struct(v, &err);
        }
        visit_end_struct(v, NULL);
        break;
    case QTYPE_QSTRING:
        visit_type_Qcow2OverlapCheckMode(v, name, &(*obj)->u.mode, &err);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "Qcow2OverlapChecks");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_Qcow2OverlapChecks(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevQcowEncryptionFormat(Visitor *v, const char *name, BlockdevQcowEncryptionFormat *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevQcowEncryptionFormat_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_BlockdevQcowEncryption_base_members(Visitor *v, q_obj_BlockdevQcowEncryption_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevQcowEncryptionFormat(v, "format", &obj->format, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevQcowEncryption_members(Visitor *v, BlockdevQcowEncryption *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_BlockdevQcowEncryption_base_members(v, (q_obj_BlockdevQcowEncryption_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->format) {
    case BLOCKDEV_QCOW_ENCRYPTION_FORMAT_AES:
        visit_type_QCryptoBlockOptionsQCow_members(v, &obj->u.aes, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevQcowEncryption(Visitor *v, const char *name, BlockdevQcowEncryption **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevQcowEncryption), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevQcowEncryption_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevQcowEncryption(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsQcow_members(Visitor *v, BlockdevOptionsQcow *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsGenericCOWFormat_members(v, (BlockdevOptionsGenericCOWFormat *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "encrypt", &obj->has_encrypt)) {
        visit_type_BlockdevQcowEncryption(v, "encrypt", &obj->encrypt, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsQcow(Visitor *v, const char *name, BlockdevOptionsQcow **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsQcow), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsQcow_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsQcow(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevQcow2EncryptionFormat(Visitor *v, const char *name, BlockdevQcow2EncryptionFormat *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevQcow2EncryptionFormat_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_BlockdevQcow2Encryption_base_members(Visitor *v, q_obj_BlockdevQcow2Encryption_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevQcow2EncryptionFormat(v, "format", &obj->format, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevQcow2Encryption_members(Visitor *v, BlockdevQcow2Encryption *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_BlockdevQcow2Encryption_base_members(v, (q_obj_BlockdevQcow2Encryption_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->format) {
    case BLOCKDEV_QCOW2_ENCRYPTION_FORMAT_AES:
        visit_type_QCryptoBlockOptionsQCow_members(v, &obj->u.aes, &err);
        break;
    case BLOCKDEV_QCOW2_ENCRYPTION_FORMAT_LUKS:
        visit_type_QCryptoBlockOptionsLUKS_members(v, &obj->u.luks, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevQcow2Encryption(Visitor *v, const char *name, BlockdevQcow2Encryption **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevQcow2Encryption), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevQcow2Encryption_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevQcow2Encryption(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsQcow2_members(Visitor *v, BlockdevOptionsQcow2 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsGenericCOWFormat_members(v, (BlockdevOptionsGenericCOWFormat *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "lazy-refcounts", &obj->has_lazy_refcounts)) {
        visit_type_bool(v, "lazy-refcounts", &obj->lazy_refcounts, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "pass-discard-request", &obj->has_pass_discard_request)) {
        visit_type_bool(v, "pass-discard-request", &obj->pass_discard_request, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "pass-discard-snapshot", &obj->has_pass_discard_snapshot)) {
        visit_type_bool(v, "pass-discard-snapshot", &obj->pass_discard_snapshot, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "pass-discard-other", &obj->has_pass_discard_other)) {
        visit_type_bool(v, "pass-discard-other", &obj->pass_discard_other, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "overlap-check", &obj->has_overlap_check)) {
        visit_type_Qcow2OverlapChecks(v, "overlap-check", &obj->overlap_check, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cache-size", &obj->has_cache_size)) {
        visit_type_int(v, "cache-size", &obj->cache_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "l2-cache-size", &obj->has_l2_cache_size)) {
        visit_type_int(v, "l2-cache-size", &obj->l2_cache_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "l2-cache-entry-size", &obj->has_l2_cache_entry_size)) {
        visit_type_int(v, "l2-cache-entry-size", &obj->l2_cache_entry_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "refcount-cache-size", &obj->has_refcount_cache_size)) {
        visit_type_int(v, "refcount-cache-size", &obj->refcount_cache_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cache-clean-interval", &obj->has_cache_clean_interval)) {
        visit_type_int(v, "cache-clean-interval", &obj->cache_clean_interval, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "encrypt", &obj->has_encrypt)) {
        visit_type_BlockdevQcow2Encryption(v, "encrypt", &obj->encrypt, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsQcow2(Visitor *v, const char *name, BlockdevOptionsQcow2 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsQcow2), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsQcow2_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsQcow2(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SshHostKeyCheckMode(Visitor *v, const char *name, SshHostKeyCheckMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &SshHostKeyCheckMode_lookup, errp);
    *obj = value;
}

void visit_type_SshHostKeyCheckHashType(Visitor *v, const char *name, SshHostKeyCheckHashType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &SshHostKeyCheckHashType_lookup, errp);
    *obj = value;
}

void visit_type_SshHostKeyHash_members(Visitor *v, SshHostKeyHash *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SshHostKeyCheckHashType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "hash", &obj->hash, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SshHostKeyHash(Visitor *v, const char *name, SshHostKeyHash **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SshHostKeyHash), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SshHostKeyHash_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SshHostKeyHash(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SshHostKeyDummy_members(Visitor *v, SshHostKeyDummy *obj, Error **errp)
{
    Error *err = NULL;

    error_propagate(errp, err);
}

void visit_type_SshHostKeyDummy(Visitor *v, const char *name, SshHostKeyDummy **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SshHostKeyDummy), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SshHostKeyDummy_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SshHostKeyDummy(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_SshHostKeyCheck_base_members(Visitor *v, q_obj_SshHostKeyCheck_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SshHostKeyCheckMode(v, "mode", &obj->mode, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SshHostKeyCheck_members(Visitor *v, SshHostKeyCheck *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_SshHostKeyCheck_base_members(v, (q_obj_SshHostKeyCheck_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->mode) {
    case SSH_HOST_KEY_CHECK_MODE_NONE:
        visit_type_SshHostKeyDummy_members(v, &obj->u.none, &err);
        break;
    case SSH_HOST_KEY_CHECK_MODE_HASH:
        visit_type_SshHostKeyHash_members(v, &obj->u.hash, &err);
        break;
    case SSH_HOST_KEY_CHECK_MODE_KNOWN_HOSTS:
        visit_type_SshHostKeyDummy_members(v, &obj->u.known_hosts, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_SshHostKeyCheck(Visitor *v, const char *name, SshHostKeyCheck **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SshHostKeyCheck), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SshHostKeyCheck_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SshHostKeyCheck(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsSsh_members(Visitor *v, BlockdevOptionsSsh *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_InetSocketAddress(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "path", &obj->path, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "user", &obj->has_user)) {
        visit_type_str(v, "user", &obj->user, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "host-key-check", &obj->has_host_key_check)) {
        visit_type_SshHostKeyCheck(v, "host-key-check", &obj->host_key_check, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsSsh(Visitor *v, const char *name, BlockdevOptionsSsh **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsSsh), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsSsh_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsSsh(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlkdebugEvent(Visitor *v, const char *name, BlkdebugEvent *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlkdebugEvent_lookup, errp);
    *obj = value;
}

void visit_type_BlkdebugInjectErrorOptions_members(Visitor *v, BlkdebugInjectErrorOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlkdebugEvent(v, "event", &obj->event, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "state", &obj->has_state)) {
        visit_type_int(v, "state", &obj->state, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "errno", &obj->has_q_errno)) {
        visit_type_int(v, "errno", &obj->q_errno, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "sector", &obj->has_sector)) {
        visit_type_int(v, "sector", &obj->sector, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "once", &obj->has_once)) {
        visit_type_bool(v, "once", &obj->once, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "immediately", &obj->has_immediately)) {
        visit_type_bool(v, "immediately", &obj->immediately, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlkdebugInjectErrorOptions(Visitor *v, const char *name, BlkdebugInjectErrorOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlkdebugInjectErrorOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlkdebugInjectErrorOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlkdebugInjectErrorOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlkdebugSetStateOptions_members(Visitor *v, BlkdebugSetStateOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlkdebugEvent(v, "event", &obj->event, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "state", &obj->has_state)) {
        visit_type_int(v, "state", &obj->state, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "new_state", &obj->new_state, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlkdebugSetStateOptions(Visitor *v, const char *name, BlkdebugSetStateOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlkdebugSetStateOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlkdebugSetStateOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlkdebugSetStateOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlkdebugInjectErrorOptionsList(Visitor *v, const char *name, BlkdebugInjectErrorOptionsList **obj, Error **errp)
{
    Error *err = NULL;
    BlkdebugInjectErrorOptionsList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (BlkdebugInjectErrorOptionsList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_BlkdebugInjectErrorOptions(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlkdebugInjectErrorOptionsList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlkdebugSetStateOptionsList(Visitor *v, const char *name, BlkdebugSetStateOptionsList **obj, Error **errp)
{
    Error *err = NULL;
    BlkdebugSetStateOptionsList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (BlkdebugSetStateOptionsList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_BlkdebugSetStateOptions(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlkdebugSetStateOptionsList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsBlkdebug_members(Visitor *v, BlockdevOptionsBlkdebug *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevRef(v, "image", &obj->image, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "config", &obj->has_config)) {
        visit_type_str(v, "config", &obj->config, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "align", &obj->has_align)) {
        visit_type_int(v, "align", &obj->align, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "max-transfer", &obj->has_max_transfer)) {
        visit_type_int32(v, "max-transfer", &obj->max_transfer, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "opt-write-zero", &obj->has_opt_write_zero)) {
        visit_type_int32(v, "opt-write-zero", &obj->opt_write_zero, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "max-write-zero", &obj->has_max_write_zero)) {
        visit_type_int32(v, "max-write-zero", &obj->max_write_zero, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "opt-discard", &obj->has_opt_discard)) {
        visit_type_int32(v, "opt-discard", &obj->opt_discard, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "max-discard", &obj->has_max_discard)) {
        visit_type_int32(v, "max-discard", &obj->max_discard, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "inject-error", &obj->has_inject_error)) {
        visit_type_BlkdebugInjectErrorOptionsList(v, "inject-error", &obj->inject_error, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "set-state", &obj->has_set_state)) {
        visit_type_BlkdebugSetStateOptionsList(v, "set-state", &obj->set_state, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsBlkdebug(Visitor *v, const char *name, BlockdevOptionsBlkdebug **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsBlkdebug), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsBlkdebug_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsBlkdebug(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsBlkverify_members(Visitor *v, BlockdevOptionsBlkverify *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevRef(v, "test", &obj->test, &err);
    if (err) {
        goto out;
    }
    visit_type_BlockdevRef(v, "raw", &obj->raw, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsBlkverify(Visitor *v, const char *name, BlockdevOptionsBlkverify **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsBlkverify), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsBlkverify_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsBlkverify(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QuorumReadPattern(Visitor *v, const char *name, QuorumReadPattern *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QuorumReadPattern_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevRefList(Visitor *v, const char *name, BlockdevRefList **obj, Error **errp)
{
    Error *err = NULL;
    BlockdevRefList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (BlockdevRefList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_BlockdevRef(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevRefList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsQuorum_members(Visitor *v, BlockdevOptionsQuorum *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "blkverify", &obj->has_blkverify)) {
        visit_type_bool(v, "blkverify", &obj->blkverify, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_BlockdevRefList(v, "children", &obj->children, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "vote-threshold", &obj->vote_threshold, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "rewrite-corrupted", &obj->has_rewrite_corrupted)) {
        visit_type_bool(v, "rewrite-corrupted", &obj->rewrite_corrupted, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "read-pattern", &obj->has_read_pattern)) {
        visit_type_QuorumReadPattern(v, "read-pattern", &obj->read_pattern, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsQuorum(Visitor *v, const char *name, BlockdevOptionsQuorum **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsQuorum), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsQuorum_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsQuorum(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SocketAddressList(Visitor *v, const char *name, SocketAddressList **obj, Error **errp)
{
    Error *err = NULL;
    SocketAddressList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (SocketAddressList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_SocketAddress(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SocketAddressList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsGluster_members(Visitor *v, BlockdevOptionsGluster *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "volume", &obj->volume, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "path", &obj->path, &err);
    if (err) {
        goto out;
    }
    visit_type_SocketAddressList(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "debug", &obj->has_debug)) {
        visit_type_int(v, "debug", &obj->debug, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "logfile", &obj->has_logfile)) {
        visit_type_str(v, "logfile", &obj->logfile, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsGluster(Visitor *v, const char *name, BlockdevOptionsGluster **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsGluster), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsGluster_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsGluster(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_IscsiTransport(Visitor *v, const char *name, IscsiTransport *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &IscsiTransport_lookup, errp);
    *obj = value;
}

void visit_type_IscsiHeaderDigest(Visitor *v, const char *name, IscsiHeaderDigest *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &IscsiHeaderDigest_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevOptionsIscsi_members(Visitor *v, BlockdevOptionsIscsi *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_IscsiTransport(v, "transport", &obj->transport, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "portal", &obj->portal, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "target", &obj->target, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "lun", &obj->has_lun)) {
        visit_type_int(v, "lun", &obj->lun, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "user", &obj->has_user)) {
        visit_type_str(v, "user", &obj->user, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "password-secret", &obj->has_password_secret)) {
        visit_type_str(v, "password-secret", &obj->password_secret, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "initiator-name", &obj->has_initiator_name)) {
        visit_type_str(v, "initiator-name", &obj->initiator_name, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "header-digest", &obj->has_header_digest)) {
        visit_type_IscsiHeaderDigest(v, "header-digest", &obj->header_digest, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "timeout", &obj->has_timeout)) {
        visit_type_int(v, "timeout", &obj->timeout, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsIscsi(Visitor *v, const char *name, BlockdevOptionsIscsi **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsIscsi), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsIscsi_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsIscsi(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_InetSocketAddressBaseList(Visitor *v, const char *name, InetSocketAddressBaseList **obj, Error **errp)
{
    Error *err = NULL;
    InetSocketAddressBaseList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (InetSocketAddressBaseList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_InetSocketAddressBase(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_InetSocketAddressBaseList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsRbd_members(Visitor *v, BlockdevOptionsRbd *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "pool", &obj->pool, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "image", &obj->image, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "conf", &obj->has_conf)) {
        visit_type_str(v, "conf", &obj->conf, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "snapshot", &obj->has_snapshot)) {
        visit_type_str(v, "snapshot", &obj->snapshot, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "user", &obj->has_user)) {
        visit_type_str(v, "user", &obj->user, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "server", &obj->has_server)) {
        visit_type_InetSocketAddressBaseList(v, "server", &obj->server, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsRbd(Visitor *v, const char *name, BlockdevOptionsRbd **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsRbd), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsRbd_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsRbd(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsSheepdog_members(Visitor *v, BlockdevOptionsSheepdog *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SocketAddress(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "vdi", &obj->vdi, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "snap-id", &obj->has_snap_id)) {
        visit_type_uint32(v, "snap-id", &obj->snap_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tag", &obj->has_tag)) {
        visit_type_str(v, "tag", &obj->tag, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsSheepdog(Visitor *v, const char *name, BlockdevOptionsSheepdog **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsSheepdog), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsSheepdog_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsSheepdog(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ReplicationMode(Visitor *v, const char *name, ReplicationMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &ReplicationMode_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevOptionsReplication_members(Visitor *v, BlockdevOptionsReplication *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsGenericFormat_members(v, (BlockdevOptionsGenericFormat *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_ReplicationMode(v, "mode", &obj->mode, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "top-id", &obj->has_top_id)) {
        visit_type_str(v, "top-id", &obj->top_id, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsReplication(Visitor *v, const char *name, BlockdevOptionsReplication **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsReplication), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsReplication_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsReplication(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NFSTransport(Visitor *v, const char *name, NFSTransport *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &NFSTransport_lookup, errp);
    *obj = value;
}

void visit_type_NFSServer_members(Visitor *v, NFSServer *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_NFSTransport(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "host", &obj->host, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_NFSServer(Visitor *v, const char *name, NFSServer **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NFSServer), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NFSServer_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NFSServer(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsNfs_members(Visitor *v, BlockdevOptionsNfs *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_NFSServer(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "path", &obj->path, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "user", &obj->has_user)) {
        visit_type_int(v, "user", &obj->user, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "group", &obj->has_group)) {
        visit_type_int(v, "group", &obj->group, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tcp-syn-count", &obj->has_tcp_syn_count)) {
        visit_type_int(v, "tcp-syn-count", &obj->tcp_syn_count, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "readahead-size", &obj->has_readahead_size)) {
        visit_type_int(v, "readahead-size", &obj->readahead_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "page-cache-size", &obj->has_page_cache_size)) {
        visit_type_int(v, "page-cache-size", &obj->page_cache_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "debug", &obj->has_debug)) {
        visit_type_int(v, "debug", &obj->debug, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsNfs(Visitor *v, const char *name, BlockdevOptionsNfs **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsNfs), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsNfs_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsNfs(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsCurlBase_members(Visitor *v, BlockdevOptionsCurlBase *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "url", &obj->url, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "readahead", &obj->has_readahead)) {
        visit_type_int(v, "readahead", &obj->readahead, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "timeout", &obj->has_timeout)) {
        visit_type_int(v, "timeout", &obj->timeout, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "username", &obj->has_username)) {
        visit_type_str(v, "username", &obj->username, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "password-secret", &obj->has_password_secret)) {
        visit_type_str(v, "password-secret", &obj->password_secret, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "proxy-username", &obj->has_proxy_username)) {
        visit_type_str(v, "proxy-username", &obj->proxy_username, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "proxy-password-secret", &obj->has_proxy_password_secret)) {
        visit_type_str(v, "proxy-password-secret", &obj->proxy_password_secret, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsCurlBase(Visitor *v, const char *name, BlockdevOptionsCurlBase **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsCurlBase), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsCurlBase_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsCurlBase(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsCurlHttp_members(Visitor *v, BlockdevOptionsCurlHttp *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsCurlBase_members(v, (BlockdevOptionsCurlBase *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "cookie", &obj->has_cookie)) {
        visit_type_str(v, "cookie", &obj->cookie, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cookie-secret", &obj->has_cookie_secret)) {
        visit_type_str(v, "cookie-secret", &obj->cookie_secret, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsCurlHttp(Visitor *v, const char *name, BlockdevOptionsCurlHttp **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsCurlHttp), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsCurlHttp_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsCurlHttp(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsCurlHttps_members(Visitor *v, BlockdevOptionsCurlHttps *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsCurlBase_members(v, (BlockdevOptionsCurlBase *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "cookie", &obj->has_cookie)) {
        visit_type_str(v, "cookie", &obj->cookie, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "sslverify", &obj->has_sslverify)) {
        visit_type_bool(v, "sslverify", &obj->sslverify, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cookie-secret", &obj->has_cookie_secret)) {
        visit_type_str(v, "cookie-secret", &obj->cookie_secret, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsCurlHttps(Visitor *v, const char *name, BlockdevOptionsCurlHttps **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsCurlHttps), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsCurlHttps_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsCurlHttps(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsCurlFtp_members(Visitor *v, BlockdevOptionsCurlFtp *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsCurlBase_members(v, (BlockdevOptionsCurlBase *)obj, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsCurlFtp(Visitor *v, const char *name, BlockdevOptionsCurlFtp **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsCurlFtp), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsCurlFtp_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsCurlFtp(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsCurlFtps_members(Visitor *v, BlockdevOptionsCurlFtps *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsCurlBase_members(v, (BlockdevOptionsCurlBase *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "sslverify", &obj->has_sslverify)) {
        visit_type_bool(v, "sslverify", &obj->sslverify, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsCurlFtps(Visitor *v, const char *name, BlockdevOptionsCurlFtps **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsCurlFtps), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsCurlFtps_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsCurlFtps(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsNbd_members(Visitor *v, BlockdevOptionsNbd *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SocketAddress(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "exp", &obj->has_exp)) {
        visit_type_str(v, "exp", &obj->exp, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tls-creds", &obj->has_tls_creds)) {
        visit_type_str(v, "tls-creds", &obj->tls_creds, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsNbd(Visitor *v, const char *name, BlockdevOptionsNbd **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsNbd), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsNbd_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsNbd(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsRaw_members(Visitor *v, BlockdevOptionsRaw *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsGenericFormat_members(v, (BlockdevOptionsGenericFormat *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "offset", &obj->has_offset)) {
        visit_type_int(v, "offset", &obj->offset, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "size", &obj->has_size)) {
        visit_type_int(v, "size", &obj->size, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsRaw(Visitor *v, const char *name, BlockdevOptionsRaw **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsRaw), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsRaw_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsRaw(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsVxHS_members(Visitor *v, BlockdevOptionsVxHS *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "vdisk-id", &obj->vdisk_id, &err);
    if (err) {
        goto out;
    }
    visit_type_InetSocketAddressBase(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "tls-creds", &obj->has_tls_creds)) {
        visit_type_str(v, "tls-creds", &obj->tls_creds, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsVxHS(Visitor *v, const char *name, BlockdevOptionsVxHS **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsVxHS), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsVxHS_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsVxHS(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsThrottle_members(Visitor *v, BlockdevOptionsThrottle *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "throttle-group", &obj->throttle_group, &err);
    if (err) {
        goto out;
    }
    visit_type_BlockdevRef(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptionsThrottle(Visitor *v, const char *name, BlockdevOptionsThrottle **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptionsThrottle), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptionsThrottle_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptionsThrottle(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BlockdevOptions_base_members(Visitor *v, q_obj_BlockdevOptions_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevDriver(v, "driver", &obj->driver, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "node-name", &obj->has_node_name)) {
        visit_type_str(v, "node-name", &obj->node_name, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "discard", &obj->has_discard)) {
        visit_type_BlockdevDiscardOptions(v, "discard", &obj->discard, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cache", &obj->has_cache)) {
        visit_type_BlockdevCacheOptions(v, "cache", &obj->cache, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "read-only", &obj->has_read_only)) {
        visit_type_bool(v, "read-only", &obj->read_only, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "force-share", &obj->has_force_share)) {
        visit_type_bool(v, "force-share", &obj->force_share, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "detect-zeroes", &obj->has_detect_zeroes)) {
        visit_type_BlockdevDetectZeroesOptions(v, "detect-zeroes", &obj->detect_zeroes, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptions_members(Visitor *v, BlockdevOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_BlockdevOptions_base_members(v, (q_obj_BlockdevOptions_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->driver) {
    case BLOCKDEV_DRIVER_BLKDEBUG:
        visit_type_BlockdevOptionsBlkdebug_members(v, &obj->u.blkdebug, &err);
        break;
    case BLOCKDEV_DRIVER_BLKVERIFY:
        visit_type_BlockdevOptionsBlkverify_members(v, &obj->u.blkverify, &err);
        break;
    case BLOCKDEV_DRIVER_BOCHS:
        visit_type_BlockdevOptionsGenericFormat_members(v, &obj->u.bochs, &err);
        break;
    case BLOCKDEV_DRIVER_CLOOP:
        visit_type_BlockdevOptionsGenericFormat_members(v, &obj->u.cloop, &err);
        break;
    case BLOCKDEV_DRIVER_DMG:
        visit_type_BlockdevOptionsGenericFormat_members(v, &obj->u.dmg, &err);
        break;
    case BLOCKDEV_DRIVER_FILE:
        visit_type_BlockdevOptionsFile_members(v, &obj->u.file, &err);
        break;
    case BLOCKDEV_DRIVER_FTP:
        visit_type_BlockdevOptionsCurlFtp_members(v, &obj->u.ftp, &err);
        break;
    case BLOCKDEV_DRIVER_FTPS:
        visit_type_BlockdevOptionsCurlFtps_members(v, &obj->u.ftps, &err);
        break;
    case BLOCKDEV_DRIVER_GLUSTER:
        visit_type_BlockdevOptionsGluster_members(v, &obj->u.gluster, &err);
        break;
    case BLOCKDEV_DRIVER_HOST_CDROM:
        visit_type_BlockdevOptionsFile_members(v, &obj->u.host_cdrom, &err);
        break;
    case BLOCKDEV_DRIVER_HOST_DEVICE:
        visit_type_BlockdevOptionsFile_members(v, &obj->u.host_device, &err);
        break;
    case BLOCKDEV_DRIVER_HTTP:
        visit_type_BlockdevOptionsCurlHttp_members(v, &obj->u.http, &err);
        break;
    case BLOCKDEV_DRIVER_HTTPS:
        visit_type_BlockdevOptionsCurlHttps_members(v, &obj->u.https, &err);
        break;
    case BLOCKDEV_DRIVER_ISCSI:
        visit_type_BlockdevOptionsIscsi_members(v, &obj->u.iscsi, &err);
        break;
    case BLOCKDEV_DRIVER_LUKS:
        visit_type_BlockdevOptionsLUKS_members(v, &obj->u.luks, &err);
        break;
    case BLOCKDEV_DRIVER_NBD:
        visit_type_BlockdevOptionsNbd_members(v, &obj->u.nbd, &err);
        break;
    case BLOCKDEV_DRIVER_NFS:
        visit_type_BlockdevOptionsNfs_members(v, &obj->u.nfs, &err);
        break;
    case BLOCKDEV_DRIVER_NULL_AIO:
        visit_type_BlockdevOptionsNull_members(v, &obj->u.null_aio, &err);
        break;
    case BLOCKDEV_DRIVER_NULL_CO:
        visit_type_BlockdevOptionsNull_members(v, &obj->u.null_co, &err);
        break;
    case BLOCKDEV_DRIVER_NVME:
        visit_type_BlockdevOptionsNVMe_members(v, &obj->u.nvme, &err);
        break;
    case BLOCKDEV_DRIVER_PARALLELS:
        visit_type_BlockdevOptionsGenericFormat_members(v, &obj->u.parallels, &err);
        break;
    case BLOCKDEV_DRIVER_QCOW2:
        visit_type_BlockdevOptionsQcow2_members(v, &obj->u.qcow2, &err);
        break;
    case BLOCKDEV_DRIVER_QCOW:
        visit_type_BlockdevOptionsQcow_members(v, &obj->u.qcow, &err);
        break;
    case BLOCKDEV_DRIVER_QED:
        visit_type_BlockdevOptionsGenericCOWFormat_members(v, &obj->u.qed, &err);
        break;
    case BLOCKDEV_DRIVER_QUORUM:
        visit_type_BlockdevOptionsQuorum_members(v, &obj->u.quorum, &err);
        break;
    case BLOCKDEV_DRIVER_RAW:
        visit_type_BlockdevOptionsRaw_members(v, &obj->u.raw, &err);
        break;
    case BLOCKDEV_DRIVER_RBD:
        visit_type_BlockdevOptionsRbd_members(v, &obj->u.rbd, &err);
        break;
    case BLOCKDEV_DRIVER_REPLICATION:
        visit_type_BlockdevOptionsReplication_members(v, &obj->u.replication, &err);
        break;
    case BLOCKDEV_DRIVER_SHEEPDOG:
        visit_type_BlockdevOptionsSheepdog_members(v, &obj->u.sheepdog, &err);
        break;
    case BLOCKDEV_DRIVER_SSH:
        visit_type_BlockdevOptionsSsh_members(v, &obj->u.ssh, &err);
        break;
    case BLOCKDEV_DRIVER_THROTTLE:
        visit_type_BlockdevOptionsThrottle_members(v, &obj->u.throttle, &err);
        break;
    case BLOCKDEV_DRIVER_VDI:
        visit_type_BlockdevOptionsGenericFormat_members(v, &obj->u.vdi, &err);
        break;
    case BLOCKDEV_DRIVER_VHDX:
        visit_type_BlockdevOptionsGenericFormat_members(v, &obj->u.vhdx, &err);
        break;
    case BLOCKDEV_DRIVER_VMDK:
        visit_type_BlockdevOptionsGenericCOWFormat_members(v, &obj->u.vmdk, &err);
        break;
    case BLOCKDEV_DRIVER_VPC:
        visit_type_BlockdevOptionsGenericFormat_members(v, &obj->u.vpc, &err);
        break;
    case BLOCKDEV_DRIVER_VVFAT:
        visit_type_BlockdevOptionsVVFAT_members(v, &obj->u.vvfat, &err);
        break;
    case BLOCKDEV_DRIVER_VXHS:
        visit_type_BlockdevOptionsVxHS_members(v, &obj->u.vxhs, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevOptions(Visitor *v, const char *name, BlockdevOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevRef(Visitor *v, const char *name, BlockdevRef **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_alternate(v, name, (GenericAlternate **)obj, sizeof(**obj),
                          &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    switch ((*obj)->type) {
    case QTYPE_QDICT:
        visit_start_struct(v, name, NULL, 0, &err);
        if (err) {
            break;
        }
        visit_type_BlockdevOptions_members(v, &(*obj)->u.definition, &err);
        if (!err) {
            visit_check_struct(v, &err);
        }
        visit_end_struct(v, NULL);
        break;
    case QTYPE_QSTRING:
        visit_type_str(v, name, &(*obj)->u.reference, &err);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "BlockdevRef");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevRef(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevRefOrNull(Visitor *v, const char *name, BlockdevRefOrNull **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_alternate(v, name, (GenericAlternate **)obj, sizeof(**obj),
                          &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    switch ((*obj)->type) {
    case QTYPE_QDICT:
        visit_start_struct(v, name, NULL, 0, &err);
        if (err) {
            break;
        }
        visit_type_BlockdevOptions_members(v, &(*obj)->u.definition, &err);
        if (!err) {
            visit_check_struct(v, &err);
        }
        visit_end_struct(v, NULL);
        break;
    case QTYPE_QSTRING:
        visit_type_str(v, name, &(*obj)->u.reference, &err);
        break;
    case QTYPE_QNULL:
        visit_type_null(v, name, &(*obj)->u.null, &err);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "BlockdevRefOrNull");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevRefOrNull(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_blockdev_del_arg_members(Visitor *v, q_obj_blockdev_del_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "node-name", &obj->node_name, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsFile_members(Visitor *v, BlockdevCreateOptionsFile *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "preallocation", &obj->has_preallocation)) {
        visit_type_PreallocMode(v, "preallocation", &obj->preallocation, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "nocow", &obj->has_nocow)) {
        visit_type_bool(v, "nocow", &obj->nocow, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsFile(Visitor *v, const char *name, BlockdevCreateOptionsFile **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsFile), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsFile_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsFile(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsGluster_members(Visitor *v, BlockdevCreateOptionsGluster *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsGluster(v, "location", &obj->location, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "preallocation", &obj->has_preallocation)) {
        visit_type_PreallocMode(v, "preallocation", &obj->preallocation, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsGluster(Visitor *v, const char *name, BlockdevCreateOptionsGluster **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsGluster), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsGluster_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsGluster(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsLUKS_members(Visitor *v, BlockdevCreateOptionsLUKS *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_QCryptoBlockCreateOptionsLUKS_members(v, (QCryptoBlockCreateOptionsLUKS *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_BlockdevRef(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsLUKS(Visitor *v, const char *name, BlockdevCreateOptionsLUKS **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsLUKS), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsLUKS_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsLUKS(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsNfs_members(Visitor *v, BlockdevCreateOptionsNfs *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsNfs(v, "location", &obj->location, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsNfs(Visitor *v, const char *name, BlockdevCreateOptionsNfs **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsNfs), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsNfs_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsNfs(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsParallels_members(Visitor *v, BlockdevCreateOptionsParallels *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevRef(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "cluster-size", &obj->has_cluster_size)) {
        visit_type_size(v, "cluster-size", &obj->cluster_size, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsParallels(Visitor *v, const char *name, BlockdevCreateOptionsParallels **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsParallels), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsParallels_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsParallels(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsQcow_members(Visitor *v, BlockdevCreateOptionsQcow *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevRef(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "backing-file", &obj->has_backing_file)) {
        visit_type_str(v, "backing-file", &obj->backing_file, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "encrypt", &obj->has_encrypt)) {
        visit_type_QCryptoBlockCreateOptions(v, "encrypt", &obj->encrypt, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsQcow(Visitor *v, const char *name, BlockdevCreateOptionsQcow **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsQcow), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsQcow_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsQcow(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevQcow2Version(Visitor *v, const char *name, BlockdevQcow2Version *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevQcow2Version_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevCreateOptionsQcow2_members(Visitor *v, BlockdevCreateOptionsQcow2 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevRef(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "version", &obj->has_version)) {
        visit_type_BlockdevQcow2Version(v, "version", &obj->version, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "backing-file", &obj->has_backing_file)) {
        visit_type_str(v, "backing-file", &obj->backing_file, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "backing-fmt", &obj->has_backing_fmt)) {
        visit_type_BlockdevDriver(v, "backing-fmt", &obj->backing_fmt, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "encrypt", &obj->has_encrypt)) {
        visit_type_QCryptoBlockCreateOptions(v, "encrypt", &obj->encrypt, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cluster-size", &obj->has_cluster_size)) {
        visit_type_size(v, "cluster-size", &obj->cluster_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "preallocation", &obj->has_preallocation)) {
        visit_type_PreallocMode(v, "preallocation", &obj->preallocation, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "lazy-refcounts", &obj->has_lazy_refcounts)) {
        visit_type_bool(v, "lazy-refcounts", &obj->lazy_refcounts, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "refcount-bits", &obj->has_refcount_bits)) {
        visit_type_int(v, "refcount-bits", &obj->refcount_bits, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsQcow2(Visitor *v, const char *name, BlockdevCreateOptionsQcow2 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsQcow2), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsQcow2_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsQcow2(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsQed_members(Visitor *v, BlockdevCreateOptionsQed *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevRef(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "backing-file", &obj->has_backing_file)) {
        visit_type_str(v, "backing-file", &obj->backing_file, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "backing-fmt", &obj->has_backing_fmt)) {
        visit_type_BlockdevDriver(v, "backing-fmt", &obj->backing_fmt, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cluster-size", &obj->has_cluster_size)) {
        visit_type_size(v, "cluster-size", &obj->cluster_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "table-size", &obj->has_table_size)) {
        visit_type_int(v, "table-size", &obj->table_size, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsQed(Visitor *v, const char *name, BlockdevCreateOptionsQed **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsQed), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsQed_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsQed(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsRbd_members(Visitor *v, BlockdevCreateOptionsRbd *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsRbd(v, "location", &obj->location, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "cluster-size", &obj->has_cluster_size)) {
        visit_type_size(v, "cluster-size", &obj->cluster_size, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsRbd(Visitor *v, const char *name, BlockdevCreateOptionsRbd **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsRbd), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsRbd_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsRbd(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SheepdogRedundancyType(Visitor *v, const char *name, SheepdogRedundancyType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &SheepdogRedundancyType_lookup, errp);
    *obj = value;
}

void visit_type_SheepdogRedundancyFull_members(Visitor *v, SheepdogRedundancyFull *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "copies", &obj->copies, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SheepdogRedundancyFull(Visitor *v, const char *name, SheepdogRedundancyFull **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SheepdogRedundancyFull), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SheepdogRedundancyFull_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SheepdogRedundancyFull(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SheepdogRedundancyErasureCoded_members(Visitor *v, SheepdogRedundancyErasureCoded *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "data-strips", &obj->data_strips, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "parity-strips", &obj->parity_strips, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SheepdogRedundancyErasureCoded(Visitor *v, const char *name, SheepdogRedundancyErasureCoded **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SheepdogRedundancyErasureCoded), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SheepdogRedundancyErasureCoded_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SheepdogRedundancyErasureCoded(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_SheepdogRedundancy_base_members(Visitor *v, q_obj_SheepdogRedundancy_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SheepdogRedundancyType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SheepdogRedundancy_members(Visitor *v, SheepdogRedundancy *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_SheepdogRedundancy_base_members(v, (q_obj_SheepdogRedundancy_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case SHEEPDOG_REDUNDANCY_TYPE_FULL:
        visit_type_SheepdogRedundancyFull_members(v, &obj->u.full, &err);
        break;
    case SHEEPDOG_REDUNDANCY_TYPE_ERASURE_CODED:
        visit_type_SheepdogRedundancyErasureCoded_members(v, &obj->u.erasure_coded, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_SheepdogRedundancy(Visitor *v, const char *name, SheepdogRedundancy **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SheepdogRedundancy), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SheepdogRedundancy_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SheepdogRedundancy(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsSheepdog_members(Visitor *v, BlockdevCreateOptionsSheepdog *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsSheepdog(v, "location", &obj->location, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "backing-file", &obj->has_backing_file)) {
        visit_type_str(v, "backing-file", &obj->backing_file, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "preallocation", &obj->has_preallocation)) {
        visit_type_PreallocMode(v, "preallocation", &obj->preallocation, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "redundancy", &obj->has_redundancy)) {
        visit_type_SheepdogRedundancy(v, "redundancy", &obj->redundancy, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "object-size", &obj->has_object_size)) {
        visit_type_size(v, "object-size", &obj->object_size, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsSheepdog(Visitor *v, const char *name, BlockdevCreateOptionsSheepdog **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsSheepdog), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsSheepdog_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsSheepdog(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsSsh_members(Visitor *v, BlockdevCreateOptionsSsh *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevOptionsSsh(v, "location", &obj->location, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsSsh(Visitor *v, const char *name, BlockdevCreateOptionsSsh **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsSsh), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsSsh_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsSsh(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsVdi_members(Visitor *v, BlockdevCreateOptionsVdi *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevRef(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "preallocation", &obj->has_preallocation)) {
        visit_type_PreallocMode(v, "preallocation", &obj->preallocation, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsVdi(Visitor *v, const char *name, BlockdevCreateOptionsVdi **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsVdi), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsVdi_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsVdi(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevVhdxSubformat(Visitor *v, const char *name, BlockdevVhdxSubformat *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevVhdxSubformat_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevCreateOptionsVhdx_members(Visitor *v, BlockdevCreateOptionsVhdx *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevRef(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "log-size", &obj->has_log_size)) {
        visit_type_size(v, "log-size", &obj->log_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "block-size", &obj->has_block_size)) {
        visit_type_size(v, "block-size", &obj->block_size, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "subformat", &obj->has_subformat)) {
        visit_type_BlockdevVhdxSubformat(v, "subformat", &obj->subformat, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "block-state-zero", &obj->has_block_state_zero)) {
        visit_type_bool(v, "block-state-zero", &obj->block_state_zero, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsVhdx(Visitor *v, const char *name, BlockdevCreateOptionsVhdx **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsVhdx), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsVhdx_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsVhdx(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevVpcSubformat(Visitor *v, const char *name, BlockdevVpcSubformat *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevVpcSubformat_lookup, errp);
    *obj = value;
}

void visit_type_BlockdevCreateOptionsVpc_members(Visitor *v, BlockdevCreateOptionsVpc *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevRef(v, "file", &obj->file, &err);
    if (err) {
        goto out;
    }
    visit_type_size(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "subformat", &obj->has_subformat)) {
        visit_type_BlockdevVpcSubformat(v, "subformat", &obj->subformat, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "force-size", &obj->has_force_size)) {
        visit_type_bool(v, "force-size", &obj->force_size, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptionsVpc(Visitor *v, const char *name, BlockdevCreateOptionsVpc **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptionsVpc), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptionsVpc_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptionsVpc(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateNotSupported_members(Visitor *v, BlockdevCreateNotSupported *obj, Error **errp)
{
    Error *err = NULL;

    error_propagate(errp, err);
}

void visit_type_BlockdevCreateNotSupported(Visitor *v, const char *name, BlockdevCreateNotSupported **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateNotSupported), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateNotSupported_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateNotSupported(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BlockdevCreateOptions_base_members(Visitor *v, q_obj_BlockdevCreateOptions_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockdevDriver(v, "driver", &obj->driver, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptions_members(Visitor *v, BlockdevCreateOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_BlockdevCreateOptions_base_members(v, (q_obj_BlockdevCreateOptions_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->driver) {
    case BLOCKDEV_DRIVER_BLKDEBUG:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.blkdebug, &err);
        break;
    case BLOCKDEV_DRIVER_BLKVERIFY:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.blkverify, &err);
        break;
    case BLOCKDEV_DRIVER_BOCHS:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.bochs, &err);
        break;
    case BLOCKDEV_DRIVER_CLOOP:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.cloop, &err);
        break;
    case BLOCKDEV_DRIVER_DMG:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.dmg, &err);
        break;
    case BLOCKDEV_DRIVER_FILE:
        visit_type_BlockdevCreateOptionsFile_members(v, &obj->u.file, &err);
        break;
    case BLOCKDEV_DRIVER_FTP:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.ftp, &err);
        break;
    case BLOCKDEV_DRIVER_FTPS:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.ftps, &err);
        break;
    case BLOCKDEV_DRIVER_GLUSTER:
        visit_type_BlockdevCreateOptionsGluster_members(v, &obj->u.gluster, &err);
        break;
    case BLOCKDEV_DRIVER_HOST_CDROM:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.host_cdrom, &err);
        break;
    case BLOCKDEV_DRIVER_HOST_DEVICE:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.host_device, &err);
        break;
    case BLOCKDEV_DRIVER_HTTP:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.http, &err);
        break;
    case BLOCKDEV_DRIVER_HTTPS:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.https, &err);
        break;
    case BLOCKDEV_DRIVER_ISCSI:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.iscsi, &err);
        break;
    case BLOCKDEV_DRIVER_LUKS:
        visit_type_BlockdevCreateOptionsLUKS_members(v, &obj->u.luks, &err);
        break;
    case BLOCKDEV_DRIVER_NBD:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.nbd, &err);
        break;
    case BLOCKDEV_DRIVER_NFS:
        visit_type_BlockdevCreateOptionsNfs_members(v, &obj->u.nfs, &err);
        break;
    case BLOCKDEV_DRIVER_NULL_AIO:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.null_aio, &err);
        break;
    case BLOCKDEV_DRIVER_NULL_CO:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.null_co, &err);
        break;
    case BLOCKDEV_DRIVER_NVME:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.nvme, &err);
        break;
    case BLOCKDEV_DRIVER_PARALLELS:
        visit_type_BlockdevCreateOptionsParallels_members(v, &obj->u.parallels, &err);
        break;
    case BLOCKDEV_DRIVER_QCOW:
        visit_type_BlockdevCreateOptionsQcow_members(v, &obj->u.qcow, &err);
        break;
    case BLOCKDEV_DRIVER_QCOW2:
        visit_type_BlockdevCreateOptionsQcow2_members(v, &obj->u.qcow2, &err);
        break;
    case BLOCKDEV_DRIVER_QED:
        visit_type_BlockdevCreateOptionsQed_members(v, &obj->u.qed, &err);
        break;
    case BLOCKDEV_DRIVER_QUORUM:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.quorum, &err);
        break;
    case BLOCKDEV_DRIVER_RAW:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.raw, &err);
        break;
    case BLOCKDEV_DRIVER_RBD:
        visit_type_BlockdevCreateOptionsRbd_members(v, &obj->u.rbd, &err);
        break;
    case BLOCKDEV_DRIVER_REPLICATION:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.replication, &err);
        break;
    case BLOCKDEV_DRIVER_SHEEPDOG:
        visit_type_BlockdevCreateOptionsSheepdog_members(v, &obj->u.sheepdog, &err);
        break;
    case BLOCKDEV_DRIVER_SSH:
        visit_type_BlockdevCreateOptionsSsh_members(v, &obj->u.ssh, &err);
        break;
    case BLOCKDEV_DRIVER_THROTTLE:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.throttle, &err);
        break;
    case BLOCKDEV_DRIVER_VDI:
        visit_type_BlockdevCreateOptionsVdi_members(v, &obj->u.vdi, &err);
        break;
    case BLOCKDEV_DRIVER_VHDX:
        visit_type_BlockdevCreateOptionsVhdx_members(v, &obj->u.vhdx, &err);
        break;
    case BLOCKDEV_DRIVER_VMDK:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.vmdk, &err);
        break;
    case BLOCKDEV_DRIVER_VPC:
        visit_type_BlockdevCreateOptionsVpc_members(v, &obj->u.vpc, &err);
        break;
    case BLOCKDEV_DRIVER_VVFAT:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.vvfat, &err);
        break;
    case BLOCKDEV_DRIVER_VXHS:
        visit_type_BlockdevCreateNotSupported_members(v, &obj->u.vxhs, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevCreateOptions(Visitor *v, const char *name, BlockdevCreateOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(BlockdevCreateOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_BlockdevCreateOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_BlockdevCreateOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_blockdev_open_tray_arg_members(Visitor *v, q_obj_blockdev_open_tray_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "id", &obj->has_id)) {
        visit_type_str(v, "id", &obj->id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "force", &obj->has_force)) {
        visit_type_bool(v, "force", &obj->force, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_blockdev_close_tray_arg_members(Visitor *v, q_obj_blockdev_close_tray_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "id", &obj->has_id)) {
        visit_type_str(v, "id", &obj->id, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_blockdev_remove_medium_arg_members(Visitor *v, q_obj_blockdev_remove_medium_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_blockdev_insert_medium_arg_members(Visitor *v, q_obj_blockdev_insert_medium_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "node-name", &obj->node_name, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockdevChangeReadOnlyMode(Visitor *v, const char *name, BlockdevChangeReadOnlyMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockdevChangeReadOnlyMode_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_blockdev_change_medium_arg_members(Visitor *v, q_obj_blockdev_change_medium_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "id", &obj->has_id)) {
        visit_type_str(v, "id", &obj->id, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "format", &obj->has_format)) {
        visit_type_str(v, "format", &obj->format, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "read-only-mode", &obj->has_read_only_mode)) {
        visit_type_BlockdevChangeReadOnlyMode(v, "read-only-mode", &obj->read_only_mode, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_BlockErrorAction(Visitor *v, const char *name, BlockErrorAction *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &BlockErrorAction_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_BLOCK_IMAGE_CORRUPTED_arg_members(Visitor *v, q_obj_BLOCK_IMAGE_CORRUPTED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "node-name", &obj->has_node_name)) {
        visit_type_str(v, "node-name", &obj->node_name, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "msg", &obj->msg, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "offset", &obj->has_offset)) {
        visit_type_int(v, "offset", &obj->offset, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "size", &obj->has_size)) {
        visit_type_int(v, "size", &obj->size, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_bool(v, "fatal", &obj->fatal, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BLOCK_IO_ERROR_arg_members(Visitor *v, q_obj_BLOCK_IO_ERROR_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "node-name", &obj->has_node_name)) {
        visit_type_str(v, "node-name", &obj->node_name, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_IoOperationType(v, "operation", &obj->operation, &err);
    if (err) {
        goto out;
    }
    visit_type_BlockErrorAction(v, "action", &obj->action, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "nospace", &obj->has_nospace)) {
        visit_type_bool(v, "nospace", &obj->nospace, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "reason", &obj->reason, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BLOCK_JOB_COMPLETED_arg_members(Visitor *v, q_obj_BLOCK_JOB_COMPLETED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockJobType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "len", &obj->len, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "offset", &obj->offset, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "speed", &obj->speed, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "error", &obj->has_error)) {
        visit_type_str(v, "error", &obj->error, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BLOCK_JOB_CANCELLED_arg_members(Visitor *v, q_obj_BLOCK_JOB_CANCELLED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockJobType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "len", &obj->len, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "offset", &obj->offset, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "speed", &obj->speed, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BLOCK_JOB_ERROR_arg_members(Visitor *v, q_obj_BLOCK_JOB_ERROR_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_IoOperationType(v, "operation", &obj->operation, &err);
    if (err) {
        goto out;
    }
    visit_type_BlockErrorAction(v, "action", &obj->action, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BLOCK_JOB_READY_arg_members(Visitor *v, q_obj_BLOCK_JOB_READY_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockJobType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "len", &obj->len, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "offset", &obj->offset, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "speed", &obj->speed, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_BLOCK_JOB_PENDING_arg_members(Visitor *v, q_obj_BLOCK_JOB_PENDING_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_BlockJobType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_PreallocMode(Visitor *v, const char *name, PreallocMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &PreallocMode_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_BLOCK_WRITE_THRESHOLD_arg_members(Visitor *v, q_obj_BLOCK_WRITE_THRESHOLD_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "node-name", &obj->node_name, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "amount-exceeded", &obj->amount_exceeded, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "write-threshold", &obj->write_threshold, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_block_set_write_threshold_arg_members(Visitor *v, q_obj_block_set_write_threshold_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "node-name", &obj->node_name, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "write-threshold", &obj->write_threshold, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_x_blockdev_change_arg_members(Visitor *v, q_obj_x_blockdev_change_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "parent", &obj->parent, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "child", &obj->has_child)) {
        visit_type_str(v, "child", &obj->child, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "node", &obj->has_node)) {
        visit_type_str(v, "node", &obj->node, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_x_blockdev_set_iothread_arg_members(Visitor *v, q_obj_x_blockdev_set_iothread_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "node-name", &obj->node_name, &err);
    if (err) {
        goto out;
    }
    visit_type_StrOrNull(v, "iothread", &obj->iothread, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "force", &obj->has_force)) {
        visit_type_bool(v, "force", &obj->force, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_block_core_c;
