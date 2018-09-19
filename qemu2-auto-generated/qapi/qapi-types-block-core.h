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

#ifndef QAPI_TYPES_BLOCK_CORE_H
#define QAPI_TYPES_BLOCK_CORE_H

#include "qapi/qapi-builtin-types.h"
#include "qapi-types-common.h"
#include "qapi-types-crypto.h"
#include "qapi-types-sockets.h"

typedef struct SnapshotInfo SnapshotInfo;

typedef struct ImageInfoSpecificQCow2EncryptionBase ImageInfoSpecificQCow2EncryptionBase;

typedef struct ImageInfoSpecificQCow2Encryption ImageInfoSpecificQCow2Encryption;

typedef struct ImageInfoSpecificQCow2 ImageInfoSpecificQCow2;

typedef struct ImageInfoList ImageInfoList;

typedef struct ImageInfoSpecificVmdk ImageInfoSpecificVmdk;

typedef struct q_obj_ImageInfoSpecificQCow2_wrapper q_obj_ImageInfoSpecificQCow2_wrapper;

typedef struct q_obj_ImageInfoSpecificVmdk_wrapper q_obj_ImageInfoSpecificVmdk_wrapper;

typedef struct q_obj_QCryptoBlockInfoLUKS_wrapper q_obj_QCryptoBlockInfoLUKS_wrapper;

typedef enum ImageInfoSpecificKind {
    IMAGE_INFO_SPECIFIC_KIND_QCOW2 = 0,
    IMAGE_INFO_SPECIFIC_KIND_VMDK = 1,
    IMAGE_INFO_SPECIFIC_KIND_LUKS = 2,
    IMAGE_INFO_SPECIFIC_KIND__MAX = 3,
} ImageInfoSpecificKind;

#define ImageInfoSpecificKind_str(val) \
    qapi_enum_lookup(&ImageInfoSpecificKind_lookup, (val))

extern const QEnumLookup ImageInfoSpecificKind_lookup;

typedef struct ImageInfoSpecific ImageInfoSpecific;

typedef struct SnapshotInfoList SnapshotInfoList;

typedef struct ImageInfo ImageInfo;

typedef struct ImageCheck ImageCheck;

typedef struct MapEntry MapEntry;

typedef struct BlockdevCacheInfo BlockdevCacheInfo;

typedef struct BlockDeviceInfo BlockDeviceInfo;

typedef enum BlockDeviceIoStatus {
    BLOCK_DEVICE_IO_STATUS_OK = 0,
    BLOCK_DEVICE_IO_STATUS_FAILED = 1,
    BLOCK_DEVICE_IO_STATUS_NOSPACE = 2,
    BLOCK_DEVICE_IO_STATUS__MAX = 3,
} BlockDeviceIoStatus;

#define BlockDeviceIoStatus_str(val) \
    qapi_enum_lookup(&BlockDeviceIoStatus_lookup, (val))

extern const QEnumLookup BlockDeviceIoStatus_lookup;

typedef struct BlockDeviceMapEntry BlockDeviceMapEntry;

typedef enum DirtyBitmapStatus {
    DIRTY_BITMAP_STATUS_ACTIVE = 0,
    DIRTY_BITMAP_STATUS_DISABLED = 1,
    DIRTY_BITMAP_STATUS_FROZEN = 2,
    DIRTY_BITMAP_STATUS_LOCKED = 3,
    DIRTY_BITMAP_STATUS__MAX = 4,
} DirtyBitmapStatus;

#define DirtyBitmapStatus_str(val) \
    qapi_enum_lookup(&DirtyBitmapStatus_lookup, (val))

extern const QEnumLookup DirtyBitmapStatus_lookup;

typedef struct BlockDirtyInfo BlockDirtyInfo;

typedef struct BlockLatencyHistogramInfo BlockLatencyHistogramInfo;

typedef struct q_obj_x_block_latency_histogram_set_arg q_obj_x_block_latency_histogram_set_arg;

typedef struct BlockDirtyInfoList BlockDirtyInfoList;

typedef struct BlockInfo BlockInfo;

typedef struct BlockMeasureInfo BlockMeasureInfo;

typedef struct BlockInfoList BlockInfoList;

typedef struct BlockDeviceTimedStats BlockDeviceTimedStats;

typedef struct BlockDeviceTimedStatsList BlockDeviceTimedStatsList;

typedef struct BlockDeviceStats BlockDeviceStats;

typedef struct BlockStats BlockStats;

typedef struct q_obj_query_blockstats_arg q_obj_query_blockstats_arg;

typedef struct BlockStatsList BlockStatsList;

typedef enum BlockdevOnError {
    BLOCKDEV_ON_ERROR_REPORT = 0,
    BLOCKDEV_ON_ERROR_IGNORE = 1,
    BLOCKDEV_ON_ERROR_ENOSPC = 2,
    BLOCKDEV_ON_ERROR_STOP = 3,
    BLOCKDEV_ON_ERROR_AUTO = 4,
    BLOCKDEV_ON_ERROR__MAX = 5,
} BlockdevOnError;

#define BlockdevOnError_str(val) \
    qapi_enum_lookup(&BlockdevOnError_lookup, (val))

extern const QEnumLookup BlockdevOnError_lookup;

typedef enum MirrorSyncMode {
    MIRROR_SYNC_MODE_TOP = 0,
    MIRROR_SYNC_MODE_FULL = 1,
    MIRROR_SYNC_MODE_NONE = 2,
    MIRROR_SYNC_MODE_INCREMENTAL = 3,
    MIRROR_SYNC_MODE__MAX = 4,
} MirrorSyncMode;

#define MirrorSyncMode_str(val) \
    qapi_enum_lookup(&MirrorSyncMode_lookup, (val))

extern const QEnumLookup MirrorSyncMode_lookup;

typedef enum BlockJobType {
    BLOCK_JOB_TYPE_COMMIT = 0,
    BLOCK_JOB_TYPE_STREAM = 1,
    BLOCK_JOB_TYPE_MIRROR = 2,
    BLOCK_JOB_TYPE_BACKUP = 3,
    BLOCK_JOB_TYPE__MAX = 4,
} BlockJobType;

#define BlockJobType_str(val) \
    qapi_enum_lookup(&BlockJobType_lookup, (val))

extern const QEnumLookup BlockJobType_lookup;

typedef enum BlockJobVerb {
    BLOCK_JOB_VERB_CANCEL = 0,
    BLOCK_JOB_VERB_PAUSE = 1,
    BLOCK_JOB_VERB_RESUME = 2,
    BLOCK_JOB_VERB_SET_SPEED = 3,
    BLOCK_JOB_VERB_COMPLETE = 4,
    BLOCK_JOB_VERB_DISMISS = 5,
    BLOCK_JOB_VERB_FINALIZE = 6,
    BLOCK_JOB_VERB__MAX = 7,
} BlockJobVerb;

#define BlockJobVerb_str(val) \
    qapi_enum_lookup(&BlockJobVerb_lookup, (val))

extern const QEnumLookup BlockJobVerb_lookup;

typedef enum BlockJobStatus {
    BLOCK_JOB_STATUS_UNDEFINED = 0,
    BLOCK_JOB_STATUS_CREATED = 1,
    BLOCK_JOB_STATUS_RUNNING = 2,
    BLOCK_JOB_STATUS_PAUSED = 3,
    BLOCK_JOB_STATUS_READY = 4,
    BLOCK_JOB_STATUS_STANDBY = 5,
    BLOCK_JOB_STATUS_WAITING = 6,
    BLOCK_JOB_STATUS_PENDING = 7,
    BLOCK_JOB_STATUS_ABORTING = 8,
    BLOCK_JOB_STATUS_CONCLUDED = 9,
    BLOCK_JOB_STATUS_NULL = 10,
    BLOCK_JOB_STATUS__MAX = 11,
} BlockJobStatus;

#define BlockJobStatus_str(val) \
    qapi_enum_lookup(&BlockJobStatus_lookup, (val))

extern const QEnumLookup BlockJobStatus_lookup;

typedef struct BlockJobInfo BlockJobInfo;

typedef struct BlockJobInfoList BlockJobInfoList;

typedef struct q_obj_block_passwd_arg q_obj_block_passwd_arg;

typedef struct q_obj_block_resize_arg q_obj_block_resize_arg;

typedef enum NewImageMode {
    NEW_IMAGE_MODE_EXISTING = 0,
    NEW_IMAGE_MODE_ABSOLUTE_PATHS = 1,
    NEW_IMAGE_MODE__MAX = 2,
} NewImageMode;

#define NewImageMode_str(val) \
    qapi_enum_lookup(&NewImageMode_lookup, (val))

extern const QEnumLookup NewImageMode_lookup;

typedef struct BlockdevSnapshotSync BlockdevSnapshotSync;

typedef struct BlockdevSnapshot BlockdevSnapshot;

typedef struct DriveBackup DriveBackup;

typedef struct BlockdevBackup BlockdevBackup;

typedef struct q_obj_change_backing_file_arg q_obj_change_backing_file_arg;

typedef struct q_obj_block_commit_arg q_obj_block_commit_arg;

typedef struct BlockDeviceInfoList BlockDeviceInfoList;

typedef struct DriveMirror DriveMirror;

typedef struct BlockDirtyBitmap BlockDirtyBitmap;

typedef struct BlockDirtyBitmapAdd BlockDirtyBitmapAdd;

typedef struct BlockDirtyBitmapSha256 BlockDirtyBitmapSha256;

typedef struct q_obj_blockdev_mirror_arg q_obj_blockdev_mirror_arg;

typedef struct BlockIOThrottle BlockIOThrottle;

typedef struct ThrottleLimits ThrottleLimits;

typedef struct q_obj_block_stream_arg q_obj_block_stream_arg;

typedef struct q_obj_block_job_set_speed_arg q_obj_block_job_set_speed_arg;

typedef struct q_obj_block_job_cancel_arg q_obj_block_job_cancel_arg;

typedef struct q_obj_block_job_pause_arg q_obj_block_job_pause_arg;

typedef struct q_obj_block_job_resume_arg q_obj_block_job_resume_arg;

typedef struct q_obj_block_job_complete_arg q_obj_block_job_complete_arg;

typedef struct q_obj_block_job_dismiss_arg q_obj_block_job_dismiss_arg;

typedef struct q_obj_block_job_finalize_arg q_obj_block_job_finalize_arg;

typedef enum BlockdevDiscardOptions {
    BLOCKDEV_DISCARD_OPTIONS_IGNORE = 0,
    BLOCKDEV_DISCARD_OPTIONS_UNMAP = 1,
    BLOCKDEV_DISCARD_OPTIONS__MAX = 2,
} BlockdevDiscardOptions;

#define BlockdevDiscardOptions_str(val) \
    qapi_enum_lookup(&BlockdevDiscardOptions_lookup, (val))

extern const QEnumLookup BlockdevDiscardOptions_lookup;

