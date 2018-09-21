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

#include "android/snapshot/common.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/metrics/AdbLivenessChecker.h"
#include "android/opengles.h"

#include <errno.h>

namespace android {
namespace snapshot {

FailureReason errnoToFailure(int error) {
    switch (error) {
        default:
        case 0:
        case 1:
            return FailureReason::InternalError;
        case EINVAL:
            return FailureReason::CorruptedData;
        case ENOENT:
            return FailureReason::NoSnapshotInImage;
        case ENOTSUP:
            return FailureReason::SnapshotsNotSupported;
    }
}

const char* failureReasonToString(FailureReason failure,
                                   SnapshotOperation op) {
    switch (failure) {
        default:
            return "unknown failure";
        case FailureReason::BadSnapshotPb:
            return "bad snapshot metadata";
        case FailureReason::CorruptedData:
            return "bad snapshot data";
        case FailureReason::NoSnapshotPb:
            return "missing snapshot files";
        case FailureReason::IncompatibleVersion:
            return "incompatible snapshot version";
        case FailureReason::NoRamFile:
            return "missing saved RAM data";
        case FailureReason::NoTexturesFile:
            return "missing saved textures data";
        case FailureReason::NoSnapshotInImage:
            return "snapshot doesn't exist";
        case FailureReason::SnapshotsNotSupported:
            return "current configuration doesn't support snapshots";
        case FailureReason::ConfigMismatchHostHypervisor:
            return "host hypervisor has changed";
        case FailureReason::ConfigMismatchHostGpu:
            return "host GPU has changed";
        case FailureReason::ConfigMismatchRenderer:
            return "different renderer configured";
        case FailureReason::ConfigMismatchFeatures:
            return "different emulator features";
        case FailureReason::ConfigMismatchAvd:
            return "different AVD configuration";
        case FailureReason::SystemImageChanged:
            return "system image changed";
        case FailureReason::InternalError:
            return "internal error";
        case FailureReason::EmulationEngineFailed:
            return "emulation engine failed";
        case FailureReason::RamFailed:
            return op == SNAPSHOT_LOAD ? "RAM loading failed"
                                       : "RAM saving failed";
        case FailureReason::TexturesFailed:
            return op == SNAPSHOT_LOAD ? "textures loading failed"
                                       : "textures saving failed";
    }
}

// bug: 116315668
// The current scheme of snapshot liveness checking
// causes more problems than it solves;
// adb reliability or idle app graphics will cause
// hang signals.
// For now, just fall back on the hang detector.
void resetSnapshotLiveness() {
    // android_resetGuestPostedAFrame();
}

bool isSnapshotAlive() {
    // bug: 116315668
    return true;
    // return metrics::AdbLivenessChecker::isEmulatorBooted() ||
    //        guest_boot_completed ||
    //        android_hasGuestPostedAFrame() ||
    //        android::featurecontrol::isEnabled(
    //             android::featurecontrol::AllowSnapshotMigration);
}

} // namespace android
} // namespace snapshot
