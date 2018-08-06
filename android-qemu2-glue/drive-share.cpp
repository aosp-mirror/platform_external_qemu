// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android-qemu2-glue/drive-share.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

#include "android/base/memory/LazyInstance.h"
#include "android/base/files/FileShareOpen.h"
#include "android/globals.h"
#include "android/multi-instance.h"
#include "android/utils/bufprint.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/path.h"
#include "android/utils/tempfile.h"

extern "C" {
#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qemu/config-file.h"
#include "qemu/option_int.h"
#include "sysemu/block-backend.h"
#include "block/block.h"
#include "block/block_int.h"
#include "sysemu/blockdev.h"
#include "sysemu/sysemu.h"
#include "qapi/qmp/qstring.h"
#include "qapi/error.h"
}

namespace {
    struct DriveShare {
        std::vector<DriveInfo*> driveInfoList = {};
        std::vector<QemuOpts*> qemuOptsList = {};
        std::unordered_map<std::string, std::string> srcImagePaths = {};
        BlockInterfaceType blockDefaultType;
    };
    static android::base::LazyInstance<DriveShare> sDriveShare =
            LAZY_INSTANCE_INIT;
    struct DriveInitParam {
        BlockInterfaceType blockDefaultType;
        android::base::FileShare shareMode;
    };
}

#define QCOW2_SUFFIX "qcow2"

typedef struct {
    const char* drive;
    const char* backing_image_path;
} DriveBackingPair;

static bool read_file_to_buf(const char* file, uint8_t* buf, size_t size){
    int fd = HANDLE_EINTR(open(file, O_RDONLY | O_BINARY));
    if (fd < 0) return false;
    ssize_t ret = HANDLE_EINTR(read(fd, buf, size));
    IGNORE_EINTR(close(fd));
    if (ret != size) return false;
    return true;
}