typedef enum BlockdevDetectZeroesOptions {
    BLOCKDEV_DETECT_ZEROES_OPTIONS_OFF = 0,
    BLOCKDEV_DETECT_ZEROES_OPTIONS_ON = 1,
    BLOCKDEV_DETECT_ZEROES_OPTIONS_UNMAP = 2,
    BLOCKDEV_DETECT_ZEROES_OPTIONS__MAX = 3,
} BlockdevDetectZeroesOptions;

#define BlockdevDetectZeroesOptions_str(val) \
    qapi_enum_lookup(&BlockdevDetectZeroesOptions_lookup, (val))

extern const QEnumLookup BlockdevDetectZeroesOptions_lookup;

typedef enum BlockdevAioOptions {
    BLOCKDEV_AIO_OPTIONS_THREADS = 0,
    BLOCKDEV_AIO_OPTIONS_NATIVE = 1,
    BLOCKDEV_AIO_OPTIONS__MAX = 2,
} BlockdevAioOptions;

#define BlockdevAioOptions_str(val) \
    qapi_enum_lookup(&BlockdevAioOptions_lookup, (val))

extern const QEnumLookup BlockdevAioOptions_lookup;

typedef struct BlockdevCacheOptions BlockdevCacheOptions;

typedef enum BlockdevDriver {
    BLOCKDEV_DRIVER_BLKDEBUG = 0,
    BLOCKDEV_DRIVER_BLKVERIFY = 1,
    BLOCKDEV_DRIVER_BOCHS = 2,
    BLOCKDEV_DRIVER_CLOOP = 3,
    BLOCKDEV_DRIVER_DMG = 4,
    BLOCKDEV_DRIVER_FILE = 5,
    BLOCKDEV_DRIVER_FTP = 6,
    BLOCKDEV_DRIVER_FTPS = 7,
    BLOCKDEV_DRIVER_GLUSTER = 8,
    BLOCKDEV_DRIVER_HOST_CDROM = 9,
    BLOCKDEV_DRIVER_HOST_DEVICE = 10,
    BLOCKDEV_DRIVER_HTTP = 11,
    BLOCKDEV_DRIVER_HTTPS = 12,
    BLOCKDEV_DRIVER_ISCSI = 13,
    BLOCKDEV_DRIVER_LUKS = 14,
    BLOCKDEV_DRIVER_NBD = 15,
    BLOCKDEV_DRIVER_NFS = 16,
    BLOCKDEV_DRIVER_NULL_AIO = 17,
    BLOCKDEV_DRIVER_NULL_CO = 18,
    BLOCKDEV_DRIVER_NVME = 19,
    BLOCKDEV_DRIVER_PARALLELS = 20,
    BLOCKDEV_DRIVER_QCOW = 21,
    BLOCKDEV_DRIVER_QCOW2 = 22,
    BLOCKDEV_DRIVER_QED = 23,
    BLOCKDEV_DRIVER_QUORUM = 24,
    BLOCKDEV_DRIVER_RAW = 25,
    BLOCKDEV_DRIVER_RBD = 26,
    BLOCKDEV_DRIVER_REPLICATION = 27,
    BLOCKDEV_DRIVER_SHEEPDOG = 28,
    BLOCKDEV_DRIVER_SSH = 29,
    BLOCKDEV_DRIVER_THROTTLE = 30,
    BLOCKDEV_DRIVER_VDI = 31,
    BLOCKDEV_DRIVER_VHDX = 32,
    BLOCKDEV_DRIVER_VMDK = 33,
    BLOCKDEV_DRIVER_VPC = 34,
    BLOCKDEV_DRIVER_VVFAT = 35,
    BLOCKDEV_DRIVER_VXHS = 36,
    BLOCKDEV_DRIVER__MAX = 37,
} BlockdevDriver;

#define BlockdevDriver_str(val) \
    qapi_enum_lookup(&BlockdevDriver_lookup, (val))

extern const QEnumLookup BlockdevDriver_lookup;

typedef struct BlockdevOptionsFile BlockdevOptionsFile;

typedef struct BlockdevOptionsNull BlockdevOptionsNull;

typedef struct BlockdevOptionsNVMe BlockdevOptionsNVMe;

typedef struct BlockdevOptionsVVFAT BlockdevOptionsVVFAT;

typedef struct BlockdevOptionsGenericFormat BlockdevOptionsGenericFormat;

typedef struct BlockdevOptionsLUKS BlockdevOptionsLUKS;

typedef struct BlockdevOptionsGenericCOWFormat BlockdevOptionsGenericCOWFormat;

typedef enum Qcow2OverlapCheckMode {
    QCOW2_OVERLAP_CHECK_MODE_NONE = 0,
    QCOW2_OVERLAP_CHECK_MODE_CONSTANT = 1,
    QCOW2_OVERLAP_CHECK_MODE_CACHED = 2,
    QCOW2_OVERLAP_CHECK_MODE_ALL = 3,
    QCOW2_OVERLAP_CHECK_MODE__MAX = 4,
} Qcow2OverlapCheckMode;

#define Qcow2OverlapCheckMode_str(val) \
    qapi_enum_lookup(&Qcow2OverlapCheckMode_lookup, (val))

extern const QEnumLookup Qcow2OverlapCheckMode_lookup;

typedef struct Qcow2OverlapCheckFlags Qcow2OverlapCheckFlags;

typedef struct Qcow2OverlapChecks Qcow2OverlapChecks;

typedef enum BlockdevQcowEncryptionFormat {
    BLOCKDEV_QCOW_ENCRYPTION_FORMAT_AES = 0,
    BLOCKDEV_QCOW_ENCRYPTION_FORMAT__MAX = 1,
} BlockdevQcowEncryptionFormat;

#define BlockdevQcowEncryptionFormat_str(val) \
    qapi_enum_lookup(&BlockdevQcowEncryptionFormat_lookup, (val))

extern const QEnumLookup BlockdevQcowEncryptionFormat_lookup;

typedef struct q_obj_BlockdevQcowEncryption_base q_obj_BlockdevQcowEncryption_base;

typedef struct BlockdevQcowEncryption BlockdevQcowEncryption;

typedef struct BlockdevOptionsQcow BlockdevOptionsQcow;

typedef enum BlockdevQcow2EncryptionFormat {
    BLOCKDEV_QCOW2_ENCRYPTION_FORMAT_AES = 0,
    BLOCKDEV_QCOW2_ENCRYPTION_FORMAT_LUKS = 1,
    BLOCKDEV_QCOW2_ENCRYPTION_FORMAT__MAX = 2,
} BlockdevQcow2EncryptionFormat;

#define BlockdevQcow2EncryptionFormat_str(val) \
    qapi_enum_lookup(&BlockdevQcow2EncryptionFormat_lookup, (val))

extern const QEnumLookup BlockdevQcow2EncryptionFormat_lookup;

typedef struct q_obj_BlockdevQcow2Encryption_base q_obj_BlockdevQcow2Encryption_base;

typedef struct BlockdevQcow2Encryption BlockdevQcow2Encryption;

typedef struct BlockdevOptionsQcow2 BlockdevOptionsQcow2;

typedef enum SshHostKeyCheckMode {
    SSH_HOST_KEY_CHECK_MODE_NONE = 0,
    SSH_HOST_KEY_CHECK_MODE_HASH = 1,
    SSH_HOST_KEY_CHECK_MODE_KNOWN_HOSTS = 2,
    SSH_HOST_KEY_CHECK_MODE__MAX = 3,
} SshHostKeyCheckMode;

#define SshHostKeyCheckMode_str(val) \
    qapi_enum_lookup(&SshHostKeyCheckMode_lookup, (val))

extern const QEnumLookup SshHostKeyCheckMode_lookup;

typedef enum SshHostKeyCheckHashType {
    SSH_HOST_KEY_CHECK_HASH_TYPE_MD5 = 0,
    SSH_HOST_KEY_CHECK_HASH_TYPE_SHA1 = 1,
    SSH_HOST_KEY_CHECK_HASH_TYPE__MAX = 2,
} SshHostKeyCheckHashType;

#define SshHostKeyCheckHashType_str(val) \
    qapi_enum_lookup(&SshHostKeyCheckHashType_lookup, (val))

extern const QEnumLookup SshHostKeyCheckHashType_lookup;

typedef struct SshHostKeyHash SshHostKeyHash;

typedef struct SshHostKeyDummy SshHostKeyDummy;

typedef struct q_obj_SshHostKeyCheck_base q_obj_SshHostKeyCheck_base;

typedef struct SshHostKeyCheck SshHostKeyCheck;

typedef struct BlockdevOptionsSsh BlockdevOptionsSsh;

typedef enum BlkdebugEvent {
    BLKDBG_L1_UPDATE = 0,
    BLKDBG_L1_GROW_ALLOC_TABLE = 1,
    BLKDBG_L1_GROW_WRITE_TABLE = 2,
    BLKDBG_L1_GROW_ACTIVATE_TABLE = 3,
    BLKDBG_L2_LOAD = 4,
    BLKDBG_L2_UPDATE = 5,
    BLKDBG_L2_UPDATE_COMPRESSED = 6,
    BLKDBG_L2_ALLOC_COW_READ = 7,
    BLKDBG_L2_ALLOC_WRITE = 8,
    BLKDBG_READ_AIO = 9,
    BLKDBG_READ_BACKING_AIO = 10,
    BLKDBG_READ_COMPRESSED = 11,
    BLKDBG_WRITE_AIO = 12,
    BLKDBG_WRITE_COMPRESSED = 13,
    BLKDBG_VMSTATE_LOAD = 14,
    BLKDBG_VMSTATE_SAVE = 15,
    BLKDBG_COW_READ = 16,
    BLKDBG_COW_WRITE = 17,
    BLKDBG_REFTABLE_LOAD = 18,
    BLKDBG_REFTABLE_GROW = 19,
    BLKDBG_REFTABLE_UPDATE = 20,
    BLKDBG_REFBLOCK_LOAD = 21,
    BLKDBG_REFBLOCK_UPDATE = 22,
    BLKDBG_REFBLOCK_UPDATE_PART = 23,
    BLKDBG_REFBLOCK_ALLOC = 24,
    BLKDBG_REFBLOCK_ALLOC_HOOKUP = 25,
    BLKDBG_REFBLOCK_ALLOC_WRITE = 26,
    BLKDBG_REFBLOCK_ALLOC_WRITE_BLOCKS = 27,
    BLKDBG_REFBLOCK_ALLOC_WRITE_TABLE = 28,
    BLKDBG_REFBLOCK_ALLOC_SWITCH_TABLE = 29,
    BLKDBG_CLUSTER_ALLOC = 30,
    BLKDBG_CLUSTER_ALLOC_BYTES = 31,
    BLKDBG_CLUSTER_FREE = 32,
    BLKDBG_FLUSH_TO_OS = 33,
    BLKDBG_FLUSH_TO_DISK = 34,
    BLKDBG_PWRITEV_RMW_HEAD = 35,
    BLKDBG_PWRITEV_RMW_AFTER_HEAD = 36,
    BLKDBG_PWRITEV_RMW_TAIL = 37,
    BLKDBG_PWRITEV_RMW_AFTER_TAIL = 38,
    BLKDBG_PWRITEV = 39,
    BLKDBG_PWRITEV_ZERO = 40,
    BLKDBG_PWRITEV_DONE = 41,
    BLKDBG_EMPTY_IMAGE_PREPARE = 42,
    BLKDBG_L1_SHRINK_WRITE_TABLE = 43,
    BLKDBG_L1_SHRINK_FREE_L2_CLUSTERS = 44,
    BLKDBG_COR_WRITE = 45,
    BLKDBG__MAX = 46,
} BlkdebugEvent;

