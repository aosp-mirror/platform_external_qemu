// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/Snapshotter.h"

#include "android/base/files/PathUtils.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/Stopwatch.h"
#include "android/base/StringFormat.h"
#include "android/crashreport/CrashReporter.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/emulation/VmLock.h"
#include "android/globals.h"
#include "android/metrics/AdbLivenessChecker.h"
#include "android/metrics/metrics.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/metrics/StudioConfig.h"
#include "android/opengl/emugl_config.h"
#include "android/snapshot/Hierarchy.h"
#include "android/snapshot/Loader.h"
#include "android/snapshot/PathUtils.h"
#include "android/snapshot/Quickboot.h"
#include "android/snapshot/Saver.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"
#include "android/snapshot/interface.h"
#include "android/utils/debug.h"
#include "android/utils/path.h"
#include "android/utils/system.h"

#include <cassert>
#include <utility>

extern "C" {
#include <emmintrin.h>
}

using android::base::LazyInstance;
using android::base::PathUtils;
using android::base::Stopwatch;
using android::base::StringFormat;
using android::base::System;
using android::crashreport::CrashReporter;
using android::metrics::MetricsReporter;
namespace pb = android_studio;

// Inspired by QEMU's bufferzero.c implementation, but simplified for the case
// when checking the whole aligned memory page.
static bool buffer_zero_sse2(const void* buf, int len) {
    buf = __builtin_assume_aligned(buf, 1024);
    __m128i t = _mm_load_si128(static_cast<const __m128i*>(buf));
    auto p = reinterpret_cast<__m128i*>(
            (reinterpret_cast<intptr_t>(buf) + 5 * 16));
    auto e =
            reinterpret_cast<__m128i*>((reinterpret_cast<intptr_t>(buf) + len));
    const __m128i zero = _mm_setzero_si128();

    /* Loop over 16-byte aligned blocks of 64.  */
    do {
        __builtin_prefetch(p);
        t = _mm_cmpeq_epi32(t, zero);
        if (_mm_movemask_epi8(t) != 0xFFFF) {
            return false;
        }
#ifdef _MSC_VER
        t = _mm_or_si128(_mm_or_si128(p[-4], p[-3]), _mm_or_si128(p[-2], p[-1]));
#else
        t = p[-4] | p[-3] | p[-2] | p[-1];
#endif
        p += 4;
    } while (p <= e);

    /* Finish the aligned tail.  */
#ifdef _WIN32
    t = _mm_or_si128(t, e[-3]);
    t = _mm_or_si128(t, e[-2]);
    t = _mm_or_si128(t, e[-1]);
#else
    t |= e[-3];
    t |= e[-2];
    t |= e[-1];
#endif
    return _mm_movemask_epi8(_mm_cmpeq_epi32(t, zero)) == 0xFFFF;
}

