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

#include "android/base/Stopwatch.h"
#include "android/cmdline-option.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
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
using android::base::StringView;
using android::base::System;
using android::metrics::MetricsReporter;
namespace pb = android_studio;

namespace android {
namespace snapshot {

static void reportFailedLoad(
        pb::EmulatorQuickbootLoad::EmulatorQuickbootLoadState state) {
    MetricsReporter::get().report([state](pb::AndroidStudioEvent* event) {
        event->mutable_emulator_details()->mutable_quickboot_load()->set_state(
                state);
    });
}

static void reportFailedSave(
        pb::EmulatorQuickbootSave::EmulatorQuickbootSaveState state) {
    MetricsReporter::get().report([state](pb::AndroidStudioEvent* event) {
        event->mutable_emulator_details()->mutable_quickboot_save()->set_state(
                state);
    });
}

constexpr StringView Quickboot::kDefaultBootSnapshot;
static Quickboot* sInstance = nullptr;

Quickboot& Quickboot::get() {
    assert(sInstance);
    return *sInstance;
}

void Quickboot::initialize(const QAndroidVmOperations& vmOps) {
    assert(!sInstance);
    sInstance = new Quickboot(vmOps);
}

void Quickboot::finalize() {
    delete sInstance;
    sInstance = nullptr;
}

void Quickboot::reportSuccessfulLoad(StringView name,
                                     System::WallDuration startTimeMs) {
    auto& loader = Snapshotter::get().loader();
    const auto durationMs = mLoadTimeMs - startTimeMs;
    const auto onDemandRamEnabled = loader.ramLoader().onDemandEnabled();
    const auto compressedRam = loader.ramLoader().compressed();
    const auto size = loader.snapshot().diskSize() +
                      loader.ramLoader().diskSize() +
                      loader.textureLoader()->diskSize();
    MetricsReporter::get().report([onDemandRamEnabled, compressedRam,
                                   durationMs, name,
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
        snapshot->set_size_bytes(int64_t(size));
    });
}

void Quickboot::reportSuccessfulSave(StringView name,
                                     System::WallDuration durationMs,
                                     System::WallDuration sessionUptimeMs) {
    auto& saver = Snapshotter::get().saver();
    const auto compressedRam = saver.ramSaver().compressed();
    const auto size = saver.snapshot().diskSize() +
                      saver.ramSaver().diskSize() +
                      saver.textureSaver()->diskSize();
    MetricsReporter::get().report([compressedRam, durationMs, sessionUptimeMs,
                                   name, size](pb::AndroidStudioEvent* event) {
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
        snapshot->set_size_bytes(int64_t(size));
    });
}

Quickboot::Quickboot(const QAndroidVmOperations& vmOps) : mVmOps(vmOps) {}

bool Quickboot::load(StringView name) {
    if (!isEnabled(featurecontrol::FastSnapshotV1)) {
        reportFailedLoad(pb::EmulatorQuickbootLoad::
                                 EMULATOR_QUICKBOOT_LOAD_COLD_FEATURE);
    }
    if (name.empty()) {
        name = kDefaultBootSnapshot;
    }

    if (android_cmdLineOptions->no_snapshot_load) {
        reportFailedLoad(
                android_hw->fastboot_forceColdBoot
                        ? pb::EmulatorQuickbootLoad::
                                  EMULATOR_QUICKBOOT_LOAD_COLD_AVD
                        : pb::EmulatorQuickbootLoad::
                                  EMULATOR_QUICKBOOT_LOAD_COLD_CMDLINE);
    } else if (!emuglConfig_current_renderer_supports_snapshot()) {
        dwarning("Selected renderer '%s' (%d) doesn't support snapshots",
                 emuglConfig_renderer_to_string(
                         emuglConfig_get_current_renderer()),
                 int(emuglConfig_get_current_renderer()));
        reportFailedLoad(pb::EmulatorQuickbootLoad::
                                 EMULATOR_QUICKBOOT_LOAD_COLD_UNSUPPORTED);
    } else {
        const auto startTimeMs = System::get()->getHighResTimeUs() / 1000;
        auto& snapshotter = Snapshotter::get();
        auto res = snapshotter.load(name.c_str());
        mLoaded = false;
        mLoadStatus = res;
        mLoadTimeMs = System::get()->getHighResTimeUs() / 1000;
        if (res == OperationStatus::Ok) {
            mLoaded = true;
            reportSuccessfulLoad(name, startTimeMs);
        } else {
            // Check if the error is about something done before the real load
            // (e.g. condition check) or we've started actually loading the VM
            // data
            if (auto failureReason =
                        snapshotter.loader().snapshot().failureReason()) {
                if (*failureReason != FailureReason::Empty &&
                    *failureReason < FailureReason::ValidationErrorLimit) {
                    dwarning(
                            "Snapshot '%s' can not be loaded (%d), state is "
                            "unchanged. Resuming with a clean boot.",
                            name.c_str(), int(*failureReason));
                    reportFailedLoad(
                            failureReason < FailureReason::
                                                    UnrecoverableErrorLimit
                                    ? pb::EmulatorQuickbootLoad::
                                              EMULATOR_QUICKBOOT_LOAD_FAILED
                                    : pb::EmulatorQuickbootLoad::
                                              EMULATOR_QUICKBOOT_LOAD_COLD_OLD_SNAPSHOT);

                } else {
                    derror("Snapshot '%s' can not be loaded (%d), emulator "
                           "stopped. Resetting the system for a clean boot.",
                           name.c_str(), int(*failureReason));
                    mVmOps.vmReset();
                    reportFailedLoad(pb::EmulatorQuickbootLoad::
                                             EMULATOR_QUICKBOOT_LOAD_FAILED);
                }
            } else {
                derror("Snapshot '%s' can not be loaded (reason not set), "
                       "emulator stopped. Resetting the system for a clean "
                       "boot.",
                       name.c_str());
                mVmOps.vmReset();
                reportFailedLoad(pb::EmulatorQuickbootLoad::
                                         EMULATOR_QUICKBOOT_LOAD_NO_SNAPSHOT);
            }
        }
    }
    return true;
}

bool Quickboot::save(StringView name) {
    if (!isEnabled(featurecontrol::FastSnapshotV1)) {
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_DISABLED_FEATURE);
        return false;
    }
    if (name.empty()) {
        name = kDefaultBootSnapshot;
    }