#define BlkdebugEvent_str(val) \
    qapi_enum_lookup(&BlkdebugEvent_lookup, (val))

extern const QEnumLookup BlkdebugEvent_lookup;

typedef struct BlkdebugInjectErrorOptions BlkdebugInjectErrorOptions;

typedef struct BlkdebugSetStateOptions BlkdebugSetStateOptions;

typedef struct BlkdebugInjectErrorOptionsList BlkdebugInjectErrorOptionsList;

typedef struct BlkdebugSetStateOptionsList BlkdebugSetStateOptionsList;

typedef struct BlockdevOptionsBlkdebug BlockdevOptionsBlkdebug;

typedef struct BlockdevOptionsBlkverify BlockdevOptionsBlkverify;

typedef enum QuorumReadPattern {
    QUORUM_READ_PATTERN_QUORUM = 0,
    QUORUM_READ_PATTERN_FIFO = 1,
    QUORUM_READ_PATTERN__MAX = 2,
} QuorumReadPattern;

#define QuorumReadPattern_str(val) \
    qapi_enum_lookup(&QuorumReadPattern_lookup, (val))

extern const QEnumLookup QuorumReadPattern_lookup;

typedef struct BlockdevRefList BlockdevRefList;

typedef struct BlockdevOptionsQuorum BlockdevOptionsQuorum;

typedef struct SocketAddressList SocketAddressList;

typedef struct BlockdevOptionsGluster BlockdevOptionsGluster;

typedef enum IscsiTransport {
    ISCSI_TRANSPORT_TCP = 0,
    ISCSI_TRANSPORT_ISER = 1,
    ISCSI_TRANSPORT__MAX = 2,
} IscsiTransport;

#define IscsiTransport_str(val) \
    qapi_enum_lookup(&IscsiTransport_lookup, (val))

extern const QEnumLookup IscsiTransport_lookup;

typedef enum IscsiHeaderDigest {
    QAPI_ISCSI_HEADER_DIGEST_CRC32C = 0,
    QAPI_ISCSI_HEADER_DIGEST_NONE = 1,
    QAPI_ISCSI_HEADER_DIGEST_CRC32C_NONE = 2,
    QAPI_ISCSI_HEADER_DIGEST_NONE_CRC32C = 3,
    QAPI_ISCSI_HEADER_DIGEST__MAX = 4,
} IscsiHeaderDigest;

#define IscsiHeaderDigest_str(val) \
    qapi_enum_lookup(&IscsiHeaderDigest_lookup, (val))

extern const QEnumLookup IscsiHeaderDigest_lookup;

typedef struct BlockdevOptionsIscsi BlockdevOptionsIscsi;

typedef struct InetSocketAddressBaseList InetSocketAddressBaseList;

typedef struct BlockdevOptionsRbd BlockdevOptionsRbd;

typedef struct BlockdevOptionsSheepdog BlockdevOptionsSheepdog;

typedef enum ReplicationMode {
    REPLICATION_MODE_PRIMARY = 0,
    REPLICATION_MODE_SECONDARY = 1,
    REPLICATION_MODE__MAX = 2,
} ReplicationMode;

#define ReplicationMode_str(val) \
    qapi_enum_lookup(&ReplicationMode_lookup, (val))

extern const QEnumLookup ReplicationMode_lookup;

typedef struct BlockdevOptionsReplication BlockdevOptionsReplication;

typedef enum NFSTransport {
    NFS_TRANSPORT_INET = 0,
    NFS_TRANSPORT__MAX = 1,
} NFSTransport;

#define NFSTransport_str(val) \
    qapi_enum_lookup(&NFSTransport_lookup, (val))

extern const QEnumLookup NFSTransport_lookup;

typedef struct NFSServer NFSServer;

typedef struct BlockdevOptionsNfs BlockdevOptionsNfs;

typedef struct BlockdevOptionsCurlBase BlockdevOptionsCurlBase;

typedef struct BlockdevOptionsCurlHttp BlockdevOptionsCurlHttp;

typedef struct BlockdevOptionsCurlHttps BlockdevOptionsCurlHttps;

typedef struct BlockdevOptionsCurlFtp BlockdevOptionsCurlFtp;

typedef struct BlockdevOptionsCurlFtps BlockdevOptionsCurlFtps;

typedef struct BlockdevOptionsNbd BlockdevOptionsNbd;

typedef struct BlockdevOptionsRaw BlockdevOptionsRaw;

typedef struct BlockdevOptionsVxHS BlockdevOptionsVxHS;

typedef struct BlockdevOptionsThrottle BlockdevOptionsThrottle;

typedef struct q_obj_BlockdevOptions_base q_obj_BlockdevOptions_base;

typedef struct BlockdevOptions BlockdevOptions;

typedef struct BlockdevRef BlockdevRef;

typedef struct BlockdevRefOrNull BlockdevRefOrNull;

typedef struct q_obj_blockdev_del_arg q_obj_blockdev_del_arg;

typedef struct BlockdevCreateOptionsFile BlockdevCreateOptionsFile;

typedef struct BlockdevCreateOptionsGluster BlockdevCreateOptionsGluster;

typedef struct BlockdevCreateOptionsLUKS BlockdevCreateOptionsLUKS;

typedef struct BlockdevCreateOptionsNfs BlockdevCreateOptionsNfs;

typedef struct BlockdevCreateOptionsParallels BlockdevCreateOptionsParallels;

typedef struct BlockdevCreateOptionsQcow BlockdevCreateOptionsQcow;

typedef enum BlockdevQcow2Version {
    BLOCKDEV_QCOW2_VERSION_V2 = 0,
    BLOCKDEV_QCOW2_VERSION_V3 = 1,
    BLOCKDEV_QCOW2_VERSION__MAX = 2,
} BlockdevQcow2Version;

#define BlockdevQcow2Version_str(val) \
    qapi_enum_lookup(&BlockdevQcow2Version_lookup, (val))

extern const QEnumLookup BlockdevQcow2Version_lookup;

typedef struct BlockdevCreateOptionsQcow2 BlockdevCreateOptionsQcow2;

typedef struct BlockdevCreateOptionsQed BlockdevCreateOptionsQed;

typedef struct BlockdevCreateOptionsRbd BlockdevCreateOptionsRbd;

typedef enum SheepdogRedundancyType {
    SHEEPDOG_REDUNDANCY_TYPE_FULL = 0,
    SHEEPDOG_REDUNDANCY_TYPE_ERASURE_CODED = 1,
    SHEEPDOG_REDUNDANCY_TYPE__MAX = 2,
} SheepdogRedundancyType;

#define SheepdogRedundancyType_str(val) \
    qapi_enum_lookup(&SheepdogRedundancyType_lookup, (val))

extern const QEnumLookup SheepdogRedundancyType_lookup;

typedef struct SheepdogRedundancyFull SheepdogRedundancyFull;

typedef struct SheepdogRedundancyErasureCoded SheepdogRedundancyErasureCoded;

typedef struct q_obj_SheepdogRedundancy_base q_obj_SheepdogRedundancy_base;

typedef struct SheepdogRedundancy SheepdogRedundancy;

typedef struct BlockdevCreateOptionsSheepdog BlockdevCreateOptionsSheepdog;

typedef struct BlockdevCreateOptionsSsh BlockdevCreateOptionsSsh;

typedef struct BlockdevCreateOptionsVdi BlockdevCreateOptionsVdi;

typedef enum BlockdevVhdxSubformat {
    BLOCKDEV_VHDX_SUBFORMAT_DYNAMIC = 0,
    BLOCKDEV_VHDX_SUBFORMAT_FIXED = 1,
    BLOCKDEV_VHDX_SUBFORMAT__MAX = 2,
} BlockdevVhdxSubformat;

#define BlockdevVhdxSubformat_str(val) \
    qapi_enum_lookup(&BlockdevVhdxSubformat_lookup, (val))

extern const QEnumLookup BlockdevVhdxSubformat_lookup;

typedef struct BlockdevCreateOptionsVhdx BlockdevCreateOptionsVhdx;

typedef enum BlockdevVpcSubformat {
    BLOCKDEV_VPC_SUBFORMAT_DYNAMIC = 0,
    BLOCKDEV_VPC_SUBFORMAT_FIXED = 1,
    BLOCKDEV_VPC_SUBFORMAT__MAX = 2,
} BlockdevVpcSubformat;

#define BlockdevVpcSubformat_str(val) \
    qapi_enum_lookup(&BlockdevVpcSubformat_lookup, (val))

extern const QEnumLookup BlockdevVpcSubformat_lookup;

typedef struct BlockdevCreateOptionsVpc BlockdevCreateOptionsVpc;

typedef struct BlockdevCreateNotSupported BlockdevCreateNotSupported;

typedef struct q_obj_BlockdevCreateOptions_base q_obj_BlockdevCreateOptions_base;

typedef struct BlockdevCreateOptions BlockdevCreateOptions;

typedef struct q_obj_blockdev_open_tray_arg q_obj_blockdev_open_tray_arg;

typedef struct q_obj_blockdev_close_tray_arg q_obj_blockdev_close_tray_arg;

typedef struct q_obj_blockdev_remove_medium_arg q_obj_blockdev_remove_medium_arg;

typedef struct q_obj_blockdev_insert_medium_arg q_obj_blockdev_insert_medium_arg;

typedef enum BlockdevChangeReadOnlyMode {
    BLOCKDEV_CHANGE_READ_ONLY_MODE_RETAIN = 0,
    BLOCKDEV_CHANGE_READ_ONLY_MODE_READ_ONLY = 1,
    BLOCKDEV_CHANGE_READ_ONLY_MODE_READ_WRITE = 2,
    BLOCKDEV_CHANGE_READ_ONLY_MODE__MAX = 3,
} BlockdevChangeReadOnlyMode;

#define BlockdevChangeReadOnlyMode_str(val) \
    qapi_enum_lookup(&BlockdevChangeReadOnlyMode_lookup, (val))

extern const QEnumLookup BlockdevChangeReadOnlyMode_lookup;

typedef struct q_obj_blockdev_change_medium_arg q_obj_blockdev_change_medium_arg;

typedef enum BlockErrorAction {
    BLOCK_ERROR_ACTION_IGNORE = 0,
    BLOCK_ERROR_ACTION_REPORT = 1,
    BLOCK_ERROR_ACTION_STOP = 2,
    BLOCK_ERROR_ACTION__MAX = 3,
} BlockErrorAction;