namespace android {
namespace snapshot {

static const System::Duration kSnapshotCrashThresholdMs = 120000; // 2 minutes

// TODO: implement an optimized SSE4 version and dynamically select it if host
// supports SSE4.
bool isBufferZeroed(const void* ptr, int32_t size) {
    assert((uintptr_t(ptr) & (1024 - 1)) == 0);  // page-aligned
    assert(size >= 1024);  // at least one small page in |size|
    return buffer_zero_sse2(ptr, size);
}

Snapshotter::Snapshotter() = default;

Snapshotter::~Snapshotter() {
    if (mVmOperations.setSnapshotCallbacks) {
        mVmOperations.setSnapshotCallbacks(nullptr, nullptr);
    }
}

static LazyInstance<Snapshotter> sInstance = {};

Snapshotter& Snapshotter::get() {
    return sInstance.get();
}

void Snapshotter::initialize(const QAndroidVmOperations& vmOperations,
                             const QAndroidEmulatorWindowAgent& windowAgent) {
    static const SnapshotCallbacks kCallbacks = {
            // ops
            {
                    // save
                    {// onStart
                     [](void* opaque, const char* name) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         return snapshot->onStartSaving(name) ? 0 : -1;
                     },
                     // onEnd
                     [](void* opaque, const char* name, int res) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         snapshot->onSavingComplete(name, res);
                     },
                     // onQuickFail
                     [](void* opaque, const char* name, int res) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         snapshot->onSavingFailed(name, res);
                     },
                     // isCanceled
                     [](void* opaque, const char* name) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         return snapshot->isSavingCanceled(name);
                     }},
                    // load
                    {// onStart
                     [](void* opaque, const char* name) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         return snapshot->onStartLoading(name) ? 0 : -1;
                     },
                     // onEnd
                     [](void* opaque, const char* name, int res) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         snapshot->onLoadingComplete(name, res);
                     },
                     // onQuickFail
                     [](void* opaque, const char* name, int res) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         snapshot->onLoadingFailed(name, res);
                     },
                     // isCanceled
                     [](void* opaque, const char* name) {
                         // TODO: Implement load cancel if necessary
                         return false;
                     }},
                    // del
                    {// onStart
                     [](void* opaque, const char* name) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         return snapshot->onStartDelete(name) ? 0 : -1;
                     },
                     // onEnd
                     [](void* opaque, const char* name, int res) {
                         auto snapshot = static_cast<Snapshotter*>(opaque);
                         snapshot->onDeletingComplete(name, res);
                     },
                     // onQuickFail
                     [](void*, const char*, int) {},
                     // isCanceled
                     [](void* opaque, const char* name) {
                         // TODO: Implement delete cancel if necessary
                         return false;
                     }},
            },
            // ramOps
            {// registerBlock
             [](void* opaque, SnapshotOperation operation,
                const RamBlock* block) {
                 auto snapshot = static_cast<Snapshotter*>(opaque);
                 if (operation == SNAPSHOT_LOAD) {
                     snapshot->mLoader->ramLoader().registerBlock(*block);
                 } else if (operation == SNAPSHOT_SAVE) {
                     snapshot->mSaver->ramSaver().registerBlock(*block);
                 }
             },
             // startLoading
             [](void* opaque) {
                 auto snapshot = static_cast<Snapshotter*>(opaque);
                 snapshot->mLoader->ramLoader().start(snapshot->isQuickboot());
                 return snapshot->mLoader->ramLoader().hasError() ? -1 : 0;
             },
             // savePage
             [](void* opaque, int64_t blockOffset, int64_t pageOffset,
                int32_t size) {
                 auto snapshot = static_cast<Snapshotter*>(opaque);
                 snapshot->mSaver->ramSaver().savePage(blockOffset, pageOffset,
                                                       size);
             },
             // savingComplete
             [](void* opaque) {
                 auto snapshot = static_cast<Snapshotter*>(opaque);
                 snapshot->mSaver->ramSaver().join();
                 return snapshot->mSaver->ramSaver().hasError() ? -1 : 0;
             },
             // loadRam
             [](void* opaque, void* hostRamPtr, uint64_t size) {
                 auto snapshot = static_cast<Snapshotter*>(opaque);

                 auto& loader = snapshot->mLoader;
                 if (!loader || loader->status() != OperationStatus::Ok ||
                     !loader->hasRamLoader()) {
                     return;
                 }

                 auto& ramLoader = loader->ramLoader();
                 if (ramLoader.onDemandEnabled() &&
                     !ramLoader.onDemandLoadingComplete()) {
                     ramLoader.loadRam(hostRamPtr, size);
                 }
             }}};

    assert(vmOperations.setSnapshotCallbacks);
    mVmOperations = vmOperations;
    mWindowAgent = windowAgent;
    mVmOperations.setSnapshotCallbacks(this, &kCallbacks);
}  // namespace snapshot

static constexpr int kDefaultMessageTimeoutMs = 10000;

static void appendFailedSave(pb::EmulatorSnapshotSaveState state,
                             FailureReason failureReason) {
    MetricsReporter::get().report([state, failureReason](pb::AndroidStudioEvent* event) {
        auto snap = event->mutable_emulator_details()->add_snapshot_saves();
        snap->set_save_state(state);
        snap->set_save_failure_reason((pb::EmulatorSnapshotFailureReason)failureReason);
    });
}

static void appendFailedLoad(pb::EmulatorSnapshotLoadState state,
                             FailureReason failureReason) {
    MetricsReporter::get().report([state, failureReason](pb::AndroidStudioEvent* event) {
        auto snap = event->mutable_emulator_details()->add_snapshot_loads();
        snap->set_load_state(state);
        snap->set_load_failure_reason((pb::EmulatorSnapshotFailureReason)failureReason);
    });
}

