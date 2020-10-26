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

#pragma once

#include <stdint.h>                                  // for uint64_t, int32_t
#include <string>                                    // for allocator, string
#include <vector>                                    // for vector

#include "android/base/Optional.h"                   // for Optional
#include "android/base/StringView.h"                 // for StringView, oper...
#include "android/base/containers/CircularBuffer.h"  // for CircularBuffer
#include "android/base/system/System.h"              // for System, System::...
#include "android/featurecontrol/Features.h"         // for Feature
#include "android/snapshot/common.h"                 // for FailureReason
#include "snapshot.pb.h"                             // for SaveStats, Confi...

namespace android {
namespace snapshot {

class Snapshot final {
public:
    Snapshot(const char* name);
    explicit Snapshot(const char* name,
                      const char* dataDir,
                      bool verifyQcow = true);
    Snapshot(const emulator_snapshot::Snapshot& snapshotPb);

    static std::vector<Snapshot> getExistingSnapshots();
    static base::Optional<Snapshot> getSnapshotById(std::string id);

    base::StringView name() const { return mName; }
    base::StringView dataDir() const { return mDataDir; }


    // An imported snapshot stores the qcow2's inside the snapshot directory.
    bool isImported();
    // The hardware.ini & protobuf contain hardcoded paths, this will try to
    // fix them up.
    bool fixImport();
    bool save();
    bool saveFailure(FailureReason reason);
    bool preload();
    bool load();
    void incrementInvalidLoads();
    void incrementSuccessfulLoads();
    bool shouldInvalidate() const;
    void addSaveStats(bool incremental,
                      const base::System::Duration duration,
                      uint64_t ramChangedBytes);
    bool areSavesSlow() const;

    uint64_t diskSize() const { return mSize; }
    uint64_t folderSize() const { return mFolderSize; }

    // Returns the snapshot's Protobuf holding its metadata.
    // If Protobuf cannot be read, sets failure reason
    // and returns null.
    const emulator_snapshot::Snapshot* getGeneralInfo();
    // Checks whether the snapshot can be loaded.
    // |writeResult| controls whether we will invalidate
    // the snapshot protobuf with an error code if the check
    // does not work out.
    const bool checkValid(bool writeResult);

    // Checks to see whether the snapshot can be loaded,
    // ignoring any virtual machine and gpu configuration.
    const bool checkOfflineValid(bool writeResult = false);

    base::Optional<FailureReason> failureReason() const;

    static base::StringView dataDir(const char* name);

    base::Optional<std::string> parent();

    // For dealing with snapshot-insensitive feature flags
    static const std::vector<android::featurecontrol::Feature>
    getSnapshotInsensitiveFeatures();
    static bool isFeatureSnapshotInsensitive(
            android::featurecontrol::Feature feature);
    void initProto();
    void saveEnabledFeatures();
    bool writeSnapshotToDisk();
    bool isCompatibleWithCurrentFeatures();
    bool isLoaded();

private:
    void loadProtobufOnce();
    bool verifyHost(const emulator_snapshot::Host& host, bool writeFailure);
    bool verifyConfig(const emulator_snapshot::Config& config,
                      bool writeFailure);
    bool verifyFeatureFlags(const emulator_snapshot::Config& config);

    std::string mName;
    std::string mDataDir;
    emulator_snapshot::Snapshot mSnapshotPb;
    // We might not load the qcow files when validating an exported snapshot.
    bool mVerifyQcow = true;
    uint64_t mSize = 0;
    uint64_t mFolderSize = 0;
    int32_t mInvalidLoads = 0;
    int32_t mSuccessfulLoads = 0;

    base::CircularBuffer<emulator_snapshot::SaveStats> mSaveStats;
    FailureReason mLatestFailureReason = FailureReason::Empty;
};

inline bool operator==(const Snapshot& l, const Snapshot& r) {
    return l.name() == r.name();
}

}  // namespace snapshot
}  // namespace android
