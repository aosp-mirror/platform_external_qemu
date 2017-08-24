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

#include "android/cmdline-option.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/opengl/emugl_config.h"
#include "android/snapshot/Snapshotter.h"
#include "android/snapshot/interface.h"
#include "android/utils/debug.h"

#include <cassert>

namespace android {
namespace snapshot {

constexpr base::StringView Quickboot::kDefaultBootSnapshot;

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

Quickboot::Quickboot(const QAndroidVmOperations& vmOps) : mVmOps(vmOps) {}

bool Quickboot::load(base::StringView name) {
    if (!isEnabled(featurecontrol::FastSnapshotV1)) {
        return false;
    }
    if (name.empty()) {
        name = kDefaultBootSnapshot;
    }

    if (!android_cmdLineOptions->no_snapshot_load &&
        emuglConfig_current_renderer_supports_snapshot()) {
        auto& snapshotter = Snapshotter::get();
        auto res = snapshotter.load(name.c_str());
        mLoadStatus = res;
        mLoadTimeMs = base::System::get()->getHighResTimeUs() / 1000;
        if (res != OperationStatus::Ok) {
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
                } else {
                    derror("Snapshot '%s' can not be loaded (%d), emulator "
                           "stopped. Resetting the system for a clean boot.",
                           name.c_str(), int(*failureReason));
                    mVmOps.vmReset();
                }
            } else {
                derror("Snapshot '%s' can not be loaded (reason not set), "
                       "emulator "
                       "stopped. Resetting the system for a clean boot.",
                       name.c_str());
                mVmOps.vmReset();
            }
        }
    }
    return true;
}

bool Quickboot::save(base::StringView name) {
    if (!isEnabled(featurecontrol::FastSnapshotV1)) {
        return false;
    }
    if (name.empty()) {
        name = kDefaultBootSnapshot;
    }

    if (!android_cmdLineOptions->no_snapshot_save &&
        emuglConfig_current_renderer_supports_snapshot()) {
        const int kMinUptimeForSavingMs = 1500;
        const auto nowMs = base::System::get()->getHighResTimeUs() / 1000;
        const auto sessionUptimeMs = nowMs - mLoadTimeMs;
        if (sessionUptimeMs > kMinUptimeForSavingMs) {
            const bool loaded = mLoadTimeMs != 0;
            if (loaded || androidSnapshot_canSave()) {
                dprint("Saving state on exit with session uptime %d ms",
                       int(sessionUptimeMs));
                Snapshotter::get().save(name.c_str());
            } else {
                // Get rid of whatever was saved as a boot snapshot as
                // the emulator ran for a while but we can't capture its
                // current state to resume later.
                dwarning(
                        "Cleaning out the default boot snapshot to "
                        "preserve changes made in the current "
                        "session.");
                Snapshotter::get().deleteSnapshot(name.c_str());
            }
        } else {
            dwarning(
                    "Skipping state saving as emulator ran for just %d "
                    "ms (<%d ms)",
                    int(sessionUptimeMs), kMinUptimeForSavingMs);
        }
    } else {
        // Again, preserve the state changes - we've ran for a while now
        // and the AVD state is different from what could be saved in
        // the default boot snapshot.
        dwarning(
                "Cleaning out the default boot snapshot to preserve "
                "the current session (command line option to not save "
                "snapshot).");
        Snapshotter::get().deleteSnapshot(name.c_str());
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