OperationStatus Snapshotter::prepareForLoading(const char* name) {
    if (mSaver && mSaver->snapshot().name() == name) {
        mSaver.reset();
    }
    mLoader.reset(new Loader(name));
    mLoader->prepare();
    return mLoader->status();
}

OperationStatus Snapshotter::load(bool isQuickboot, const char* name) {
    mLastLoadDuration = android::base::kNullopt;
    mIsQuickboot = isQuickboot;
    Stopwatch sw;
    mVmOperations.snapshotLoad(name, this, nullptr);
    mIsQuickboot = false;
    mLastLoadDuration.emplace(sw.elapsedUs() / 1000);

    // if we end up deleting in the loader (for whatever reaosn),
    // at least make sure mLoader is not null.
    if (!mLoader) {
        onLoadingFailed(name, -EINVAL);
    }

    // If -EINVAL was the failure error,
    // we didn't reallocate mLoader, or even deallocated it.
    // Quit early here.
    if (!mLoader) {
        return OperationStatus::Error;
    }

    mLoadedSnapshotFile =
            (mLoader->status() == OperationStatus::Ok) ? name : "";
    return mLoader->status();
}

void Snapshotter::finishLoading() {
    if (mLoader) {
        mLoader->join();
    }
}

void Snapshotter::prepareLoaderForSaving(const char* name) {
    if (!mLoader) {
        return;
    }
    if (mLoader->snapshot().name() != name ||
        mLoader->status() != OperationStatus::Ok) {
        mLoader.reset();
    } else {
        mLoader->synchronize(mIsOnExit && (mRamFile.empty() || !mRamFileShared));
    }
}

void Snapshotter::callCallbacks(Operation op, Stage stage) {
    for (auto&& cb : mCallbacks) {
        cb(op, stage);
    }
}

void Snapshotter::fillSnapshotMetrics(pb::AndroidStudioEvent* event,
                                      const SnapshotOperationStats& stats) {

    pb::EmulatorSnapshot* snapshot = nullptr;

    if (stats.forSave) {
        snapshot = event->mutable_emulator_details()->add_snapshot_saves();
    } else {
        snapshot = event->mutable_emulator_details()->add_snapshot_loads();
    }

    snapshot->set_name(MetricsReporter::get().anonymize(stats.name));

    if (stats.compressedRam) {
        snapshot->set_flags(pb::SNAPSHOT_FLAGS_RAM_COMPRESSED_BIT);
    }

    if (stats.compressedTextures) {
        snapshot->set_flags(snapshot->flags() | pb::SNAPSHOT_FLAGS_TEXTURES_COMPRESSED_BIT);
    }

    if (stats.usingHDD) {
        snapshot->set_flags(snapshot->flags() | pb::SNAPSHOT_FLAGS_HDD_BIT);
    }

    snapshot->set_lazy_loaded(stats.onDemandRamEnabled);
    snapshot->set_incrementally_saved(stats.incrementallySaved);

    snapshot->set_size_bytes(int64_t(stats.diskSize +
                                     stats.ramSize +
                                     stats.texturesSize));
    snapshot->set_ram_size_bytes(int64_t(stats.ramSize));
    snapshot->set_textures_size_bytes(int64_t(stats.texturesSize));

    if (stats.forSave) {
        snapshot->set_save_state(
                pb::EmulatorSnapshotSaveState::EMULATOR_SNAPSHOT_SAVE_SUCCEEDED_NORMAL);
        snapshot->set_save_duration_ms(uint64_t(stats.durationMs));
        snapshot->set_ram_save_duration_ms(int64_t(stats.ramDurationMs));
        snapshot->set_textures_save_duration_ms(int64_t(stats.texturesDurationMs));

    } else {
        snapshot->set_load_state(
                pb::EmulatorSnapshotLoadState::EMULATOR_SNAPSHOT_LOAD_SUCCEEDED_NORMAL);
        snapshot->set_load_duration_ms(uint64_t(stats.durationMs));
        snapshot->set_ram_load_duration_ms(int64_t(stats.ramDurationMs));
        snapshot->set_textures_load_duration_ms(int64_t(stats.texturesDurationMs));
    }

    // Also report some common host machine stats so we can correlate performance with
    // host machine config.
    auto memUsageProto = event->mutable_emulator_performance_stats()->add_memory_usage();
    memUsageProto->set_resident_memory(stats.memUsage.resident);
    memUsageProto->set_resident_memory_max(stats.memUsage.resident_max);
    memUsageProto->set_virtual_memory(stats.memUsage.virt);
    memUsageProto->set_virtual_memory_max(stats.memUsage.virt_max);
    memUsageProto->set_total_phys_memory(stats.memUsage.total_phys_memory);
    memUsageProto->set_total_page_file(stats.memUsage.total_page_file);
    android_metrics_fill_common_info(true /* opengl alive */, event);
}