    if (android_cmdLineOptions->no_snapshot_save) {
        dwarning("Discarding the changed state (command-line flag).");
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_DISABLED_CMDLINE);
    } else if (!emuglConfig_current_renderer_supports_snapshot()) {
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
        reportFailedSave(pb::EmulatorQuickbootSave::
                                 EMULATOR_QUICKBOOT_SAVE_SKIPPED_UNSUPPORTED);
    } else {
        const int kMinUptimeForSavingMs = 1500;
        const auto nowMs = System::get()->getHighResTimeUs() / 1000;
        const auto sessionUptimeMs = nowMs - mLoadTimeMs;
        if (sessionUptimeMs > kMinUptimeForSavingMs) {
            if (mLoaded || androidSnapshot_canSave()) {
                dprint("Saving state on exit with session uptime %d ms",
                       int(sessionUptimeMs));
                Stopwatch sw;
                auto res = Snapshotter::get().save(name.c_str());
                if (res == OperationStatus::Ok) {
                    reportSuccessfulSave(name, sw.elapsedUs() / 1000,
                                         sessionUptimeMs);
                } else {
                    dwarning("State saving failed, cleaning out the snapshot.");
                    Snapshotter::get().deleteSnapshot(name.c_str());
                    reportFailedSave(pb::EmulatorQuickbootSave::
                                             EMULATOR_QUICKBOOT_SAVE_FAILED);
                }
            } else {
                // Get rid of whatever was saved as a boot snapshot as
                // the emulator ran for a while but we can't capture its
                // current state to resume later.
                dwarning(
                        "Cleaning out the default boot snapshot to "
                        "preserve changes made in the current "
                        "session.");
                Snapshotter::get().deleteSnapshot(name.c_str());
                reportFailedSave(
                        pb::EmulatorQuickbootSave::
                                EMULATOR_QUICKBOOT_SAVE_SKIPPED_UNSUPPORTED);
            }
        } else {
            dwarning(
                    "Skipping state saving as emulator ran for just %d "
                    "ms (<%d ms)",
                    int(sessionUptimeMs), kMinUptimeForSavingMs);
            reportFailedSave(
                    pb::EmulatorQuickbootSave::
                            EMULATOR_QUICKBOOT_SAVE_SKIPPED_LOW_UPTIME);
        }
    }
    return true;
}

}  // namespace snapshot
}  // namespace android

bool androidSnapshot_quickbootLoad(const char* name) {
    return android::snapshot::Quickboot::get().load(name);
}

bool androidSnapshot_quickbootSave(const char* name) {
    return android::snapshot::Quickboot::get().save(name);
}