static bool parseQemuOptForQcow2(bool wipeData) {
    /* First, determine if any of the backing images have been altered.
     * QCoW2 images won't work in that case, and need to be recreated (this
     * will obliterate previous snapshots).
     * The most reliable way to do this is to cache some sort of checksum of
     * the image files, but we go the easier (and faster) route and cache the
     * version number that is specified in build.prop.
     */
    const char* avd_data_dir = avdInfo_getContentPath(android_avdInfo);
    static const char sysimg_version_number_cache_basename[] =
        "version_num.cache";
    char* sysimg_version_number_cache_path =
        path_join(avd_data_dir, sysimg_version_number_cache_basename);
    bool reset_version_number_cache = false;
    if (!path_exists(sysimg_version_number_cache_path)) {
        /* File with previously saved version number doesn't exist,
         * we'll create it later.
         */
        reset_version_number_cache = true;
    } else {
        FILE* vn_cache_file = fopen(sysimg_version_number_cache_path, "r");
        int sysimg_version_number = -1;
        /* If the file with version number contained an error, or the
         * saved version number doesn't match the current one, we'll
         * update it later.
         */
        reset_version_number_cache =
            vn_cache_file == NULL ||
            fscanf(vn_cache_file, "%d", &sysimg_version_number) != 1 ||
            sysimg_version_number !=
                avdInfo_getSysImgIncrementalVersion(android_avdInfo);
        if (vn_cache_file) {
            fclose(vn_cache_file);
        }
    }

    /* List of paths to all images that can be mounted.*/
    const DriveBackingPair image_paths[] = {
        {"system", android_hw->disk_systemPartition_path ?: android_hw->disk_systemPartition_initPath},
        {"vendor", android_hw->disk_vendorPartition_path ?: android_hw->disk_vendorPartition_initPath},
        {"cache", android_hw->disk_cachePartition_path},
        {"userdata", android_hw->disk_dataPartition_path},
        {"sdcard", android_hw->hw_sdCard_path},
        {"encrypt", android_hw->disk_encryptionKeyPartition_path},
    };
    /* List of paths to all images for cros.*/
    const DriveBackingPair image_paths_hw_arc[] = {
        {"system", android_hw->disk_systemPartition_initPath},
        {"vendor", android_hw->disk_vendorPartition_initPath},
        {"userdata", android_hw->disk_dataPartition_path},
    };
    int count = sizeof(image_paths) / sizeof(image_paths[0]);
    const DriveBackingPair* images = image_paths;
    if (android_hw->hw_arc) {
        count = sizeof(image_paths_hw_arc) / sizeof(image_paths_hw_arc[0]);
        images = image_paths_hw_arc;
    }
    int p;
    for (p = 0; p < count; p++) {
        sDriveShare->srcImagePaths.emplace(images[p].drive,
                qemu_opt_get(qemu_opts_find(qemu_find_opts("drive"),
                    images[p].drive), "file"));
        const char* backing_image_path = images[p].backing_image_path;
        if (!backing_image_path ||
            *backing_image_path == '\0') {
            /* If the path is NULL or empty, just ignore it.*/
            continue;
        }
        char* image_basename = path_basename(backing_image_path);
        char* qcow2_path_buffer = NULL;
        const char* qcow2_image_path = NULL;
        bool need_create_tmp = false;
        // ChromeOS and Android pass parameters differently
        if (android_hw->hw_arc) {
            if (p < 2) {
                /* System & vendor image are special cases, the backing image is
                 * in the SDK folder, but the QCoW2 image that the emulator
                 * uses is created on a per-AVD basis and is placed in the
                 * AVD's data folder.
                 */
                const char qcow2_suffix[] = "." QCOW2_SUFFIX;
                size_t path_size = strlen(image_basename) + sizeof(qcow2_suffix)
                        + 1;
                char* image_qcow2_basename = (char*)malloc(path_size);
                bufprint(image_qcow2_basename, image_qcow2_basename + path_size,
                        "%s%s", image_basename, qcow2_suffix);
                qcow2_path_buffer =
                    path_join(avd_data_dir, image_qcow2_basename);
                free(image_qcow2_basename);
            } else {
                /* For all the other images except system image,
                 * just create another file alongside them
                 * with a 'qcow2' extension
                 */
                const char qcow2_suffix[] = "." QCOW2_SUFFIX;
                size_t path_size = strlen(backing_image_path) + sizeof(
                        qcow2_suffix) + 1;
                qcow2_path_buffer = (char*)malloc(path_size);
                bufprint(qcow2_path_buffer, qcow2_path_buffer + path_size,
                        "%s%s", backing_image_path, qcow2_suffix);
            }
            qcow2_image_path = qcow2_path_buffer;
            sDriveShare->srcImagePaths[images[p].drive] = qcow2_image_path;
        } else {
            QemuOpts* opts = qemu_opts_find(qemu_find_opts("drive"),
                    images[p].drive);
            if (!opts) {
                fprintf(stderr, "drive %s not found\n", images[p].drive);
                continue;
            }
            qcow2_image_path = qemu_opt_get(opts, "file");
            const char qcow2_suffix[] = "." QCOW2_SUFFIX;
            if (strcmp(qcow2_suffix, qcow2_image_path
                    + strlen(qcow2_image_path) - strlen(qcow2_suffix))) {
                // We are not using qcow2
                fprintf(stderr, "emplaced drive %s->%s\n", images[p].drive,
                        sDriveShare->srcImagePaths[images[p].drive].c_str());
                continue;
            }
        }
        fprintf(stderr, "emplaced drive %s->%s\n", images[p].drive,
                sDriveShare->srcImagePaths[images[p].drive].c_str());

        Error* img_creation_error = NULL;
        if (!path_exists(qcow2_image_path) ||
            wipeData ||
            reset_version_number_cache) {
            const char* fmt = "raw";
            uint8_t buf[BLOCK_PROBE_BUF_SIZE];
            BlockDriver *drv;
            drv = bdrv_find_format("qcow2");
            if (drv && read_file_to_buf(backing_image_path, buf, sizeof(buf)) &&
                drv->bdrv_probe(buf, sizeof(buf), backing_image_path) >= 100) {
                fmt = "qcow2";
            }
            bdrv_img_create(
                qcow2_image_path,
                QCOW2_SUFFIX,
                /*absolute path only for sys vendor*/
                p < 2 ? backing_image_path : image_basename,
                fmt,
                NULL,
                -1,
                0,
                &img_creation_error,
                true);
        }
        free(image_basename);
        free(qcow2_path_buffer);
        if (img_creation_error) {
            error_report("%s", error_get_pretty(img_creation_error));
            return false;
        }
    }

    /* Update version number cache if necessary. */
    if (reset_version_number_cache) {
        FILE* vn_cache_file = fopen(sysimg_version_number_cache_path, "w");
        if (vn_cache_file) {
            fprintf(vn_cache_file,
                    "%d\n",
                    avdInfo_getSysImgIncrementalVersion(android_avdInfo));
            fclose(vn_cache_file);
        }
    }
    free(sysimg_version_number_cache_path);

    return true;
}

static std::string initMountDrivePath(const char* id,
        android::base::FileShare shareMode) {
    assert(sDriveShare->srcImagePaths.count(id));
    if (shareMode == android::base::FileShare::Write
            || strcmp("system", id)
            || strcmp("vendor", id)) {
        return sDriveShare->srcImagePaths[id];
    } else {
        // Create a temp qcow2-on-qcow2
        Error* img_creation_error = NULL;
        TempFile* img = tempfile_create();
        const char* imgPath = tempfile_path(img);
        bdrv_img_create(imgPath,
                QCOW2_SUFFIX,
                sDriveShare->srcImagePaths[id].c_str(),
                "qcow2",
                nullptr,
                -1,
                0,
                &img_creation_error,
                true);
        if (img_creation_error) {
            tempfile_close(img);
            error_report("%s", error_get_pretty(img_creation_error));
            return "";
        }
        return imgPath;
    }
}

static void printBdrv(BlockDriverState* bs) {
    fprintf(stderr, "bs %p refcnt %d file %s backing file %s\n",
            bs, bs->refcnt, bs->filename, bs->backing_file);
}