Snapshotter::SnapshotOperationStats Snapshotter::getSaveStats(const char* name,
                                                              System::Duration durationMs) {
    auto& save = saver();
    const auto compressedRam = save.ramSaver().compressed();
    const auto compressedTextures = save.textureSaver()->compressed();
    const auto diskSize = save.snapshot().diskSize();
    const auto ramSize = save.ramSaver().diskSize();
    const auto texturesSize = save.textureSaver()->diskSize();

    System::Duration ramDurationMs = 0;
    System::Duration texturesDurationMs = 0;
    save.ramSaver().getDuration(&ramDurationMs); ramDurationMs /= 1000;
    save.textureSaver()->getDuration(&texturesDurationMs); texturesDurationMs /= 1000;

    return {
        true /* for save */,
        std::string(name),
        durationMs,
        false /* on-demand ram loading N/A for save */,
        save.incrementallySaved(),
        compressedRam,
        compressedTextures,
        save.memUsage(),
        save.isHDD(),
        (int64_t)diskSize,
        (int64_t)ramSize,
        (int64_t)texturesSize,
        ramDurationMs,
        texturesDurationMs,
    };
}

Snapshotter::SnapshotOperationStats Snapshotter::getLoadStats(const char* name,
                                                              System::Duration durationMs) {
    SnapshotOperationStats defaultStats;

    auto& load = loader();
    const auto onDemandRamEnabled = load.ramLoader().onDemandEnabled();
    const auto compressedRam = load.ramLoader().compressed();
    const auto compressedTextures = load.textureLoader()->compressed();
    const auto diskSize = load.snapshot().diskSize();
    const auto ramSize = load.ramLoader().diskSize();
    const auto texturesSize = load.textureLoader()->diskSize();
    System::Duration ramDurationMs = 0;
    load.ramLoader().getDuration(&ramDurationMs);
    ramDurationMs /= 1000;
    return {
        false /* not for save */,
        name,
        durationMs,
        onDemandRamEnabled,
        false /* not incrementally saved */,
        compressedRam,
        compressedTextures,
        load.memUsage(),
        load.isHDD(),
        (int64_t)diskSize,
        (int64_t)ramSize,
        (int64_t)texturesSize,
        ramDurationMs,
        0 /* TODO: texture lazy/bg load duration */,
    };
}


void Snapshotter::appendSuccessfulSave(const char* name,
                                       System::Duration durationMs) {
    if (!mSaver ||
        !mSaver->textureSaver()) return;

    auto stats = getSaveStats(name, durationMs);
    MetricsReporter::get().report([stats](pb::AndroidStudioEvent* event) {
        fillSnapshotMetrics(event, stats);
    });
}

void Snapshotter::appendSuccessfulLoad(const char* name,
                                       System::Duration durationMs) {
    if (!mLoader ||
        !mLoader->textureLoader()) return;

    loader().reportSuccessful();
    auto stats = getLoadStats(name, durationMs);
    MetricsReporter::get().report([stats](pb::AndroidStudioEvent* event) {
        fillSnapshotMetrics(event, stats);
    });
}

void Snapshotter::showError(const std::string& message) {
    mWindowAgent.showMessage(message.c_str(), WINDOW_MESSAGE_ERROR,
                             kDefaultMessageTimeoutMs);
    dwarning(message.c_str());
}