#define BlockErrorAction_str(val) \
    qapi_enum_lookup(&BlockErrorAction_lookup, (val))

extern const QEnumLookup BlockErrorAction_lookup;

typedef struct q_obj_BLOCK_IMAGE_CORRUPTED_arg q_obj_BLOCK_IMAGE_CORRUPTED_arg;

typedef struct q_obj_BLOCK_IO_ERROR_arg q_obj_BLOCK_IO_ERROR_arg;

typedef struct q_obj_BLOCK_JOB_COMPLETED_arg q_obj_BLOCK_JOB_COMPLETED_arg;

typedef struct q_obj_BLOCK_JOB_CANCELLED_arg q_obj_BLOCK_JOB_CANCELLED_arg;

typedef struct q_obj_BLOCK_JOB_ERROR_arg q_obj_BLOCK_JOB_ERROR_arg;

typedef struct q_obj_BLOCK_JOB_READY_arg q_obj_BLOCK_JOB_READY_arg;

typedef struct q_obj_BLOCK_JOB_PENDING_arg q_obj_BLOCK_JOB_PENDING_arg;

typedef enum PreallocMode {
    PREALLOC_MODE_OFF = 0,
    PREALLOC_MODE_METADATA = 1,
    PREALLOC_MODE_FALLOC = 2,
    PREALLOC_MODE_FULL = 3,
    PREALLOC_MODE__MAX = 4,
} PreallocMode;

#define PreallocMode_str(val) \
    qapi_enum_lookup(&PreallocMode_lookup, (val))

extern const QEnumLookup PreallocMode_lookup;

typedef struct q_obj_BLOCK_WRITE_THRESHOLD_arg q_obj_BLOCK_WRITE_THRESHOLD_arg;

typedef struct q_obj_block_set_write_threshold_arg q_obj_block_set_write_threshold_arg;

typedef struct q_obj_x_blockdev_change_arg q_obj_x_blockdev_change_arg;

typedef struct q_obj_x_blockdev_set_iothread_arg q_obj_x_blockdev_set_iothread_arg;

struct SnapshotInfo {
    char *id;
    char *name;
    int64_t vm_state_size;
    int64_t date_sec;
    int64_t date_nsec;
    int64_t vm_clock_sec;
    int64_t vm_clock_nsec;
};

void qapi_free_SnapshotInfo(SnapshotInfo *obj);

struct ImageInfoSpecificQCow2EncryptionBase {
    BlockdevQcow2EncryptionFormat format;
};

void qapi_free_ImageInfoSpecificQCow2EncryptionBase(ImageInfoSpecificQCow2EncryptionBase *obj);

struct ImageInfoSpecificQCow2Encryption {
    /* Members inherited from ImageInfoSpecificQCow2EncryptionBase: */
    BlockdevQcow2EncryptionFormat format;
    /* Own members: */
    union { /* union tag is @format */
        QCryptoBlockInfoQCow aes;
        QCryptoBlockInfoLUKS luks;
    } u;
};

static inline ImageInfoSpecificQCow2EncryptionBase *qapi_ImageInfoSpecificQCow2Encryption_base(const ImageInfoSpecificQCow2Encryption *obj)
{
    return (ImageInfoSpecificQCow2EncryptionBase *)obj;
}

void qapi_free_ImageInfoSpecificQCow2Encryption(ImageInfoSpecificQCow2Encryption *obj);

struct ImageInfoSpecificQCow2 {
    char *compat;
    bool has_lazy_refcounts;
    bool lazy_refcounts;
    bool has_corrupt;
    bool corrupt;
    int64_t refcount_bits;
    bool has_encrypt;
    ImageInfoSpecificQCow2Encryption *encrypt;
};

void qapi_free_ImageInfoSpecificQCow2(ImageInfoSpecificQCow2 *obj);

struct ImageInfoList {
    ImageInfoList *next;
    ImageInfo *value;
};

void qapi_free_ImageInfoList(ImageInfoList *obj);

struct ImageInfoSpecificVmdk {
    char *create_type;
    int64_t cid;
    int64_t parent_cid;
    ImageInfoList *extents;
};

void qapi_free_ImageInfoSpecificVmdk(ImageInfoSpecificVmdk *obj);

struct q_obj_ImageInfoSpecificQCow2_wrapper {
    ImageInfoSpecificQCow2 *data;
};

struct q_obj_ImageInfoSpecificVmdk_wrapper {
    ImageInfoSpecificVmdk *data;
};

struct q_obj_QCryptoBlockInfoLUKS_wrapper {
    QCryptoBlockInfoLUKS *data;
};

struct ImageInfoSpecific {
    ImageInfoSpecificKind type;
    union { /* union tag is @type */
        q_obj_ImageInfoSpecificQCow2_wrapper qcow2;
        q_obj_ImageInfoSpecificVmdk_wrapper vmdk;
        q_obj_QCryptoBlockInfoLUKS_wrapper luks;
    } u;
};

void qapi_free_ImageInfoSpecific(ImageInfoSpecific *obj);

struct SnapshotInfoList {
    SnapshotInfoList *next;
    SnapshotInfo *value;
};

void qapi_free_SnapshotInfoList(SnapshotInfoList *obj);

struct ImageInfo {
    char *filename;
    char *format;
    bool has_dirty_flag;
    bool dirty_flag;
    bool has_actual_size;
    int64_t actual_size;
    int64_t virtual_size;
    bool has_cluster_size;
    int64_t cluster_size;
    bool has_encrypted;
    bool encrypted;
    bool has_compressed;
    bool compressed;
    bool has_backing_filename;
    char *backing_filename;
    bool has_full_backing_filename;
    char *full_backing_filename;
    bool has_backing_filename_format;
    char *backing_filename_format;
    bool has_snapshots;
    SnapshotInfoList *snapshots;
    bool has_backing_image;
    ImageInfo *backing_image;
    bool has_format_specific;
    ImageInfoSpecific *format_specific;
};

void qapi_free_ImageInfo(ImageInfo *obj);

struct ImageCheck {
    char *filename;
    char *format;
    int64_t check_errors;
    bool has_image_end_offset;
    int64_t image_end_offset;
    bool has_corruptions;
    int64_t corruptions;
    bool has_leaks;
    int64_t leaks;
    bool has_corruptions_fixed;
    int64_t corruptions_fixed;
    bool has_leaks_fixed;
    int64_t leaks_fixed;
    bool has_total_clusters;
    int64_t total_clusters;
    bool has_allocated_clusters;
    int64_t allocated_clusters;
    bool has_fragmented_clusters;
    int64_t fragmented_clusters;
    bool has_compressed_clusters;
    int64_t compressed_clusters;
};

void qapi_free_ImageCheck(ImageCheck *obj);

struct MapEntry {
    int64_t start;
    int64_t length;
    bool data;
    bool zero;
    int64_t depth;
    bool has_offset;
    int64_t offset;
    bool has_filename;
    char *filename;
};

void qapi_free_MapEntry(MapEntry *obj);

struct BlockdevCacheInfo {
    bool writeback;
    bool direct;
    bool no_flush;
};

void qapi_free_BlockdevCacheInfo(BlockdevCacheInfo *obj);

struct BlockDeviceInfo {
    char *file;
    bool has_node_name;
    char *node_name;
    bool ro;
    char *drv;
    bool has_backing_file;
    char *backing_file;
    int64_t backing_file_depth;
    bool encrypted;
    bool encryption_key_missing;
    BlockdevDetectZeroesOptions detect_zeroes;
    int64_t bps;
    int64_t bps_rd;
    int64_t bps_wr;
    int64_t iops;
    int64_t iops_rd;
    int64_t iops_wr;
    ImageInfo *image;
    bool has_bps_max;
    int64_t bps_max;
    bool has_bps_rd_max;
    int64_t bps_rd_max;
    bool has_bps_wr_max;
    int64_t bps_wr_max;
    bool has_iops_max;
    int64_t iops_max;
    bool has_iops_rd_max;
    int64_t iops_rd_max;
    bool has_iops_wr_max;
    int64_t iops_wr_max;
    bool has_bps_max_length;
    int64_t bps_max_length;
    bool has_bps_rd_max_length;
    int64_t bps_rd_max_length;
    bool has_bps_wr_max_length;
    int64_t bps_wr_max_length;
    bool has_iops_max_length;
    int64_t iops_max_length;
    bool has_iops_rd_max_length;
    int64_t iops_rd_max_length;
    bool has_iops_wr_max_length;
    int64_t iops_wr_max_length;
    bool has_iops_size;
    int64_t iops_size;
    bool has_group;
    char *group;
    BlockdevCacheInfo *cache;
    int64_t write_threshold;
};

void qapi_free_BlockDeviceInfo(BlockDeviceInfo *obj);

struct BlockDeviceMapEntry {
    int64_t start;
    int64_t length;
    int64_t depth;
    bool zero;
    bool data;
    bool has_offset;
    int64_t offset;
};

void qapi_free_BlockDeviceMapEntry(BlockDeviceMapEntry *obj);

struct BlockDirtyInfo {
    bool has_name;
    char *name;
    int64_t count;
    uint32_t granularity;
    DirtyBitmapStatus status;
};

void qapi_free_BlockDirtyInfo(BlockDirtyInfo *obj);

struct BlockLatencyHistogramInfo {
    uint64List *boundaries;
    uint64List *bins;
};

void qapi_free_BlockLatencyHistogramInfo(BlockLatencyHistogramInfo *obj);

struct q_obj_x_block_latency_histogram_set_arg {
    char *device;
    bool has_boundaries;
    uint64List *boundaries;
    bool has_boundaries_read;
    uint64List *boundaries_read;
    bool has_boundaries_write;
    uint64List *boundaries_write;
    bool has_boundaries_flush;
    uint64List *boundaries_flush;
};

struct BlockDirtyInfoList {
    BlockDirtyInfoList *next;
    BlockDirtyInfo *value;
};

void qapi_free_BlockDirtyInfoList(BlockDirtyInfoList *obj);

struct BlockInfo {
    char *device;
    bool has_qdev;
    char *qdev;
    char *type;
    bool removable;
    bool locked;
    bool has_inserted;
    BlockDeviceInfo *inserted;
    bool has_tray_open;
    bool tray_open;
    bool has_io_status;
    BlockDeviceIoStatus io_status;
    bool has_dirty_bitmaps;
    BlockDirtyInfoList *dirty_bitmaps;
};

void qapi_free_BlockInfo(BlockInfo *obj);

struct BlockMeasureInfo {
    int64_t required;
    int64_t fully_allocated;
};

void qapi_free_BlockMeasureInfo(BlockMeasureInfo *obj);

struct BlockInfoList {
    BlockInfoList *next;
    BlockInfo *value;
};

void qapi_free_BlockInfoList(BlockInfoList *obj);

struct BlockDeviceTimedStats {
    int64_t interval_length;
    int64_t min_rd_latency_ns;
    int64_t max_rd_latency_ns;
    int64_t avg_rd_latency_ns;
    int64_t min_wr_latency_ns;
    int64_t max_wr_latency_ns;
    int64_t avg_wr_latency_ns;
    int64_t min_flush_latency_ns;
    int64_t max_flush_latency_ns;
    int64_t avg_flush_latency_ns;
    double avg_rd_queue_depth;
    double avg_wr_queue_depth;
};

