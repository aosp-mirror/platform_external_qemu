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

#include "android/snapshot/Quickboot.h"

#include "android/adb-server.h"
#include "android/base/Stopwatch.h"
#include "android/base/StringFormat.h"
#include "android/base/async/ThreadLooper.h"
#include "android/cmdline-option.h"
#include "android/crashreport/CrashReporter.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/metrics/AdbLivenessChecker.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/opengl/emugl_config.h"
#include "android/snapshot/Snapshotter.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"
#include "android/snapshot/interface.h"
#include "android/utils/debug.h"

#include <cassert>

using android::base::Stopwatch;
using android::base::StringFormat;
using android::base::StringView;
using android::base::System;
using android::crashreport::CrashReporter;
using android::metrics::MetricsReporter;
namespace pb = android_studio;

static constexpr int kDefaultMessageTimeoutMs = 10000;

namespace android {
namespace snapshot {

static void reportFailedLoad(
        pb::EmulatorQuickbootLoad::EmulatorQuickbootLoadState state,
        FailureReason failureReason) {
    MetricsReporter::get().report([state, failureReason](pb::AndroidStudioEvent* event) {
        event->mutable_emulator_details()->mutable_quickboot_load()->set_state(
                state);
        event->mutable_emulator_details()->mutable_quickboot_load()->
            mutable_snapshot()->set_load_failure_reason(
                (pb::EmulatorSnapshotFailureReason)failureReason);
    });
}

static void reportFailedSave(
        pb::EmulatorQuickbootSave::EmulatorQuickbootSaveState state) {
    MetricsReporter::get().report([state](pb::AndroidStudioEvent* event) {
        event->mutable_emulator_details()->mutable_quickboot_save()->set_state(
                state);
        event->mutable_emulator_details()->mutable_quickboot_save()->
            mutable_snapshot()->set_save_failure_reason(
                (pb::EmulatorSnapshotFailureReason)FailureReason::Empty);
    });
}

static void reportAdbConnectionRetries(uint32_t retries) {
    MetricsReporter::get().report([retries](pb::AndroidStudioEvent* event) {
        event->mutable_emulator_details()->mutable_quickboot_load()->
            set_adb_connection_retries(retries);
    });
}

static const char* failureToString(FailureReason failure,
                                   SnapshotOperation op) {
    switch (failure) {
        default:
            return "Unknown failure";
        case FailureReason::BadSnapshotPb:
        case FailureReason::CorruptedData:
            return "Bad snapshot data";
        case FailureReason::NoSnapshotPb:
            return "Missing snapshot files";
        case FailureReason::IncompatibleVersion:
            return "Incompatible snapshot version";
        case FailureReason::NoRamFile:
            return "Missing saved RAM data";
        case FailureReason::NoTexturesFile:
            return "Missing saved textures data";
        case FailureReason::NoSnapshotInImage:
            return "Snapshot doesn't exist";
        case FailureReason::SnapshotsNotSupported:
            return "Current configuration doesn't support snapshots";
        case FailureReason::ConfigMismatchHostHypervisor:
            return "Host hypervisor has changed";
        case FailureReason::ConfigMismatchHostGpu:
            return "Host GPU has changed";
        case FailureReason::ConfigMismatchRenderer:
            return "Different renderer configured";
        case FailureReason::ConfigMismatchFeatures:
            return "Different emulator features";
        case FailureReason::ConfigMismatchAvd:
            return "Different AVD configuration";
        case FailureReason::SystemImageChanged:
            return "System image changed";
        case FailureReason::InternalError:
            return "Internal error";
        case FailureReason::EmulationEngineFailed:
            return "Emulation engine failed";
        case FailureReason::RamFailed:
            return op == SNAPSHOT_LOAD ? "RAM loading failed"
                                       : "RAM saving failed";
        case FailureReason::TexturesFailed:
            return op == SNAPSHOT_LOAD ? "Textures loading failed"
                                       : "Textures saving failed";
    }
}

constexpr StringView Quickboot::kDefaultBootSnapshot;
static Quickboot* sInstance = nullptr;

Quickboot& Quickboot::get() {
    assert(sInstance);
    return *sInstance;
}

void Quickboot::initialize(const QAndroidVmOperations& vmOps,
                           const QAndroidEmulatorWindowAgent& window) {
    assert(!sInstance);
    sInstance = new Quickboot(vmOps, window);
}

void Quickboot::finalize() {
    delete sInstance;
    sInstance = nullptr;
}

Quickboot::~Quickboot() { }

void Quickboot::reportSuccessfulLoad(StringView name,
                                     System::WallDuration startTimeMs) {
    auto& loader = Snapshotter::get().loader();
    loader.reportSuccessful();
    const auto durationMs = mLoadTimeMs - startTimeMs;
    const auto onDemandRamEnabled = loader.ramLoader().onDemandEnabled();
    const auto compressedRam = loader.ramLoader().compressed();
    const auto compressedTextures = loader.textureLoader()->compressed();
    const auto size = loader.snapshot().diskSize() +
                      loader.ramLoader().diskSize() +
                      loader.textureLoader()->diskSize();
    MetricsReporter::get().report([onDemandRamEnabled, compressedRam,
                                   compressedTextures, durationMs, name,
                                   size](pb::AndroidStudioEvent* event) {
        auto load = event->mutable_emulator_details()->mutable_quickboot_load();
        load->set_state(
                pb::EmulatorQuickbootLoad::EMULATOR_QUICKBOOT_LOAD_SUCCEEDED);
        load->set_duration_ms(durationMs);
        load->set_on_demand_ram_enabled(onDemandRamEnabled);
        auto snapshot = load->mutable_snapshot();
        snapshot->set_name(MetricsReporter::get().anonymize(name));
        if (compressedRam) {
            snapshot->set_flags(pb::SNAPSHOT_FLAGS_RAM_COMPRESSED_BIT);
        }
        if (compressedTextures) {
            snapshot->set_flags(pb::SNAPSHOT_FLAGS_TEXTURES_COMPRESSED_BIT);
        }
        snapshot->set_size_bytes(int64_t(size));
    });
}

void Quickboot::reportSuccessfulSave(StringView name,
                                     System::WallDuration durationMs,
                                     System::WallDuration sessionUptimeMs) {
    auto& saver = Snapshotter::get().saver();
    const auto compressedRam = saver.ramSaver().compressed();
    const auto compressedTextures = saver.textureSaver()->compressed();
    const auto size = saver.snapshot().diskSize() +
                      saver.ramSaver().diskSize() +
                      saver.textureSaver()->diskSize();
    MetricsReporter::get().report([compressedRam, compressedTextures,
                                   durationMs, sessionUptimeMs, name,
                                   size](pb::AndroidStudioEvent* event) {
        auto save = event->mutable_emulator_details()->mutable_quickboot_save();
        save->set_state(
                pb::EmulatorQuickbootSave::EMULATOR_QUICKBOOT_SAVE_SUCCEEDED);
        save->set_duration_ms(durationMs);
        save->set_sesion_uptime_ms(sessionUptimeMs);
        auto snapshot = save->mutable_snapshot();
        snapshot->set_name(MetricsReporter::get().anonymize(name));
        if (compressedRam) {
            snapshot->set_flags(pb::SNAPSHOT_FLAGS_RAM_COMPRESSED_BIT);
        }
        if (compressedTextures) {
            snapshot->set_flags(pb::SNAPSHOT_FLAGS_TEXTURES_COMPRESSED_BIT);
        }
        snapshot->set_size_bytes(int64_t(size));
    });
}

constexpr int kLivenessTimerTimeoutMs = 100;
constexpr int kBootTimeoutMs = 7 * 1000;
constexpr int kMaxAdbConnectionRetries = 1;

static int bootTimeoutMs() {
    if (avdInfo_is_x86ish(android_avdInfo)) {
        return kBootTimeoutMs;
    }
    return kBootTimeoutMs * 5;
}

void Quickboot::startLivenessMonitor() {
    mLivenessTimer->startRelative(kLivenessTimerTimeoutMs);
}

void Quickboot::onLivenessTimer() {
    if (metrics::AdbLivenessChecker::isEmulatorBooted()) {
        VERBOSE_PRINT(snapshot, "Guest came online in %.3f sec after loading",
                      (System::get()->getHighResTimeUs() / 1000 - mLoadTimeMs) /
                              1000.0);
        // done here: snapshot loaded fine and emulator's working.
        return;
    }

    const auto nowMs = System::get()->getHighResTimeUs() / 1000;
    if (int64_t(nowMs - mLoadTimeMs) > bootTimeoutMs()) {
        if (mAdbConnectionRetries < kMaxAdbConnectionRetries) {
            mWindow.showMessage(
                    StringFormat("Guest isn't online in %d seconds, "
                                 "retrying ADB connection",
                                 int(nowMs - mLoadTimeMs) / 1000)
                            .c_str(),
                    WINDOW_MESSAGE_ERROR, kDefaultMessageTimeoutMs);
            android_adb_reset_connection();
            mLoadTimeMs = nowMs;
            mAdbConnectionRetries++;
            reportAdbConnectionRetries(mAdbConnectionRetries);
        } else {
            // The VM hasn't started for long enough since the end of snapshot
            // loading, let's reset it.
            mWindow.showMessage(
                    StringFormat("Guest isn't online in %d seconds, "
                                 "restarting and deleting boot snapshot",
                                 int(nowMs - mLoadTimeMs) / 1000)
                            .c_str(),
                    WINDOW_MESSAGE_ERROR, kDefaultMessageTimeoutMs);
            Snapshotter::get().deleteSnapshot(mLoadedSnapshotName.c_str());
            reportFailedLoad(
                    pb::EmulatorQuickbootLoad::EMULATOR_QUICKBOOT_LOAD_HUNG,
                    FailureReason::AdbOffline);
            mVmOps.vmReset();
            return;
        }
    }

    mLivenessTimer->startRelative(kLivenessTimerTimeoutMs);
}

Quickboot::Quickboot(const QAndroidVmOperations& vmOps,
                     const QAndroidEmulatorWindowAgent& window)
    : mVmOps(vmOps),
      mWindow(window),
      mLivenessTimer(base::ThreadLooper::get()->createTimer(
              [](void* opaque, base::Looper::Timer*) {
                  static_cast<Quickboot*>(opaque)->onLivenessTimer();
              },
              this)) {}

bool Quickboot::load(StringView name) {
    if (!isEnabled(featurecontrol::FastSnapshotV1)) {
        reportFailedLoad(pb::EmulatorQuickbootLoad::
                                 EMULATOR_QUICKBOOT_LOAD_COLD_FEATURE,
                         FailureReason::Empty);
    }
    if (name.empty()) {
        name = kDefaultBootSnapshot;
    }

    // Invalidate quickboot snapshot if the crash reporter trips.
    // It's possible the crash was not due to snapshot load,
    // but it's better than crashing over and over in
    // the same load.
    // Don't try to delete it completely as that is a heavyweight
    // operation and we are in the middle of crashing.
    CrashReporter::get()->addCrashCallback([this, &name]() {
        Snapshotter::get().onCrashedSnapshot(name.c_str());
    });

    if (android_cmdLineOptions->no_snapshot_load) {
        if (!android_hw->fastboot_forceColdBoot) {
            // Only display a message if this is a one-time-like thing (command
            // line), and not an AVD option.
            mWindow.showMessage("Performing cold boot: requested by the user",
                                WINDOW_MESSAGE_INFO, kDefaultMessageTimeoutMs);
        }
        reportFailedLoad(
                android_hw->fastboot_forceColdBoot
                        ? pb::EmulatorQuickbootLoad::
                                  EMULATOR_QUICKBOOT_LOAD_COLD_AVD
                        : pb::EmulatorQuickbootLoad::
                                  EMULATOR_QUICKBOOT_LOAD_COLD_CMDLINE,
                FailureReason::Empty);
    } else if (!emuglConfig_current_renderer_supports_snapshot()) {
        mWindow.showMessage(
                StringFormat("Performing cold boot: selected renderer '%s' "
                             "doesn't support snapshots",
                             emuglConfig_renderer_to_string(
                                     emuglConfig_get_current_renderer()))
                        .c_str(),
                WINDOW_MESSAGE_INFO, kDefaultMessageTimeoutMs);
        reportFailedLoad(pb::EmulatorQuickbootLoad::
                                 EMULATOR_QUICKBOOT_LOAD_COLD_UNSUPPORTED,
                         FailureReason::Empty);
    } else {
        const auto startTimeMs = System::get()->getHighResTimeUs() / 1000;
        auto& snapshotter = Snapshotter::get();
        auto res = snapshotter.load(true /* isQuickboot */, name.c_str());
        mLoaded = false;
        mLoadStatus = res;
        mLoadTimeMs = System::get()->getHighResTimeUs() / 1000;
        if (res == OperationStatus::Ok) {
            mLoaded = true;
            mLoadedSnapshotName = name;
            reportSuccessfulLoad(name, startTimeMs);
            startLivenessMonitor();
        } else {
            // Check if the error is about something done before the real load
            // (e.g. condition check) or we've started actually loading the VM
            // data
            if (auto failureReason =
                        snapshotter.loader().snapshot().failureReason()) {
                if (*failureReason != FailureReason::Empty &&
                    *failureReason < FailureReason::ValidationErrorLimit) {
                    mWindow.showMessage(
                            StringFormat("Performing clean boot: %s",
                                         failureToString(*failureReason,
                                                         SNAPSHOT_LOAD))
                                    .c_str(),
                            WINDOW_MESSAGE_INFO, kDefaultMessageTimeoutMs);
                    reportFailedLoad(
                            failureReason < FailureReason::
                                                    UnrecoverableErrorLimit
                                    ? pb::EmulatorQuickbootLoad::
                                              EMULATOR_QUICKBOOT_LOAD_FAILED
                                    : pb::EmulatorQuickbootLoad::
                                              EMULATOR_QUICKBOOT_LOAD_COLD_OLD_SNAPSHOT,
                            *failureReason);

                } else {
                    mWindow.showMessage(
                            StringFormat("Resetting for clean boot: %s",
                                         failureToString(*failureReason,
                                                         SNAPSHOT_LOAD))
                                    .c_str(),
                            WINDOW_MESSAGE_WARNING, kDefaultMessageTimeoutMs);
                    mVmOps.vmReset();
                    Snapshotter::get().loader().reportInvalid();
                    reportFailedLoad(pb::EmulatorQuickbootLoad::
                                             EMULATOR_QUICKBOOT_LOAD_FAILED,
                                     *failureReason);
                }
            } else {
                mWindow.showMessage(
                        "Performing clean boot: snapshot failed to load",
                        WINDOW_MESSAGE_WARNING, kDefaultMessageTimeoutMs);
                mVmOps.vmReset();
                reportFailedLoad(pb::EmulatorQuickbootLoad::
                                         EMULATOR_QUICKBOOT_LOAD_NO_SNAPSHOT,
                                 FailureReason::Empty);
            }
        }
    }

    return true;
}

bool Quickboot::save(StringView name) {
    // TODO: detect if emulator was restarted since loading.
    const bool shouldTrySaving =
            mLoaded || metrics::AdbLivenessChecker::isEmulatorBooted();

    if (!shouldTrySaving) {
        // Emulator hasn't booted yet, and this isn't a quickboot-loaded
        // session. Don't save.
        dwarning("Skipping state saving as emulator not finished booting.");
        reportFailedSave(
                pb::EmulatorQuickbootSave::
                        EMULATOR_QUICKBOOT_SAVE_SKIPPED_NOT_BOOTED);
        return false;
    }

    mLivenessTimer->stop();

    if (!isEnabled(featurecontrol::FastSnapshotV1)) {
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_DISABLED_FEATURE);
        return false;
    }