bool Snapshotter::checkSafeToSave(const char* name, bool reportMetrics) {
    const bool shouldTrySaving =
        isSnapshotAlive();

    if (!shouldTrySaving) {
        showError("Skipping snapshot save: "
                  "Emulator not booted (or ADB not online)");
        if (reportMetrics) {
            appendFailedSave(
                pb::EmulatorSnapshotSaveState::
                    EMULATOR_SNAPSHOT_SAVE_SKIPPED_NOT_BOOTED,
                FailureReason::AdbOffline);
        }
        return false;
    }

    if (!name) {
        showError("Skipping snapshot save: "
                  "Null snapshot name");
        if (reportMetrics) {
            appendFailedSave(
                pb::EmulatorSnapshotSaveState::
                    EMULATOR_SNAPSHOT_SAVE_SKIPPED_NO_SNAPSHOT,
                FailureReason::NoSnapshotPb);
        }
        return false;
    }

    if (!emuglConfig_current_renderer_supports_snapshot()) {
        showError(
            StringFormat("Skipping snapshot save: "
                         "Renderer type '%s' (%d) "
                         "doesn't support snapshotting",
                         emuglConfig_renderer_to_string(
                                 emuglConfig_get_current_renderer()),
                         int(emuglConfig_get_current_renderer())));
        if (reportMetrics) {
            appendFailedSave(pb::EmulatorSnapshotSaveState::
                                 EMULATOR_SNAPSHOT_SAVE_SKIPPED_UNSUPPORTED,
                             FailureReason::SnapshotsNotSupported);
        }
        return false;
    }

    // Check the disk capacity.
    // Snapshots vary in size. They can be close to a GB.
    // Rather than taking all the remaining disk space,
    // save only if we have 2 GB of space available.
    if (android_avdInfo &&
        System::isUnderDiskPressure(avdInfo_getContentPath(android_avdInfo))) {
        showError("Not saving snapshot: Disk space < 2 GB");
        if (reportMetrics) {
            appendFailedSave(
                pb::EmulatorSnapshotSaveState::
                    EMULATOR_SNAPSHOT_SAVE_SKIPPED_DISK_PRESSURE,
                FailureReason::OutOfDiskSpace);
        }
        return false;
    }

    // Check whether skipping snapshot saves was set.
    if (mVmOperations.isSnapshotSaveSkipped()) {
        showError("Skipping snapshot save: "
                  "current state "
                  "doesn't support snapshotting");
        if (reportMetrics) {
            appendFailedSave(pb::EmulatorSnapshotSaveState::
                                 EMULATOR_SNAPSHOT_SAVE_SKIPPED_UNSUPPORTED,
                             FailureReason::SnapshotsNotSupported);
        }
        return false;
    }

    return true;
}

bool Snapshotter::checkSafeToLoad(const char* name, bool reportMetrics) {
    if (!name) {
        showError("Skipping snapshot load: "
                  "Null snapshot name");
        if (reportMetrics) {
            appendFailedLoad(pb::EmulatorSnapshotLoadState::
                                 EMULATOR_SNAPSHOT_LOAD_NO_SNAPSHOT,
                             FailureReason::NoSnapshotPb);
        }
        return false;
    }

    if (!emuglConfig_current_renderer_supports_snapshot()) {
        showError(
            StringFormat("Skipping snapshot load of '%s': "
                         "Renderer type '%s' (%d) "
                         "doesn't support snapshotting",
                         name,
                         emuglConfig_renderer_to_string(
                                 emuglConfig_get_current_renderer()),
                         int(emuglConfig_get_current_renderer())));
        if (reportMetrics) {
            appendFailedLoad(pb::EmulatorSnapshotLoadState::
                                 EMULATOR_SNAPSHOT_LOAD_SKIPPED_UNSUPPORTED,
                             FailureReason::SnapshotsNotSupported);
        }
        return false;
    }
    return true;
}

void Snapshotter::handleGenericSave(const char* name,
                                    OperationStatus saveStatus,
                                    bool reportMetrics) {
    if (saveStatus != OperationStatus::Ok) {
        showError(
            StringFormat(
                "Snapshot save for snapshot '%s' failed. "
                "Cleaning it out", name));

        if (reportMetrics) {
            if (mSaver) {
                if (auto failureReason = saver().snapshot().failureReason()) {
                    appendFailedSave(pb::EmulatorSnapshotSaveState::
                                             EMULATOR_SNAPSHOT_SAVE_FAILED,
                                     *failureReason);
                } else {
                    appendFailedSave(pb::EmulatorSnapshotSaveState::
                                             EMULATOR_SNAPSHOT_SAVE_FAILED,
                                     FailureReason::InternalError);
                }
            } else {
                appendFailedSave(pb::EmulatorSnapshotSaveState::
                                         EMULATOR_SNAPSHOT_SAVE_FAILED,
                                 FailureReason::InternalError);
            }
        }

        deleteSnapshot(name);

    } else {
        if (reportMetrics) {
            appendSuccessfulSave(name,
                                 mLastSaveDuration ? *mLastSaveDuration : 1234);
        }
    }
}

