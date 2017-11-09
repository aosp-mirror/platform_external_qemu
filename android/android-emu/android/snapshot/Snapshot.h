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

#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/snapshot/common.h"
#include "android/snapshot/proto/snapshot.pb.h"

#include <string>

namespace android {
namespace snapshot {

class Snapshot final {
public:
    Snapshot(const char* name);

    base::StringView name() const { return mName; }
    base::StringView dataDir() const { return mDataDir; }

    bool save();
    bool saveFailure(FailureReason reason);
    bool preload();
    bool load();
    void incrementInvalidLoads();
    void incrementSuccessfulLoads();
    bool shouldInvalidate() const;

    uint64_t diskSize() const { return mSize; }

    base::Optional<FailureReason> failureReason() const;

    static base::StringView dataDir(const char* name);

private:
    bool verifyHost(const emulator_snapshot::Host& host);
    bool verifyConfig(const emulator_snapshot::Config& config);
    bool writeSnapshotToDisk();

    std::string mName;
    std::string mDataDir;
    emulator_snapshot::Snapshot mSnapshotPb;
    uint64_t mSize = 0;
    int32_t mInvalidLoads = 0;
    int32_t mSuccessfulLoads = 0;
};

inline bool operator==(const Snapshot& l, const Snapshot& r) {
    return l.name() == r.name();
}

}  // namespace snapshot
}  // namespace android