void qapi_free_BlockDeviceTimedStats(BlockDeviceTimedStats *obj);

struct BlockDeviceTimedStatsList {
    BlockDeviceTimedStatsList *next;
    BlockDeviceTimedStats *value;
};

void qapi_free_BlockDeviceTimedStatsList(BlockDeviceTimedStatsList *obj);

struct BlockDeviceStats {
    int64_t rd_bytes;
    int64_t wr_bytes;
    int64_t rd_operations;
    int64_t wr_operations;
    int64_t flush_operations;
    int64_t flush_total_time_ns;
    int64_t wr_total_time_ns;
    int64_t rd_total_time_ns;
    int64_t wr_highest_offset;
    int64_t rd_merged;
    int64_t wr_merged;
    bool has_idle_time_ns;
    int64_t idle_time_ns;
    int64_t failed_rd_operations;
    int64_t failed_wr_operations;
    int64_t failed_flush_operations;
    int64_t invalid_rd_operations;
    int64_t invalid_wr_operations;
    int64_t invalid_flush_operations;
    bool account_invalid;
    bool account_failed;
    BlockDeviceTimedStatsList *timed_stats;
    bool has_x_rd_latency_histogram;
    BlockLatencyHistogramInfo *x_rd_latency_histogram;
    bool has_x_wr_latency_histogram;
    BlockLatencyHistogramInfo *x_wr_latency_histogram;
    bool has_x_flush_latency_histogram;
    BlockLatencyHistogramInfo *x_flush_latency_histogram;
};

void qapi_free_BlockDeviceStats(BlockDeviceStats *obj);

struct BlockStats {
    bool has_device;
    char *device;
    bool has_node_name;
    char *node_name;
    BlockDeviceStats *stats;
    bool has_parent;
    BlockStats *parent;
    bool has_backing;
    BlockStats *backing;
};

void qapi_free_BlockStats(BlockStats *obj);

struct q_obj_query_blockstats_arg {
    bool has_query_nodes;
    bool query_nodes;
};

struct BlockStatsList {
    BlockStatsList *next;
    BlockStats *value;
};

void qapi_free_BlockStatsList(BlockStatsList *obj);

struct BlockJobInfo {
    char *type;
    char *device;
    int64_t len;
    int64_t offset;
    bool busy;
    bool paused;
    int64_t speed;
    BlockDeviceIoStatus io_status;
    bool ready;
    BlockJobStatus status;
    bool auto_finalize;
    bool auto_dismiss;
};

void qapi_free_BlockJobInfo(BlockJobInfo *obj);

struct BlockJobInfoList {
    BlockJobInfoList *next;
    BlockJobInfo *value;
};

void qapi_free_BlockJobInfoList(BlockJobInfoList *obj);

struct q_obj_block_passwd_arg {
    bool has_device;
    char *device;
    bool has_node_name;
    char *node_name;
    char *password;
};

struct q_obj_block_resize_arg {
    bool has_device;
    char *device;
    bool has_node_name;
    char *node_name;
    int64_t size;
};

struct BlockdevSnapshotSync {
    bool has_device;
    char *device;
    bool has_node_name;
    char *node_name;
    char *snapshot_file;
    bool has_snapshot_node_name;
    char *snapshot_node_name;
    bool has_format;
    char *format;
    bool has_mode;
    NewImageMode mode;
};

void qapi_free_BlockdevSnapshotSync(BlockdevSnapshotSync *obj);

struct BlockdevSnapshot {
    char *node;
    char *overlay;
};

void qapi_free_BlockdevSnapshot(BlockdevSnapshot *obj);

struct DriveBackup {
    bool has_job_id;
    char *job_id;
    char *device;
    char *target;
    bool has_format;
    char *format;
    MirrorSyncMode sync;
    bool has_mode;
    NewImageMode mode;
    bool has_speed;
    int64_t speed;
    bool has_bitmap;
    char *bitmap;
    bool has_compress;
    bool compress;
    bool has_on_source_error;
    BlockdevOnError on_source_error;
    bool has_on_target_error;
    BlockdevOnError on_target_error;
    bool has_auto_finalize;
    bool auto_finalize;
    bool has_auto_dismiss;
    bool auto_dismiss;
};

void qapi_free_DriveBackup(DriveBackup *obj);

struct BlockdevBackup {
    bool has_job_id;
    char *job_id;
    char *device;
    char *target;
    MirrorSyncMode sync;
    bool has_speed;
    int64_t speed;
    bool has_compress;
    bool compress;
    bool has_on_source_error;
    BlockdevOnError on_source_error;
    bool has_on_target_error;
    BlockdevOnError on_target_error;
    bool has_auto_finalize;
    bool auto_finalize;
    bool has_auto_dismiss;
    bool auto_dismiss;
};

void qapi_free_BlockdevBackup(BlockdevBackup *obj);

struct q_obj_change_backing_file_arg {
    char *device;
    char *image_node_name;
    char *backing_file;
};

struct q_obj_block_commit_arg {
    bool has_job_id;
    char *job_id;
    char *device;
    bool has_base;
    char *base;
    bool has_top;
    char *top;
    bool has_backing_file;
    char *backing_file;
    bool has_speed;
    int64_t speed;
    bool has_filter_node_name;
    char *filter_node_name;
};

struct BlockDeviceInfoList {
    BlockDeviceInfoList *next;
    BlockDeviceInfo *value;
};

void qapi_free_BlockDeviceInfoList(BlockDeviceInfoList *obj);

struct DriveMirror {
    bool has_job_id;
    char *job_id;
    char *device;
    char *target;
    bool has_format;
    char *format;
    bool has_node_name;
    char *node_name;
    bool has_replaces;
    char *replaces;
    MirrorSyncMode sync;
    bool has_mode;
    NewImageMode mode;
    bool has_speed;
    int64_t speed;
    bool has_granularity;
    uint32_t granularity;
    bool has_buf_size;
    int64_t buf_size;
    bool has_on_source_error;
    BlockdevOnError on_source_error;
    bool has_on_target_error;
    BlockdevOnError on_target_error;
    bool has_unmap;
    bool unmap;
};

void qapi_free_DriveMirror(DriveMirror *obj);

struct BlockDirtyBitmap {
    char *node;
    char *name;
};

void qapi_free_BlockDirtyBitmap(BlockDirtyBitmap *obj);

struct BlockDirtyBitmapAdd {
    char *node;
    char *name;
    bool has_granularity;
    uint32_t granularity;
    bool has_persistent;
    bool persistent;
    bool has_autoload;
    bool autoload;
};

void qapi_free_BlockDirtyBitmapAdd(BlockDirtyBitmapAdd *obj);

struct BlockDirtyBitmapSha256 {
    char *sha256;
};

void qapi_free_BlockDirtyBitmapSha256(BlockDirtyBitmapSha256 *obj);

struct q_obj_blockdev_mirror_arg {
    bool has_job_id;
    char *job_id;
    char *device;
    char *target;
    bool has_replaces;
    char *replaces;
    MirrorSyncMode sync;
    bool has_speed;
    int64_t speed;
    bool has_granularity;
    uint32_t granularity;
    bool has_buf_size;
    int64_t buf_size;
    bool has_on_source_error;
    BlockdevOnError on_source_error;
    bool has_on_target_error;
    BlockdevOnError on_target_error;
    bool has_filter_node_name;
    char *filter_node_name;
};

struct BlockIOThrottle {
    bool has_device;
    char *device;
    bool has_id;
    char *id;
    int64_t bps;
    int64_t bps_rd;
    int64_t bps_wr;
    int64_t iops;
    int64_t iops_rd;
    int64_t iops_wr;
    bool has_bps_max;
    int64_t bps_max;
    bool has_bps_rd_max;
    int64_t bps_rd_max;
    bool has_bps_wr_max;
    int64_t bps_wr_max;
    bool has_iops_max;
    int64_t iops_max;
    bool has_iops_rd_max;
    int64_t iops_rd_max;
    bool has_iops_wr_max;
    int64_t iops_wr_max;
    bool has_bps_max_length;
    int64_t bps_max_length;
    bool has_bps_rd_max_length;
    int64_t bps_rd_max_length;
    bool has_bps_wr_max_length;
    int64_t bps_wr_max_length;
    bool has_iops_max_length;
    int64_t iops_max_length;
    bool has_iops_rd_max_length;
    int64_t iops_rd_max_length;
    bool has_iops_wr_max_length;
    int64_t iops_wr_max_length;
    bool has_iops_size;
    int64_t iops_size;
    bool has_group;
    char *group;
};

void qapi_free_BlockIOThrottle(BlockIOThrottle *obj);

struct ThrottleLimits {
    bool has_iops_total;
    int64_t iops_total;
    bool has_iops_total_max;
    int64_t iops_total_max;
    bool has_iops_total_max_length;
    int64_t iops_total_max_length;
    bool has_iops_read;
    int64_t iops_read;
    bool has_iops_read_max;
    int64_t iops_read_max;
    bool has_iops_read_max_length;
    int64_t iops_read_max_length;
    bool has_iops_write;
    int64_t iops_write;
    bool has_iops_write_max;
    int64_t iops_write_max;
    bool has_iops_write_max_length;
    int64_t iops_write_max_length;
    bool has_bps_total;
    int64_t bps_total;
    bool has_bps_total_max;
    int64_t bps_total_max;
    bool has_bps_total_max_length;
    int64_t bps_total_max_length;
    bool has_bps_read;
    int64_t bps_read;
    bool has_bps_read_max;
    int64_t bps_read_max;
    bool has_bps_read_max_length;
    int64_t bps_read_max_length;
    bool has_bps_write;
    int64_t bps_write;
    bool has_bps_write_max;
    int64_t bps_write_max;
    bool has_bps_write_max_length;
    int64_t bps_write_max_length;
    bool has_iops_size;
    int64_t iops_size;
};

void qapi_free_ThrottleLimits(ThrottleLimits *obj);

struct q_obj_block_stream_arg {
    bool has_job_id;
    char *job_id;
    char *device;
    bool has_base;
    char *base;
    bool has_base_node;
    char *base_node;
    bool has_backing_file;
    char *backing_file;
    bool has_speed;
    int64_t speed;
    bool has_on_error;
    BlockdevOnError on_error;
};

struct q_obj_block_job_set_speed_arg {
    char *device;
    int64_t speed;
};

struct q_obj_block_job_cancel_arg {
    char *device;
    bool has_force;
    bool force;
};

struct q_obj_block_job_pause_arg {
    char *device;
};

struct q_obj_block_job_resume_arg {
    char *device;
};

struct q_obj_block_job_complete_arg {
    char *device;
};

struct q_obj_block_job_dismiss_arg {
    char *id;
};

struct q_obj_block_job_finalize_arg {
    char *id;
};

