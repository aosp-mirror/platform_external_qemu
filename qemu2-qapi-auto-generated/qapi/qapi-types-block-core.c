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
#include "qapi-types-block-core.h"
#include "qapi-visit-block-core.h"

void qapi_free_SnapshotInfo(SnapshotInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SnapshotInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ImageInfoSpecificQCow2EncryptionBase(ImageInfoSpecificQCow2EncryptionBase *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ImageInfoSpecificQCow2EncryptionBase(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ImageInfoSpecificQCow2Encryption(ImageInfoSpecificQCow2Encryption *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ImageInfoSpecificQCow2Encryption(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ImageInfoSpecificQCow2(ImageInfoSpecificQCow2 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ImageInfoSpecificQCow2(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ImageInfoList(ImageInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ImageInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ImageInfoSpecificVmdk(ImageInfoSpecificVmdk *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ImageInfoSpecificVmdk(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup ImageInfoSpecificKind_lookup = {
    .array = (const char *const[]) {
        [IMAGE_INFO_SPECIFIC_KIND_QCOW2] = "qcow2",
        [IMAGE_INFO_SPECIFIC_KIND_VMDK] = "vmdk",
        [IMAGE_INFO_SPECIFIC_KIND_LUKS] = "luks",
    },
    .size = IMAGE_INFO_SPECIFIC_KIND__MAX
};

void qapi_free_ImageInfoSpecific(ImageInfoSpecific *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ImageInfoSpecific(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SnapshotInfoList(SnapshotInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SnapshotInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ImageInfo(ImageInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ImageInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ImageCheck(ImageCheck *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ImageCheck(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_MapEntry(MapEntry *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_MapEntry(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCacheInfo(BlockdevCacheInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCacheInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockDeviceInfo(BlockDeviceInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDeviceInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlockDeviceIoStatus_lookup = {
    .array = (const char *const[]) {
        [BLOCK_DEVICE_IO_STATUS_OK] = "ok",
        [BLOCK_DEVICE_IO_STATUS_FAILED] = "failed",
        [BLOCK_DEVICE_IO_STATUS_NOSPACE] = "nospace",
    },
    .size = BLOCK_DEVICE_IO_STATUS__MAX
};

void qapi_free_BlockDeviceMapEntry(BlockDeviceMapEntry *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDeviceMapEntry(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup DirtyBitmapStatus_lookup = {
    .array = (const char *const[]) {
        [DIRTY_BITMAP_STATUS_ACTIVE] = "active",
        [DIRTY_BITMAP_STATUS_DISABLED] = "disabled",
        [DIRTY_BITMAP_STATUS_FROZEN] = "frozen",
        [DIRTY_BITMAP_STATUS_LOCKED] = "locked",
    },
    .size = DIRTY_BITMAP_STATUS__MAX
};

void qapi_free_BlockDirtyInfo(BlockDirtyInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDirtyInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockLatencyHistogramInfo(BlockLatencyHistogramInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockLatencyHistogramInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockDirtyInfoList(BlockDirtyInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDirtyInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockInfo(BlockInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockMeasureInfo(BlockMeasureInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockMeasureInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockInfoList(BlockInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockDeviceTimedStats(BlockDeviceTimedStats *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDeviceTimedStats(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockDeviceTimedStatsList(BlockDeviceTimedStatsList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDeviceTimedStatsList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockDeviceStats(BlockDeviceStats *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDeviceStats(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockStats(BlockStats *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockStats(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockStatsList(BlockStatsList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockStatsList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlockdevOnError_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_ON_ERROR_REPORT] = "report",
        [BLOCKDEV_ON_ERROR_IGNORE] = "ignore",
        [BLOCKDEV_ON_ERROR_ENOSPC] = "enospc",
        [BLOCKDEV_ON_ERROR_STOP] = "stop",
        [BLOCKDEV_ON_ERROR_AUTO] = "auto",
    },
    .size = BLOCKDEV_ON_ERROR__MAX
};

const QEnumLookup MirrorSyncMode_lookup = {
    .array = (const char *const[]) {
        [MIRROR_SYNC_MODE_TOP] = "top",
        [MIRROR_SYNC_MODE_FULL] = "full",
        [MIRROR_SYNC_MODE_NONE] = "none",
        [MIRROR_SYNC_MODE_INCREMENTAL] = "incremental",
    },
    .size = MIRROR_SYNC_MODE__MAX
};

const QEnumLookup BlockJobType_lookup = {
    .array = (const char *const[]) {
        [BLOCK_JOB_TYPE_COMMIT] = "commit",
        [BLOCK_JOB_TYPE_STREAM] = "stream",
        [BLOCK_JOB_TYPE_MIRROR] = "mirror",
        [BLOCK_JOB_TYPE_BACKUP] = "backup",
    },
    .size = BLOCK_JOB_TYPE__MAX
};

const QEnumLookup BlockJobVerb_lookup = {
    .array = (const char *const[]) {
        [BLOCK_JOB_VERB_CANCEL] = "cancel",
        [BLOCK_JOB_VERB_PAUSE] = "pause",
        [BLOCK_JOB_VERB_RESUME] = "resume",
        [BLOCK_JOB_VERB_SET_SPEED] = "set-speed",
        [BLOCK_JOB_VERB_COMPLETE] = "complete",
        [BLOCK_JOB_VERB_DISMISS] = "dismiss",
        [BLOCK_JOB_VERB_FINALIZE] = "finalize",
    },
    .size = BLOCK_JOB_VERB__MAX
};

const QEnumLookup BlockJobStatus_lookup = {
    .array = (const char *const[]) {
        [BLOCK_JOB_STATUS_UNDEFINED] = "undefined",
        [BLOCK_JOB_STATUS_CREATED] = "created",
        [BLOCK_JOB_STATUS_RUNNING] = "running",
        [BLOCK_JOB_STATUS_PAUSED] = "paused",
        [BLOCK_JOB_STATUS_READY] = "ready",
        [BLOCK_JOB_STATUS_STANDBY] = "standby",
        [BLOCK_JOB_STATUS_WAITING] = "waiting",
        [BLOCK_JOB_STATUS_PENDING] = "pending",
        [BLOCK_JOB_STATUS_ABORTING] = "aborting",
        [BLOCK_JOB_STATUS_CONCLUDED] = "concluded",
        [BLOCK_JOB_STATUS_NULL] = "null",
    },
    .size = BLOCK_JOB_STATUS__MAX
};

void qapi_free_BlockJobInfo(BlockJobInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockJobInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockJobInfoList(BlockJobInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockJobInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup NewImageMode_lookup = {
    .array = (const char *const[]) {
        [NEW_IMAGE_MODE_EXISTING] = "existing",
        [NEW_IMAGE_MODE_ABSOLUTE_PATHS] = "absolute-paths",
    },
    .size = NEW_IMAGE_MODE__MAX
};

void qapi_free_BlockdevSnapshotSync(BlockdevSnapshotSync *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevSnapshotSync(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevSnapshot(BlockdevSnapshot *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevSnapshot(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_DriveBackup(DriveBackup *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_DriveBackup(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevBackup(BlockdevBackup *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevBackup(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockDeviceInfoList(BlockDeviceInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDeviceInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_DriveMirror(DriveMirror *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_DriveMirror(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockDirtyBitmap(BlockDirtyBitmap *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDirtyBitmap(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockDirtyBitmapAdd(BlockDirtyBitmapAdd *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDirtyBitmapAdd(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockDirtyBitmapSha256(BlockDirtyBitmapSha256 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockDirtyBitmapSha256(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockIOThrottle(BlockIOThrottle *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockIOThrottle(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ThrottleLimits(ThrottleLimits *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ThrottleLimits(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlockdevDiscardOptions_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_DISCARD_OPTIONS_IGNORE] = "ignore",
        [BLOCKDEV_DISCARD_OPTIONS_UNMAP] = "unmap",
    },
    .size = BLOCKDEV_DISCARD_OPTIONS__MAX
};

const QEnumLookup BlockdevDetectZeroesOptions_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_DETECT_ZEROES_OPTIONS_OFF] = "off",
        [BLOCKDEV_DETECT_ZEROES_OPTIONS_ON] = "on",
        [BLOCKDEV_DETECT_ZEROES_OPTIONS_UNMAP] = "unmap",
    },
    .size = BLOCKDEV_DETECT_ZEROES_OPTIONS__MAX
};

const QEnumLookup BlockdevAioOptions_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_AIO_OPTIONS_THREADS] = "threads",
        [BLOCKDEV_AIO_OPTIONS_NATIVE] = "native",
    },
    .size = BLOCKDEV_AIO_OPTIONS__MAX
};

void qapi_free_BlockdevCacheOptions(BlockdevCacheOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCacheOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlockdevDriver_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_DRIVER_BLKDEBUG] = "blkdebug",
        [BLOCKDEV_DRIVER_BLKVERIFY] = "blkverify",
        [BLOCKDEV_DRIVER_BOCHS] = "bochs",
        [BLOCKDEV_DRIVER_CLOOP] = "cloop",
        [BLOCKDEV_DRIVER_DMG] = "dmg",
        [BLOCKDEV_DRIVER_FILE] = "file",
        [BLOCKDEV_DRIVER_FTP] = "ftp",
        [BLOCKDEV_DRIVER_FTPS] = "ftps",
        [BLOCKDEV_DRIVER_GLUSTER] = "gluster",
        [BLOCKDEV_DRIVER_HOST_CDROM] = "host_cdrom",
        [BLOCKDEV_DRIVER_HOST_DEVICE] = "host_device",
        [BLOCKDEV_DRIVER_HTTP] = "http",
        [BLOCKDEV_DRIVER_HTTPS] = "https",
        [BLOCKDEV_DRIVER_ISCSI] = "iscsi",
        [BLOCKDEV_DRIVER_LUKS] = "luks",
        [BLOCKDEV_DRIVER_NBD] = "nbd",
        [BLOCKDEV_DRIVER_NFS] = "nfs",
        [BLOCKDEV_DRIVER_NULL_AIO] = "null-aio",
        [BLOCKDEV_DRIVER_NULL_CO] = "null-co",
        [BLOCKDEV_DRIVER_NVME] = "nvme",
        [BLOCKDEV_DRIVER_PARALLELS] = "parallels",
        [BLOCKDEV_DRIVER_QCOW] = "qcow",
        [BLOCKDEV_DRIVER_QCOW2] = "qcow2",
        [BLOCKDEV_DRIVER_QED] = "qed",
        [BLOCKDEV_DRIVER_QUORUM] = "quorum",
        [BLOCKDEV_DRIVER_RAW] = "raw",
        [BLOCKDEV_DRIVER_RBD] = "rbd",
        [BLOCKDEV_DRIVER_REPLICATION] = "replication",
        [BLOCKDEV_DRIVER_SHEEPDOG] = "sheepdog",
        [BLOCKDEV_DRIVER_SSH] = "ssh",
        [BLOCKDEV_DRIVER_THROTTLE] = "throttle",
        [BLOCKDEV_DRIVER_VDI] = "vdi",
        [BLOCKDEV_DRIVER_VHDX] = "vhdx",
        [BLOCKDEV_DRIVER_VMDK] = "vmdk",
        [BLOCKDEV_DRIVER_VPC] = "vpc",
        [BLOCKDEV_DRIVER_VVFAT] = "vvfat",
        [BLOCKDEV_DRIVER_VXHS] = "vxhs",
    },
    .size = BLOCKDEV_DRIVER__MAX
};

void qapi_free_BlockdevOptionsFile(BlockdevOptionsFile *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsFile(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsNull(BlockdevOptionsNull *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsNull(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsNVMe(BlockdevOptionsNVMe *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsNVMe(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsVVFAT(BlockdevOptionsVVFAT *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsVVFAT(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsGenericFormat(BlockdevOptionsGenericFormat *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsGenericFormat(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsLUKS(BlockdevOptionsLUKS *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsLUKS(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsGenericCOWFormat(BlockdevOptionsGenericCOWFormat *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsGenericCOWFormat(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup Qcow2OverlapCheckMode_lookup = {
    .array = (const char *const[]) {
        [QCOW2_OVERLAP_CHECK_MODE_NONE] = "none",
        [QCOW2_OVERLAP_CHECK_MODE_CONSTANT] = "constant",
        [QCOW2_OVERLAP_CHECK_MODE_CACHED] = "cached",
        [QCOW2_OVERLAP_CHECK_MODE_ALL] = "all",
    },
    .size = QCOW2_OVERLAP_CHECK_MODE__MAX
};

void qapi_free_Qcow2OverlapCheckFlags(Qcow2OverlapCheckFlags *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_Qcow2OverlapCheckFlags(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_Qcow2OverlapChecks(Qcow2OverlapChecks *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_Qcow2OverlapChecks(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlockdevQcowEncryptionFormat_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_QCOW_ENCRYPTION_FORMAT_AES] = "aes",
    },
    .size = BLOCKDEV_QCOW_ENCRYPTION_FORMAT__MAX
};

void qapi_free_BlockdevQcowEncryption(BlockdevQcowEncryption *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevQcowEncryption(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsQcow(BlockdevOptionsQcow *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsQcow(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlockdevQcow2EncryptionFormat_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_QCOW2_ENCRYPTION_FORMAT_AES] = "aes",
        [BLOCKDEV_QCOW2_ENCRYPTION_FORMAT_LUKS] = "luks",
    },
    .size = BLOCKDEV_QCOW2_ENCRYPTION_FORMAT__MAX
};

void qapi_free_BlockdevQcow2Encryption(BlockdevQcow2Encryption *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevQcow2Encryption(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsQcow2(BlockdevOptionsQcow2 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsQcow2(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup SshHostKeyCheckMode_lookup = {
    .array = (const char *const[]) {
        [SSH_HOST_KEY_CHECK_MODE_NONE] = "none",
        [SSH_HOST_KEY_CHECK_MODE_HASH] = "hash",
        [SSH_HOST_KEY_CHECK_MODE_KNOWN_HOSTS] = "known_hosts",
    },
    .size = SSH_HOST_KEY_CHECK_MODE__MAX
};

const QEnumLookup SshHostKeyCheckHashType_lookup = {
    .array = (const char *const[]) {
        [SSH_HOST_KEY_CHECK_HASH_TYPE_MD5] = "md5",
        [SSH_HOST_KEY_CHECK_HASH_TYPE_SHA1] = "sha1",
    },
    .size = SSH_HOST_KEY_CHECK_HASH_TYPE__MAX
};

void qapi_free_SshHostKeyHash(SshHostKeyHash *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SshHostKeyHash(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SshHostKeyDummy(SshHostKeyDummy *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SshHostKeyDummy(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SshHostKeyCheck(SshHostKeyCheck *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SshHostKeyCheck(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsSsh(BlockdevOptionsSsh *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsSsh(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlkdebugEvent_lookup = {
    .array = (const char *const[]) {
        [BLKDBG_L1_UPDATE] = "l1_update",
        [BLKDBG_L1_GROW_ALLOC_TABLE] = "l1_grow_alloc_table",
        [BLKDBG_L1_GROW_WRITE_TABLE] = "l1_grow_write_table",
        [BLKDBG_L1_GROW_ACTIVATE_TABLE] = "l1_grow_activate_table",
        [BLKDBG_L2_LOAD] = "l2_load",
        [BLKDBG_L2_UPDATE] = "l2_update",
        [BLKDBG_L2_UPDATE_COMPRESSED] = "l2_update_compressed",
        [BLKDBG_L2_ALLOC_COW_READ] = "l2_alloc_cow_read",
        [BLKDBG_L2_ALLOC_WRITE] = "l2_alloc_write",
        [BLKDBG_READ_AIO] = "read_aio",
        [BLKDBG_READ_BACKING_AIO] = "read_backing_aio",
        [BLKDBG_READ_COMPRESSED] = "read_compressed",
        [BLKDBG_WRITE_AIO] = "write_aio",
        [BLKDBG_WRITE_COMPRESSED] = "write_compressed",
        [BLKDBG_VMSTATE_LOAD] = "vmstate_load",
        [BLKDBG_VMSTATE_SAVE] = "vmstate_save",
        [BLKDBG_COW_READ] = "cow_read",
        [BLKDBG_COW_WRITE] = "cow_write",
        [BLKDBG_REFTABLE_LOAD] = "reftable_load",
        [BLKDBG_REFTABLE_GROW] = "reftable_grow",
        [BLKDBG_REFTABLE_UPDATE] = "reftable_update",
        [BLKDBG_REFBLOCK_LOAD] = "refblock_load",
        [BLKDBG_REFBLOCK_UPDATE] = "refblock_update",
        [BLKDBG_REFBLOCK_UPDATE_PART] = "refblock_update_part",
        [BLKDBG_REFBLOCK_ALLOC] = "refblock_alloc",
        [BLKDBG_REFBLOCK_ALLOC_HOOKUP] = "refblock_alloc_hookup",
        [BLKDBG_REFBLOCK_ALLOC_WRITE] = "refblock_alloc_write",
        [BLKDBG_REFBLOCK_ALLOC_WRITE_BLOCKS] = "refblock_alloc_write_blocks",
        [BLKDBG_REFBLOCK_ALLOC_WRITE_TABLE] = "refblock_alloc_write_table",
        [BLKDBG_REFBLOCK_ALLOC_SWITCH_TABLE] = "refblock_alloc_switch_table",
        [BLKDBG_CLUSTER_ALLOC] = "cluster_alloc",
        [BLKDBG_CLUSTER_ALLOC_BYTES] = "cluster_alloc_bytes",
        [BLKDBG_CLUSTER_FREE] = "cluster_free",
        [BLKDBG_FLUSH_TO_OS] = "flush_to_os",
        [BLKDBG_FLUSH_TO_DISK] = "flush_to_disk",
        [BLKDBG_PWRITEV_RMW_HEAD] = "pwritev_rmw_head",
        [BLKDBG_PWRITEV_RMW_AFTER_HEAD] = "pwritev_rmw_after_head",
        [BLKDBG_PWRITEV_RMW_TAIL] = "pwritev_rmw_tail",
        [BLKDBG_PWRITEV_RMW_AFTER_TAIL] = "pwritev_rmw_after_tail",
        [BLKDBG_PWRITEV] = "pwritev",
        [BLKDBG_PWRITEV_ZERO] = "pwritev_zero",
        [BLKDBG_PWRITEV_DONE] = "pwritev_done",
        [BLKDBG_EMPTY_IMAGE_PREPARE] = "empty_image_prepare",
        [BLKDBG_L1_SHRINK_WRITE_TABLE] = "l1_shrink_write_table",
        [BLKDBG_L1_SHRINK_FREE_L2_CLUSTERS] = "l1_shrink_free_l2_clusters",
        [BLKDBG_COR_WRITE] = "cor_write",
    },
    .size = BLKDBG__MAX
};

void qapi_free_BlkdebugInjectErrorOptions(BlkdebugInjectErrorOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlkdebugInjectErrorOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlkdebugSetStateOptions(BlkdebugSetStateOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlkdebugSetStateOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlkdebugInjectErrorOptionsList(BlkdebugInjectErrorOptionsList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlkdebugInjectErrorOptionsList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlkdebugSetStateOptionsList(BlkdebugSetStateOptionsList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlkdebugSetStateOptionsList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsBlkdebug(BlockdevOptionsBlkdebug *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsBlkdebug(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsBlkverify(BlockdevOptionsBlkverify *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsBlkverify(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup QuorumReadPattern_lookup = {
    .array = (const char *const[]) {
        [QUORUM_READ_PATTERN_QUORUM] = "quorum",
        [QUORUM_READ_PATTERN_FIFO] = "fifo",
    },
    .size = QUORUM_READ_PATTERN__MAX
};

void qapi_free_BlockdevRefList(BlockdevRefList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevRefList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsQuorum(BlockdevOptionsQuorum *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsQuorum(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SocketAddressList(SocketAddressList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SocketAddressList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsGluster(BlockdevOptionsGluster *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsGluster(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup IscsiTransport_lookup = {
    .array = (const char *const[]) {
        [ISCSI_TRANSPORT_TCP] = "tcp",
        [ISCSI_TRANSPORT_ISER] = "iser",
    },
    .size = ISCSI_TRANSPORT__MAX
};

const QEnumLookup IscsiHeaderDigest_lookup = {
    .array = (const char *const[]) {
        [QAPI_ISCSI_HEADER_DIGEST_CRC32C] = "crc32c",
        [QAPI_ISCSI_HEADER_DIGEST_NONE] = "none",
        [QAPI_ISCSI_HEADER_DIGEST_CRC32C_NONE] = "crc32c-none",
        [QAPI_ISCSI_HEADER_DIGEST_NONE_CRC32C] = "none-crc32c",
    },
    .size = QAPI_ISCSI_HEADER_DIGEST__MAX
};

void qapi_free_BlockdevOptionsIscsi(BlockdevOptionsIscsi *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsIscsi(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_InetSocketAddressBaseList(InetSocketAddressBaseList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_InetSocketAddressBaseList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsRbd(BlockdevOptionsRbd *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsRbd(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsSheepdog(BlockdevOptionsSheepdog *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsSheepdog(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup ReplicationMode_lookup = {
    .array = (const char *const[]) {
        [REPLICATION_MODE_PRIMARY] = "primary",
        [REPLICATION_MODE_SECONDARY] = "secondary",
    },
    .size = REPLICATION_MODE__MAX
};

void qapi_free_BlockdevOptionsReplication(BlockdevOptionsReplication *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsReplication(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup NFSTransport_lookup = {
    .array = (const char *const[]) {
        [NFS_TRANSPORT_INET] = "inet",
    },
    .size = NFS_TRANSPORT__MAX
};

void qapi_free_NFSServer(NFSServer *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_NFSServer(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsNfs(BlockdevOptionsNfs *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsNfs(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsCurlBase(BlockdevOptionsCurlBase *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsCurlBase(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsCurlHttp(BlockdevOptionsCurlHttp *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsCurlHttp(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsCurlHttps(BlockdevOptionsCurlHttps *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsCurlHttps(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsCurlFtp(BlockdevOptionsCurlFtp *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsCurlFtp(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsCurlFtps(BlockdevOptionsCurlFtps *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsCurlFtps(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsNbd(BlockdevOptionsNbd *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsNbd(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsRaw(BlockdevOptionsRaw *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsRaw(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsVxHS(BlockdevOptionsVxHS *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsVxHS(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptionsThrottle(BlockdevOptionsThrottle *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptionsThrottle(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevOptions(BlockdevOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevRef(BlockdevRef *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevRef(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevRefOrNull(BlockdevRefOrNull *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevRefOrNull(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsFile(BlockdevCreateOptionsFile *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsFile(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsGluster(BlockdevCreateOptionsGluster *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsGluster(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsLUKS(BlockdevCreateOptionsLUKS *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsLUKS(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsNfs(BlockdevCreateOptionsNfs *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsNfs(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsParallels(BlockdevCreateOptionsParallels *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsParallels(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsQcow(BlockdevCreateOptionsQcow *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsQcow(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlockdevQcow2Version_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_QCOW2_VERSION_V2] = "v2",
        [BLOCKDEV_QCOW2_VERSION_V3] = "v3",
    },
    .size = BLOCKDEV_QCOW2_VERSION__MAX
};

void qapi_free_BlockdevCreateOptionsQcow2(BlockdevCreateOptionsQcow2 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsQcow2(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsQed(BlockdevCreateOptionsQed *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsQed(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsRbd(BlockdevCreateOptionsRbd *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsRbd(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup SheepdogRedundancyType_lookup = {
    .array = (const char *const[]) {
        [SHEEPDOG_REDUNDANCY_TYPE_FULL] = "full",
        [SHEEPDOG_REDUNDANCY_TYPE_ERASURE_CODED] = "erasure-coded",
    },
    .size = SHEEPDOG_REDUNDANCY_TYPE__MAX
};

void qapi_free_SheepdogRedundancyFull(SheepdogRedundancyFull *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SheepdogRedundancyFull(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SheepdogRedundancyErasureCoded(SheepdogRedundancyErasureCoded *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SheepdogRedundancyErasureCoded(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SheepdogRedundancy(SheepdogRedundancy *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SheepdogRedundancy(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsSheepdog(BlockdevCreateOptionsSheepdog *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsSheepdog(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsSsh(BlockdevCreateOptionsSsh *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsSsh(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptionsVdi(BlockdevCreateOptionsVdi *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsVdi(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlockdevVhdxSubformat_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_VHDX_SUBFORMAT_DYNAMIC] = "dynamic",
        [BLOCKDEV_VHDX_SUBFORMAT_FIXED] = "fixed",
    },
    .size = BLOCKDEV_VHDX_SUBFORMAT__MAX
};

void qapi_free_BlockdevCreateOptionsVhdx(BlockdevCreateOptionsVhdx *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsVhdx(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlockdevVpcSubformat_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_VPC_SUBFORMAT_DYNAMIC] = "dynamic",
        [BLOCKDEV_VPC_SUBFORMAT_FIXED] = "fixed",
    },
    .size = BLOCKDEV_VPC_SUBFORMAT__MAX
};

void qapi_free_BlockdevCreateOptionsVpc(BlockdevCreateOptionsVpc *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptionsVpc(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateNotSupported(BlockdevCreateNotSupported *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateNotSupported(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_BlockdevCreateOptions(BlockdevCreateOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_BlockdevCreateOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup BlockdevChangeReadOnlyMode_lookup = {
    .array = (const char *const[]) {
        [BLOCKDEV_CHANGE_READ_ONLY_MODE_RETAIN] = "retain",
        [BLOCKDEV_CHANGE_READ_ONLY_MODE_READ_ONLY] = "read-only",
        [BLOCKDEV_CHANGE_READ_ONLY_MODE_READ_WRITE] = "read-write",
    },
    .size = BLOCKDEV_CHANGE_READ_ONLY_MODE__MAX
};

const QEnumLookup BlockErrorAction_lookup = {
    .array = (const char *const[]) {
        [BLOCK_ERROR_ACTION_IGNORE] = "ignore",
        [BLOCK_ERROR_ACTION_REPORT] = "report",
        [BLOCK_ERROR_ACTION_STOP] = "stop",
    },
    .size = BLOCK_ERROR_ACTION__MAX
};

const QEnumLookup PreallocMode_lookup = {
    .array = (const char *const[]) {
        [PREALLOC_MODE_OFF] = "off",
        [PREALLOC_MODE_METADATA] = "metadata",
        [PREALLOC_MODE_FALLOC] = "falloc",
        [PREALLOC_MODE_FULL] = "full",
    },
    .size = PREALLOC_MODE__MAX
};
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_block_core_c;