void Snapshotter::handleGenericLoad(const char* name,
                                    OperationStatus loadStatus,
                                    bool reportMetrics) {
    if (loadStatus != OperationStatus::Ok && hasLoader()) {
        // Check if the error is about something done as just a check or
        // we've started actually loading the VM data
        if (auto failureReason = loader().snapshot().failureReason()) {
            if (reportMetrics) {
                appendFailedLoad(pb::EmulatorSnapshotLoadState::
                                     EMULATOR_SNAPSHOT_LOAD_FAILED,
                                 *failureReason);
            }
            if (*failureReason != FailureReason::Empty &&
                *failureReason < FailureReason::ValidationErrorLimit) {
                showError(
                    StringFormat(
                        "Snapshot '%s' can not be loaded (%d). "
                        "Continuing current session",
                        name, int(*failureReason)));
            } else {
                showError(
                    StringFormat(
                        "Snapshot '%s' can not be loaded (%d). "
                        "Fatal error, resetting current session",
                        name, int(*failureReason)));
                deleteSnapshot(name);
                mVmOperations.vmReset();
            }
        } else {
            showError(
                StringFormat(
                    "Snapshot '%s' can not be loaded (reason not set). "
                    "Fatal error, resetting current session", name));
            deleteSnapshot(name);
            mVmOperations.vmReset();
            if (reportMetrics) {
                appendFailedLoad(pb::EmulatorSnapshotLoadState::
                                     EMULATOR_SNAPSHOT_LOAD_FAILED,
                                 FailureReason::InternalError);
            }
        }
    } else {
        if (reportMetrics && hasLoader()) {
            appendSuccessfulLoad(name,
                    mLastLoadDuration ? *mLastLoadDuration : 0);
        }
    }
}

OperationStatus Snapshotter::prepareForSaving(const char* name) {
    prepareLoaderForSaving(name);
    mVmOperations.vmStop();
    mSaver.reset(new Saver(
            name, (mLoader && mLoader->hasRamLoader() &&
                   mLoader->status() != OperationStatus::Error)
                          ? &mLoader->ramLoader()
                          : nullptr,
            mIsOnExit,
            mRamFile,
            mRamFileShared,
            mIsRemapping));
    mVmOperations.vmStart();
    mSaver->prepare();
    return mSaver->status();
}

OperationStatus Snapshotter::save(bool isOnExit, const char* name) {
    mLastSaveDuration = android::base::kNullopt;
    mLastSaveUptimeMs =
        System::Duration(System::get()->getProcessTimes().wallClockMs);
    Stopwatch sw;
    mIsOnExit = isOnExit;
    if (mIsOnExit) {
        mVmOperations.setExiting();
    }
    mVmOperations.snapshotSave(name, this, nullptr);
    mLastSaveDuration.emplace(sw.elapsedUs() / 1000);
    // In unit tests, we don't have a saver, so trivially succeed.
    return mSaver ? mSaver->status() : OperationStatus::Ok;
}

void Snapshotter::setRamFile(const char* path, bool shared) {
    mRamFile = path;
    mRamFileShared = shared;
}

void Snapshotter::setRamFileShared(bool shared) {
    mRamFileShared = shared;
}

void Snapshotter::cancelSave() {
    if (!mSaver)
        return;

    mSaver->cancel();
}

bool Snapshotter::isSavingCanceled(const char* name) const {
    if (!mSaver)
        return false;

    if (name && (mSaver->snapshot().name() != base::StringView(name))) {
        return false;
    }

    return false;
}

OperationStatus Snapshotter::saveGeneric(const char* name) {
    OperationStatus res = OperationStatus::Error;
    if (checkSafeToSave(name)) {
        res = save(false /* not on exit */, name);
        handleGenericSave(name, res,
                          /* report metrics */ mSaver ? true : false);
    }
    return res;
}