    if (android_cmdLineOptions->no_snapshot_save) {
        // Command line says not to save
        mWindow.showMessage("Discarding the changed state: command-line flag",
                            WINDOW_MESSAGE_INFO, kDefaultMessageTimeoutMs);

        dwarning("Discarding the changed state (command-line flag).");
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_DISABLED_CMDLINE);
        return false;
    }

    if (android_avdParams->flags & AVDINFO_NO_SNAPSHOT_SAVE_ON_EXIT) {
        // UI says not to save
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_DISABLED_UI);
        return false;
    }

    if (name.empty()) {
        name = kDefaultBootSnapshot;
    }

    const int kMinUptimeForSavingMs = 1500;
    const auto nowMs = System::get()->getHighResTimeUs() / 1000;
    const auto sessionUptimeMs =
            nowMs - (mLoadTimeMs ? mLoadTimeMs : mStartTimeMs);
    const bool ranLongEnoughForSaving = sessionUptimeMs > kMinUptimeForSavingMs;

    // TODO: when cleaning the current 'default_boot' snapshot, save the reason
    //  of its invalidation in it - this way emulator will be able to give a
    //  better idea on the next clean boot other than "no snapshot".

    if (!emuglConfig_current_renderer_supports_snapshot()) {
        if (shouldTrySaving && ranLongEnoughForSaving) {
            // Preserve the state changes - we've ran for a while now
            // and the AVD state is different from what could be saved in
            // the default boot snapshot.
            dwarning(
                    "Cleaning out the default boot snapshot to preserve the "
                    "current session (renderer type '%s' (%d) doesn't support "
                    "snapshotting).",
                    emuglConfig_renderer_to_string(
                            emuglConfig_get_current_renderer()),
                    int(emuglConfig_get_current_renderer()));
            Snapshotter::get().deleteSnapshot(name.c_str());
        } else {
            dwarning(
                    "Skipping boot snapshot saving (renderer type '%s' (%d) "
                    "doesn't support snapshotting).",
                    emuglConfig_renderer_to_string(
                            emuglConfig_get_current_renderer()),
                    int(emuglConfig_get_current_renderer()));
        }
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_SKIPPED_UNSUPPORTED);
        return false;
    }

    if (!ranLongEnoughForSaving) {
        dwarning(
                "Skipping state saving as emulator ran for just %d "
                "ms (<%d ms)",
                int(sessionUptimeMs), kMinUptimeForSavingMs);
        reportFailedSave(
                pb::EmulatorQuickbootSave::
                        EMULATOR_QUICKBOOT_SAVE_SKIPPED_LOW_UPTIME);
        return false;
    }

    dprint("Saving state on exit with session uptime %d ms",
           int(sessionUptimeMs));
    Stopwatch sw;
    auto res = Snapshotter::get().save(name.c_str());
    if (res != OperationStatus::Ok) {
        mWindow.showMessage(
                "State saving failed, cleaning out the snapshot",
                WINDOW_MESSAGE_WARNING, kDefaultMessageTimeoutMs);

        dwarning("State saving failed, cleaning out the snapshot.");
        Snapshotter::get().deleteSnapshot(name.c_str());
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_FAILED);
        return false;
    }

    reportSuccessfulSave(name, sw.elapsedUs() / 1000,
                         sessionUptimeMs);
    return true;
}

void Quickboot::invalidate(StringView name) {
    if (name.empty()) {
        name = kDefaultBootSnapshot;
    }
    Snapshotter::get().deleteSnapshot(name.c_str());
}

}  // namespace snapshot
}  // namespace android

bool androidSnapshot_quickbootLoad(const char* name) {
    return android::snapshot::Quickboot::get().load(name);
}

bool androidSnapshot_quickbootSave(const char* name) {
    return android::snapshot::Quickboot::get().save(name);
}

void androidSnapshot_quickbootInvalidate(const char* name) {
    android::snapshot::Quickboot::get().invalidate(name);
}