struct BlockdevCacheOptions {
    bool has_direct;
    bool direct;
    bool has_no_flush;
    bool no_flush;
};

void qapi_free_BlockdevCacheOptions(BlockdevCacheOptions *obj);

struct BlockdevOptionsFile {
    char *filename;
    bool has_pr_manager;
    char *pr_manager;
    bool has_locking;
    OnOffAuto locking;
    bool has_aio;
    BlockdevAioOptions aio;
};

void qapi_free_BlockdevOptionsFile(BlockdevOptionsFile *obj);

struct BlockdevOptionsNull {
    bool has_size;
    int64_t size;
    bool has_latency_ns;
    uint64_t latency_ns;
};

void qapi_free_BlockdevOptionsNull(BlockdevOptionsNull *obj);

struct BlockdevOptionsNVMe {
    char *device;
    int64_t q_namespace;
};

void qapi_free_BlockdevOptionsNVMe(BlockdevOptionsNVMe *obj);

struct BlockdevOptionsVVFAT {
    char *dir;
    bool has_fat_type;
    int64_t fat_type;
    bool has_floppy;
    bool floppy;
    bool has_label;
    char *label;
    bool has_rw;
    bool rw;
};

void qapi_free_BlockdevOptionsVVFAT(BlockdevOptionsVVFAT *obj);

struct BlockdevOptionsGenericFormat {
    BlockdevRef *file;
};

void qapi_free_BlockdevOptionsGenericFormat(BlockdevOptionsGenericFormat *obj);

struct BlockdevOptionsLUKS {
    /* Members inherited from BlockdevOptionsGenericFormat: */
    BlockdevRef *file;
    /* Own members: */
    bool has_key_secret;
    char *key_secret;
};

static inline BlockdevOptionsGenericFormat *qapi_BlockdevOptionsLUKS_base(const BlockdevOptionsLUKS *obj)
{
    return (BlockdevOptionsGenericFormat *)obj;
}

void qapi_free_BlockdevOptionsLUKS(BlockdevOptionsLUKS *obj);

struct BlockdevOptionsGenericCOWFormat {
    /* Members inherited from BlockdevOptionsGenericFormat: */
    BlockdevRef *file;
    /* Own members: */
    bool has_backing;
    BlockdevRefOrNull *backing;
};

static inline BlockdevOptionsGenericFormat *qapi_BlockdevOptionsGenericCOWFormat_base(const BlockdevOptionsGenericCOWFormat *obj)
{
    return (BlockdevOptionsGenericFormat *)obj;
}

void qapi_free_BlockdevOptionsGenericCOWFormat(BlockdevOptionsGenericCOWFormat *obj);

struct Qcow2OverlapCheckFlags {
    bool has_q_template;
    Qcow2OverlapCheckMode q_template;
    bool has_main_header;
    bool main_header;
    bool has_active_l1;
    bool active_l1;
    bool has_active_l2;
    bool active_l2;
    bool has_refcount_table;
    bool refcount_table;
    bool has_refcount_block;
    bool refcount_block;
    bool has_snapshot_table;
    bool snapshot_table;
    bool has_inactive_l1;
    bool inactive_l1;
    bool has_inactive_l2;
    bool inactive_l2;
};

void qapi_free_Qcow2OverlapCheckFlags(Qcow2OverlapCheckFlags *obj);

struct Qcow2OverlapChecks {
    QType type;
    union { /* union tag is @type */
        Qcow2OverlapCheckFlags flags;
        Qcow2OverlapCheckMode mode;
    } u;
};

void qapi_free_Qcow2OverlapChecks(Qcow2OverlapChecks *obj);

struct q_obj_BlockdevQcowEncryption_base {
    BlockdevQcowEncryptionFormat format;
};

struct BlockdevQcowEncryption {
    BlockdevQcowEncryptionFormat format;
    union { /* union tag is @format */
        QCryptoBlockOptionsQCow aes;
    } u;
};

void qapi_free_BlockdevQcowEncryption(BlockdevQcowEncryption *obj);

struct BlockdevOptionsQcow {
    /* Members inherited from BlockdevOptionsGenericCOWFormat: */
    BlockdevRef *file;
    bool has_backing;
    BlockdevRefOrNull *backing;
    /* Own members: */
    bool has_encrypt;
    BlockdevQcowEncryption *encrypt;
};

static inline BlockdevOptionsGenericCOWFormat *qapi_BlockdevOptionsQcow_base(const BlockdevOptionsQcow *obj)
{
    return (BlockdevOptionsGenericCOWFormat *)obj;
}

void qapi_free_BlockdevOptionsQcow(BlockdevOptionsQcow *obj);

struct q_obj_BlockdevQcow2Encryption_base {
    BlockdevQcow2EncryptionFormat format;
};

struct BlockdevQcow2Encryption {
    BlockdevQcow2EncryptionFormat format;
    union { /* union tag is @format */
        QCryptoBlockOptionsQCow aes;
        QCryptoBlockOptionsLUKS luks;
    } u;
};

void qapi_free_BlockdevQcow2Encryption(BlockdevQcow2Encryption *obj);

struct BlockdevOptionsQcow2 {
    /* Members inherited from BlockdevOptionsGenericCOWFormat: */
    BlockdevRef *file;
    bool has_backing;
    BlockdevRefOrNull *backing;
    /* Own members: */
    bool has_lazy_refcounts;
    bool lazy_refcounts;
    bool has_pass_discard_request;
    bool pass_discard_request;
    bool has_pass_discard_snapshot;
    bool pass_discard_snapshot;
    bool has_pass_discard_other;
    bool pass_discard_other;
    bool has_overlap_check;
    Qcow2OverlapChecks *overlap_check;
    bool has_cache_size;
    int64_t cache_size;
    bool has_l2_cache_size;
    int64_t l2_cache_size;
    bool has_l2_cache_entry_size;
    int64_t l2_cache_entry_size;
    bool has_refcount_cache_size;
    int64_t refcount_cache_size;
    bool has_cache_clean_interval;
    int64_t cache_clean_interval;
    bool has_encrypt;
    BlockdevQcow2Encryption *encrypt;
};

static inline BlockdevOptionsGenericCOWFormat *qapi_BlockdevOptionsQcow2_base(const BlockdevOptionsQcow2 *obj)
{
    return (BlockdevOptionsGenericCOWFormat *)obj;
}

void qapi_free_BlockdevOptionsQcow2(BlockdevOptionsQcow2 *obj);

struct SshHostKeyHash {
    SshHostKeyCheckHashType type;
    char *hash;
};

void qapi_free_SshHostKeyHash(SshHostKeyHash *obj);

struct SshHostKeyDummy {
    char qapi_dummy_for_empty_struct;
};

void qapi_free_SshHostKeyDummy(SshHostKeyDummy *obj);

struct q_obj_SshHostKeyCheck_base {
    SshHostKeyCheckMode mode;
};

struct SshHostKeyCheck {
    SshHostKeyCheckMode mode;
    union { /* union tag is @mode */
        SshHostKeyDummy none;
        SshHostKeyHash hash;
        SshHostKeyDummy known_hosts;
    } u;
};

void qapi_free_SshHostKeyCheck(SshHostKeyCheck *obj);

struct BlockdevOptionsSsh {
    InetSocketAddress *server;
    char *path;
    bool has_user;
    char *user;
    bool has_host_key_check;
    SshHostKeyCheck *host_key_check;
};

void qapi_free_BlockdevOptionsSsh(BlockdevOptionsSsh *obj);

struct BlkdebugInjectErrorOptions {
    BlkdebugEvent event;
    bool has_state;
    int64_t state;
    bool has_q_errno;
    int64_t q_errno;
    bool has_sector;
    int64_t sector;
    bool has_once;
    bool once;
    bool has_immediately;
    bool immediately;
};

void qapi_free_BlkdebugInjectErrorOptions(BlkdebugInjectErrorOptions *obj);

struct BlkdebugSetStateOptions {
    BlkdebugEvent event;
    bool has_state;
    int64_t state;
    int64_t new_state;
};

void qapi_free_BlkdebugSetStateOptions(BlkdebugSetStateOptions *obj);

struct BlkdebugInjectErrorOptionsList {
    BlkdebugInjectErrorOptionsList *next;
    BlkdebugInjectErrorOptions *value;
};

void qapi_free_BlkdebugInjectErrorOptionsList(BlkdebugInjectErrorOptionsList *obj);

struct BlkdebugSetStateOptionsList {
    BlkdebugSetStateOptionsList *next;
    BlkdebugSetStateOptions *value;
};

void qapi_free_BlkdebugSetStateOptionsList(BlkdebugSetStateOptionsList *obj);

struct BlockdevOptionsBlkdebug {
    BlockdevRef *image;
    bool has_config;
    char *config;
    bool has_align;
    int64_t align;
    bool has_max_transfer;
    int32_t max_transfer;
    bool has_opt_write_zero;
    int32_t opt_write_zero;
    bool has_max_write_zero;
    int32_t max_write_zero;
    bool has_opt_discard;
    int32_t opt_discard;
    bool has_max_discard;
    int32_t max_discard;
    bool has_inject_error;
    BlkdebugInjectErrorOptionsList *inject_error;
    bool has_set_state;
    BlkdebugSetStateOptionsList *set_state;
};

void qapi_free_BlockdevOptionsBlkdebug(BlockdevOptionsBlkdebug *obj);

struct BlockdevOptionsBlkverify {
    BlockdevRef *test;
    BlockdevRef *raw;
};

void qapi_free_BlockdevOptionsBlkverify(BlockdevOptionsBlkverify *obj);

struct BlockdevRefList {
    BlockdevRefList *next;
    BlockdevRef *value;
};

void qapi_free_BlockdevRefList(BlockdevRefList *obj);

struct BlockdevOptionsQuorum {
    bool has_blkverify;
    bool blkverify;
    BlockdevRefList *children;
    int64_t vote_threshold;
    bool has_rewrite_corrupted;
    bool rewrite_corrupted;
    bool has_read_pattern;
    QuorumReadPattern read_pattern;
};

void qapi_free_BlockdevOptionsQuorum(BlockdevOptionsQuorum *obj);

struct SocketAddressList {
    SocketAddressList *next;
    SocketAddress *value;
};

void qapi_free_SocketAddressList(SocketAddressList *obj);

struct BlockdevOptionsGluster {
    char *volume;
    char *path;
    SocketAddressList *server;
    bool has_debug;
    int64_t debug;
    bool has_logfile;
    char *logfile;
};

void qapi_free_BlockdevOptionsGluster(BlockdevOptionsGluster *obj);

struct BlockdevOptionsIscsi {
    IscsiTransport transport;
    char *portal;
    char *target;
    bool has_lun;
    int64_t lun;
    bool has_user;
    char *user;
    bool has_password_secret;
    char *password_secret;
    bool has_initiator_name;
    char *initiator_name;
    bool has_header_digest;
    IscsiHeaderDigest header_digest;
    bool has_timeout;
    int64_t timeout;
};

void qapi_free_BlockdevOptionsIscsi(BlockdevOptionsIscsi *obj);