// This is for C-style function pointer
extern "C" {
static int drive_init(void* opaque, QemuOpts* opts,
        Error** errp) {
    DriveInitParam* param = (DriveInitParam*)opaque;
    const char* id = opts->id;
    fprintf(stderr, "id %s file %s\n", id, qemu_opt_get(opts, "file"));
    if (id) {
        std::string path = initMountDrivePath(id, param->shareMode);
        qemu_opt_set(opts, "file", path.c_str(), errp);
    }
    DriveInfo* driveInfo = drive_new(opts, param->blockDefaultType);
    assert(driveInfo);
    return nullptr == driveInfo;
}

static int drive_reinit(void* opaque, QemuOpts* opts,
        Error** errp) {
    DriveInitParam* param = (DriveInitParam*)opaque;
    const char* id = opts->id;
    fprintf(stderr, "id %s file %s\n", id, qemu_opt_get(opts, "file"));
    if (id) {
        std::string path = initMountDrivePath(id, param->shareMode);
        qemu_opt_set(opts, "file", path.c_str(), errp);
    }
    QDict *bs_opts = qdict_new();
    qemu_opts_to_qdict(opts, bs_opts);
    const char* file = qemu_opt_get(opts, "file");
    QemuOpts* legacy_opts = qemu_opts_create(&qemu_legacy_drive_opts, NULL, 0,
                               nullptr);
    qemu_opts_absorb_qdict(legacy_opts, bs_opts, nullptr);
    //const char* id = qdict_get_try_str(bs_opts, "id");
    QemuOpts* drive_opts = qemu_opts_create(&qemu_common_drive_opts, id, 1,
            nullptr);
    qemu_opts_absorb_qdict(drive_opts, bs_opts, nullptr);

    bool read_only = qemu_opt_get_bool(legacy_opts, BDRV_OPT_READ_ONLY, false);
    printf("read only %d\n", read_only);
    printf("id %s\n", id);
    qdict_set_default_str(bs_opts, BDRV_OPT_CACHE_DIRECT, "off");
    qdict_set_default_str(bs_opts, BDRV_OPT_CACHE_NO_FLUSH, "off");
    qdict_set_default_str(bs_opts, BDRV_OPT_READ_ONLY,
            read_only ? "on" : "off");
    qdict_del(bs_opts, "id");

    printf("creating new drive %s\n", file);
    assert(file && file[0]);
    Error *local_err = NULL;
    printf("filename: %s\n", file);
    qdict_print(bs_opts);
    BlockDriverState *bs = bdrv_open(file, nullptr, bs_opts, 0, &local_err);
    QDECREF(bs_opts);
    if (local_err) {
        error_report_err(local_err);
        assert(false);
    }

    BlockBackend* blk = blk_by_name(id);
    assert(blk);
    assert(bs);
    int res = blk_insert_bs(blk, bs, nullptr);
    assert(res == 0);
    return res;
}
}

static void updateDriveShareMode(android::base::FileShare shareMode) {
    //for (auto& driveInfo : sDriveShare->driveInfoList) {
    //    drive_uninit(driveInfo);
    //}
    BdrvNextIterator it;
    BlockDriverState* bs = bdrv_first(&it);
    while (bs) {
        printBdrv(bs);
        bs = bdrv_next(&it);
    }
    std::vector<BlockBackend*> blkList;
    blkList.reserve(sDriveShare->driveInfoList.size());
    for (const auto& driveInfo : sDriveShare->driveInfoList) {
        for (BlockBackend* blk = blk_next(NULL); blk; blk = blk_next(blk)) {
            if (blk_legacy_dinfo(blk) == driveInfo) {
                blkList.push_back(blk);
                break;
            }
        }
    }
    assert(blkList.size() == sDriveShare->driveInfoList.size());
    printf("close all bdrv\n");
    //blockdev_close_all_bdrv_states();
    /*bs = bdrv_first(&it);
    while (bs) {
        printBdrv(bs);
        bdrv_unref(bs);
        bs = bdrv_first(&it);
    }*/
    bdrv_close_all();
    printf("close all bdrv done\n");
    bs = bdrv_first(&it);
    while (bs) {
        printBdrv(bs);
        bs = bdrv_next(&it);
    }
    DriveInitParam param = {sDriveShare->blockDefaultType, shareMode};
    if (qemu_opts_foreach(qemu_find_opts("drive"), drive_reinit,
                          &param, NULL)) {
        assert(false);
    }
}

extern "C" int android_drive_share_init(bool wipe_data, bool read_only,
        BlockInterfaceType blockDefaultType) {
    if (!parseQemuOptForQcow2(wipe_data)) {
        return -1;
    }
    android::multiinstance::setUpdateDriveShareModeFunc(updateDriveShareMode);
    DriveInitParam param = {blockDefaultType,
            read_only ? android::base::FileShare::Read
                      : android::base::FileShare::Write};
    sDriveShare->blockDefaultType = blockDefaultType;
    if (qemu_opts_foreach(qemu_find_opts("drive"), drive_init,
                          &param, NULL)) {
        return -1;
    }
    return 0;
}