OperationStatus Snapshotter::loadGeneric(const char* name) {
    OperationStatus res = OperationStatus::Error;
    if (checkSafeToLoad(name)) {
        res = load(false /* not quickboot */, name);
        handleGenericLoad(name, res);
    }
    return res;
}

void Snapshotter::deleteSnapshot(const char* name) {
    std::string nameWithStorage(name);
    fprintf(stderr, "%s: for %s\n", __func__, nameWithStorage.c_str());
    invalidateSnapshot(nameWithStorage.c_str());

    // then delete the folder and refresh hierarchy
    path_delete_dir(getSnapshotDir(nameWithStorage.c_str()).c_str());
    // bug: 129763714
    // Hierarchy::get()->currentInfo();
}

void Snapshotter::invalidateSnapshot(const char* name) {
    base::StringView nameString = name ? name : kDefaultBootSnapshot;
    auto nameValidated = base::c_str(nameString);

    Snapshot tombstone(nameValidated);

    if (name == mLoadedSnapshotFile) {
        // We're deleting the "loaded" snapshot, so first finish any pending
        // load, and then clear the snapshot file.  Do it under the VM lock to
        // finish background loading faster (i.e., no interference from VCPUs)
        CrashReporter::get()->hangDetector().pause(true);
        android::RecursiveScopedVmLock lock;
        if (mLoader && mLoader->status() == OperationStatus::Ok) {
            mLoader->synchronize(false /* not on exit */);
        }
        mLoadedSnapshotFile.clear();
        CrashReporter::get()->hangDetector().pause(false);
    }

    mIsInvalidating = true;
    mVmOperations.snapshotDelete(name, this, nullptr);

    // then delete kRamFileName / kTexturesFileName / kMappedRamFileName
    path_delete_file(
            PathUtils::join(getSnapshotDir(nameValidated), kRamFileName)
                    .c_str());
    path_delete_file(
            PathUtils::join(getSnapshotDir(nameValidated), kTexturesFileName)
                    .c_str());
    path_delete_file(
            PathUtils::join(getSnapshotDir(nameValidated), kMappedRamFileName)
                    .c_str());

    tombstone.saveFailure(FailureReason::Tombstone);
}

bool Snapshotter::areSavesSlow(const char* name) {
    Snapshot s(name);
    return s.areSavesSlow();
}

void Snapshotter::listSnapshots(void* opaque,
                                int (*cbOut)(void*, const char*, int),
                                int (*cbErr)(void*, const char*, int)) {
    mVmOperations.snapshotList(opaque, cbOut, cbErr);
}

void Snapshotter::onCrashedSnapshot(const char* name) {
    // if it's been less than 2 minutes since the load,
    // consider it a snapshot fail.
    if (System::Duration(System::get()->getProcessTimes().wallClockMs) -
                mLastLoadUptimeMs <
        kSnapshotCrashThresholdMs) {
        onLoadingFailed(name, -EINVAL);
    }
}

bool Snapshotter::onStartSaving(const char* name) {
    CrashReporter::get()->hangDetector().pause(true);
    callCallbacks(Operation::Save, Stage::Start);
    prepareLoaderForSaving(name);
    if (!mSaver || isComplete(*mSaver)) {
        mSaver.reset(new Saver(
                name, (mLoader && mLoader->hasRamLoader() &&
                       mLoader->status() != OperationStatus::Error)
                              ? &mLoader->ramLoader()
                              : nullptr,
                mIsOnExit,
                mRamFile,
                mRamFileShared,
                mIsRemapping));
    }
    if (mSaver->status() == OperationStatus::Error) {
        onSavingComplete(name, -1);
        return false;
    }
    return true;
}

bool Snapshotter::onSavingComplete(const char* name, int res) {
    assert(mSaver && name == mSaver->snapshot().name());
    mSaver->complete(res == 0);
    CrashReporter::get()->hangDetector().pause(false);
    callCallbacks(Operation::Save, Stage::End);
    bool good = mSaver->status() != OperationStatus::Error &&
                mSaver->status() != OperationStatus::Canceled;

    // bug: 129763714
    // if (good) {
    //     Hierarchy::get()->currentInfo();
    // }

    return good;
}

void Snapshotter::onSavingFailed(const char* name, int res) {
    // Well, we haven't started anything and it failed already - nothing to do.
}