struct InetSocketAddressBaseList {
    InetSocketAddressBaseList *next;
    InetSocketAddressBase *value;
};

void qapi_free_InetSocketAddressBaseList(InetSocketAddressBaseList *obj);

struct BlockdevOptionsRbd {
    char *pool;
    char *image;
    bool has_conf;
    char *conf;
    bool has_snapshot;
    char *snapshot;
    bool has_user;
    char *user;
    bool has_server;
    InetSocketAddressBaseList *server;
};

void qapi_free_BlockdevOptionsRbd(BlockdevOptionsRbd *obj);

struct BlockdevOptionsSheepdog {
    SocketAddress *server;
    char *vdi;
    bool has_snap_id;
    uint32_t snap_id;
    bool has_tag;
    char *tag;
};

void qapi_free_BlockdevOptionsSheepdog(BlockdevOptionsSheepdog *obj);

struct BlockdevOptionsReplication {
    /* Members inherited from BlockdevOptionsGenericFormat: */
    BlockdevRef *file;
    /* Own members: */
    ReplicationMode mode;
    bool has_top_id;
    char *top_id;
};

static inline BlockdevOptionsGenericFormat *qapi_BlockdevOptionsReplication_base(const BlockdevOptionsReplication *obj)
{
    return (BlockdevOptionsGenericFormat *)obj;
}

void qapi_free_BlockdevOptionsReplication(BlockdevOptionsReplication *obj);

struct NFSServer {
    NFSTransport type;
    char *host;
};

void qapi_free_NFSServer(NFSServer *obj);

struct BlockdevOptionsNfs {
    NFSServer *server;
    char *path;
    bool has_user;
    int64_t user;
    bool has_group;
    int64_t group;
    bool has_tcp_syn_count;
    int64_t tcp_syn_count;
    bool has_readahead_size;
    int64_t readahead_size;
    bool has_page_cache_size;
    int64_t page_cache_size;
    bool has_debug;
    int64_t debug;
};

void qapi_free_BlockdevOptionsNfs(BlockdevOptionsNfs *obj);

struct BlockdevOptionsCurlBase {
    char *url;
    bool has_readahead;
    int64_t readahead;
    bool has_timeout;
    int64_t timeout;
    bool has_username;
    char *username;
    bool has_password_secret;
    char *password_secret;
    bool has_proxy_username;
    char *proxy_username;
    bool has_proxy_password_secret;
    char *proxy_password_secret;
};

void qapi_free_BlockdevOptionsCurlBase(BlockdevOptionsCurlBase *obj);

struct BlockdevOptionsCurlHttp {
    /* Members inherited from BlockdevOptionsCurlBase: */
    char *url;
    bool has_readahead;
    int64_t readahead;
    bool has_timeout;
    int64_t timeout;
    bool has_username;
    char *username;
    bool has_password_secret;
    char *password_secret;
    bool has_proxy_username;
    char *proxy_username;
    bool has_proxy_password_secret;
    char *proxy_password_secret;
    /* Own members: */
    bool has_cookie;
    char *cookie;
    bool has_cookie_secret;
    char *cookie_secret;
};

static inline BlockdevOptionsCurlBase *qapi_BlockdevOptionsCurlHttp_base(const BlockdevOptionsCurlHttp *obj)
{
    return (BlockdevOptionsCurlBase *)obj;
}

void qapi_free_BlockdevOptionsCurlHttp(BlockdevOptionsCurlHttp *obj);

struct BlockdevOptionsCurlHttps {
    /* Members inherited from BlockdevOptionsCurlBase: */
    char *url;
    bool has_readahead;
    int64_t readahead;
    bool has_timeout;
    int64_t timeout;
    bool has_username;
    char *username;
    bool has_password_secret;
    char *password_secret;
    bool has_proxy_username;
    char *proxy_username;
    bool has_proxy_password_secret;
    char *proxy_password_secret;
    /* Own members: */
    bool has_cookie;
    char *cookie;
    bool has_sslverify;
    bool sslverify;
    bool has_cookie_secret;
    char *cookie_secret;
};

static inline BlockdevOptionsCurlBase *qapi_BlockdevOptionsCurlHttps_base(const BlockdevOptionsCurlHttps *obj)
{
    return (BlockdevOptionsCurlBase *)obj;
}

void qapi_free_BlockdevOptionsCurlHttps(BlockdevOptionsCurlHttps *obj);

struct BlockdevOptionsCurlFtp {
    /* Members inherited from BlockdevOptionsCurlBase: */
    char *url;
    bool has_readahead;
    int64_t readahead;
    bool has_timeout;
    int64_t timeout;
    bool has_username;
    char *username;
    bool has_password_secret;
    char *password_secret;
    bool has_proxy_username;
    char *proxy_username;
    bool has_proxy_password_secret;
    char *proxy_password_secret;
    /* Own members: */
};

static inline BlockdevOptionsCurlBase *qapi_BlockdevOptionsCurlFtp_base(const BlockdevOptionsCurlFtp *obj)
{
    return (BlockdevOptionsCurlBase *)obj;
}

void qapi_free_BlockdevOptionsCurlFtp(BlockdevOptionsCurlFtp *obj);

struct BlockdevOptionsCurlFtps {
    /* Members inherited from BlockdevOptionsCurlBase: */
    char *url;
    bool has_readahead;
    int64_t readahead;
    bool has_timeout;
    int64_t timeout;
    bool has_username;
    char *username;
    bool has_password_secret;
    char *password_secret;
    bool has_proxy_username;
    char *proxy_username;
    bool has_proxy_password_secret;
    char *proxy_password_secret;
    /* Own members: */
    bool has_sslverify;
    bool sslverify;
};

static inline BlockdevOptionsCurlBase *qapi_BlockdevOptionsCurlFtps_base(const BlockdevOptionsCurlFtps *obj)
{
    return (BlockdevOptionsCurlBase *)obj;
}

void qapi_free_BlockdevOptionsCurlFtps(BlockdevOptionsCurlFtps *obj);

struct BlockdevOptionsNbd {
    SocketAddress *server;
    bool has_exp;
    char *exp;
    bool has_tls_creds;
    char *tls_creds;
};

void qapi_free_BlockdevOptionsNbd(BlockdevOptionsNbd *obj);

struct BlockdevOptionsRaw {
    /* Members inherited from BlockdevOptionsGenericFormat: */
    BlockdevRef *file;
    /* Own members: */
    bool has_offset;
    int64_t offset;
    bool has_size;
    int64_t size;
};

static inline BlockdevOptionsGenericFormat *qapi_BlockdevOptionsRaw_base(const BlockdevOptionsRaw *obj)
{
    return (BlockdevOptionsGenericFormat *)obj;
}

void qapi_free_BlockdevOptionsRaw(BlockdevOptionsRaw *obj);

struct BlockdevOptionsVxHS {
    char *vdisk_id;
    InetSocketAddressBase *server;
    bool has_tls_creds;
    char *tls_creds;
};

void qapi_free_BlockdevOptionsVxHS(BlockdevOptionsVxHS *obj);

struct BlockdevOptionsThrottle {
    char *throttle_group;
    BlockdevRef *file;
};

void qapi_free_BlockdevOptionsThrottle(BlockdevOptionsThrottle *obj);

struct q_obj_BlockdevOptions_base {
    BlockdevDriver driver;
    bool has_node_name;
    char *node_name;
    bool has_discard;
    BlockdevDiscardOptions discard;
    bool has_cache;
    BlockdevCacheOptions *cache;
    bool has_read_only;
    bool read_only;
    bool has_force_share;
    bool force_share;
    bool has_detect_zeroes;
    BlockdevDetectZeroesOptions detect_zeroes;
};

struct BlockdevOptions {
    BlockdevDriver driver;
    bool has_node_name;
    char *node_name;
    bool has_discard;
    BlockdevDiscardOptions discard;
    bool has_cache;
    BlockdevCacheOptions *cache;
    bool has_read_only;
    bool read_only;
    bool has_force_share;
    bool force_share;
    bool has_detect_zeroes;
    BlockdevDetectZeroesOptions detect_zeroes;
    union { /* union tag is @driver */
        BlockdevOptionsBlkdebug blkdebug;
        BlockdevOptionsBlkverify blkverify;
        BlockdevOptionsGenericFormat bochs;
        BlockdevOptionsGenericFormat cloop;
        BlockdevOptionsGenericFormat dmg;
        BlockdevOptionsFile file;
        BlockdevOptionsCurlFtp ftp;
        BlockdevOptionsCurlFtps ftps;
        BlockdevOptionsGluster gluster;
        BlockdevOptionsFile host_cdrom;
        BlockdevOptionsFile host_device;
        BlockdevOptionsCurlHttp http;
        BlockdevOptionsCurlHttps https;
        BlockdevOptionsIscsi iscsi;
        BlockdevOptionsLUKS luks;
        BlockdevOptionsNbd nbd;
        BlockdevOptionsNfs nfs;
        BlockdevOptionsNull null_aio;
        BlockdevOptionsNull null_co;
        BlockdevOptionsNVMe nvme;
        BlockdevOptionsGenericFormat parallels;
        BlockdevOptionsQcow2 qcow2;
        BlockdevOptionsQcow qcow;
        BlockdevOptionsGenericCOWFormat qed;
        BlockdevOptionsQuorum quorum;
        BlockdevOptionsRaw raw;
        BlockdevOptionsRbd rbd;
        BlockdevOptionsReplication replication;
        BlockdevOptionsSheepdog sheepdog;
        BlockdevOptionsSsh ssh;
        BlockdevOptionsThrottle throttle;
        BlockdevOptionsGenericFormat vdi;
        BlockdevOptionsGenericFormat vhdx;
        BlockdevOptionsGenericCOWFormat vmdk;
        BlockdevOptionsGenericFormat vpc;
        BlockdevOptionsVVFAT vvfat;
        BlockdevOptionsVxHS vxhs;
    } u;
};

void qapi_free_BlockdevOptions(BlockdevOptions *obj);

struct BlockdevRef {
    QType type;
    union { /* union tag is @type */
        BlockdevOptions definition;
        char *reference;
    } u;
};

void qapi_free_BlockdevRef(BlockdevRef *obj);

struct BlockdevRefOrNull {
    QType type;
    union { /* union tag is @type */
        BlockdevOptions definition;
        char *reference;
        QNull *null;
    } u;
};

void qapi_free_BlockdevRefOrNull(BlockdevRefOrNull *obj);

struct q_obj_blockdev_del_arg {
    char *node_name;
};

struct BlockdevCreateOptionsFile {
    char *filename;
    uint64_t size;
    bool has_preallocation;
    PreallocMode preallocation;
    bool has_nocow;
    bool nocow;
};

void qapi_free_BlockdevCreateOptionsFile(BlockdevCreateOptionsFile *obj);

struct BlockdevCreateOptionsGluster {
    BlockdevOptionsGluster *location;
    uint64_t size;
    bool has_preallocation;
    PreallocMode preallocation;
};

void qapi_free_BlockdevCreateOptionsGluster(BlockdevCreateOptionsGluster *obj);

struct BlockdevCreateOptionsLUKS {
    /* Members inherited from QCryptoBlockCreateOptionsLUKS: */
    bool has_key_secret;
    char *key_secret;
    bool has_cipher_alg;
    QCryptoCipherAlgorithm cipher_alg;
    bool has_cipher_mode;
    QCryptoCipherMode cipher_mode;
    bool has_ivgen_alg;
    QCryptoIVGenAlgorithm ivgen_alg;
    bool has_ivgen_hash_alg;
    QCryptoHashAlgorithm ivgen_hash_alg;
    bool has_hash_alg;
    QCryptoHashAlgorithm hash_alg;
    bool has_iter_time;
    int64_t iter_time;
    /* Own members: */
    BlockdevRef *file;
    uint64_t size;
};

static inline QCryptoBlockCreateOptionsLUKS *qapi_BlockdevCreateOptionsLUKS_base(const BlockdevCreateOptionsLUKS *obj)
{
    return (QCryptoBlockCreateOptionsLUKS *)obj;
}

void qapi_free_BlockdevCreateOptionsLUKS(BlockdevCreateOptionsLUKS *obj);

struct BlockdevCreateOptionsNfs {
    BlockdevOptionsNfs *location;
    uint64_t size;
};

void qapi_free_BlockdevCreateOptionsNfs(BlockdevCreateOptionsNfs *obj);

struct BlockdevCreateOptionsParallels {
    BlockdevRef *file;
    uint64_t size;
    bool has_cluster_size;
    uint64_t cluster_size;
};

void qapi_free_BlockdevCreateOptionsParallels(BlockdevCreateOptionsParallels *obj);

struct BlockdevCreateOptionsQcow {
    BlockdevRef *file;
    uint64_t size;
    bool has_backing_file;
    char *backing_file;
    bool has_encrypt;
    QCryptoBlockCreateOptions *encrypt;
};

void qapi_free_BlockdevCreateOptionsQcow(BlockdevCreateOptionsQcow *obj);

struct BlockdevCreateOptionsQcow2 {
    BlockdevRef *file;
    uint64_t size;
    bool has_version;
    BlockdevQcow2Version version;
    bool has_backing_file;
    char *backing_file;
    bool has_backing_fmt;
    BlockdevDriver backing_fmt;
    bool has_encrypt;
    QCryptoBlockCreateOptions *encrypt;
    bool has_cluster_size;
    uint64_t cluster_size;
    bool has_preallocation;
    PreallocMode preallocation;
    bool has_lazy_refcounts;
    bool lazy_refcounts;
    bool has_refcount_bits;
    int64_t refcount_bits;
};

void qapi_free_BlockdevCreateOptionsQcow2(BlockdevCreateOptionsQcow2 *obj);

struct BlockdevCreateOptionsQed {
    BlockdevRef *file;
    uint64_t size;
    bool has_backing_file;
    char *backing_file;
    bool has_backing_fmt;
    BlockdevDriver backing_fmt;
    bool has_cluster_size;
    uint64_t cluster_size;
    bool has_table_size;
    int64_t table_size;
};

void qapi_free_BlockdevCreateOptionsQed(BlockdevCreateOptionsQed *obj);

struct BlockdevCreateOptionsRbd {
    BlockdevOptionsRbd *location;
    uint64_t size;
    bool has_cluster_size;
    uint64_t cluster_size;
};

void qapi_free_BlockdevCreateOptionsRbd(BlockdevCreateOptionsRbd *obj);

struct SheepdogRedundancyFull {
    int64_t copies;
};

void qapi_free_SheepdogRedundancyFull(SheepdogRedundancyFull *obj);

struct SheepdogRedundancyErasureCoded {
    int64_t data_strips;
    int64_t parity_strips;
};

void qapi_free_SheepdogRedundancyErasureCoded(SheepdogRedundancyErasureCoded *obj);

struct q_obj_SheepdogRedundancy_base {
    SheepdogRedundancyType type;
};

struct SheepdogRedundancy {
    SheepdogRedundancyType type;
    union { /* union tag is @type */
        SheepdogRedundancyFull full;
        SheepdogRedundancyErasureCoded erasure_coded;
    } u;
};

void qapi_free_SheepdogRedundancy(SheepdogRedundancy *obj);

struct BlockdevCreateOptionsSheepdog {
    BlockdevOptionsSheepdog *location;
    uint64_t size;
    bool has_backing_file;
    char *backing_file;
    bool has_preallocation;
    PreallocMode preallocation;
    bool has_redundancy;
    SheepdogRedundancy *redundancy;
    bool has_object_size;
    uint64_t object_size;
};

void qapi_free_BlockdevCreateOptionsSheepdog(BlockdevCreateOptionsSheepdog *obj);

struct BlockdevCreateOptionsSsh {
    BlockdevOptionsSsh *location;
    uint64_t size;
};

void qapi_free_BlockdevCreateOptionsSsh(BlockdevCreateOptionsSsh *obj);

struct BlockdevCreateOptionsVdi {
    BlockdevRef *file;
    uint64_t size;
    bool has_preallocation;
    PreallocMode preallocation;
};

void qapi_free_BlockdevCreateOptionsVdi(BlockdevCreateOptionsVdi *obj);

struct BlockdevCreateOptionsVhdx {
    BlockdevRef *file;
    uint64_t size;
    bool has_log_size;
    uint64_t log_size;
    bool has_block_size;
    uint64_t block_size;
    bool has_subformat;
    BlockdevVhdxSubformat subformat;
    bool has_block_state_zero;
    bool block_state_zero;
};

void qapi_free_BlockdevCreateOptionsVhdx(BlockdevCreateOptionsVhdx *obj);

struct BlockdevCreateOptionsVpc {
    BlockdevRef *file;
    uint64_t size;
    bool has_subformat;
    BlockdevVpcSubformat subformat;
    bool has_force_size;
    bool force_size;
};

void qapi_free_BlockdevCreateOptionsVpc(BlockdevCreateOptionsVpc *obj);

struct BlockdevCreateNotSupported {
    char qapi_dummy_for_empty_struct;
};

void qapi_free_BlockdevCreateNotSupported(BlockdevCreateNotSupported *obj);

struct q_obj_BlockdevCreateOptions_base {
    BlockdevDriver driver;
};

struct BlockdevCreateOptions {
    BlockdevDriver driver;
    union { /* union tag is @driver */
        BlockdevCreateNotSupported blkdebug;
        BlockdevCreateNotSupported blkverify;
        BlockdevCreateNotSupported bochs;
        BlockdevCreateNotSupported cloop;
        BlockdevCreateNotSupported dmg;
        BlockdevCreateOptionsFile file;
        BlockdevCreateNotSupported ftp;
        BlockdevCreateNotSupported ftps;
        BlockdevCreateOptionsGluster gluster;
        BlockdevCreateNotSupported host_cdrom;
        BlockdevCreateNotSupported host_device;
        BlockdevCreateNotSupported http;
        BlockdevCreateNotSupported https;
        BlockdevCreateNotSupported iscsi;
        BlockdevCreateOptionsLUKS luks;
        BlockdevCreateNotSupported nbd;
        BlockdevCreateOptionsNfs nfs;
        BlockdevCreateNotSupported null_aio;
        BlockdevCreateNotSupported null_co;
        BlockdevCreateNotSupported nvme;
        BlockdevCreateOptionsParallels parallels;
        BlockdevCreateOptionsQcow qcow;
        BlockdevCreateOptionsQcow2 qcow2;
        BlockdevCreateOptionsQed qed;
        BlockdevCreateNotSupported quorum;
        BlockdevCreateNotSupported raw;
        BlockdevCreateOptionsRbd rbd;
        BlockdevCreateNotSupported replication;
        BlockdevCreateOptionsSheepdog sheepdog;
        BlockdevCreateOptionsSsh ssh;
        BlockdevCreateNotSupported throttle;
        BlockdevCreateOptionsVdi vdi;
        BlockdevCreateOptionsVhdx vhdx;
        BlockdevCreateNotSupported vmdk;
        BlockdevCreateOptionsVpc vpc;
        BlockdevCreateNotSupported vvfat;
        BlockdevCreateNotSupported vxhs;
    } u;
};

void qapi_free_BlockdevCreateOptions(BlockdevCreateOptions *obj);

struct q_obj_blockdev_open_tray_arg {
    bool has_device;
    char *device;
    bool has_id;
    char *id;
    bool has_force;
    bool force;
};

struct q_obj_blockdev_close_tray_arg {
    bool has_device;
    char *device;
    bool has_id;
    char *id;
};

struct q_obj_blockdev_remove_medium_arg {
    char *id;
};

struct q_obj_blockdev_insert_medium_arg {
    char *id;
    char *node_name;
};

struct q_obj_blockdev_change_medium_arg {
    bool has_device;
    char *device;
    bool has_id;
    char *id;
    char *filename;
    bool has_format;
    char *format;
    bool has_read_only_mode;
    BlockdevChangeReadOnlyMode read_only_mode;
};

struct q_obj_BLOCK_IMAGE_CORRUPTED_arg {
    char *device;
    bool has_node_name;
    char *node_name;
    char *msg;
    bool has_offset;
    int64_t offset;
    bool has_size;
    int64_t size;
    bool fatal;
};

struct q_obj_BLOCK_IO_ERROR_arg {
    char *device;
    bool has_node_name;
    char *node_name;
    IoOperationType operation;
    BlockErrorAction action;
    bool has_nospace;
    bool nospace;
    char *reason;
};

struct q_obj_BLOCK_JOB_COMPLETED_arg {
    BlockJobType type;
    char *device;
    int64_t len;
    int64_t offset;
    int64_t speed;
    bool has_error;
    char *error;
};

struct q_obj_BLOCK_JOB_CANCELLED_arg {
    BlockJobType type;
    char *device;
    int64_t len;
    int64_t offset;
    int64_t speed;
};

struct q_obj_BLOCK_JOB_ERROR_arg {
    char *device;
    IoOperationType operation;
    BlockErrorAction action;
};

struct q_obj_BLOCK_JOB_READY_arg {
    BlockJobType type;
    char *device;
    int64_t len;
    int64_t offset;
    int64_t speed;
};

struct q_obj_BLOCK_JOB_PENDING_arg {
    BlockJobType type;
    char *id;
};

struct q_obj_BLOCK_WRITE_THRESHOLD_arg {
    char *node_name;
    uint64_t amount_exceeded;
    uint64_t write_threshold;
};

struct q_obj_block_set_write_threshold_arg {
    char *node_name;
    uint64_t write_threshold;
};

struct q_obj_x_blockdev_change_arg {
    char *parent;
    bool has_child;
    char *child;
    bool has_node;
    char *node;
};

struct q_obj_x_blockdev_set_iothread_arg {
    char *node_name;
    StrOrNull *iothread;
    bool has_force;
    bool force;
};

#endif /* QAPI_TYPES_BLOCK_CORE_H */