bool Snapshotter::onStartLoading(const char* name) {
    mLoadedSnapshotFile.clear();
    CrashReporter::get()->hangDetector().pause(true);
    callCallbacks(Operation::Load, Stage::Start);
    mSaver.reset();
    if (!mLoader || isComplete(*mLoader)) {
        if (mLoader) {
            mLoader->interrupt();
        }
        mLoader.reset(new Loader(name));
    }
    mLoader->start();
    if (mLoader->status() == OperationStatus::Error) {
        onLoadingComplete(name, -1);
        return false;
    }
    return true;
}

bool Snapshotter::onLoadingComplete(const char* name, int res) {
    assert(mLoader && name == mLoader->snapshot().name());

    if (mUsingHdd) {
        finishLoading();
    }

    mLoader->complete(res == 0);

    CrashReporter::get()->hangDetector().pause(false);
    mLastLoadUptimeMs =
            System::Duration(System::get()->getProcessTimes().wallClockMs);
    callCallbacks(Operation::Load, Stage::End);
    if (mLoader->status() == OperationStatus::Error) {
        auto failureReason = mLoader->snapshot().failureReason();
        int failureReasonForQemu =
            (int)(failureReason ?
                  *failureReason : FailureReason::InternalError);
        mVmOperations.setFailureReason(
            name, failureReasonForQemu);
        return false;
    }
    mLoadedSnapshotFile = name;
    // bug: 129763714
    // if (good) {
    //     Hierarchy::get()->currentInfo();
    // }
    return true;
}

void Snapshotter::onLoadingFailed(const char* name, int err) {
    assert(err < 0);
    mSaver.reset();
    if (err == -EINVAL) {  // corrupted snapshot. abort immediately,
                           // try not to do anything since this could be
                           // in the crash handler
        if (mLoader) mLoader->onInvalidSnapshotLoad();
        return;
    }
    mLoader.reset(new Loader(name, -err));
    mLoader->complete(false);
    mLoadedSnapshotFile.clear();

    auto failureReason = mLoader->snapshot().failureReason();
    mVmOperations.setFailureReason(
        name, (int)(failureReason ?
                    *failureReason : errnoToFailure(-err)));
}

bool Snapshotter::onStartDelete(const char*) {
    CrashReporter::get()->hangDetector().pause(true);
    return true;
}

bool Snapshotter::onDeletingComplete(const char* name, int res) {
    if (res == 0) {
        if (mSaver && mSaver->snapshot().name() == name) {
            mSaver.reset();
        }
        if (mLoader && mLoader->snapshot().name() == name) {
            mLoader.reset();
        }
        if (!mIsInvalidating) {
            path_delete_dir(base::c_str(Snapshot::dataDir(name)));
        }
    }
    CrashReporter::get()->hangDetector().pause(false);
    mIsInvalidating = false;
    return true;
}

void Snapshotter::addOperationCallback(Callback&& cb) {
    if (cb) {
        mCallbacks.emplace_back(std::move(cb));
    }
}

}  // namespace snapshot
}  // namespace android

void androidSnapshot_initialize(
        const QAndroidVmOperations* vmOperations,
        const QAndroidEmulatorWindowAgent* windowAgent) {
    using android::base::Version;

    static constexpr auto kMinStudioVersion = Version(3, 0, 0);
    // Make sure the installed AndroidStudio is able to handle the snapshots
    // feature.
    if (isEnabled(android::featurecontrol::FastSnapshotV1)) {
        auto version = android::studio::lastestAndroidStudioVersion();
        if (version.isValid() && version < kMinStudioVersion) {
            auto prettyVersion = Version(version.component<Version::kMajor>(),
                                         version.component<Version::kMinor>(),
                                         version.component<Version::kMicro>());
            VERBOSE_PRINT(init,
                          "Disabling snapshot boot - need Android Studio %s "
                          " but found %s",
                          kMinStudioVersion.toString().c_str(),
                          prettyVersion.toString().c_str());
            setEnabledOverride(android::featurecontrol::FastSnapshotV1, false);
        }
    }

    android::snapshot::sInstance->initialize(*vmOperations, *windowAgent);
    android::snapshot::Quickboot::initialize(*vmOperations, *windowAgent);
}

void androidSnapshot_finalize() {
    android::snapshot::Quickboot::finalize();
    android::snapshot::sInstance.clear();
}
